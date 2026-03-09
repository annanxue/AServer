module( "db_activity", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert


--------------------------------resource magnate-----------------------------------------------

function init_select_resource_magnate_sql()
    local sql = 
    [[
        SELECT `begin_time`, `end_time`, %s
        from RESOURCE_MAGNATE_TBL where player_id = ?;
    ]]
    local field_format = [[
        `%s_magnate_total`,
        `%s_magnate_today`,
        `%s_magnate_stage`,
        `%s_magnate_mall`,
        `%s_magnate_total_rewarded`,
        `%s_magnate_today_rewarded`
    ]]

    local field_tbl = {}
    for magnate_id = player_t.MAGNATE_BEGIN, player_t.MAGNATE_END do
        local _, _, name = player_t.get_rank_type(magnate_id)
        name = string.lower(name)

        local field = string.format(field_format, name, name, name, name, name, name)
        table.insert(field_tbl, field)        
    end
    return string.format(sql, table.concat(field_tbl, ','))
end

function init_select_resource_magnate_sql_result_type()

    local field_tbl = {}

    table.insert(field_tbl, "ii")

    local field_format = "iissii"
    for magnate_id = player_t.MAGNATE_BEGIN, player_t.MAGNATE_END do
        table.insert(field_tbl, field_format)        
    end
    return table.concat(field_tbl, '')
end

function init_replace_resource_magnate_sql()
    local sql = 
    [[
        REPLACE INTO RESOURCE_MAGNATE_TBL ( `player_id`, `begin_time`, `end_time`,
        %s
        ) VALUES 
        (
            ?,
            ?,
            ?,
            %s
        )
    ]]
    local field_format = [[
        `%s_magnate_total`,
        `%s_magnate_today`,
        `%s_magnate_stage`,
        `%s_magnate_mall`,
        `%s_magnate_total_rewarded`,
        `%s_magnate_today_rewarded`
    ]]

    local field_tbl = {}
    local placeholder_tbl = {}
    for magnate_id = player_t.MAGNATE_BEGIN, player_t.MAGNATE_END do
        local _, _, name = player_t.get_rank_type(magnate_id)
        name = string.lower(name)

        local field = string.format(field_format, name, name, name, name, name, name)
        table.insert(field_tbl, field)     
        
        local placeholder = "?,?,?,?,?,?"
        table.insert(placeholder_tbl, placeholder)
    end
    return string.format(sql, table.concat(field_tbl, ','), table.concat(placeholder_tbl, ','))
end

function init_replace_resource_magnate_sql_param_type()

    local field_format = "iissii"

    local field_tbl = {}
    for magnate_id = player_t.MAGNATE_BEGIN, player_t.MAGNATE_END do
        table.insert(field_tbl, field_format)        
    end
    return 'iii'..table.concat(field_tbl, '')
end


SELECT_RESOURCE_MAGNATE_SQL = init_select_resource_magnate_sql()

-- [[
--     SELECT
--     `gold_magnate_total`,
--     `gold_magnate_today`,
--     `gold_magnate_stage`,
--     `gold_magnate_mall`,
--     `gold_magnate_total_rewarded`,
--     `gold_magnate_today_rewarded`
--     from RESOURCE_MAGNATE_TBL where player_id = ?;
-- ]]


SELECT_RESOURCE_MAGNATE_SQL_PARAM_TYPE = "i"
SELECT_RESOURCE_MAGNATE_SQL_RESULT_TYPE = init_select_resource_magnate_sql_result_type()

REPLACE_RESOURCE_MAGNATE_SQL = init_replace_resource_magnate_sql()
-- [[
--     REPLACE INTO RESOURCE_MAGNATE_TBL ( `player_id`,
--         `gold_magnate_total`, `gold_magnate_today`, `gold_magnate_stage`, `gold_magnate_mall` 
--     ) VALUES 
--     (
--         ?,
--         ?,
--         ?,
--         ?,
--         ?
--     )
-- ]]
REPLACE_RESOURCE_MAGNATE_SQL_PARAM_TYPE = init_replace_resource_magnate_sql_param_type()


----------------------------resource magnate---------------------------------

-- 包含所有存库所需字段
function create_default_db_resource_magnate()
    local resource_magnate = {}

    resource_magnate.begin_time_ = 0
    resource_magnate.end_time_ = 0

    for magnate_id = player_t.MAGNATE_BEGIN, player_t.MAGNATE_END do

        local magnate = {}
        magnate.total_ = 0
        magnate.today_ = 0
        magnate.stage_state_ = {}
        magnate.mall_buy_count_ = {}
        magnate.total_rewarded_ = player_t.ACTIVITY_STATE_UNCOMPLETED
        magnate.today_rewarded_ = player_t.ACTIVITY_STATE_UNCOMPLETED

        resource_magnate[magnate_id] = magnate
    end
    return resource_magnate
end

function do_get_player_resource_magnate( _player_id )

    local resource_magnate = {}

    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_RESOURCE_MAGNATE_SQL, SELECT_RESOURCE_MAGNATE_SQL_PARAM_TYPE, _player_id, SELECT_RESOURCE_MAGNATE_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_activity](do_get_player_resource_magnate) c_stmt_format_query on SELECT_RESOURCE_MAGNATE_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        resource_magnate = create_default_db_resource_magnate()
        return utils.serialize_table(resource_magnate)
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then

            resource_magnate.begin_time_, resource_magnate.end_time_ = database:c_stmt_get("begin_time", "end_time")

            for magnate_id = player_t.MAGNATE_BEGIN, player_t.MAGNATE_END do
                local _, _, name = player_t.get_rank_type(magnate_id)
                name = string.lower(name)
                local magnate = {}
                magnate.total_, magnate.today_, magnate.stage_state_, magnate.mall_buy_count_, magnate.total_rewarded_, magnate.today_rewarded_
                    = database:c_stmt_get(name.."_magnate_total", name.."_magnate_today", name.."_magnate_stage", name.."_magnate_mall", name.."_magnate_total_rewarded", name.."_magnate_today_rewarded")
                
                magnate.stage_state_ = loadstring(magnate.stage_state_)()
                magnate.mall_buy_count_ = loadstring(magnate.mall_buy_count_)()

                resource_magnate[magnate_id] = magnate
            end

        end
    end
    return utils.serialize_table(resource_magnate)
end

function do_save_player_resource_magnate( _player )

    local resource_magnate = loadstring(_player.resource_magnate_)()

    local param_tbl = {}
    table.insert(param_tbl, _player.player_id_)
    table.insert(param_tbl, resource_magnate.begin_time_)
    table.insert(param_tbl, resource_magnate.end_time_)
    for magnate_id = player_t.MAGNATE_BEGIN, player_t.MAGNATE_END do
        local magnate = resource_magnate[magnate_id]
        table.insert(param_tbl, magnate.total_)
        table.insert(param_tbl, magnate.today_)
        table.insert(param_tbl, utils.serialize_table(magnate.stage_state_))
        table.insert(param_tbl, utils.serialize_table(magnate.mall_buy_count_))
        table.insert(param_tbl, magnate.total_rewarded_)
        table.insert(param_tbl, magnate.today_rewarded_)
    end

    return dbserver.g_db_character:c_stmt_format_modify( REPLACE_RESOURCE_MAGNATE_SQL, REPLACE_RESOURCE_MAGNATE_SQL_PARAM_TYPE, unpack(param_tbl) )
end

