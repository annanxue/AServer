module( "db_guild_weekend_battle", package.seeall )

SELECT_GUILD_WEEKEND_BATTLE_DATA_SQL = [[
    SELECT `guild_id`, `pvp_data`
    from GUILD_WEEKEND_BATTLE_TBL where not deleted; 
]]

SELECT_GUILD_WEEKEND_BATTLE_DATA_SQL_RES_TYPE = "is"

REPLACE_GUILD_WEEKEND_BATTLE_DATA_SQL = [[
    REPLACE INTO GUILD_WEEKEND_BATTLE_TBL
    ( `guild_id`, `pvp_data` ) VALUES
    (
       ?,
       ?
    )
]]

REPLACE_GUILD_WEEKEND_BATTLE_DATA_SQL_PARAM_TYPE = "is"

DELETE_GUILD_WEEKEND_BATTLE_DATA_SQL = [[
    UPDATE GUILD_WEEKEND_BATTLE_TBL SET deleted = true where guild_id = ?; 
]]

DELETE_GUILD_WEEKEND_BATTLE_DATA_SQL_PARAM_TYPE = "i"

function do_get_all_guild_data()
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_GUILD_WEEKEND_BATTLE_DATA_SQL, "", SELECT_GUILD_WEEKEND_BATTLE_DATA_SQL_RES_TYPE ) < 0 then
        c_err( "[db_guild_weekend_battle](do_get_all_guild_data) c_stmt_format_query on SELECT_GUILD_WEEKEND_BATTLE_DATA_SQL failed" )
        return nil
    end

    local data_map = {}
    local rows = database:c_stmt_row_num() 
    for i = 1, rows do
        if database:c_stmt_fetch() == 0 then
            local guild_id, pvp_data = database:c_stmt_get( "guild_id", "pvp_data" )
            data_map[guild_id] = pvp_data
        else
            c_err( "[db_guild_weekend_battle](do_get_all_guild_data) c_stmt_fetch failed, index: %d", i )
            return nil
        end
    end
    return data_map
end

function get_all_guild_data()
    local data_map = do_get_all_guild_data() or {}
    dbserver.remote_call_game( "guild_weekend_enroll.on_get_all_gw_data", data_map )
end

function save_guild_battle_data( _guild_id, _data_str )
    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( REPLACE_GUILD_WEEKEND_BATTLE_DATA_SQL, REPLACE_GUILD_WEEKEND_BATTLE_DATA_SQL_PARAM_TYPE, _guild_id, _data_str ) < 0 then
        c_err( "[db_guild_weekend_battle](save_guild_battle_data)update data db error! guild_id: %d, data_str: %s", _guild_id, _data_str )
        return false
    end
    return true
end

function delete_guild_pvp_data( _guild_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( DELETE_GUILD_WEEKEND_BATTLE_DATA_SQL, DELETE_GUILD_WEEKEND_BATTLE_DATA_SQL_PARAM_TYPE, _guild_id ) < 0 then
        c_err( "[db_guild_weekend_battle](delete_guild_pvp_data)delete data db error! guild_id: %d", _guild_id )
        return false
    end
    return true
end

