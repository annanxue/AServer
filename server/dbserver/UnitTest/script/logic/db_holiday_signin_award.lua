module( "db_holiday_signin_award", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_HOLIDAY_SIGNIN_AWARD_STR = "{}"

SELECT_HOLIDAY_SIGNIN_AWARD_SQL = [[
    SELECT
    `holiday_signin_award`
    from HOLIDAY_SIGNIN_AWARD_TBL where player_id = ?;
]]

SELECT_HOLIDAY_SIGNIN_AWARD_SQL_RESULT_TYPE = "s"

REPLACE_HOLIDAY_SIGNIN_AWARD_SQL = [[
    REPLACE INTO HOLIDAY_SIGNIN_AWARD_TBL ( `player_id`, `holiday_signin_award` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_HOLIDAY_SIGNIN_AWARD_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_holiday_signin_award( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_HOLIDAY_SIGNIN_AWARD_SQL, "i", _player_id, SELECT_HOLIDAY_SIGNIN_AWARD_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_holiday_signin_award](do_get_player_holiday_signin_award) c_stmt_format_query on SELECT_HOLIDAY_SIGNIN_AWARD_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_HOLIDAY_SIGNIN_AWARD_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local holiday_signin_award_str = database:c_stmt_get( "holiday_signin_award" )
            return holiday_signin_award_str
        end
    end
    return nil
end

function do_save_player_holiday_signin_award( _player )
    local player_id = _player.player_id_
    local holiday_signin_award = _player.holiday_signin_award_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_HOLIDAY_SIGNIN_AWARD_SQL, REPLACE_HOLIDAY_SIGNIN_AWARD_SQL_PARAM_TYPE, player_id, holiday_signin_award )
    return ret
end

