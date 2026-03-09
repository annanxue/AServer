module( "db_monthly_signin_award", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_MONTHLY_SIGNIN_AWARD_STR = "{}"

SELECT_MONTHLY_SIGNIN_AWARD_SQL = [[
    SELECT
    `monthly_signin_award`
    from MONTHLY_SIGNIN_AWARD_TBL where player_id = ?;
]]

SELECT_MONTHLY_SIGNIN_AWARD_SQL_RESULT_TYPE = "s"

REPLACE_MONTHLY_SIGNIN_AWARD_SQL = [[
    REPLACE INTO MONTHLY_SIGNIN_AWARD_TBL ( `player_id`, `monthly_signin_award` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_MONTHLY_SIGNIN_AWARD_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_monthly_signin_award( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_MONTHLY_SIGNIN_AWARD_SQL, "i", _player_id, SELECT_MONTHLY_SIGNIN_AWARD_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_monthly_signin_award](do_get_player_monthly_signin_award) c_stmt_format_query on SELECT_MONTHLY_SIGNIN_AWARD_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_MONTHLY_SIGNIN_AWARD_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local monthly_signin_award_str = database:c_stmt_get( "monthly_signin_award" )
            return monthly_signin_award_str
        end
    end
    return nil
end

function do_save_player_monthly_signin_award( _player )
    local player_id = _player.player_id_
    local monthly_signin_award = _player.monthly_signin_award_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_MONTHLY_SIGNIN_AWARD_SQL, REPLACE_MONTHLY_SIGNIN_AWARD_SQL_PARAM_TYPE, player_id, monthly_signin_award )
    return ret
end

