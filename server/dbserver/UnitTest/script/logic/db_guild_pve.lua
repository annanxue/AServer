module( "db_guild_pve", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_GUILD_PVE_STR = "do return {} end"

SELECT_GUILD_PVE_SQL = [[
    SELECT
    `guild_pve`
    from GUILD_PVE_TBL where player_id = ?;
]]

SELECT_GUILD_PVE_SQL_RESULT_TYPE = "s"

REPLACE_GUILD_PVE_SQL = [[
    REPLACE INTO GUILD_PVE_TBL ( `player_id`, `guild_pve` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_GUILD_PVE_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_guild_pve( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_GUILD_PVE_SQL, "i", _player_id, SELECT_GUILD_PVE_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_guild_pve](do_get_player_guild_pve) c_stmt_format_query on SELECT_GUILD_PVE_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_GUILD_PVE_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local guild_pve_str = database:c_stmt_get( "guild_pve" )
            return guild_pve_str
        end
    end
    return nil
end

function do_save_player_guild_pve( _player )
    local player_id = _player.player_id_
    local guild_pve = _player.guild_pve_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_GUILD_PVE_SQL, REPLACE_GUILD_PVE_SQL_PARAM_TYPE, player_id, guild_pve )
    return ret
end

