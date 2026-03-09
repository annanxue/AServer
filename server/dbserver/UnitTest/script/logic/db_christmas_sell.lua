module( "db_christmas_sell", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_CHRISTMAS_SELL_STR = "{}"

SELECT_CHRISTMAS_SELL_SQL = [[
    SELECT
    `christmas_sell`
    from CHRISTMAS_SELL_TBL where player_id = ?;
]]

SELECT_CHRISTMAS_SELL_SQL_RESULT_TYPE = "s"

REPLACE_CHRISTMAS_SELL_SQL = [[
    REPLACE INTO CHRISTMAS_SELL_TBL ( `player_id`, `christmas_sell` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_CHRISTMAS_SELL_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_christmas_sell( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_CHRISTMAS_SELL_SQL, "i", _player_id, SELECT_CHRISTMAS_SELL_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_christmas_sell](do_get_player_christmas_sell) c_stmt_format_query on SELECT_CHRISTMAS_SELL_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_CHRISTMAS_SELL_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local christmas_sell_str = database:c_stmt_get( "christmas_sell" )
            return christmas_sell_str
        end
    end
    return nil
end

function do_save_player_christmas_sell( _player )
    local player_id = _player.player_id_
    local christmas_sell = _player.christmas_sell_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_CHRISTMAS_SELL_SQL, REPLACE_CHRISTMAS_SELL_SQL_PARAM_TYPE, player_id, christmas_sell )
    return ret
end

