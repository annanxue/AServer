module( "db_like", package.seeall )

-- ------------------------------------
-- Local Lib Functions
-- ------------------------------------

-- ------------------------------------
-- SQL STRING FORMAT
-- ------------------------------------

local SELECT_LIKE_SQL = [[
    SELECT 
    `like_str` 
    FROM PLAYER_LIKE_TBL WHERE player_id = ?;
]]

local SELECT_LIKE_RESULT_TYPE = "s"

local MODIFY_LIKE_SQL = [[
    REPLACE INTO PLAYER_LIKE_TBL 
    ( `player_id`, `like_str` ) VALUES
    ( ?, ? );
]]

local MODIFY_LIKE_PARAM_TYPE = "is" 

-- ------------------------------------
-- Logic Functions
-- ------------------------------------

function do_get_player_like( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_LIKE_SQL, "i", _player_id, SELECT_LIKE_RESULT_TYPE ) < 0 then
        c_err( "[db_like](do_get_player_like) c_stmt_format_query on SELECT_LIKE_SQL failed, player_id: %d", _player_id )
        return
    end

    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return { like_str_ = "{}" }
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local like_str = database:c_stmt_get( "like_str" )
            return { like_str_ = like_str }
        end
    end
end

function do_save_player_like( _player )
    local player_id = _player.player_id_
    local like = _player.like_
    local like_str = like.like_str_
    
    return dbserver.g_db_character:c_stmt_format_modify( MODIFY_LIKE_SQL, 
    MODIFY_LIKE_PARAM_TYPE, player_id, like_str )
end
