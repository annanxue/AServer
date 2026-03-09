module( "db_celebration", package.seeall )

local sformat, tinsert, diff_day = string.format, table.insert, utils.diff_day
local serialize_table, ostime = utils.serialize_table, os.time
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_CELEBRATION_STR = ""

SELECT_CELEBRATION_SQL = [[
    SELECT
    `celebration`
    from CELEBRATION_TBL where player_id = ?;
]]

SELECT_CELEBRATION_SQL_RESULT_TYPE = "s"

REPLACE_CELEBRATION_SQL = [[
    REPLACE INTO CELEBRATION_TBL ( `player_id`, `celebration` ) VALUES 
    (
        ?,
        ?
    )
]]


REPLACE_CELEBRATION_SQL_PARAM_TYPE = "is"

SELECT_ALL_CELEBRATION_SQL = [[
    SELECT
            CELEBRATION_TBL.player_id,
            PLAYER_TBL.`player_name`,
            PLAYER_TBL.`job_id`,
            `celebration`
    from    CELEBRATION_TBL 
    LEFT JOIN PLAYER_TBL 
    ON      CELEBRATION_TBL.player_id = PLAYER_TBL.player_id;
]]


----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_celebration( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_CELEBRATION_SQL, "i", _player_id, SELECT_CELEBRATION_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_celebration](do_get_player_celebration) c_stmt_format_query on SELECT_CELEBRATION_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_CELEBRATION_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local celebration_str = database:c_stmt_get( "celebration" )
            return celebration_str
        end
    end
    return nil
end

function do_save_player_celebration( _player )
    local player_id = _player.player_id_
    local celebration = _player.celebration_
    local ret = dbserver.g_db_character:c_stmt_format_modify( REPLACE_CELEBRATION_SQL, REPLACE_CELEBRATION_SQL_PARAM_TYPE, player_id, celebration )
    return ret
end

function reset_player_lottery_times( _player_id, _lucky_times )
    local cele_str = do_get_player_celebration( _player_id )
    if not cele_str or cele_str == DEFAULT_CELEBRATION_STR then
        return
    end

    local cele_data = loadstring( cele_str )()
    cele_data[4].lucky_times_ = _lucky_times
    cele_data[4].updatetime_ = ostime()

    local new_cele_str = serialize_table( cele_data )
    dbserver.g_db_character:c_stmt_format_modify( REPLACE_CELEBRATION_SQL, REPLACE_CELEBRATION_SQL_PARAM_TYPE, _player_id, new_cele_str )

    local player = db_login.g_player_cache:get( _player_id ) 
    if player and player.celebration_ then
        player.celebration_ = new_cele_str
    end
    c_log( "[db_celebration](reset_player_lottery_times) set lottery lucky times, player_id:%d, times:%d", _player_id, _lucky_times )
end
