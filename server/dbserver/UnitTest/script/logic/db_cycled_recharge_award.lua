module( "db_cycled_recharge_award", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_CYCLED_RECHARGE_AWARD_STR = "{}"

SELECT_CYCLED_RECHARGE_AWARD_SQL = [[
    SELECT
    `cycled_recharge_award`
    from CYCLED_RECHARGE_AWARD_TBL where player_id = ?;
]]

SELECT_CYCLED_RECHARGE_AWARD_SQL_RESULT_TYPE = "s"

REPLACE_CYCLED_RECHARGE_AWARD_SQL = [[
    REPLACE INTO CYCLED_RECHARGE_AWARD_TBL ( `player_id`, `cycled_recharge_award` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_CYCLED_RECHARGE_AWARD_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_cycled_recharge_award( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_CYCLED_RECHARGE_AWARD_SQL, "i", _player_id, SELECT_CYCLED_RECHARGE_AWARD_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_cycled_recharge_award](do_get_player_cycled_recharge_award) c_stmt_format_query on SELECT_CYCLED_RECHARGE_AWARD_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_CYCLED_RECHARGE_AWARD_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local cycled_recharge_award_str = database:c_stmt_get( "cycled_recharge_award" )
            return cycled_recharge_award_str
        end
    end
    return nil
end

function do_save_player_cycled_recharge_award( _player )
    local player_id = _player.player_id_
    local cycled_recharge_award = _player.cycled_recharge_award_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_CYCLED_RECHARGE_AWARD_SQL, REPLACE_CYCLED_RECHARGE_AWARD_SQL_PARAM_TYPE, player_id, cycled_recharge_award )
    return ret
end

