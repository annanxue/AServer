module( "db_guild_donation", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_GUILD_DONATION_STR = "{}"

SELECT_GUILD_DONATION_SQL = [[
    SELECT
    `guild_donation`
    from GUILD_DONATION_TBL where player_id = ?;
]]

SELECT_GUILD_DONATION_SQL_RESULT_TYPE = "s"

REPLACE_GUILD_DONATION_SQL = [[
    REPLACE INTO GUILD_DONATION_TBL ( `player_id`, `guild_donation` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_GUILD_DONATION_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_guild_donation( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_GUILD_DONATION_SQL, "i", _player_id, SELECT_GUILD_DONATION_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_guild_donation](do_get_player_guild_donation) c_stmt_format_query on SELECT_GUILD_DONATION_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_GUILD_DONATION_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local guild_donation_str = database:c_stmt_get( "guild_donation" )
            return guild_donation_str
        end
    end
    return nil
end

function do_save_player_guild_donation( _player )
    local player_id = _player.player_id_
    local guild_donation = _player.guild_donation_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_GUILD_DONATION_SQL, REPLACE_GUILD_DONATION_SQL_PARAM_TYPE, player_id, guild_donation )
    return ret
end

