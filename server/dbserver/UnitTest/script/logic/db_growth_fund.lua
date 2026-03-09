module( "db_growth_fund", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_GROWTH_FUND_STR = "{}"

SELECT_GROWTH_FUND_SQL = [[
    SELECT
    `growth_fund`
    from GROWTH_FUND_TBL where player_id = ?;
]]

SELECT_GROWTH_FUND_SQL_RESULT_TYPE = "s"

REPLACE_GROWTH_FUND_SQL = [[
    REPLACE INTO GROWTH_FUND_TBL ( `player_id`, `growth_fund` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_GROWTH_FUND_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_growth_fund( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_GROWTH_FUND_SQL, "i", _player_id, SELECT_GROWTH_FUND_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_growth_fund](do_get_player_growth_fund) c_stmt_format_query on SELECT_GROWTH_FUND_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_GROWTH_FUND_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local growth_fund_str = database:c_stmt_get( "growth_fund" )
            return growth_fund_str
        end
    end
    return nil
end

function do_save_player_growth_fund( _player )
    local player_id = _player.player_id_
    local growth_fund = _player.growth_fund_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_GROWTH_FUND_SQL, REPLACE_GROWTH_FUND_SQL_PARAM_TYPE, player_id, growth_fund )
    return ret
end

