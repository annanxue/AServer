module( "db_chat", package.seeall )

local sformat, tinsert, ser_table = string.format, table.insert, utils.serialize_table

MAX_OFFLIEN_MSG_NUM = 10
----------------------------------------------------
--                  sql default
----------------------------------------------------
DEFAULT_CHAT_STR = ""

SELECT_CHAT_SQL = [[
    SELECT
    `chat`
    from CHAT_TBL where player_id = ?;
]]

SELECT_CHAT_SQL_RESULT_TYPE = "s"

REPLACE_CHAT_SQL = [[
    REPLACE INTO CHAT_TBL ( `player_id`, `chat` ) VALUES 
    (
        ?,
        ?
    )
]]

REPLACE_CHAT_SQL_PARAM_TYPE = "is"
----------------------------------------------------
--                  local
----------------------------------------------------
function do_get_player_chat( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_CHAT_SQL, "i", _player_id, SELECT_CHAT_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_chat](do_get_player_chat) c_stmt_format_query on SELECT_CHAT_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return DEFAULT_CHAT_STR
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local chat_str = database:c_stmt_get( "chat" )
            return chat_str
        end
    end
    return nil
end

function do_save_player_chat( _player )
    local player_id = _player.player_id_
    local chat = _player.chat_
    local ret =  dbserver.g_db_character:c_stmt_format_modify( REPLACE_CHAT_SQL, REPLACE_CHAT_SQL_PARAM_TYPE, player_id, chat )
    return ret
end

function add_offline_chat_msg(_player_id, _chat_msg)
    local chat_str = do_get_player_chat(_player_id)
    local player =
    {
        player_id_ = _player_id, 
        chat_ = chat_str,  
    }
    
    player_t.unserialize_player_chat_from_peer(player)
    local chat_list = player.chat_tbl_.chat_list_
    local new_chat = nil
    for _, chat in pairs(chat_list) do
        if chat.player_id_ == _chat_msg.pid_ and chat.server_id_ == _chat_msg.svr_id_ then
            new_chat = chat
            break
        end
    end

    if not new_chat then 
        local count = #chat_list    
        if count >= player_t.MAX_RECENT_CHAT_COUNT then
            table.remove(chat_list, count) 
        end

        new_chat = 
        {
            player_id_ = _chat_msg.pid_,
            server_id_ = _chat_msg.svr_id_, 
            icon_id_ = _chat_msg.icon_id_,
            gs_ = _chat_msg.gs_, 
            player_name_ = _chat_msg.player_name_, 
            level_ = _chat_msg.level_, 
            msg_list_ = {}
        }

        tinsert(chat_list, 1, new_chat)
    end

    local msg_list = new_chat.msg_list_
    if #msg_list == MAX_OFFLIEN_MSG_NUM then
        table.remove(msg_list, 1) 
    end
    table.insert(msg_list, _chat_msg)

    local save_str = ser_table(player.chat_tbl_)
    dbserver.g_db_character:c_stmt_format_modify( REPLACE_CHAT_SQL, REPLACE_CHAT_SQL_PARAM_TYPE, _player_id, save_str)

    local cache = db_login.g_player_cache:get( _player_id )
    if cache then
        cache.chat_ = save_str
    end

    c_log( "[db_chat](add_offline_chat_msg) player id: %d add offline msg success", _player_id )
end
