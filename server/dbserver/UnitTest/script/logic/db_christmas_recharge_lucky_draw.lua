module( "db_christmas_recharge_lucky_draw", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_CHRISTMAS_RECHARGE_LUCKY_DRAW_STR = "{}"

SELECT_CHRISTMAS_RECHARGE_LUCKY_DRAW_SQL = [[
    SELECT
    `christmas_recharge_lucky_draw`
    from CHRISTMAS_RECHARGE_LUCKY_DRAW_TBL where player_id = ?;
]]

SELECT_CHRISTMAS_RECHARGE_LUCKY_DRAW_SQL_RESULT_TYPE = "s"

REPLACE_CHRISTMAS_RECHARGE_LUCKY_DRAW_SQL = [[
    REPLACE INTO CHRISTMAS_RECHARGE_LUCKY_DRAW_TBL ( `player_id`, `christmas_recharge_lucky_draw` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_CHRISTMAS_RECHARGE_LUCKY_DRAW_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_christmas_recharge_lucky_draw( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_CHRISTMAS_RECHARGE_LUCKY_DRAW_SQL, "i", _player_id, SELECT_CHRISTMAS_RECHARGE_LUCKY_DRAW_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_christmas_recharge_lucky_draw](do_get_player_christmas_recharge_lucky_draw) c_stmt_format_query on SELECT_CHRISTMAS_RECHARGE_LUCKY_DRAW_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_CHRISTMAS_RECHARGE_LUCKY_DRAW_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local christmas_recharge_lucky_draw_str = database:c_stmt_get( "christmas_recharge_lucky_draw" )
            return christmas_recharge_lucky_draw_str
        end
    end
    return nil
end

function do_save_player_christmas_recharge_lucky_draw( _player )
    local player_id = _player.player_id_
    local christmas_recharge_lucky_draw = _player.christmas_recharge_lucky_draw_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_CHRISTMAS_RECHARGE_LUCKY_DRAW_SQL, REPLACE_CHRISTMAS_RECHARGE_LUCKY_DRAW_SQL_PARAM_TYPE, player_id, christmas_recharge_lucky_draw )
    return ret
end

