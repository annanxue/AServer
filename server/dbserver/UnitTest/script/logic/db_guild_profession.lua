module( "db_guild_profession", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_GUILD_PROFESSION_STR = "{}"

SELECT_GUILD_PROFESSION_SQL = [[
    SELECT
    `guild_profession`
    from GUILD_PROFESSION_TBL where player_id = ?;
]]

SELECT_GUILD_PROFESSION_SQL_RESULT_TYPE = "s"

REPLACE_GUILD_PROFESSION_SQL = [[
    REPLACE INTO GUILD_PROFESSION_TBL ( `player_id`, `guild_profession` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_GUILD_PROFESSION_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_guild_profession( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_GUILD_PROFESSION_SQL, "i", _player_id, SELECT_GUILD_PROFESSION_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_guild_profession](do_get_player_guild_profession) c_stmt_format_query on SELECT_GUILD_PROFESSION_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_GUILD_PROFESSION_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local guild_profession_str = database:c_stmt_get( "guild_profession" )
            return guild_profession_str
        end
    end
    return nil
end

function do_save_player_guild_profession( _player )
    local player_id = _player.player_id_
    local guild_profession = _player.guild_profession_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_GUILD_PROFESSION_SQL, REPLACE_GUILD_PROFESSION_SQL_PARAM_TYPE, player_id, guild_profession )
    return ret
end

