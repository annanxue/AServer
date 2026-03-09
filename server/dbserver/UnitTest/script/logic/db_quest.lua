module( "db_quest", package.seeall )

local sformat = string.format

DEFAULT_QUEST_STR = "{}"

SELECT_QUEST_SQL = [[
    SELECT
    `quest`
    from QUEST_TBL where player_id = ?;
]]

SELECT_QUEST_SQL_RESULT_TYPE = "s"

REPLACE_QUEST_SQL = [[
    REPLACE INTO QUEST_TBL ( `player_id`, `quest` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_QUEST_SQL_PARAM_TYPE = "is"

function do_get_player_quest( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_QUEST_SQL, "i", _player_id, SELECT_QUEST_SQL_RESULT_TYPE ) < 0 then
        c_err( "c_stmt_format_query on SELECT_QUEST_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_QUEST_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local quest_str = database:c_stmt_get( "quest" )
            return quest_str
        end
    end
    return nil
end

function do_save_player_quest( _player )
    local player_id = _player.player_id_
    local quest = _player.quest_
    return dbserver.g_db_character:c_stmt_format_modify( REPLACE_QUEST_SQL, REPLACE_QUEST_SQL_PARAM_TYPE, player_id, quest )
end

