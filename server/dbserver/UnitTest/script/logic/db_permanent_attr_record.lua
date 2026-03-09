module( "db_permanent_attr_record", package.seeall )

SELECT_PERMANENT_ATTR_RECORD_SQL = [[
    SELECT
    `permanent_attr_record`
    from PLAYER_PERMANENT_ATTR_RECORD_TBL where player_id = ?;
]]

SELECT_PERMANENT_ATTR_RECORD_SQL_RES_TYPE = "s"

REPLACE_PERMANENT_ATTR_RECORD_SQL = [[
    REPLACE INTO PLAYER_PERMANENT_ATTR_RECORD_TBL
    ( `player_id`, `permanent_attr_record` ) VALUES
    (
       ?,
       ?
    )
]]

REPLACE_PERMANENT_ATTR_RECORD_PARAM_TYPE = "is"

function do_get_player_permanent_attr_record( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_PERMANENT_ATTR_RECORD_SQL, "i", _player_id, SELECT_PERMANENT_ATTR_RECORD_SQL_RES_TYPE ) < 0 then
        c_err( "[db_permanent_attr_record](do_get_player_permanent_attr_record) c_stmt_format_query on SELECT_PERMANENT_ATTR_RECORD_SQL failed, player_id: %d", _player_id )
        return nil
    end

    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return "{}"
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local record_str = database:c_stmt_get( "permanent_attr_record" )
            return record_str
        end
    end
end

function do_save_player_permanent_attr_record( _player )
    local player_id = _player.player_id_ 
    return dbserver.g_db_character:c_stmt_format_modify( REPLACE_PERMANENT_ATTR_RECORD_SQL, 
           REPLACE_PERMANENT_ATTR_RECORD_PARAM_TYPE, player_id, _player.permanent_attr_record_str_ )
end

