module( "db_element", package.seeall )

local sformat = string.format

DEFAULT_PVP_ELEMENT = 0
DEFAULT_PVE_ELEMENT = 0
DEFAULT_MASTER_SKILLS_STR = ""
DEFAULT_PRACTICE_SKILLS_STR = ""

SELECT_ELEMENT_SQL = [[
    SELECT
    `pvp_element`,
    `pve_element`,
    `master_skills`,
    `practice_skills`
    from ELEMENT_TBL where player_id = ?;
]]

SELECT_ELEMENT_SQL_RESULT_TYPE = "iiss"

REPLACE_ELEMENT_SQL = [[
    REPLACE INTO ELEMENT_TBL ( `player_id`, `pvp_element`, `pve_element`, `master_skills`, `practice_skills` ) VALUES 
    (
        ?,
        ?,
        ?,
        ?,
        ?
    )
]]

REPLACE_ELEMENT_SQL_PARAM_TYPE = "iiiss"

function do_get_player_element( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_ELEMENT_SQL, "i", _player_id, SELECT_ELEMENT_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_element](do_get_player_element) query failed, player id: %d", _player_id )
        return
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_PVP_ELEMENT, DEFAULT_PVE_ELEMENT, DEFAULT_MASTER_SKILLS_STR, DEFAULT_PRACTICE_SKILLS_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local pvp_element = database:c_stmt_get( "pvp_element" )
            local pve_element = database:c_stmt_get( "pve_element" )
            local master_skills_str = database:c_stmt_get( "master_skills" )
            local practice_skills_str = database:c_stmt_get( "practice_skills" )
            return pvp_element, pve_element, master_skills_str, practice_skills_str
        end
    end
    return
end

function do_save_player_element( _player )
    local player_id           = _player.player_id_
    local pvp_element         = _player.pvp_element_
    local pve_element         = _player.pve_element_
    local master_skills_str   = _player.master_skills_str_
    local practice_skills_str = _player.practice_skills_str_
    local ret = dbserver.g_db_character:c_stmt_format_modify( REPLACE_ELEMENT_SQL
                                                            , REPLACE_ELEMENT_SQL_PARAM_TYPE
                                                            , player_id
                                                            , pvp_element
                                                            , pve_element
                                                            , master_skills_str
                                                            , practice_skills_str
                                                            )
    if ret < 0 then
        c_err( "[db_element](do_save_player_element) REPLACE_ELEMENT_SQL error, player_id: %d, ret: %d", player_id, ret )
    end
    c_log( "[db_element](do_save_player_element) player_id: %d, ret: %d", player_id, ret )
    return ret
end

