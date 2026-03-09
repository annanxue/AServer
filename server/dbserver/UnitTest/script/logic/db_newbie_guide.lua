module( "db_newbie_guide", package.seeall )

SELECT_NEWBIE_GUIDE_SQL = [[
    SELECT
    `newbie_guide_record`
    from NEWBIE_GUIDE_TBL where player_id = ?; 
]]

SELECT_NEWBIE_GUIDE_SQL_RES_TYPE = "s"

REPLACE_NEWBIE_GUIDE_SQL = [[
    REPLACE INTO NEWBIE_GUIDE_TBL
    ( `player_id`, `newbie_guide_record` ) VALUES
    (
       ?,
       ?
    )
]]

REPLACE_NEWBIE_GUIDE_PARAM_TYPE = "is"

function do_get_player_newbie_guide_record( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_NEWBIE_GUIDE_SQL, "i", _player_id, SELECT_NEWBIE_GUIDE_SQL_RES_TYPE ) < 0 then
        c_err( "[db_newbie_guide](do_get_player_newbie_guide_record) c_stmt_format_query on SELECT_NEWBIE_GUIDE_SQL failed, pid: %d.", _player_id )
        return nil
    end

    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return "{{},{},{}}"
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local record_str = database:c_stmt_get( "newbie_guide_record" )
            return record_str
        end
    end
end

function do_save_player_newbie_guide_record( _player )
    local player_id = _player.player_id_ 
    return dbserver.g_db_character:c_stmt_format_modify( REPLACE_NEWBIE_GUIDE_SQL, REPLACE_NEWBIE_GUIDE_PARAM_TYPE, player_id, _player.newbie_guide_record_str_ )
end

