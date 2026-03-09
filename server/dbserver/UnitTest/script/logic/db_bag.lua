module( "db_bag", package.seeall )

SELECT_BAG_SQL = [[
    SELECT
    `capacity`,
    `items`
    from BAG_TBL where player_id = ?;
]]

SELECT_WAREHOUSE_SQL = [[
    SELECT
    `capacity`,
    `items`
    from WAREHOUSE_TBL where player_id = ?;
]]

SELECT_BAG_SQL_RESULT_TYPE = "is"

REPLACE_BAG_SQL = [[
    REPLACE INTO BAG_TBL 
    ( `player_id`, `capacity`, `items` ) VALUES
    (
        ?,
        ?,
        ?
    )
]]

REPLACE_WAREHOUSE_SQL = [[
    REPLACE INTO WAREHOUSE_TBL 
    ( `player_id`, `capacity`, `items` ) VALUES
    (
        ?,
        ?,
        ?
    )
]]

REPLACE_BAG_SQL_PARAM_TYPE = "iis"


function do_get_player_bag_base( _player_id, _select_sql, _init_capacity )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( _select_sql, "i", _player_id, SELECT_BAG_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_bag] c_stmt_format_query on %s failed, player id: %d", _select_sql, _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        local bag =
        {
            capacity_ = _init_capacity, 
            items_str_ = "{}"
        }
        return bag
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local capacity, items_str = database:c_stmt_get( "capacity", "items" )
            local bag =
            {
                --capacity_ = capacity,
                --sell bag capacity? no way!
                capacity_ = _init_capacity,
                items_str_ = items_str
            }
            return bag
        end
    end
    return nil
end

function do_get_player_bag( _player_id )
    return do_get_player_bag_base( _player_id, SELECT_BAG_SQL, bag_t.BAG_ITEM_NUM_INIT )
end 

function do_get_player_warehouse( _player_id )
    return do_get_player_bag_base( _player_id, SELECT_WAREHOUSE_SQL, bag_t.WAREHOUSE_ITEM_NUM_INIT )
end 

function do_save_player_bag( _player )
    local player_id = _player.player_id_
    local bag = _player.bag_
    local capacity = bag.capacity_
    local items_str = bag.items_str_
    return dbserver.g_db_character:c_stmt_format_modify( REPLACE_BAG_SQL, REPLACE_BAG_SQL_PARAM_TYPE, player_id, capacity, items_str )
end

function do_save_player_warehouse( _player )
    local player_id = _player.player_id_
    local warehouse = _player.warehouse_
    local capacity = warehouse.capacity_
    local items_str = warehouse.items_str_
    return dbserver.g_db_character:c_stmt_format_modify( REPLACE_WAREHOUSE_SQL, REPLACE_BAG_SQL_PARAM_TYPE, player_id, capacity, items_str )
end

function save_player_bag_by_ar( _player_id, _ar )
    local bag = bag_t.get_bag_from_ar( _ar )
    local player = db_login.g_player_cache:get( _player_id ) or {}
    player.player_id_ = _player_id
    player.bag_ = bag
    do_save_player_bag( player )
end

