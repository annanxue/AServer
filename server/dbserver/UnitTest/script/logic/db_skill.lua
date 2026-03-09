module( "db_skill", package.seeall )

local sformat = string.format

DEFAULT_JOB_SKILLS_STR = "{}"
DEFAULT_LEARNT_TALENTS_STR = "{}"
DEFAULT_UNLOCKED_TALENTS_STR = "{}"

SELECT_SKILL_SQL = [[
    SELECT
    `job_skills`,
    `learnt_talents`,
    `unlocked_talents`
    from SKILL_TBL where player_id = ?;
]]

SELECT_SKILL_SQL_RESULT_TYPE = "sss"

REPLACE_SKILL_SQL = [[
    REPLACE INTO SKILL_TBL ( `player_id`, `job_skills`, `learnt_talents`, `unlocked_talents` ) VALUES 
    (
        ?,
        ?,
        ?,
        ?
    )
]]

REPLACE_SKILL_SQL_PARAM_TYPE = "isss"

function do_get_player_skill_talent( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_SKILL_SQL, "i", _player_id, SELECT_SKILL_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_skill]c_stmt_format_query on SELECT_SKILL_SQL failed, player id: %d", _player_id )
        return
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_SKILL_STR, DEFAULT_LEARNT_TALENTS_STR, DEFAULT_UNLOCKED_TALENTS_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local job_skills_str = database:c_stmt_get( "job_skills" )
            local learnt_talents_str = database:c_stmt_get( "learnt_talents" )
            local unlocked_talents_str = database:c_stmt_get( "unlocked_talents" )
            return job_skills_str, learnt_talents_str, unlocked_talents_str
        end
    end
    return
end

function do_save_player_skill_talent( _player )
    local player_id = _player.player_id_
    local job_skills_str = _player.job_skills_str_
    local learnt_talents_str = _player.learnt_talents_str_
    local unlocked_talents_str = _player.unlocked_talents_str_
    local ret = dbserver.g_db_character:c_stmt_format_modify( REPLACE_SKILL_SQL
                                                            , REPLACE_SKILL_SQL_PARAM_TYPE
                                                            , player_id
                                                            , job_skills_str
                                                            , learnt_talents_str
                                                            , unlocked_talents_str
                                                            )
    if ret < 0 then
        c_err( "[db_skill](do_save_player_skill_talent) REPLACE_SKILL_SQL error, player_id: %d, ret: %d", player_id, ret )
    end
    c_log( "[db_skill](do_save_player_skill_talent) player_id: %d, ret: %d", player_id, ret )
    return ret
end

