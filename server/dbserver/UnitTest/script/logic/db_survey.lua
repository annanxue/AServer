module( "db_survey", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_SURVEY_STR = "{}"

SELECT_SURVEY_SQL = [[
    SELECT
    `survey`
    from SURVEY_TBL where player_id = ?;
]]

SELECT_SURVEY_SQL_RESULT_TYPE = "s"

REPLACE_SURVEY_SQL = [[
    REPLACE INTO SURVEY_TBL ( `player_id`, `survey` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_SURVEY_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_survey( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_SURVEY_SQL, "i", _player_id, SELECT_SURVEY_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_survey](do_get_player_survey) c_stmt_format_query on SELECT_SURVEY_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_SURVEY_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local survey_str = database:c_stmt_get( "survey" )
            return survey_str
        end
    end
    return nil
end

function do_save_player_survey( _player )
    local player_id = _player.player_id_
    local survey = _player.survey_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_SURVEY_SQL, REPLACE_SURVEY_SQL_PARAM_TYPE, player_id, survey )
    return ret
end
