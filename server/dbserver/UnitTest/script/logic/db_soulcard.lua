module( "db_soulcard", package.seeall )

SELECT_SOULCARD_SQL = [[
    SELECT
    `soulcards`
    from SOULCARD_TBL where player_id = ?;
]]

SELECT_SOULCARD_SQL_RESULT_TYPE = "s"

REPLACE_SOULCARD_SQL = [[
    REPLACE INTO SOULCARD_TBL 
    ( `player_id`, `soulcards` ) VALUES
    (
        ?,
        ?
    )
]]

REPLACE_SOULCARD_SQL_PARAM_TYPE = "is"

function do_get_player_soulcards( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_SOULCARD_SQL, "i", _player_id, SELECT_SOULCARD_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_soulcard] c_stmt_format_query on %s failed, player id: %d", SELECT_SOULCARD_SQL, _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        local soulcard_data =
        {
            soulcards_str_ = "{}"
        }
        return soulcard_data
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local soulcards_str = database:c_stmt_get( "soulcards" )
            local soulcard_data =
            {
                soulcards_str_ = soulcards_str
            }
            return soulcard_data
        end
    end
    return nil
end 

function do_save_player_soulcards( _player )
    local player_id = _player.player_id_
    local soulcard_data = _player.soulcard_data_
    local soulcards_str = soulcard_data.soulcards_str_
    return dbserver.g_db_character:c_stmt_format_modify( REPLACE_SOULCARD_SQL, REPLACE_SOULCARD_SQL_PARAM_TYPE, player_id, soulcards_str )
end
