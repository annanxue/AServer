module( "db_online_num", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_EGG_STR = ""

SELECT_EGG_SQL_RESULT_TYPE = "s"

REPLACE_ONLINE_SQL = [[
    REPLACE INTO ONLINE_TBL ( `online_time`, `online_num` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_ONLINE_SQL_PARAM_TYPE = "ii"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_save_player_online_num( _time, _num )
    c_err("[do_save_player_online_num] _time: %d, _num: %d", _time, _num)
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_ONLINE_SQL, REPLACE_ONLINE_SQL_PARAM_TYPE, _time, _num )
    return ret
end
