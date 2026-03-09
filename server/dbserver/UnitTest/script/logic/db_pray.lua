module( "db_pray", package.seeall )

SELECT_PRAY_SQL = [[
    SELECT
    `counters`
    from PRAY_TBL where player_id = ?;
]]
SELECT_PRAY_SQL_RESULT_TYPE = "s"

REPLACE_PRAY_SQL = [[
    REPLACE INTO PRAY_TBL 
    ( `player_id`, `counters` ) VALUES
    (
        ?,
        ?
    )
]]
REPLACE_PRAY_SQL_PARAM_TYPE = "is"

function do_get_player_pray( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_PRAY_SQL, "i", _player_id, SELECT_PRAY_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_pray] c_stmt_format_query on SELECT_PRAY_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        local pray_str = "{}"
        return pray_str
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local pray_str = database:c_stmt_get( "counters" )
            return pray_str
        end
    end
    return nil
end

function do_save_player_pray( _player )
    local player_id = _player.player_id_
    local pray_str = _player.pray_str_
    return dbserver.g_db_character:c_stmt_format_modify( REPLACE_PRAY_SQL, REPLACE_PRAY_SQL_PARAM_TYPE, player_id, pray_str )
end

