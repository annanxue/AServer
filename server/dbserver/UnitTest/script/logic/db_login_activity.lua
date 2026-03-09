module( "db_login_activity", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_LOGIN_ACTIVITY_STR = ""

SELECT_LOGIN_ACTIVITY_SQL = [[
    SELECT
    `login_activity`
    from LOGIN_ACTIVITY_TBL where player_id = ?;
]]

SELECT_LOGIN_ACTIVITY_SQL_RESULT_TYPE = "s"

REPLACE_LOGIN_ACTIVITY_SQL = [[
    REPLACE INTO LOGIN_ACTIVITY_TBL ( `player_id`, `login_activity` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_LOGIN_ACTIVITY_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_login_activity( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_LOGIN_ACTIVITY_SQL, "i", _player_id, SELECT_LOGIN_ACTIVITY_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_login_activity](do_get_player_login_activity) c_stmt_format_query on SELECT_LOGIN_ACTIVITY_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_LOGIN_ACTIVITY_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local login_activity_str = database:c_stmt_get( "login_activity" )
            return login_activity_str
        end
    end
    return nil
end

function do_save_player_login_activity( _player )
    local player_id = _player.player_id_
    local login_activity = _player.login_activity_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_LOGIN_ACTIVITY_SQL, REPLACE_LOGIN_ACTIVITY_SQL_PARAM_TYPE, player_id, login_activity )
    return ret
end

