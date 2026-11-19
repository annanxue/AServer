# AServer Robot Smoke Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在 Windows 上启动 AServer 的最小本机服务链，通过本地登录/配置 HTTP stub 和现有 RobotClient 验证 LoginServer、GameServer 的核心协议连接、认证消息和 10 秒稳定性窗口。

**Architecture:** 在生产配置之外创建临时运行时目录，复制 `ahxt_server/server/luacommon` 和 AServer 的服务脚本，动态生成 `127.0.0.1` 配置。新增默认关闭的 `SmokeMode`，只跳过外部 Auth/DB 的硬依赖并保留内部网络协议；PowerShell 运行器负责 stub、进程编排、日志断言、结果 JSON 和清理。

**Tech Stack:** Windows PowerShell 5+/PowerShell 7、.NET `System.Net.Sockets.TcpListener`、CMake/MSVC Release、LuaJIT、现有 C++ NetMng/RobotClient。

## Global Constraints

- 使用同级 `ahxt_server/server/luacommon` 作为临时 Lua 公共运行时，不将其永久复制进 AServer。
- 原始 `*.lua.svn` 配置不修改；所有配置改写发生在 `%TEMP%\\AServer-smoke-*`。
- `SmokeMode` 默认关闭；正常启动路径和外部服务认证逻辑保持不变。
- 本次冒烟测试不依赖真实 Auth、MySQL、BF、GM、支付服务，不把 TCP 建连误报为完整业务回归。
- 不写入真实账号、密码或远端服务凭据；不修改 Windows 防火墙或系统服务。
- 测试退出码：`0` 全部通过，`1` 代码/配置/协议失败，`2` 明确外部依赖阻塞。
- 失败时保留临时目录、`result.json`、stdout/stderr、应用日志、端口快照和进程快照；成功时清理临时目录。

---

### Task 1: 为 SmokeMode 解除硬阻塞的外部依赖

**Files:**

- Modify: `server_engine/base/app_curl_module.cpp:586-610`
- Modify: `server/gameserver/game_svr_module.cpp:351-445`
- Modify: `server/gameserver/db_client_module.cpp:114-147`
- Test: `build_msvc` Release targets and the smoke configuration fields

**Interfaces:**

- Consumes: top-level Lua `AppConfig.SmokeMode` and existing module configuration.
- Produces: default-off behavior; with `SmokeMode=1`, CurlModule does not call the remote `auth.php`, GameSvr listens immediately, DBClient does not start its reconnect thread.

- [ ] **Step 1: Add the CurlModule smoke branch.**

在 `CurlModule::app_class_init()` 读取 `SmokeMode`，保留 `curl_global_init()` 和 `g_http_mng->start(task_num)`，将远程调用包在默认分支中：

```cpp
int smoke_mode = 0;
APP_GET_NUMBER("SmokeMode", smoke_mode);

curl_global_init(CURL_GLOBAL_ALL);
if (smoke_mode == 0) {
    query_auth();
    query_lib_timeout();
    query_app_key(app_base_->L);
    decode_app_key();
} else {
    g_app_key = "smoke-test-key";
    LOG(2)("[CurlModule] SmokeMode enabled; skip remote auth/time/app-key queries");
}
```

`is_lib_time_out()` 只在 `smoke_mode == 0` 时检查；SmokeMode 分支直接启动现有 `HttpMng`。

- [ ] **Step 2: Start GameServer immediately only in SmokeMode.**

在 `GameSvrModule::app_class_init()` 读取 `SmokeMode`，完成现有 `set_start_param()` 后执行：

```cpp
if (smoke_mode != 0) {
    const int ret = g_gamesvr->start_server(max_con, port, crypt, ip, poll_wait_time, proc_thread_num);
    if (ret < 0) {
        ERR(2)("[GameSvrModule] SmokeMode start_server failed, port: %d", port);
        return false;
    }
    Ctrl::set_netmng(g_gamesvr);
    LOG(2)("===========GameSvr SmokeMode Listener Started===========");
} else {
    LOG(2)("===========WAIT GLOBAL SYSTEM DATA FROM DB to START SERVER===========");
}
```

正常模式继续等待 `dbclient.on_db_get_all_global_data()`，不改变生产启动顺序。

- [ ] **Step 3: Skip the GameServer DB reconnect thread only in SmokeMode.**

在 `DBClientModule::app_class_init()` 读取顶层 `SmokeMode`，在解析 `DBClient` 表之后增加：

```cpp
if (smoke_mode != 0) {
    LOG(2)("===========DBClient Module SmokeMode: external DB disabled===========");
    return true;
}
```

不能删除或改变正常 `g_connect_thread->init()` / `start()` 路径。

- [ ] **Step 4: Reconfigure and build the touched targets.**

Run:

```powershell
$env:PATH = "D:\vcpkg;$env:PATH"
cmake -S E:/C++/AHServer/AServer -B E:/C++/AHServer/AServer/build_msvc -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=D:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build E:/C++/AHServer/AServer/build_msvc --config Release --parallel 8
```

Expected: exit code `0`; `bin/Release/gameserver.exe`, `loginserver.exe`, `logserver.exe`, and `robotclient.exe` are present.

- [ ] **Step 5: Commit the isolated C++ behavior change.**

```powershell
cd E:/C++/AHServer/AServer
git add server_engine/base/app_curl_module.cpp server/gameserver/game_svr_module.cpp server/gameserver/db_client_module.cpp
git commit -m "test: add opt-in server smoke mode"
```

### Task 2: Add deterministic Lua smoke runtime overrides

**Files:**

- Create: `tests/robot_smoke/runtime/login_db_service.lua`
- Create: `tests/robot_smoke/runtime/game_dbclient_smoke.lua`
- Test: generated temporary login/game Lua trees

**Interfaces:**

- Consumes: copied AServer service scripts and `AppConfig.SmokeMode`.
- Produces: in-memory login account/token behavior and direct successful game certify callback without any MySQL socket.

- [ ] **Step 1: Create the in-memory login DB service.**

`tests/robot_smoke/runtime/login_db_service.lua` must define the functions used by `player_login_server.lua`, `game_login_server.lua`, `global.lua`, and `gm_cmd.lua`, with no `DataBase:c_new()`, `c_read_mysql()`, or module-load assertion. The core functions use this exact state shape:

```lua
module("db_service", package.seeall)

local accounts = {}
local function account_info(account_id)
    local value = accounts[account_id]
    if not value then return nil end
    return {
        account_id_ = account_id,
        machine_code_ = value.machine_code_,
        login_token_ = value.login_token_,
        queue_free_ = 1,
        forbid_login_ = 0,
        channel_account_ = value.channel_account_,
        channel_id_ = value.channel_id_,
        channel_label_ = value.channel_label_,
        is_prev_test_user_ = true,
        return_bind_diamond_ = 0,
        return_non_bind_diamond_ = 0,
    }
end

function init() return true end
function on_exit() accounts = {} end
function is_new_account(account_id) return accounts[account_id] == nil end
function update_account_info(account_id, machine_code, login_token, channel_account, channel_id, channel_label)
    accounts[account_id] = {
        machine_code_ = machine_code,
        login_token_ = login_token,
        channel_account_ = channel_account,
        channel_id_ = channel_id,
        channel_label_ = channel_label,
    }
    return 0
end
function invalid_account_token(account_id) return 0 end
function get_account_info(account_id)
    local value = account_info(account_id)
    return value ~= nil, value
end
function verify_account(account_id, login_token)
    local value = account_info(account_id)
    return value ~= nil and value.login_token_ == login_token
end
function get_active_info_by_account_id() return true end
function get_active_info_by_code() return true, 1, 0 end
function update_active_info() return true end
function get_account_info_by_machine_code() return {} end
function get_machine_forbid_end_time() return 0 end
function get_all_forbid_account() return {} end

function verify_and_save_account_when_enter_login(_dpid, _platform, _version, _account_id, _mcode, _sdk_time_stamp, _sdk_sign, _channel_account, _channel_id, _channel_label, _server_id, _device_name, _system_name, _active_code)
    local is_new = is_new_account(_account_id)
    local login_token = 24681357
    update_account_info(_account_id, _mcode, login_token, _channel_account, _channel_id, _channel_label)
    return true, login_code.LOGIN_CERTIFY_OK, login_token, is_new
end

function verify_account_when_enter_game(_account_id, _login_token)
    local value = account_info(_account_id)
    if not value then return login_code.LOGIN_ACCOUNT_NOT_FOUND, 0 end
    if value.login_token_ ~= _login_token then return login_code.LOGIN_ACCOUNT_TIMEOUT, 0 end
    return login_code.LOGIN_CERTIFY_OK, 1
end
```

The returned token is deterministic for the smoke run; the HTTP stub sign validates the SDK request separately, and the GameServer receives the same token from RobotClient.

- [ ] **Step 2: Create the GameServer DB smoke override.**

`tests/robot_smoke/runtime/game_dbclient_smoke.lua` contains the replacement body for `send_certify_to_db()` used in the copied `script/net/dbclient.lua`:

```lua
function send_certify_to_db(_dpid, _account_id)
    if AppConfig and AppConfig.SmokeMode == 1 then
        gamesvr.send_certify_code_to_client(_dpid, login_code.CERTIFY_OK)
        c_log("[dbclient] SmokeMode certify accepted account:%d", _account_id)
        return
    end
    local client_ip_str = g_gamesvr:c_get_player_ip(_dpid)
    local ar = g_ar
    ar:flush_before_send(msg.DB_TYPE_CERTIFY)
    ar:write_ulong(_dpid)
    ar:write_double(_account_id)
    ar:write_string(client_ip_str)
    ar:send_one_ar(g_dbclient, 0)
end
```

The runtime patch leaves all normal DB message functions intact. It is applied only to the temporary copy and only when generating a SmokeMode run.

- [ ] **Step 3: Validate the overrides are isolated.**

Run a PowerShell text check against the source tree and temporary-copy rules:

```powershell
$source = Get-Content E:/C++/AHServer/AServer/server/loginserver/UnitTest/script/db/db_service.lua -Raw
if ($source -match 'SmokeMode|smoke-test-key') { throw 'production login DB script was modified' }
```

Expected: no exception; only `tests/robot_smoke/runtime/*.lua` contains smoke-specific behavior.

- [ ] **Step 4: Commit the runtime override files.**

```powershell
cd E:/C++/AHServer/AServer
git add tests/robot_smoke/runtime/login_db_service.lua tests/robot_smoke/runtime/game_dbclient_smoke.lua
git commit -m "test: add isolated Lua smoke runtime overrides"
```

### Task 3: Implement the Windows smoke runner and local HTTP stub

**Files:**

- Create: `tests/robot_smoke/run_core_smoke.ps1`
- Modify: `README_VCPKG.md` with the tested invocation and exit-code meanings
- Test: `tests/robot_smoke/run_core_smoke.ps1 -ValidateOnly`

**Interfaces:**

- Consumes: `-RepoRoot`, `-AhxtRoot`, `-BuildRoot`, optional `-KeepTemp`, optional `-ValidateOnly`.
- Produces: `result.json`, per-service `stdout.log`/`stderr.log`, application logs, port/process snapshots, and process exit code `0/1/2`.

- [ ] **Step 1: Add deterministic helper functions.**

Implement these PowerShell functions with the exact signatures:

```powershell
function Get-FreeTcpPort { param() }
function Wait-TcpPort { param([string]$Address, [int]$Port, [int]$TimeoutSeconds = 20) }
function Copy-LuaRuntime { param([string]$SourceRoot, [string]$DestinationRoot) }
function New-SmokeConfig { param([string]$TemplatePath, [string]$OutputPath, [hashtable]$Replacements) }
function Start-ServiceProcess { param([string]$Name, [string]$ExePath, [string]$WorkingDirectory, [string]$ConfigPath, [string]$StdoutPath, [string]$StderrPath) }
function Add-Check { param([string]$Name, [bool]$Passed, [string]$Details) }
```

`Get-FreeTcpPort` binds an IPv4 `TcpListener` to `127.0.0.1:0`, reads the assigned port, stops the listener, and returns the integer. `Wait-TcpPort` retries a `TcpClient` connection once every 200 ms until the deadline. Every helper must throw a descriptive exception on invalid input instead of returning a false success.

- [ ] **Step 2: Resolve link placeholder files into the temporary runtime.**

`Copy-LuaRuntime` must:

1. Copy `E:/C++/AHServer/ahxt_server/server/luacommon` to `<temp>/common/luacommon`.
2. Copy `server/*/UnitTest/script` into `<temp>/<service>/script`.
3. Replace each one-line file whose trimmed content starts with `link ` by copying the referenced directory from the same service’s `UnitTest` location or from `<temp>/common/luacommon`.
4. Copy `ahxt_server/server/luacommon/setting/resource` to each service runtime’s `<temp>/<service>/resource`.
5. Patch every copied `main.lua` so its package-path branch is exactly `if g_debug or (AppConfig and AppConfig.SmokeMode == 1) then package.path = "./?.lua;./?.lc" else package.path = "./?.lc" end`, allowing Release binaries to load source Lua files only for the smoke run.
6. Replace copied `robotclient` `luacommon/net/http_mng.lua` `TEST_LOGIN_URL` with the local stub URL.
7. Replace copied login `script/db/db_service.lua` with `tests/robot_smoke/runtime/login_db_service.lua`.
8. Replace the copied game `script/net/dbclient.lua` `send_certify_to_db()` body using `game_dbclient_smoke.lua`.

- [ ] **Step 3: Generate four isolated configurations.**

Use `New-SmokeConfig` to generate `.lua` files with these exact values:

| Service | Listen/config values |
|---|---|
| logserver | `Daemon=0`, `LogSvr.IP=127.0.0.1`, `LogSvr.Port=$logPort`, `Log.LogPathPrefix='./log/log'`, `SmokeMode=1` |
| loginserver | `Daemon=0`, `GameLoginSvr.Port=$gameLoginPort`, `PlayerLoginSvr.Port=$loginPort`, `LogClient.IP=127.0.0.1`, `LogClient.Port=$logPort`, `GameSvrConfigUrl=$stubBaseUrl`, `SmokeMode=1` |
| gameserver | `Daemon=0`, `GameSvr.IP=127.0.0.1`, `GameSvr.Port=$gamePort`, `GameSvr.ServerId=1`, `LoginClient.IP=127.0.0.1`, `LoginClient.Port=$gameLoginPort`, `LogClient.IP=127.0.0.1`, `LogClient.Port=$logPort`, `SmokeMode=1` |
| robotclient | `Daemon=0`, `RobotClient.PlayerNum=1`, `RobotClient.RobotType=1`, `RobotClient.LoginSvrIP=127.0.0.1`, `RobotClient.LoginSvrPort=$loginPort`, `RobotClient.GameSvrIP=127.0.0.1`, `RobotClient.GameSvrPort=$gamePort`, `RobotClient.GameSvrID=1`, `RobotClient.LoginType='test'`, `SmokeMode=1` |

Create all `log/<name>` directories before starting a process because the existing `create_dir()` implementation only walks slash-delimited parent components.

- [ ] **Step 4: Implement the local HTTP stub with `TcpListener`.**

`Start-HttpStub` runs a background PowerShell job using `System.Net.Sockets.TcpListener` and writes request lines to `stub.log`. It must return:

- `/inc_login.php`: JSON with `uid=123456789`, current `timestamp`, `p_uid='aserver-smoke'`, `channelId='smoke'`, `channelLable='smoke'`, and MD5 lower-case `sign` for `123456789|<timestamp>&1000002|smoke-test-key`.
- `/server_list.ini`: `return {[1]={GameServerId=1,Ip='127.0.0.1',IsOpen=true,Name='AServer Smoke',Port=<gamePort>,ServerStartDate='2014/11/1',Status=0,ZoneId=1}}`.
- `/zone_list.ini`: `return {{MaxZoneInGroup=99,MinZoneInGroup=1,ZoneGroupId=1,ZoneGroupName='smoke'}}`.
- `/white_list.ini`: `return {['127.0.0.1']=true}`.

Set `Content-Type`, `Content-Length`, and `Connection: close` in every response. The stub must accept repeated requests until the runner stops its job.

- [ ] **Step 5: Implement process lifecycle and checks.**

Start in this order: logserver, wait for `$logPort`; loginserver, wait for `$loginPort` and `$gameLoginPort` plus `update server list succ`; gameserver, wait for `$gamePort` and `gameserver login`; robotclient.

Capture each process with `Start-Process -WorkingDirectory <serviceRoot> -ArgumentList @('-c', <configPath>) -RedirectStandardOutput ... -RedirectStandardError ... -PassThru`.

Use these check names and evidence:

```text
binaries
logserver_started
loginserver_started
gameserver_started
robot_login_connected
local_login_stub_response
robot_game_connected
robot_login_result
robot_game_certify
heartbeat_window
no_startup_fatal
```

For log matching, accept the existing messages `[ROBOT](on_loginsvr_connect)`, `[ROBOT](on_gamesvr_connect)`, `[ROBOT](on_login_result)account id`, `[ROBOT](on_game_certify_result) certify ok`, and `[dbclient] SmokeMode certify accepted`. Treat `Lua error`, `Lua traceback`, `assert`, `fatal error`, `load cfg_file fail`, `Lua Module Start Failed`, and unexpected process exit as failures. Do not treat expected optional-client connection retries as a pass condition.

- [ ] **Step 6: Write result files and cleanup behavior.**

Write each check as an object with `name`, `passed`, `details`, and ISO timestamp to `<temp>/result.json`. On failure, write `ports.json`, `processes.json`, and leave the directory. On success, print all `PASS` lines, stop the HTTP stub and service process trees, then remove the temporary directory unless `-KeepTemp` was passed.

Return `2` only when the binary check passes but a named external prerequisite is detected and the runner cannot enter SmokeMode; return `1` for all code/config/protocol failures.

- [ ] **Step 7: Add the validate-only self-test.**

`-ValidateOnly` must exercise `Get-FreeTcpPort`, `New-SmokeConfig` against a temporary one-line config, `Copy-LuaRuntime` link resolution, and MD5 sign generation without starting AServer. Run:

```powershell
pwsh -NoProfile -File E:/C++/AHServer/AServer/tests/robot_smoke/run_core_smoke.ps1 -ValidateOnly
```

Expected output contains `PASS validate_port`, `PASS validate_config`, `PASS validate_lua_runtime`, and exit code `0`.

- [ ] **Step 8: Document and commit the runner.**

Add this invocation to `README_VCPKG.md`:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File E:/C++/AHServer/AServer/tests/robot_smoke/run_core_smoke.ps1 -KeepTemp
```

Document that a successful run prints `PASS` checks and a failed run prints the preserved temp directory. Commit:

```powershell
cd E:/C++/AHServer/AServer
git add tests/robot_smoke/run_core_smoke.ps1 README_VCPKG.md
git commit -m "test: add Windows core robot smoke runner"
```

### Task 4: Run the smoke test, diagnose logs, and fix only observed defects

**Files:**

- Modify: only the source/config/runtime files identified by the first failing check
- Test: `tests/robot_smoke/run_core_smoke.ps1`
- Artifacts: temporary `result.json`, service logs, stub log, process/port snapshots

**Interfaces:**

- Consumes: Tasks 1-3 binaries, runtime overrides, and runner.
- Produces: repeatable exit code `0` or an explicit `1/2` diagnosis with preserved evidence.

- [ ] **Step 1: Build the complete Release set.**

```powershell
cmake --build E:/C++/AHServer/AServer/build_msvc --config Release --parallel 8
```

Expected: exit code `0` and all four required executables exist.

- [ ] **Step 2: Run the runner with evidence retention.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File E:/C++/AHServer/AServer/tests/robot_smoke/run_core_smoke.ps1 -KeepTemp
```

Expected successful checks include `robot_login_connected`, `local_login_stub_response`, `robot_game_connected`, `robot_login_result`, `robot_game_certify`, and `heartbeat_window`.

- [ ] **Step 3: Diagnose in dependency order.**

Read `<temp>/result.json` first, then the first failing service log. Apply fixes in this order: missing DLL/working directory; Lua link/runtime path; log directory creation; port/config mismatch; Curl/DB/Game SmokeMode; LoginServer server-list or account-token behavior; GameServer certify callback; RobotClient event handling. Do not relax a failed protocol assertion merely because TCP connected.

- [ ] **Step 4: Rebuild and rerun after every source fix.**

Use the same CMake and PowerShell commands. A fix is accepted only when the failing check passes and no earlier check regresses. Keep the final successful `result.json` path in the final report before cleanup.

- [ ] **Step 5: Commit verified fixes.**

```powershell
cd E:/C++/AHServer/AServer
git add server_engine/base/app_curl_module.cpp server/gameserver/game_svr_module.cpp server/gameserver/db_client_module.cpp tests/robot_smoke/runtime/login_db_service.lua tests/robot_smoke/runtime/game_dbclient_smoke.lua tests/robot_smoke/run_core_smoke.ps1 README_VCPKG.md
git commit -m "fix: make Windows robot smoke test pass"
```

### Task 5: Final verification and handoff

**Files:**

- Verify: `tests/robot_smoke/run_core_smoke.ps1`
- Verify: `README_VCPKG.md`
- Verify: current Git status and final commit history

- [ ] **Step 1: Run the validate-only check.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File E:/C++/AHServer/AServer/tests/robot_smoke/run_core_smoke.ps1 -ValidateOnly
```

Expected: exit code `0`.

- [ ] **Step 2: Run the full smoke test without retaining temp files.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File E:/C++/AHServer/AServer/tests/robot_smoke/run_core_smoke.ps1
```

Expected: exit code `0`, every required check is `PASS`, and the success temp directory is removed.

- [ ] **Step 3: Confirm no generated runtime artifacts are tracked.**

```powershell
cd E:/C++/AHServer/AServer
git status --short
git --no-pager log -5 --oneline
```

Expected: only intended source, test, documentation, and design/plan files are tracked; no `%TEMP%` runtime, logs, `result.json`, or process dump is inside the repository.

- [ ] **Step 4: Report exact evidence.**

The final report must state the build command, smoke command, exit codes, passed checks, any intentionally skipped external services, final commit IDs, and the preserved failure directory if the run did not pass.
