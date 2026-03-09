module( "db_player_chat_gift", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_PLAYER_CHAT_GIFT_STR = "{}"

SELECT_PLAYER_CHAT_GIFT_SQL = [[
    SELECT
    `player_chat_gift`
    from PLAYER_CHAT_GIFT_TBL where player_id = ?;
]]

SELECT_PLAYER_CHAT_GIFT_SQL_RESULT_TYPE = "s"

REPLACE_PLAYER_CHAT_GIFT_SQL = [[
    REPLACE INTO PLAYER_CHAT_GIFT_TBL ( `player_id`, `player_chat_gift` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_PLAYER_CHAT_GIFT_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_player_chat_gift( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_PLAYER_CHAT_GIFT_SQL, "i", _player_id, SELECT_PLAYER_CHAT_GIFT_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_player_chat_gift](do_get_player_player_chat_gift) c_stmt_format_query on SELECT_PLAYER_CHAT_GIFT_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_PLAYER_CHAT_GIFT_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local player_chat_gift_str = database:c_stmt_get( "player_chat_gift" )
            return player_chat_gift_str
        end
    end
    return nil
end

function do_save_player_player_chat_gift( _player )
    local player_id = _player.player_id_
    local player_chat_gift = _player.player_chat_gift_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_PLAYER_CHAT_GIFT_SQL, REPLACE_PLAYER_CHAT_GIFT_SQL_PARAM_TYPE, player_id, player_chat_gift )
    return ret
end

