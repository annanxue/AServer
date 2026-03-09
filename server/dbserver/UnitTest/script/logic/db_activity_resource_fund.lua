module( "db_activity", package.seeall )

local sformat, tinsert, ter = string.format, table.insert, utils.serialize_table

------------------------------资源基金------------------------------


SELECT_RESOURCE_FUND_SQL = [[
    SELECT
    `begin_time`, `end_time`, `recharge`, `reward`
    from RESOURCE_FUND_TBL where player_id = ?;
]]

SELECT_RESOURCE_FUND_SQL_RESULT_TYPE = "iiis"

REPLACE_RESOURCE_FUND_SQL = [[
    REPLACE INTO RESOURCE_FUND_TBL ( `player_id`, `begin_time`, `end_time`, `recharge`, `reward` ) VALUES
    (
        ?,
        ?,
        ?,
        ?,
        ?
    )
]]
REPLACE_RESOURCE_FUND_SQL_PARAM_TYPE = "iiiis"

local function create_default()
    local resource_fund = {}
    resource_fund.begin_time_ = 0
    resource_fund.end_time_ = 0
    resource_fund.recharge_ = 0
    resource_fund.reward_ = {}
    return resource_fund
end

function do_get_player_resource_fund( _player_id )

    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_RESOURCE_FUND_SQL, "i", _player_id, SELECT_RESOURCE_FUND_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_activity](do_get_player_resource_fund) c_stmt_format_query on SELECT_RESOURCE_FUND_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return create_default()
    elseif rows == 1 then

        if database:c_stmt_fetch() == 0 then
            local resource_fund = {}
            resource_fund.begin_time_ = database:c_stmt_get( "begin_time" )
            resource_fund.end_time_ = database:c_stmt_get( "end_time" )
            resource_fund.recharge_ = database:c_stmt_get( "recharge" )
            resource_fund.reward_ = loadstring(database:c_stmt_get( "reward" ))()
            return resource_fund
        else
            c_err( "[db_activity](do_get_player_resource_fund) c_stmt_fetch error, player id: %d", _player_id )
        end

    end

    return nil

end

function do_save_player_resource_fund( _player )

    local resource_fund = _player.resource_fund_

    return dbserver.g_db_character:c_stmt_format_modify(REPLACE_RESOURCE_FUND_SQL, REPLACE_RESOURCE_FUND_SQL_PARAM_TYPE,
        _player.player_id_, resource_fund.begin_time_, resource_fund.end_time_, resource_fund.recharge_, ter(resource_fund.reward_))

end
