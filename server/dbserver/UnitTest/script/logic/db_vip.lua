module( "db_vip", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_VIP_STR = "{}"

SELECT_VIP_SQL = [[
    SELECT
    `vip`
    from VIP_TBL where player_id = ?;
]]

SELECT_VIP_SQL_RESULT_TYPE = "s"

REPLACE_VIP_SQL = [[
    REPLACE INTO VIP_TBL ( `player_id`, `vip` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_VIP_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_vip( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_VIP_SQL, "i", _player_id, SELECT_VIP_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_vip](do_get_player_vip) c_stmt_format_query on SELECT_VIP_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_VIP_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local vip_str = database:c_stmt_get( "vip" )
            return vip_str
        end
    end
    return nil
end

function do_save_player_vip( _player )
    local player_id = _player.player_id_
    local vip = _player.vip_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_VIP_SQL, REPLACE_VIP_SQL_PARAM_TYPE, player_id, vip )
    return ret
end

