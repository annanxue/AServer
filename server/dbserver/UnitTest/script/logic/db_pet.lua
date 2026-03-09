module( "db_pet", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_PET_STR = ""

SELECT_PET_SQL = [[
    SELECT
    `pet`
    from PET_TBL where player_id = ?;
]]

SELECT_PET_SQL_RESULT_TYPE = "s"

REPLACE_PET_SQL = [[
    REPLACE INTO PET_TBL ( `player_id`, `pet` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_PET_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_pet( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_PET_SQL, "i", _player_id, SELECT_PET_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_pet](do_get_player_pet) c_stmt_format_query on SELECT_PET_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_PET_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local pet_str = database:c_stmt_get( "pet" )
            return pet_str
        end
    end
    return nil
end

function do_save_player_pet( _player )
    local player_id = _player.player_id_
    local pet = _player.pet_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_PET_SQL, REPLACE_PET_SQL_PARAM_TYPE, player_id, pet )
    return ret
end

