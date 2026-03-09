module( "db_guild_system", package.seeall )

local os_time, tinsert = os.time, table.insert

SELECT_ALL_GUILDS_SQL = [[
    SELECT 
    `guild_id`, 
    `guild_name`,
    `attributes`,
    `members`,
    `apply_list`,
    `trust_list`,
    `black_list`,
    `professions`,
    `donation`,
    `guild_pve`,
    `boutique`,
    `score_list`,
    `daily_task`
    from GUILD_SYSTEM_TBL 
    where not deleted; 
]]
SELECT_GUILD_SQL_PARAM_TYPE = ""
SELECT_GUILD_SQL_RESULT_TYPE = "issssssssssss"

SELECT_GUILD_BY_NAME_SQL = [[
    SELECT 
    `guild_id`
    from GUILD_SYSTEM_TBL 
    where guild_name = ? and not deleted; 
]]
SELECT_GUILD_BY_NAME_SQL_PARAM_TYPE = "s"
SELECT_GUILD_BY_NAME_SQL_RESULT_TYPE = "i"

INSERT_GUILD_SQL = [[
    INSERT into GUILD_SYSTEM_TBL 
    ( 
    `guild_name`,
    `attributes`,
    `members`
    ) 
    VALUES 
    ( ?, ?, ?);
]]
INSERT_GUILD_SQL_PARAM_TYPE = "sss"

UPDATE_GUILD_SQL = [[
    UPDATE GUILD_SYSTEM_TBL 
    set 
    guild_name = ?,
    attributes = ?, 
    members = ? , 
    apply_list = ?, 
    trust_list = ?, 
    black_list = ?, 
    professions = ?,
    donation = ?,
    guild_pve = ?,
    boutique = ?,
    score_list = ?,
    daily_task = ?
    where guild_id = ?;
]]
UPDATE_GUILD_SQL_PARAM_TYPE = "ssssssssssssi"

DELETE_GUILD_SQL = [[
    UPDATE GUILD_SYSTEM_TBL set deleted = true where guild_id = ?;
]]
DELETE_GUILD_SQL_PARAM_TYPE = "i"

-- ------------------------------------
-- Local Functions
-- ------------------------------------

function create_guild( _name, _attributes, _members )
    local time = c_cpu_ms()

    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( INSERT_GUILD_SQL, INSERT_GUILD_SQL_PARAM_TYPE, 
                                _name,
                                _attributes, 
                                _members) < 0 then
        c_err( "[db_guild_system](create_guild)failed to insert guild, members: %s, name:%s", _members, _name )
        return -1
    end

    local guild_id = database:c_stmt_get_insert_id()
    c_log( "[db_guild_system](create_guild)guild_id:%d, members: %s, _name:%s", guild_id, _members, _name )

    time = c_cpu_ms() - time
    c_prof( "[db_guild_system](create_guild)guild_id: %d, members: %s, _name:%s, use time: %dms", guild_id, _members, _name, time )

    return guild_id
end

function save_guild( _guild_id, _guild_name, _attributes, _members, _apply_list, _trust_list, _black_list, _professions, _donation, _guild_pve, _boutique, _score_list , _daily_task )
    local time = c_cpu_ms()

    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( UPDATE_GUILD_SQL, UPDATE_GUILD_SQL_PARAM_TYPE, 
                                _guild_name,
                                _attributes, 
                                _members,
                                _apply_list,
                                _trust_list,
                                _black_list,
                                _professions,
                                _donation,
                                _guild_pve,
                                _boutique,
                                _score_list, 
                                _daily_task,
                                _guild_id) < 0 then
        c_err( "[db_guild_system](save_guild)failed to save guild, id:%d, attr:[%s], members:[%s]", _guild_id, _attributes, _members )
        return false
    end

    time = c_cpu_ms() - time
    c_prof( "[db_guild_system](save_guild)guild_id: %d, use time: %dms", _guild_id, time )

    return true
end

function delete_guild( _guild_id )
    local time = c_cpu_ms()

    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( DELETE_GUILD_SQL, DELETE_GUILD_SQL_PARAM_TYPE, _guild_id) < 0 then
        c_err( "[db_guild_system](delete_guild)failed to delete guild, id:%d", _guild_id )
        return false
    end

    time = c_cpu_ms() - time
    c_prof( "[db_guild_system](delete_guild)guild_id: %d, use time: %dms", _guild_id, time )

    return true
end

function get_all_guilds()
    local time = c_cpu_ms()

    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_ALL_GUILDS_SQL, SELECT_GUILD_SQL_PARAM_TYPE, SELECT_GUILD_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_guild_system](get_all_guilds)  SELECT_ALL_GUILDS_SQL failed" )
        return nil
    end

    local all_guilds = {}
    local rows = database:c_stmt_row_num()

    for i = 1, rows, 1 do
        if database:c_stmt_fetch() == 0 then
            local guild_id, guild_name, attr, members, apply_list, trust_list, black_list, professions, donation, guild_pve, boutique, score_list,daily_task
                = database:c_stmt_get( "guild_id", "guild_name", "attributes", "members", "apply_list", "trust_list", "black_list", "professions", "donation", "guild_pve", "boutique", "score_list" , "daily_task" )
            tinsert( all_guilds, { guild_id, guild_name, attr, members, apply_list, trust_list, black_list, professions, donation, guild_pve, boutique, score_list , daily_task })
        else
            c_err( "[db_guild_system](get_all_guilds) c_stmt_fetch failed, index:%d", i)
            return nil
        end
    end

    time = c_cpu_ms() - time
    c_prof( "[db_guild_system](get_all_guilds)rows:%d, use time: %dms", rows, time )

    return all_guilds
end

local function get_guild_id_by_name( _name )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_GUILD_BY_NAME_SQL, SELECT_GUILD_BY_NAME_SQL_PARAM_TYPE, _name, SELECT_GUILD_BY_NAME_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_guild_system](get_guild_id_by_name)  SELECT_ALL_GUILDS_SQL failed" )
        return nil
    end

    local rows = database:c_stmt_row_num()
    if rows == 0 then return nil end

    if rows >= 1 then
        if database:c_stmt_fetch() == 0 then
            local guild_id = database:c_stmt_get( "guild_id" )
            return guild_id
        else
            c_err( "[db_guild_system](get_guild_id_by_name) c_stmt_fetch failed, index:%d", i)
            return nil
        end
    end
    return nil
end

-- ------------------------------------
-- Logic Functions
-- ------------------------------------

function do_create_guild( _player_id, _guild_name, _attributes, _members )
    local exist_guild_id = get_guild_id_by_name( _guild_name )
    if exist_guild_id then
        c_trace("[db_guild_system](do_delete_guild) guild name:[%s] alread exist, _player_id:%d", _guild_name, _player_id )
        dbserver.remote_call_game( "guild_mng.on_create_guild_name_exist", _guild_name, _player_id )
        return 
    end

    local guild_id = create_guild( _guild_name, _attributes, _members )
    dbserver.remote_call_game( "guild_mng.on_create_guild_result", _player_id, guild_id, _guild_name, _attributes, _members )
end

function do_get_all_guilds()
    local all_guilds = get_all_guilds()
    c_assert( all_guilds, "[db_guild_system](do_get_all_guilds) failed to get guilds system data! please check")

    for _, guild_info in ipairs( all_guilds ) do
        dbserver.remote_call_game( "guild_mng.on_get_guild_data", guild_info )
    end
end

do_save_guild = save_guild
do_delete_guild = delete_guild
