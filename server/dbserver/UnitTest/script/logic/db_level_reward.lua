module( "db_level_reward", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_LEVEL_REWARD_STR = ""

SELECT_LEVEL_REWARD_SQL = [[
    SELECT
    `level_reward`
    from LEVEL_REWARD_TBL where player_id = ?;
]]

SELECT_LEVEL_REWARD_SQL_RESULT_TYPE = "s"

REPLACE_LEVEL_REWARD_SQL = [[
    REPLACE INTO LEVEL_REWARD_TBL ( `player_id`, `level_reward` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_LEVEL_REWARD_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_level_reward( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_LEVEL_REWARD_SQL, "i", _player_id, SELECT_LEVEL_REWARD_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_level_reward](do_get_player_level_reward) c_stmt_format_query on SELECT_LEVEL_REWARD_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_LEVEL_REWARD_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local level_reward_str = database:c_stmt_get( "level_reward" )
            return level_reward_str
        end
    end
    return nil
end

function do_save_player_level_reward( _player )
    local player_id = _player.player_id_
    local level_reward = _player.level_reward_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_LEVEL_REWARD_SQL, REPLACE_LEVEL_REWARD_SQL_PARAM_TYPE, player_id, level_reward )
    return ret
end

