module( "db_mastery", package.seeall )

-- ------------------------------------
-- SQL STRING FORMAT
-- ------------------------------------

local SELECT_MASTERY_SQL = [[
    SELECT
    `mastery_main`,
    `mastery_str`
    from PLAYER_MASTERY_TBL where player_id = ?;
]]

local SELECT_MASTERY_RESULT_TYPE = "is"

local MODIFY_MASTERY_SQL = [[
    REPLACE INTO PLAYER_MASTERY_TBL 
    ( `player_id`, `mastery_main`, `mastery_str` ) VALUES
    ( ?, ?, ? );
]]

local MODIFY_MASTERY_PARAM_TYPE = "iis"

local DEFAULT_MASTERY_MAIN = 1
local DEFAULT_MASTERY_STR = ""

-- ------------------------------------
-- Logic Functions
-- ------------------------------------

function do_get_player_mastery( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_MASTERY_SQL, "i", _player_id, SELECT_MASTERY_RESULT_TYPE ) < 0 then
        c_err( "[db_mastery](do_get_player_mastery) c_stmt_format_query on SELECT_MASTERY_SQL failed, player_id: %d", _player_id )
        return 
    end

    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return { mastery_main_ = DEFAULT_MASTERY_MAIN, mastery_str_ = DEFAULT_MASTERY_STR } 
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local mastery_main, mastery_str = database:c_stmt_get( "mastery_main", "mastery_str" )
            return { mastery_main_ = mastery_main, mastery_str_ = mastery_str }
        end
    end
end

function do_save_player_mastery( _player )
    local player_id = _player.player_id_
    local mastery = _player.mastery_
    local mastery_main = mastery.mastery_main_
    local mastery_str = mastery.mastery_str_
    return dbserver.g_db_character:c_stmt_format_modify( MODIFY_MASTERY_SQL, MODIFY_MASTERY_PARAM_TYPE, player_id, mastery_main, mastery_str )
end
