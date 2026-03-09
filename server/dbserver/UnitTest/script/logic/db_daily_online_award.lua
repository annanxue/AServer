module( "db_daily_online_award", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_DAILY_ONLINE_AWARD_STR = "{}"

SELECT_DAILY_ONLINE_AWARD_SQL = [[
    SELECT
    `daily_online_award`
    from DAILY_ONLINE_AWARD_TBL where player_id = ?;
]]

SELECT_DAILY_ONLINE_AWARD_SQL_RESULT_TYPE = "s"

REPLACE_DAILY_ONLINE_AWARD_SQL = [[
    REPLACE INTO DAILY_ONLINE_AWARD_TBL ( `player_id`, `daily_online_award` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_DAILY_ONLINE_AWARD_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_daily_online_award( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_DAILY_ONLINE_AWARD_SQL, "i", _player_id, SELECT_DAILY_ONLINE_AWARD_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_daily_online_award](do_get_player_daily_online_award) c_stmt_format_query on SELECT_DAILY_ONLINE_AWARD_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_DAILY_ONLINE_AWARD_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local daily_online_award_str = database:c_stmt_get( "daily_online_award" )
            return daily_online_award_str
        end
    end
    return nil
end

function do_save_player_daily_online_award( _player )
    local player_id = _player.player_id_
    local daily_online_award = _player.daily_online_award_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_DAILY_ONLINE_AWARD_SQL, REPLACE_DAILY_ONLINE_AWARD_SQL_PARAM_TYPE, player_id, daily_online_award )
    return ret
end

