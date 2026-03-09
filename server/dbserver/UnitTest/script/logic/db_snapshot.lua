module( "db_snapshot", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_SNAPSHOT_STR = "do local ret = {} return ret end"

SELECT_SNAPSHOT_SQL = [[
    SELECT
    `snapshot`
    from SNAPSHOT_TBL where player_id = ?;
]]

SELECT_SNAPSHOT_SQL_RESULT_TYPE = "s"

REPLACE_SNAPSHOT_SQL = [[
    REPLACE INTO SNAPSHOT_TBL ( `player_id`, `snapshot` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_SNAPSHOT_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_snapshot( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_SNAPSHOT_SQL, "i", _player_id, SELECT_SNAPSHOT_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_snapshot](do_get_player_snapshot) c_stmt_format_query on SELECT_SNAPSHOT_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_SNAPSHOT_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local snapshot_str = database:c_stmt_get( "snapshot" )
            return snapshot_str
        end
    end
    return nil
end

function do_save_player_snapshot( _player )
    local player_id = _player.player_id_
    local snapshot = _player.snapshot_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_SNAPSHOT_SQL, REPLACE_SNAPSHOT_SQL_PARAM_TYPE, player_id, snapshot )
    return ret
end

