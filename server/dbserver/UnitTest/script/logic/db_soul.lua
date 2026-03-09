module( "db_soul", package.seeall )

-- ------------------------------------
-- SQL STRING FORMAT
-- ------------------------------------
local SELECT_SOUL_SQL = [[
    SELECT
    `soul_main`,
    `soul_str`
    from PLAYER_SOUL_TBL where player_id = ?;
]]

local SELECT_SOUL_RESULT_TYPE = "is"

local MODIFY_SOUL_SQL = [[
    REPLACE INTO PLAYER_SOUL_TBL 
    ( `player_id`, `soul_main`, `soul_str` ) VALUES
    ( ?, ?, ? );
]]

local MODIFY_SOUL_PARAM_TYPE = "iis"

local DEFAULT_SOUL_MAIN = 1 
local DEFAULT_SOUL_STR = "1,0,0,0,0,0,0,0,0" 

-- ------------------------------------
-- Logic Functions
-- ------------------------------------

function do_get_player_soul( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_SOUL_SQL, "i", _player_id, SELECT_SOUL_RESULT_TYPE ) < 0 then
        c_err( "[db_soul](do_get_player_soul) c_stmt_format_query on SELECT_SOUL_SQL failed, player_id: %d", _player_id )
        return
    end

    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return { soul_main_ = DEFAULT_SOUL_MAIN, soul_str_ = DEFAULT_SOUL_STR }  
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local soul_main, soul_str = database:c_stmt_get( "soul_main", "soul_str" )
            return { soul_main_ = soul_main, soul_str_ = soul_str }
        end
    end
end

function do_save_player_soul( _player )
    local player_id = _player.player_id_
    local soul = _player.soul_
    local soul_main = soul.soul_main_
    local soul_str = soul.soul_str_
    return dbserver.g_db_character:c_stmt_format_modify( MODIFY_SOUL_SQL, MODIFY_SOUL_PARAM_TYPE, player_id, soul_main, soul_str )
end
