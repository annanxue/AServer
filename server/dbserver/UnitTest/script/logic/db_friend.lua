module( "db_friend", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_FRIEND_STR = "{}"

SELECT_FRIEND_SQL = [[
    SELECT
    `common_str`,
    `normal_str`,
    `black_str`,
    `pending_str`,
    `friend_history_list_str`
    from FRIEND_TBL where player_id = ?;
]]

SELECT_FRIEND_SQL_RESULT_TYPE = "sssss"

REPLACE_FRIEND_SQL = [[
    REPLACE INTO FRIEND_TBL ( `player_id`, `common_str`, `normal_str`,`black_str`,`pending_str`,`friend_history_list_str` ) VALUES 
    (
        ?,
        ?,
        ?,
        ?,
        ?,
        ?
    )
]]

REPLACE_FRIEND_SQL_PARAM_TYPE = "isssss"

SELECT_PLAYER_BY_ID_AND_NAME = [[ 
     SELECT `player_id`, `player_name`,`deleted` from PLAYER_TBL where (player_id = ? or player_name = ? )limit 1;
     ]]

SELECT_PLAYER_BY_ID_AND_NAME_RESULT_TYPE = "isi"

----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_friend( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_FRIEND_SQL, "i", _player_id, SELECT_FRIEND_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_friend](do_get_player_friend) c_stmt_format_query on SELECT_FRIEND_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        --return DEFAULT_FRIEND_STR
        return {}
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local common_str, normal_str, black_str, pending_str, friend_history_list_str
                = database:c_stmt_get( "common_str", "normal_str", "black_str", "pending_str", "friend_history_list_str" )
            local friend_str_tbl = {
                common_str_ = common_str,
                normal_str_ = normal_str,
                black_str_ = black_str,
                pending_str_ = pending_str,
                friend_history_list_str_ = friend_history_list_str,
            }
            return friend_str_tbl
        end
    end
    return nil
end

function do_save_player_friend( _player )
    local player_id = _player.player_id_
    local friend_str_tbl = _player.friend_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( 
        REPLACE_FRIEND_SQL, REPLACE_FRIEND_SQL_PARAM_TYPE, player_id, 
        friend_str_tbl.common_str_, friend_str_tbl.normal_str_, friend_str_tbl.black_str_, friend_str_tbl.pending_str_, friend_str_tbl.friend_history_list_str_ )
        --c_err("save friend.normal_str:%s", friend_str_tbl.normal_str_)
    return ret
end

function query_player_by_id_and_name( _player_id, _player_name )
    local database = dbserver.g_db_character
    local player_id = _player_id or -1
    local player_name = _player_name or ""
    if database:c_stmt_format_query( SELECT_PLAYER_BY_ID_AND_NAME, "is", player_id, player_name, SELECT_PLAYER_BY_ID_AND_NAME_RESULT_TYPE) < 0 then
        c_err( "[db_friend](query_player_by_id_and_name) c_stmt_format_query on SELECT_PLAYER_BY_ID_AND_NAME failed, player id: %d", player_id )
        return nil, nil, nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return nil, nil, nil
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local player_id, player_name, deleted = database:c_stmt_get( "player_id", "player_name", "deleted" )
            return player_id, player_name, deleted
        end
    end
    return nil, nil, nil
end
