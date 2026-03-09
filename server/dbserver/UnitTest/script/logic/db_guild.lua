module( "db_guild", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_GUILD_STR = "{}"

SELECT_GUILD_SQL = [[
    SELECT
    `data`
    from PLAYER_GUILD_TBL where player_id = ?;
]]

SELECT_GUILD_SQL_RESULT_TYPE = "s"

REPLACE_GUILD_SQL = [[
    REPLACE INTO PLAYER_GUILD_TBL ( `player_id`, `data` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_GUILD_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_guild( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_GUILD_SQL, "i", _player_id, SELECT_GUILD_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_guild](do_get_player_guild) c_stmt_format_query on SELECT_GUILD_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_GUILD_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local guild_str = database:c_stmt_get( "data" )
            return guild_str
        end
    end
    return nil
end

function do_save_player_guild( _player )
    local player_id = _player.player_id_
    local guild = _player.guild_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_GUILD_SQL, REPLACE_GUILD_SQL_PARAM_TYPE, player_id, guild )
    return ret
end

