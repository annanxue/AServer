module( "code_generator", package.seeall )
local sformat, tinsert, tconcat, rand, tremove, floor, abs = string.format, table.insert, table.concat, math.random, table.remove, math.floor, math.abs
local ostime, slen, ssub, slower = os.time, string.len, string.sub, string.lower

local function sub( _str, _length )
    local len = slen( _str )
    local start = rand(1, len - _length)
    local stop = start + _length
    return ssub( _str, start, stop ) 
end
-- generate active code
-- _server_id
-- _count
-- _length
function active_code( _server_id, _count, _length )
    local total = {}
    local timestamp = ostime()

    local seed = timestamp

    for i = 1, _count, 1 do
        local sign = c_md5( _server_id .. "+" .. seed )
        sign = slower( sign )
        local code = sub( sign, _length )
        if not total[code] then
            total[code] = 1
        else
            i = i - 1
        end

        seed = code
    end

    local sql_value_tbl = {}
    local txt_value_tbl = {}
    for code, _ in pairs( total ) do
        tinsert( sql_value_tbl, sformat("('%s', '%s')", code, _server_id) )
        tinsert( txt_value_tbl, sformat("%s %s", code, _server_id) )
    end

    local txt_file = sformat("active_code_server_%s.txt", _server_id)
    local file = io.open( txt_file, "w+" )

    file:write( tconcat( txt_value_tbl, "\n" ) )
    file:close()


    local sql_file = sformat("active_code_server_%s.sql", _server_id)

    file = io.open( sql_file, "w+"  )

    file:write( sformat("INSERT INTO ACTIVE_CODE( `active_code`, `server_id` ) VALUES %s;", tconcat( sql_value_tbl, "," )) )
    file:close()
end 

