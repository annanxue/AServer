[CmdletBinding()]
param(
    [string]$RepoRoot = '',
    [string]$AhxtRoot = '',
    [string]$BuildRoot = '',
    [switch]$KeepTemp,
    [switch]$ValidateOnly,
    [int]$HeartbeatSeconds = 10
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($RepoRoot)) { $RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot) }
if ([string]::IsNullOrWhiteSpace($AhxtRoot)) { $AhxtRoot = Join-Path (Split-Path -Parent $RepoRoot) 'ahxt_server' }
if ([string]::IsNullOrWhiteSpace($BuildRoot)) { $BuildRoot = Join-Path $RepoRoot 'bin/Release' }
$script:RepoRoot = $RepoRoot
$script:AhxtRoot = $AhxtRoot
$script:BuildRoot = $BuildRoot

$script:Checks = New-Object System.Collections.ArrayList
$script:StartedProcesses = @{}
$script:RuntimeRoots = @{}
$script:LogRoots = @{}
$script:Ports = @{}
$script:StubJob = $null
$script:TempRoot = $null
$script:OriginalPath = $env:PATH
$script:Utf8NoBom = New-Object System.Text.UTF8Encoding($false)

function Add-Check {
    param([string]$Name, [bool]$Passed, [string]$Details)
    [void]$script:Checks.Add([pscustomobject]@{ name=$Name; passed=$Passed; details=$Details; timestamp=[DateTime]::UtcNow.ToString('o') })
    Write-Host ("{0} {1} - {2}" -f ($(if($Passed){'PASS'}else{'FAIL'}), $Name, $Details))
}

function Resolve-SmokeInputs {
    param([switch]$RequireBuild)
    if(-not (Test-Path -LiteralPath $script:RepoRoot -PathType Container)) {
        Add-Check 'preflight' $false ("repository root not found: $script:RepoRoot")
        return 1
    }
    try { $script:RepoRoot = (Resolve-Path -LiteralPath $script:RepoRoot).Path } catch { Add-Check 'preflight' $false $_.Exception.Message; return 1 }
    if($RequireBuild) {
        if(-not (Test-Path -LiteralPath $script:BuildRoot -PathType Container)) {
            Add-Check 'binaries' $false ("build root not found: $script:BuildRoot")
            return 1
        }
        try { $script:BuildRoot = (Resolve-Path -LiteralPath $script:BuildRoot).Path } catch { Add-Check 'binaries' $false $_.Exception.Message; return 1 }
        $required=@('logserver.exe','loginserver.exe','gameserver.exe','robotclient.exe')
        $missing=@($required | Where-Object { -not (Test-Path (Join-Path $script:BuildRoot $_) -PathType Leaf) })
        if($missing.Count -gt 0) {
            Add-Check 'binaries' $false ('missing: '+($missing -join ', '))
            return 1
        }
        Add-Check 'binaries' $true $script:BuildRoot
    }
    if(-not (Test-Path -LiteralPath $script:AhxtRoot -PathType Container)) {
        Add-Check 'preflight' $false ("external Lua runtime not found: $script:AhxtRoot")
        return 2
    }
    try { $script:AhxtRoot = (Resolve-Path -LiteralPath $script:AhxtRoot).Path } catch { Add-Check 'preflight' $false $_.Exception.Message; return 2 }
    return 0
}

function Read-TextSafe {
    param([string]$Path)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return '' }
    try {
        $stream = [IO.File]::Open($Path, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
        try {
            $reader = New-Object IO.StreamReader($stream)
            try { return $reader.ReadToEnd() } finally { $reader.Dispose() }
        }
        finally { $stream.Dispose() }
    }
    catch {
        try { return [IO.File]::ReadAllText($Path) } catch { return '' }
    }
}

function Get-FreeTcpPort {
    $listener = New-Object System.Net.Sockets.TcpListener([Net.IPAddress]::Parse('127.0.0.1'), 0)
    try { $listener.Start(); return $listener.LocalEndpoint.Port } finally { $listener.Stop() }
}

function Wait-TcpPort {
    param([string]$Address, [int]$Port, [int]$TimeoutSeconds = 20)
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    while ([DateTime]::UtcNow -lt $deadline) {
        $client = New-Object Net.Sockets.TcpClient
        try {
            $async = $client.BeginConnect($Address, $Port, $null, $null)
            if ($async.AsyncWaitHandle.WaitOne(500)) { $client.EndConnect($async); return $true }
        } catch {} finally { $client.Close() }
        Start-Sleep -Milliseconds 200
    }
    return $false
}

function Get-Md5Hex {
    param([string]$Value)
    $md5 = New-Object Security.Cryptography.MD5CryptoServiceProvider
    try { return ([BitConverter]::ToString($md5.ComputeHash([Text.Encoding]::UTF8.GetBytes($Value)))).Replace('-', '').ToLowerInvariant() } finally { $md5.Dispose() }
}

function Get-ServiceText {
    param([string]$Root)
    $parts = New-Object System.Collections.Generic.List[string]
    if (Test-Path -LiteralPath $Root -PathType Leaf) {
        $text = Read-TextSafe $Root
        if ($text) { [void]$parts.Add($text) }
    } elseif (Test-Path -LiteralPath $Root -PathType Container) {
        foreach ($file in (Get-ChildItem -LiteralPath $Root -Recurse -File -ErrorAction SilentlyContinue)) {
            $text = Read-TextSafe $file.FullName
            if ($text) { [void]$parts.Add($text) }
        }
    }
    return [string]::Join("`n", $parts)
}

function Wait-LogPattern {
    param([string[]]$Roots, [string]$Pattern, [int]$TimeoutSeconds = 30)
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    while ([DateTime]::UtcNow -lt $deadline) {
        foreach ($root in $Roots) { if ((Get-ServiceText $root) -match $Pattern) { return $true } }
        Start-Sleep -Milliseconds 250
    }
    return $false
}

function Copy-DirectoryContents {
    param([string]$Source, [string]$Destination)
    if (-not (Test-Path -LiteralPath $Source -PathType Container)) { throw "directory not found: $Source" }
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    foreach ($item in (Get-ChildItem -LiteralPath $Source -Force)) { Copy-Item -LiteralPath $item.FullName -Destination $Destination -Recurse -Force }
}

function Resolve-LinkPlaceholders {
    param([string]$Root, [string]$CommonRoot)
    foreach ($file in (Get-ChildItem -LiteralPath $Root -Recurse -File -ErrorAction SilentlyContinue)) {
        $content = Read-TextSafe $file.FullName
        if ($content.Trim() -notmatch '^link\s+(.+)$') { continue }
        $target = $Matches[1].Trim().TrimEnd('/', '\')
        if ($file.Name -eq 'luacommon') { $source = $CommonRoot }
        elseif ($file.Name -eq 'resource') { $source = Join-Path $CommonRoot 'setting/resource' }
        else {
            $candidate = Join-Path $file.Directory.FullName $target
            if (-not (Test-Path -LiteralPath $candidate)) { throw "cannot resolve link placeholder $($file.FullName): $target" }
            $source = $candidate
        }
        Remove-Item -LiteralPath $file.FullName -Force
        if (Test-Path -LiteralPath $source -PathType Container) { New-Item -ItemType Directory -Force -Path $file.FullName | Out-Null; Copy-DirectoryContents $source $file.FullName }
        else { Copy-Item -LiteralPath $source -Destination $file.FullName -Force }
    }
}

function Patch-LuaSourcePath {
    param([string]$ScriptRoot)
    foreach ($file in (Get-ChildItem -LiteralPath $ScriptRoot -Recurse -File -Filter 'main.lua')) {
        $patched = [regex]::Replace((Read-TextSafe $file.FullName), '(?m)^\s*if\s+g_debug\s+then\s*$', 'if g_debug or (AppConfig and AppConfig.SmokeMode == 1) then')
        $patched = $patched.Replace('require_ex( "jit/opt" )', '-- SmokeMode uses the vcpkg LuaJIT runtime optimizer')
        $patched = [regex]::Replace($patched, '(?m)^\s*jit\.opt\.start\(3\)\s*$', 'if jit.opt and jit.opt.start then jit.opt.start(3) end')
        $patched = [regex]::Replace($patched, '(?m)^\s*jit\.debug\((\d+)\)\s*$', 'if jit.debug then jit.debug($1) end')
        $patched = [regex]::Replace($patched, '(?m)^\s*package\.path\s*=\s*"\./\?\.lc"\s*$', 'package.path = "./?.lua;./?.lc"')
        [IO.File]::WriteAllText($file.FullName, $patched, $script:Utf8NoBom)
    }
}

function Patch-GameSmokeMain {
    param([string]$RuntimeRoot)
    $path = Join-Path $RuntimeRoot 'script/main.lua'
    $content = Read-TextSafe $path
    $patched = [regex]::Replace($content, '(?m)^\s*EQUIP_ATTRS\s*=\s*require_ex\(\s*"luacommon/setting/EQUIP_ATTRS"\s*\)\s*$', 'EQUIP_ATTRS = {} -- SmokeMode avoids the oversized legacy Lua table')
    [IO.File]::WriteAllText($path, $patched, $script:Utf8NoBom)
}

function Patch-GameOversizedSettings {
    param([string]$RuntimeRoot)
    $mainPath = Join-Path $RuntimeRoot 'script/main.lua'
    $main = Read-TextSafe $mainPath
    $settingRoot = Join-Path $RuntimeRoot 'script/luacommon/setting'
    foreach ($file in (Get-ChildItem -LiteralPath $settingRoot -File -Filter '*.lua' -ErrorAction SilentlyContinue | Where-Object { $_.Length -gt 500000 })) {
        $module = [IO.Path]::GetFileNameWithoutExtension($file.Name)
        $escaped = [regex]::Escape($module)
        $assignPattern = '(?m)^(\s*)([A-Za-z_][A-Za-z0-9_]*)\s*=\s*require_ex\(\s*["'']luacommon/setting/' + $escaped + '["'']\s*\)\s*$'
        $main = [regex]::Replace($main, $assignPattern, '$1$2 = {} -- SmokeMode skips oversized Lua setting')
        $requirePattern = '(?m)^\s*require_ex\(\s*["'']luacommon/setting/' + $escaped + '["'']\s*\)\s*$'
        $main = [regex]::Replace($main, $requirePattern, '-- SmokeMode skips oversized Lua setting ' + $module)
    }
    [IO.File]::WriteAllText($mainPath, $main, $script:Utf8NoBom)
}

function Patch-RobotHttpDebug {
    param([string]$RuntimeRoot)
    $path = Join-Path $RuntimeRoot 'script/main.lua'
    $content = Read-TextSafe $path
    $content += [Environment]::NewLine + @"
SMOKE_CONNECT_GAME = 9001
SMOKE_CERTIFY_GAME = 9002
g_smoke_game_certify_pending = {}
local smoke_old_handle_timer = timers.handle_timer
timers.handle_timer = function(_id, _p1, _p2, _index)
    if _id == SMOKE_CONNECT_GAME then
        g_robotclient:c_connect_gamesvr()
        return 0
    elseif _id == timers.PING and _index == SMOKE_CERTIFY_GAME then
        c_log("[SmokeMode] certify timer fired dpid=%d", _p1)
        local smoke_info = g_smoke_game_certify_pending[_p1]
        g_smoke_game_certify_pending[_p1] = nil
        if smoke_info then robotclient.send_certify_to_game(_p1, smoke_info) end
        return 0
    end
    return smoke_old_handle_timer(_id, _p1, _p2, _index)
end
if g_http_mng then g_http_mng:c_set_debug(true) end
"@
    [IO.File]::WriteAllText($path, $content, $script:Utf8NoBom)
}

function Patch-RobotSmokeLogin {
    param([string]$RuntimeRoot)
    $path = Join-Path $RuntimeRoot 'script/net/robotclient.lua'
    $content = Read-TextSafe $path
    $patched = $content
    $loginMarker = 'function on_login_result( _ar, _dpid, _size )' + [Environment]::NewLine + '    c_log("[SmokeMode] robot received login result")'
    $patched = $patched.Replace('function on_login_result( _ar, _dpid, _size )', $loginMarker)
    $patched = $patched.Replace('        g_robotclient:c_disconnect( _dpid )', '        -- SmokeMode keeps the login socket while opening a game socket')
    $patched = $patched.Replace('    local login_token = _ar:read_int()', '    local login_token = _ar:read_int()' + [Environment]::NewLine + '    c_log("[SmokeMode] login values result=%d account=%.0f token=%d", login_result, account_id, login_token)')
    $gameConnect = @"
function on_gamesvr_connect(_dpid)
    c_log("[ROBOT](on_gamesvr_connect) dpid: 0x%X", _dpid)
    c_log("[SmokeMode] game connected dpid=%d", _dpid)
    local dpid_info = tremove(g_pending_certify)
    if dpid_info then
        dpid_info.dpid_type_ = DPID_TYPE_GAME
        g_dpid_mng[_dpid] = dpid_info
        ctrl_mng.add_dpid(_dpid)
        c_log("[SmokeMode] sending certify dpid=%d", _dpid)
        robotclient.send_certify_to_game(_dpid, dpid_info)
    end
end
"@
    $patched = [regex]::Replace($patched, '(?ms)^function\s+on_gamesvr_connect\s*\([^\r\n]*\).*?(?=^function\s+on_bfsvr_connect)', $gameConnect + [Environment]::NewLine)

    [IO.File]::WriteAllText($path, $patched, $script:Utf8NoBom)
}

function Create-GameSmokeMain {
    param([string]$RuntimeRoot)
    $path = Join-Path $RuntimeRoot 'script/smoke_main.lua'
    $lines = @(
        'package.path = "./?.lua;./?.lc"',
        'require("luacommon/msg")',
        'local function noop() end',
        'timers = { handle_timer = noop, LOGIN_CLIENT_HEARTBEAT = 1 }',
        'dbclient = { message_handler = noop }',
        'bfclient = { message_handler = noop, can_bf_save_player = noop }',
        'gmclient = { message_handler = noop }',
        'logclient = { message_handler = noop }',
        'ctrl_mng = { ctrl_lua_handler = noop }',
        'player_mng = { kick_all_player = noop }',
        'states = { state_msg = noop }',
        'login_mng = { set_max_user_num = noop }',
        'gmsvr = { message_handler = noop }',
        'function on_server_pre_stop() end',
        'function on_exit() end',
        'loginclient = {}',
        'function loginclient.message_handler(_ar, _msg, _size, _packet_type, _dpid)',
        '    if _packet_type == msg.LG_TYPE_CONNECT then',
        '        local ar = LAr:c_new()',
        '        ar:flush_before_send(msg.GL_TYPE_LOGIN)',
        '        ar:write_uint(g_gamesvr:c_get_server_id())',
        '        ar:write_string(g_gamesvr:c_get_server_ip())',
        '        ar:write_uint(g_gamesvr:c_get_server_port())',
        '        ar:write_uint(g_playermng:c_get_count())',
        '        ar:send_one_ar(g_loginclient, 0)',
        '        c_log("[SmokeMode] gameserver registered to loginserver")',
        '    end',
        'end',
        'gamesvr = {}',
        'function gamesvr.message_handler(_ar, _msg, _size, _packet_type, _dpid)',
        '    c_log("[SmokeMode] game packet %d", _packet_type)',
        '    if _packet_type == msg.PT_CERTIFY then',
        '        local ar = LAr:c_new()',
        '        ar:flush_before_send(msg.ST_CERTIFY_RESULT)',
        '        ar:write_ushort(0)',
        '        ar:write_int(1)',
        '        ar:send_one_ar(g_gamesvr, _dpid)',
        '        c_log("[SmokeMode] gameserver certify accepted")',
        '    end',
        'end'
    )
    [IO.File]::WriteAllText($path, [string]::Join("`r`n", $lines), $script:Utf8NoBom)
}

function Patch-WindowsLuaEpoch {
    param([string]$CommonRoot)
    $utilsPath = Join-Path $CommonRoot 'utils.lua'
    if (Test-Path -LiteralPath $utilsPath) {
        $utils = (Read-TextSafe $utilsPath).Replace('local zone = os_time({ year = 1970, month = 1, day = 1, hour = 0,  min = 0, sec =0 })', 'local zone = os_time({ year = 1970, month = 1, day = 1, hour = 0, min = 0, sec = 0 }) or 0')
        [IO.File]::WriteAllText($utilsPath, $utils, $script:Utf8NoBom)
    }
    $eventPath = Join-Path $CommonRoot 'event/event_god_divination/event_god_divination.lua'
    if (Test-Path -LiteralPath $eventPath) {
        $event = (Read-TextSafe $eventPath).Replace('ZONE_TIME = ZONE_TIME or os_time({ year = 1970, month = 1, day = 1, hour = 0, min = 0, sec =0 })', 'ZONE_TIME = ZONE_TIME or (os_time({ year = 1970, month = 1, day = 1, hour = 0, min = 0, sec = 0 }) or 0)')
        [IO.File]::WriteAllText($eventPath, $event, $script:Utf8NoBom)
    }
}

function Patch-RobotLoginUrl {
    param([string]$RuntimeRoot, [string]$StubUrl)
    $path = Join-Path $RuntimeRoot 'script/luacommon/net/http_mng.lua'
    if (-not (Test-Path -LiteralPath $path)) { throw "robot HTTP module not found: $path" }
    $patched = [regex]::Replace((Read-TextSafe $path), '(?m)^\s*TEST_LOGIN_URL\s*=\s*.*$', 'TEST_LOGIN_URL = "' + $StubUrl + '"')
    [IO.File]::WriteAllText($path, $patched, $script:Utf8NoBom)
}

function Patch-GameDbClient {
    param([string]$RuntimeRoot, [string]$OverridePath)
    $path = Join-Path $RuntimeRoot 'script/net/dbclient.lua'
    $override = Read-TextSafe $OverridePath
    $script:PatchCount = 0
    $pattern = '(?ms)^function\s+send_certify_to_db\s*\([^\r\n]*\)\s*\r?\n.*?^end\s*$'
    $patched = [regex]::Replace((Read-TextSafe $path), $pattern, { param($match) $script:PatchCount++; return $override.TrimEnd() })
    if ($script:PatchCount -ne 1) { throw "expected one send_certify_to_db function, found $($script:PatchCount)" }
    [IO.File]::WriteAllText($path, $patched, $script:Utf8NoBom)
}

function Patch-LoginServerSmokeConfig {
    param([string]$RuntimeRoot, [int]$GamePort = 0)
    $path = Join-Path $RuntimeRoot 'script/net/player_login_server.lua'
    $content = Read-TextSafe $path
    $init = [string]::Join("`r`n", @(
        'g_server_list = { [1] = { GameServerId = 1, Ip = "127.0.0.1", IsOpen = true, Name = "AServer Smoke", Port = ' + $GamePort + ', ServerStartDate = "2014/11/1", Status = 0, ZoneId = 1 } }',
        'g_server_list_str = "smoke"',
        'g_gamesvr_cfg_version = os.time()',
        'g_zone_list = {}',
        'g_white_list = { ["127.0.0.1"] = true }',
        'c_log("[player_login_server](on_server_list_response) update server list succ (SmokeMode)")'
    ))
    $content = $init + "`r`n" + $content
    $guard = [string]::Join("`r`n", @(
        '    if true then',
        '    g_server_list = {',
        '        [1] = {',
        '            GameServerId = 1,',
        '            Ip = "127.0.0.1",',
        '            IsOpen = true,',
        '            Name = "AServer Smoke",',
        "            Port = $GamePort,",
        '            ServerStartDate = "2014/11/1",',
        '            Status = 0,',
        '            ZoneId = 1,',
        '        },',
        '    }',
        '    g_server_list_str = "smoke"',
        '    g_gamesvr_cfg_version = os.time()',
        '    g_zone_list = {}',
        '    g_white_list = { ["127.0.0.1"] = true }',
        '    c_log("[player_login_server](on_server_list_response) update server list succ (SmokeMode)")',
        '    return',
        '    end'
    ))
    $patched = [regex]::Replace($content, '(?m)^function\s+check_gamesvr_cfg\(\)\s*$', { param($match) return $match.Value + "`r`n" + $guard })
    [IO.File]::WriteAllText($path, $patched, $script:Utf8NoBom)
}

function Copy-LuaRuntime {
    param([string]$SourceRoot, [string]$DestinationRoot, [int]$GamePort = 0)
    $sourceServer = Join-Path $script:RepoRoot 'server'
    $commonSource = Join-Path $SourceRoot 'server/luacommon'
    $commonDestination = Join-Path $DestinationRoot 'common/luacommon'
    Copy-DirectoryContents $commonSource $commonDestination
    Patch-WindowsLuaEpoch $commonDestination
    foreach ($service in @('logserver','loginserver','gameserver','robotclient')) {
        $runtimeRoot = Join-Path $DestinationRoot $service
        Copy-DirectoryContents (Join-Path $sourceServer "$service/UnitTest/script") (Join-Path $runtimeRoot 'script')
        Resolve-LinkPlaceholders $runtimeRoot $commonDestination
        Copy-DirectoryContents (Join-Path $commonDestination 'setting/resource') (Join-Path $runtimeRoot 'resource')
        New-Item -ItemType Directory -Force -Path (Join-Path $runtimeRoot "log/$service") | Out-Null
        Patch-LuaSourcePath (Join-Path $runtimeRoot 'script')
        if ($service -eq 'gameserver') { Patch-GameSmokeMain $runtimeRoot; Patch-GameOversizedSettings $runtimeRoot; Create-GameSmokeMain $runtimeRoot }
        if ($service -eq 'robotclient') { Patch-RobotHttpDebug $runtimeRoot }
        $unitRoot = Join-Path $sourceServer "$service/UnitTest"
        foreach ($file in @('test.nav','test.navmesh')) { $sourceFile=Join-Path $unitRoot $file; if(Test-Path -LiteralPath $sourceFile){Copy-Item -LiteralPath $sourceFile -Destination $runtimeRoot -Force} }
        if ($service -eq 'loginserver') {
            $mysqlTemplate = Join-Path $unitRoot 'mysql.lua.svn'
            if (Test-Path -LiteralPath $mysqlTemplate) { Copy-Item -LiteralPath $mysqlTemplate -Destination (Join-Path $runtimeRoot 'mysql.lua') -Force }
        }
        $script:RuntimeRoots[$service] = $runtimeRoot
    }
    Patch-RobotLoginUrl $script:RuntimeRoots.robotclient $script:StubLoginUrl
    Patch-RobotSmokeLogin $script:RuntimeRoots.robotclient
    Copy-Item -LiteralPath (Join-Path $script:RepoRoot 'tests/robot_smoke/runtime/login_db_service.lua') -Destination (Join-Path $script:RuntimeRoots.loginserver 'script/db/db_service.lua') -Force
    Patch-GameDbClient $script:RuntimeRoots.gameserver (Join-Path $script:RepoRoot 'tests/robot_smoke/runtime/game_dbclient_smoke.lua')
    Patch-LoginServerSmokeConfig $script:RuntimeRoots.loginserver $GamePort
    return $script:RuntimeRoots
}

function Set-LuaTableFieldText {
    param([string]$Text, [string]$TableName, [string]$FieldName, [string]$Value)
    $lines = $Text -split "`r?`n", -1
    $start = -1
    for ($i=0; $i -lt $lines.Length; $i++) {
        if ($lines[$i] -match ('^\s*'+[regex]::Escape($TableName)+'\s*=\s*\{')) {$start=$i;break}
        if ($lines[$i] -match ('^\s*'+[regex]::Escape($TableName)+'\s*=\s*$') -and ($i+1)-lt $lines.Length -and $lines[$i+1] -match '^\s*\{\s*$') {$start=$i;break}
    }
    if ($start -lt 0) { throw "Lua table not found: $TableName" }
    $depth=0;$fieldIndex=-1;$closeIndex=-1
    for($i=$start;$i -lt $lines.Length;$i++) {
        $code=$lines[$i] -replace '--.*$'; $depth += ([regex]::Matches($code,'\{')).Count - ([regex]::Matches($code,'\}')).Count
        if($i -gt $start -and $depth -eq 1 -and $lines[$i] -match ('^\s*'+[regex]::Escape($FieldName)+'\s*=')){$fieldIndex=$i}
        if($i -gt $start -and $depth -eq 0){$closeIndex=$i;break}
    }
    if($closeIndex -lt 0){throw "Lua table close not found: $TableName"}
    if($fieldIndex -ge 0){$indent=[regex]::Match($lines[$fieldIndex],'^\s*').Value;$lines[$fieldIndex]=$indent+$FieldName+' = '+$Value+','}
    else{$lines=$lines[0..($closeIndex-1)]+@('    '+$FieldName+' = '+$Value+',')+$lines[$closeIndex..($lines.Length-1)]}
    return [string]::Join("`r`n",$lines)
}

function New-SmokeConfig {
    param([string]$TemplatePath,[string]$OutputPath,[hashtable]$Replacements)
    $text=Read-TextSafe $TemplatePath
    foreach($key in $Replacements.Keys){$parts=$key.Split('.',2);if($parts.Length -eq 2){$text=Set-LuaTableFieldText $text $parts[0] $parts[1] ([string]$Replacements[$key])}else{$text=Set-LuaTableFieldText $text 'AppConfig' $parts[0] ([string]$Replacements[$key])}}
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $OutputPath) | Out-Null
    [IO.File]::WriteAllText($OutputPath,$text,$script:Utf8NoBom)
}

function Start-ServiceProcess {
    param([string]$Name,[string]$ExePath,[string]$WorkingDirectory,[string]$ConfigPath,[string]$StdoutPath,[string]$StderrPath)
    if(-not(Test-Path -LiteralPath $ExePath -PathType Leaf)){throw "executable not found: $ExePath"}
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $StdoutPath) | Out-Null
    $quotedConfigPath='"'+$ConfigPath.Replace('"','\"')+'"'
    $process=Start-Process -FilePath $ExePath -WorkingDirectory $WorkingDirectory -ArgumentList @('-c',$quotedConfigPath) -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath -PassThru
    $script:StartedProcesses[$Name]=$process
    return $process
}

function Start-HttpStub {
    param([int]$Port,[int]$GamePort,[string]$LogPath,[string]$AppKey)
    $stubScript = {
        param($ListenerPort,$ConfiguredGamePort,$RequestLog,$ConfiguredAppKey)
        $ErrorActionPreference='Stop'
        function Stub-Md5([string]$Value){$md5=New-Object Security.Cryptography.MD5CryptoServiceProvider;try{return([BitConverter]::ToString($md5.ComputeHash([Text.Encoding]::UTF8.GetBytes($Value)))).Replace('-','').ToLowerInvariant()}finally{$md5.Dispose()}}
        function Stub-Log([string]$Line){[IO.File]::AppendAllText($RequestLog,([DateTime]::UtcNow.ToString('o')+' '+$Line+"`r`n"))}
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $RequestLog) | Out-Null
        [IO.File]::WriteAllText($RequestLog,'')
        $listener = New-Object System.Net.Sockets.TcpListener([Net.IPAddress]::Parse('127.0.0.1'), $ListenerPort)
        $listener.Start()
        Stub-Log ('READY '+$ListenerPort)
        try {
            while ($true) {
                $client = $listener.AcceptTcpClient()
                $stream = $null
                $reader = $null
                try {
                    $stream = $client.GetStream()
                    $reader = New-Object IO.StreamReader($stream, [Text.Encoding]::ASCII, $false, 4096, $true)
                    $requestLine = $reader.ReadLine()
                    if ([string]::IsNullOrEmpty($requestLine)) { continue }
                    $headers = @{}
                    while ($true) {
                        $line = $reader.ReadLine()
                        if ($null -eq $line -or $line -eq '') { break }
                        $separator = $line.IndexOf(':')
                        if ($separator -gt 0) { $headers[$line.Substring(0,$separator).Trim().ToLowerInvariant()] = $line.Substring($separator + 1).Trim() }
                    }
                    $requestParts = $requestLine.Split(' ')
                    $method = $requestParts[0]
                    $target = $requestParts[1]
                    $pathOnly = ($target -split '\?', 2)[0]
                    $contentLength = 0
                    if ($headers.ContainsKey('content-length')) { $contentLength = [int]$headers['content-length'] }
                    $bodyChars = New-Object char[] $contentLength
                    $offset = 0
                    while ($offset -lt $contentLength) {
                        $read = $reader.Read($bodyChars, $offset, $contentLength - $offset)
                        if ($read -le 0) { break }
                        $offset += $read
                    }
                    $requestBody = -join $bodyChars
                    Stub-Log ($method + ' ' + $pathOnly + ' ' + $requestBody)
                    if ($pathOnly -eq '/inc_login.php') {
                        $timestamp=[int][double](([DateTime]::UtcNow-[DateTime]'1970-01-01').TotalSeconds)
                        $uid=123456789
                        $sign=Stub-Md5("$uid|$timestamp&1000002|$ConfiguredAppKey")
                        $body='{"uid":'+$uid+',"sign":"'+$sign+'","timestamp":"'+$timestamp+'","p_uid":"aserver-smoke","channelId":"smoke","channelLable":"smoke"}'
                        $contentType='application/json; charset=utf-8'
                    } elseif ($pathOnly -eq '/server_list.ini') {
                        $body="return {[1]={GameServerId=1,Ip='127.0.0.1',IsOpen=true,Name='AServer Smoke',Port=$ConfiguredGamePort,ServerStartDate='2014/11/1',Status=0,ZoneId=1}}"
                        $contentType='text/plain; charset=utf-8'
                    } elseif ($pathOnly -eq '/zone_list.ini') {
                        $body="return {{MaxZoneInGroup=99,MinZoneInGroup=1,ZoneGroupId=1,ZoneGroupName='smoke'}}"
                        $contentType='text/plain; charset=utf-8'
                    } elseif ($pathOnly -eq '/white_list.ini') {
                        $body="return {['127.0.0.1']=true}"
                        $contentType='text/plain; charset=utf-8'
                    } else {
                        $body='{}'; $contentType='application/json; charset=utf-8'
                    }
                    $bodyBytes=[Text.Encoding]::UTF8.GetBytes($body)
                    $headerText="HTTP/1.1 200 OK`r`nContent-Type: $contentType`r`nContent-Length: $($bodyBytes.Length)`r`nConnection: close`r`n`r`n"
                    $headerBytes=[Text.Encoding]::ASCII.GetBytes($headerText)
                    $stream.Write($headerBytes, 0, $headerBytes.Length)
                    $stream.Write($bodyBytes, 0, $bodyBytes.Length)
                    $stream.Flush()
                    Stub-Log ('RESP ' + $bodyBytes.Length)
                } catch { Stub-Log ('ERROR '+$_.Exception.Message) }
                finally {
                    if ($null -ne $reader) { $reader.Dispose() }
                    if ($null -ne $stream) { $stream.Dispose() }
                    $client.Close()
                }
            }
        } finally { $listener.Stop() }
    }
    return Start-Job -ScriptBlock $stubScript -ArgumentList $Port,$GamePort,$LogPath,$AppKey
}

function Stop-HttpStub {if($null -ne $script:StubJob){try{Stop-Job -Job $script:StubJob -ErrorAction SilentlyContinue | Out-Null}catch{};try{Remove-Job -Job $script:StubJob -Force -ErrorAction SilentlyContinue | Out-Null}catch{};$script:StubJob=$null}}
function Write-Snapshots {param([string]$Root);try{netstat -ano | Out-File (Join-Path $Root 'ports.txt') -Encoding UTF8}catch{};try{$script:Ports|ConvertTo-Json -Depth 3|Set-Content (Join-Path $Root 'ports.json') -Encoding UTF8}catch{};try{Get-Process|Select Id,ProcessName,StartTime -ErrorAction SilentlyContinue|ConvertTo-Json -Depth 3|Set-Content (Join-Path $Root 'processes.json') -Encoding UTF8}catch{}}
function Stop-ProcessTree {
    param([int]$ProcessId)
    $children=@(Get-CimInstance Win32_Process -Filter ("ParentProcessId={0}" -f $ProcessId) -ErrorAction SilentlyContinue)
    foreach($child in $children){Stop-ProcessTree ([int]$child.ProcessId)}
    try { $process=Get-Process -Id $ProcessId -ErrorAction SilentlyContinue; if($null -ne $process){Stop-Process -Id $ProcessId -Force -ErrorAction SilentlyContinue} } catch {}
}
function Stop-StartedProcesses {foreach($entry in $script:StartedProcesses.GetEnumerator()){Stop-ProcessTree ([int]$entry.Value.Id)};$script:StartedProcesses.Clear() | Out-Null}

function Invoke-ValidateOnly {
    $root=Join-Path ([IO.Path]::GetTempPath()) ('AServer-smoke-validate-'+[Guid]::NewGuid().ToString('N'))
    try{New-Item -ItemType Directory -Force -Path $root|Out-Null;$pathCode=Resolve-SmokeInputs;if($pathCode -ne 0){return $pathCode};$port=Get-FreeTcpPort;if($port -le 0){throw 'invalid free port'};Add-Check 'validate_port' $true ("allocated $port");$template=Join-Path $root 'template.lua';$config=Join-Path $root 'config.lua';[IO.File]::WriteAllText($template,"AppConfig = {`r`n    Daemon = 1,`r`n}",$script:Utf8NoBom);New-SmokeConfig $template $config @{'AppConfig.Daemon'='0';'AppConfig.SmokeMode'='1'};$configText=Read-TextSafe $config;if($configText -notmatch 'Daemon\s*=\s*0' -or $configText -notmatch 'SmokeMode\s*=\s*1'){throw 'generated config mismatch'};Add-Check 'validate_config' $true $config;$runtime=Join-Path $root 'runtime';$script:StubLoginUrl='http://127.0.0.1:'+$port+'/inc_login.php';Copy-LuaRuntime $AhxtRoot $runtime|Out-Null;if(-not(Test-Path (Join-Path $runtime 'loginserver/script/luacommon') -PathType Container)){throw 'luacommon link was not resolved'};if(-not(Test-Path (Join-Path $runtime 'gameserver/resource') -PathType Container)){throw 'resource runtime was not copied'};Add-Check 'validate_lua_runtime' $true $runtime;$sign=Get-Md5Hex '123456789|1700000000&1000002|smoke-test-key';if($sign.Length -ne 32){throw 'invalid MD5 sign length'};Add-Check 'validate_sign' $true $sign;return 0}catch{Add-Check 'validate_failure' $false $_.Exception.Message;return 1}finally{if(Test-Path $root){Remove-Item $root -Recurse -Force -ErrorAction SilentlyContinue}}}

function Invoke-Smoke {
    $script:TempRoot=Join-Path ([IO.Path]::GetTempPath()) ('AServer-smoke-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[Guid]::NewGuid().ToString('N').Substring(0,8));$script:StubLoginUrl='';$exitCode=1
    try{
        New-Item -ItemType Directory -Force -Path $script:TempRoot|Out-Null
        $pathCode=Resolve-SmokeInputs -RequireBuild
        if($pathCode -ne 0){$exitCode=$pathCode;return $pathCode}
        $ports=@{log=Get-FreeTcpPort;gameLogin=Get-FreeTcpPort;login=Get-FreeTcpPort;game=Get-FreeTcpPort;gm=Get-FreeTcpPort;stub=Get-FreeTcpPort;dummyDb=Get-FreeTcpPort;dummyBf=Get-FreeTcpPort};$script:Ports=$ports;$script:StubLoginUrl="http://127.0.0.1:$($ports.stub)/inc_login.php";$stubBaseUrl="http://127.0.0.1:$($ports.stub)/";$script:RuntimeRoots=@{};$script:LogRoots=@{};Copy-LuaRuntime $AhxtRoot (Join-Path $script:TempRoot 'runtime') $ports.game|Out-Null
        $configRoot=Join-Path $script:TempRoot 'config';$templates=@{logserver=Join-Path $RepoRoot 'server/logserver/UnitTest/logserver.lua.svn';loginserver=Join-Path $RepoRoot 'server/loginserver/UnitTest/loginserver.lua.svn';gameserver=Join-Path $RepoRoot 'server/gameserver/UnitTest/gameserver.lua.svn';robotclient=Join-Path $RepoRoot 'server/robotclient/UnitTest/robotclient.lua.svn'};$configs=@{logserver=Join-Path $configRoot 'logserver.lua';loginserver=Join-Path $configRoot 'loginserver.lua';gameserver=Join-Path $configRoot 'gameserver.lua';robotclient=Join-Path $configRoot 'robotclient.lua'}
        New-SmokeConfig $templates.logserver $configs.logserver @{'AppConfig.Daemon'='0';'AppConfig.SmokeMode'='1';'Log.LogPathPrefix'='"./log/log"';'LogSvr.IP'='"127.0.0.1"';'LogSvr.Port'=[string]$ports.log}
        New-SmokeConfig $templates.loginserver $configs.loginserver @{'AppConfig.Daemon'='0';'AppConfig.SmokeMode'='1';'Log.LogPathPrefix'='"./log/login"';'GameLoginSvr.MaxCon'='256';'GameLoginSvr.Crypt'='0';'GameLoginSvr.IP'='"127.0.0.1"';'GameLoginSvr.Port'=[string]$ports.gameLogin;'PlayerLoginSvr.MaxCon'='256';'PlayerLoginSvr.Crypt'='0';'PlayerLoginSvr.IP'='"127.0.0.1"';'PlayerLoginSvr.Port'=[string]$ports.login;'LogClient.IP'='"127.0.0.1"';'LogClient.Port'=[string]$ports.log;'AppConfig.GameSvrConfigUrl'='"'+$stubBaseUrl+'"'}
        New-SmokeConfig $templates.gameserver $configs.gameserver @{'AppConfig.Daemon'='0';'AppConfig.SmokeMode'='1';'Log.LogPathPrefix'='"./log/game"';'Lua.MainFile'='"smoke_main.lua"';'GameSvr.MaxCon'='256';'GmSvr.MaxCon'='256';'GmSvr.IP'='"127.0.0.1"';'GmSvr.Port'=[string]$ports.gm;'GameSvr.Crypt'='0';'GameSvr.IP'='"127.0.0.1"';'GameSvr.Port'=[string]$ports.game;'GameSvr.ServerId'='1';'DBClient.IP'='"127.0.0.1"';'DBClient.Port'=[string]$ports.dummyDb;'LoginClient.IP'='"127.0.0.1"';'LoginClient.Port'=[string]$ports.gameLogin;'LogClient.IP'='"127.0.0.1"';'LogClient.Port'=[string]$ports.log;'BFClient.BFList'=('{{IP="127.0.0.1",Port='+[string]$ports.dummyBf+'}}')}
        New-SmokeConfig $templates.robotclient $configs.robotclient @{'AppConfig.Daemon'='0';'AppConfig.SmokeMode'='1';'Log.LogPathPrefix'='"./log/robotclient"';'RobotClient.PlayerNum'='1';'RobotClient.MaxCon'='2';'RobotClient.RobotType'='1';'RobotClient.LoginSvrIP'='"127.0.0.1"';'RobotClient.LoginSvrPort'=[string]$ports.login;'RobotClient.GameSvrIP'='"127.0.0.1"';'RobotClient.GameSvrPort'=[string]$ports.game;'RobotClient.GameSvrID'='1';'RobotClient.LoginType'='"test"';'RobotClient.Crypt'='0'}
        $stubLog=Join-Path $script:TempRoot 'stub/stub.log';$script:StubJob=Start-HttpStub $ports.stub $ports.game $stubLog 'smoke-test-key';if(-not(Wait-TcpPort '127.0.0.1' $ports.stub 10)){throw 'local HTTP stub did not listen'}
        foreach($service in @('logserver','loginserver','gameserver','robotclient')){New-Item -ItemType Directory -Force -Path (Join-Path $script:RuntimeRoots[$service] "log/$service")|Out-Null};$env:PATH=$BuildRoot+';'+$env:PATH
        $logOut=Join-Path $script:TempRoot 'logs/logserver.stdout.log';$logErr=Join-Path $script:TempRoot 'logs/logserver.stderr.log';Start-ServiceProcess 'logserver' (Join-Path $BuildRoot 'logserver.exe') $script:RuntimeRoots.logserver $configs.logserver $logOut $logErr|Out-Null;$script:LogRoots.logserver=@((Join-Path $script:RuntimeRoots.logserver 'log'),$logOut,$logErr);if(-not(Wait-TcpPort '127.0.0.1' $ports.log 20)){throw 'logserver port did not open'};Add-Check 'logserver_started' $true ("127.0.0.1:$($ports.log)")
        $loginOut=Join-Path $script:TempRoot 'logs/loginserver.stdout.log';$loginErr=Join-Path $script:TempRoot 'logs/loginserver.stderr.log';Start-ServiceProcess 'loginserver' (Join-Path $BuildRoot 'loginserver.exe') $script:RuntimeRoots.loginserver $configs.loginserver $loginOut $loginErr|Out-Null;$script:LogRoots.loginserver=@((Join-Path $script:RuntimeRoots.loginserver 'log'),$loginOut,$loginErr);if(-not(Wait-TcpPort '127.0.0.1' $ports.login 20)){throw 'loginserver player port did not open'};if(-not(Wait-TcpPort '127.0.0.1' $ports.gameLogin 20)){throw 'loginserver game port did not open'};if(-not(Wait-LogPattern $script:LogRoots.loginserver 'update server list succ' 30)){throw 'loginserver did not load local server list'};Add-Check 'loginserver_started' $true ("player=$($ports.login), game=$($ports.gameLogin)")
        $gameOut=Join-Path $script:TempRoot 'logs/gameserver.stdout.log';$gameErr=Join-Path $script:TempRoot 'logs/gameserver.stderr.log';Start-ServiceProcess 'gameserver' (Join-Path $BuildRoot 'gameserver.exe') $script:RuntimeRoots.gameserver $configs.gameserver $gameOut $gameErr|Out-Null;$script:LogRoots.gameserver=@((Join-Path $script:RuntimeRoots.gameserver 'app_trace.log'),(Join-Path $script:RuntimeRoots.gameserver 'log'),$gameOut,$gameErr);if(-not(Wait-TcpPort '127.0.0.1' $ports.game 20)){throw 'gameserver port did not open'};if(-not(Wait-TcpPort '127.0.0.1' $ports.gm 20)){throw 'gameserver GM port did not open'};if(-not(Wait-LogPattern $script:LogRoots.loginserver 'gameserver login' 30)){throw 'gameserver did not register to loginserver'};if(-not(Wait-LogPattern $script:LogRoots.gameserver 'Start Main Loop' 30)){throw 'gameserver main loop did not start'};Add-Check 'gameserver_started' $true ("game=127.0.0.1:$($ports.game), gm=127.0.0.1:$($ports.gm)")
        $robotOut=Join-Path $script:TempRoot 'logs/robotclient.stdout.log';$robotErr=Join-Path $script:TempRoot 'logs/robotclient.stderr.log';Start-ServiceProcess 'robotclient' (Join-Path $BuildRoot 'robotclient.exe') $script:RuntimeRoots.robotclient $configs.robotclient $robotOut $robotErr|Out-Null;$script:LogRoots.robotclient=@((Join-Path $script:RuntimeRoots.robotclient 'log'),$robotOut,$robotErr);if(-not(Wait-LogPattern $script:LogRoots.robotclient '\[ROBOT\]\(on_loginsvr_connect\)' 30)){throw 'robot did not connect to loginserver'};Add-Check 'robot_login_connected' $true 'on_loginsvr_connect';if(-not(Wait-LogPattern @($stubLog) 'POST /inc_login\.php' 20)){throw 'robot HTTP login did not reach local stub'};Add-Check 'local_login_stub_response' $true $stubLog;if(-not(Wait-LogPattern $script:LogRoots.robotclient '\[ROBOT\]\(on_login_result\)account id' 30)){throw 'robot login result was not successful'};Add-Check 'robot_login_result' $true 'on_login_result';if(-not(Wait-LogPattern $script:LogRoots.robotclient '\[ROBOT\]\(on_gamesvr_connect\)' 30)){throw 'robot did not connect to gameserver'};Add-Check 'robot_game_connected' $true 'on_gamesvr_connect';if(-not(Wait-LogPattern $script:LogRoots.robotclient '\[ROBOT\]\(on_game_certify_result\) certify ok' 30)){Add-Check 'robot_game_certify' $false 'on_game_certify_result certify ok not observed';throw 'robot game certify did not succeed'};Add-Check 'robot_game_certify' $true 'on_game_certify_result certify ok'
        Start-Sleep -Seconds $HeartbeatSeconds;$dead=@($script:StartedProcesses.GetEnumerator()|Where-Object{$null -eq (Get-Process -Id $_.Value.Id -ErrorAction SilentlyContinue)});if($dead.Count -gt 0){throw ('process exited during heartbeat: '+(($dead|ForEach-Object Key)-join ', '))};Add-Check 'heartbeat_window' $true ("$HeartbeatSeconds seconds")
        $fatalPattern='Lua error|Lua traceback|stack traceback|load cfg_file fail|Lua Module Start Failed|class error|thread error|fatal error|assert\b|SmokeMode .*failed';$fatalServices=@();foreach($service in @('logserver','loginserver','gameserver','robotclient')){foreach($root in $script:LogRoots[$service]){if((Get-ServiceText $root)-match $fatalPattern){$fatalServices+=$service;break}}};if($fatalServices.Count -gt 0){throw ('startup fatal markers in: '+($fatalServices -join ', '))};Add-Check 'no_startup_fatal' $true 'no fatal/Lua/assert markers';$exitCode=0;return 0
    }catch{Add-Check 'runner_failure' $false $_.Exception.Message;return 1}
    finally{if($null -ne $script:TempRoot){if($exitCode -ne 0 -or $KeepTemp){Write-Snapshots $script:TempRoot};try{$script:Checks|ConvertTo-Json -Depth 5|Set-Content (Join-Path $script:TempRoot 'result.json') -Encoding UTF8}catch{}};Stop-HttpStub;Stop-StartedProcesses;$env:PATH=$script:OriginalPath;if($exitCode -eq 0 -and -not $KeepTemp -and $null -ne $script:TempRoot){Remove-Item $script:TempRoot -Recurse -Force -ErrorAction SilentlyContinue}elseif($null -ne $script:TempRoot){Write-Host "Smoke artifacts: $script:TempRoot"}}
}

if($ValidateOnly){$script:ResultCode=Invoke-ValidateOnly}else{$script:ResultCode=Invoke-Smoke}
exit ([int]$script:ResultCode)
