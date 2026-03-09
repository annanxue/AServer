module( "db_title", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert, utils.serialize_table
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_TITLE_STR = "do return {} end"

SELECT_TITLE_SQL = [[
    SELECT
    `title`
    from TITLE_TBL where player_id = ?;
]]

SELECT_TITLE_SQL_RESULT_TYPE = "s"

REPLACE_TITLE_SQL = [[
    REPLACE INTO TITLE_TBL ( `player_id`, `title` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_TITLE_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_title( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_TITLE_SQL, "i", _player_id, SELECT_TITLE_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_title](do_get_player_title) c_stmt_format_query on SELECT_TITLE_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_TITLE_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local title_str = database:c_stmt_get( "title" )
            return title_str
        end
    end
    return nil
end

function do_save_player_title( _player )
    local player_id = _player.player_id_
    local title = _player.title_data_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_TITLE_SQL, REPLACE_TITLE_SQL_PARAM_TYPE, player_id, title )
    return ret
end

function add_player_title(_player_id, _title_id)
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_TITLE_SQL, "i", _player_id, SELECT_TITLE_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_title](do_get_player_title) c_stmt_format_query on SELECT_TITLE_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    local title_str = DEFAULT_TITLE_STR
    if rows == 1 and database:c_stmt_fetch() == 0 then
        title_str = database:c_stmt_get( "title" )
    end

    local title_tbl = loadstring(title_str)()
    title_tbl[_title_id] = true
    title_str = ser_table(title_tbl) 

    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_TITLE_SQL, REPLACE_TITLE_SQL_PARAM_TYPE, _player_id, title_str )

    local player = db_login.g_player_cache:get( _player_id )
    if player then
        player.title_data_ = title_str
    end
    return ret
end
                                                   
