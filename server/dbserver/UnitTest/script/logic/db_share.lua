module( "db_share", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_SHARE_STR = ""

SELECT_SHARE_SQL = [[
    SELECT
    `share`
    from SHARE_TBL where player_id = ?;
]]

SELECT_SHARE_SQL_RESULT_TYPE = "s"

REPLACE_SHARE_SQL = [[
    REPLACE INTO SHARE_TBL ( `player_id`, `share` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_SHARE_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_share( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_SHARE_SQL, "i", _player_id, SELECT_SHARE_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_share](do_get_player_share) c_stmt_format_query on SELECT_SHARE_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_SHARE_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local share_str = database:c_stmt_get( "share" )
            return share_str
        end
    end
    return nil
end

function do_save_player_share( _player )
    local player_id = _player.player_id_
    local share = _player.share_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_SHARE_SQL, REPLACE_SHARE_SQL_PARAM_TYPE, player_id, share )
    return ret
end

