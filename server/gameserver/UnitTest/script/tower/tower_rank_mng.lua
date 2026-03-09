module("tower_rank_mng", package.seeall)

local tinsert, tremove, tostring = table.insert, table.remove, tostring
local ser_table = utils.serialize_table
local loadstring = loadstring


g_tower_rank_list = g_tower_rank_list or {}
g_rank_version = g_rank_version or 0
g_is_rank_list_inited = g_is_rank_list_inited or false
g_need_give_award_after_init = g_need_give_award_after_init or false
g_player_to_rank_info = g_player_to_rank_info or {}

AWARD_MAIL_ID = 84

function init_tower_rank_list( _rank_list )
    g_tower_rank_list = _rank_list
    for rank, rank_info in ipairs( _rank_list ) do
        g_player_to_rank_info[rank_info.player_id_] = rank_info
    end

    -- 爬塔名人堂只要前三名的数据
    init_for_hall( 1 )
    init_for_hall( 2 )
    init_for_hall( 3 )

    g_rank_version = os.time()
    g_is_rank_list_inited = true
    c_log( "[tower_rank_mng](init_tower_rank_list) rank list init done" )

    if g_need_give_award_after_init then
        give_award_and_reset_rank_list()
    end

end

function update_player_tower_rank( _player )
    local player_id = _player.player_id_
    local my_rank_info = g_player_to_rank_info[player_id]
    local max_pass = _player.tower_.max_pass_
    local old_rank = share_const.TOWER_RANK_NUM + 1

    if my_rank_info then
        -- 已在榜内则更新
        my_rank_info.max_pass_id_    = max_pass.max_pass_id_
        my_rank_info.min_cost_time_  = max_pass.min_cost_time_
        my_rank_info.pass_timestamp_ = max_pass.pass_timestamp_
        old_rank = my_rank_info.rank_
    else
        -- 未在榜内则新建并放在榜尾
        my_rank_info =
        {
            player_id_       = player_id,
            player_name_     = _player.player_name_,
            rank_            = #g_tower_rank_list + 1,
            max_pass_id_     = max_pass.max_pass_id_,
            min_cost_time_   = max_pass.min_cost_time_,
            pass_timestamp_  = max_pass.pass_timestamp_,
        }
        tinsert( g_tower_rank_list, my_rank_info )
        g_player_to_rank_info[player_id] = my_rank_info
    end

    -- 依次跟前面的玩家比
    for rank = my_rank_info.rank_ - 1, 1, -1 do
        local other_rank_info = g_tower_rank_list[rank]
        if compare_tower_rank( my_rank_info, other_rank_info ) then
            -- 比得过，交换位置
            my_rank_info.rank_ = rank
            g_tower_rank_list[rank] = my_rank_info
            g_tower_rank_list[rank + 1] = other_rank_info
            other_rank_info.rank_ = rank + 1
        else
            -- 比不过，不比了
            break
        end
    end 
    
    -- 清除被挤出榜外的玩家
    local last_rank_info = g_tower_rank_list[share_const.TOWER_RANK_NUM + 1]
    if last_rank_info then
        g_tower_rank_list[share_const.TOWER_RANK_NUM + 1] = nil
        g_player_to_rank_info[last_rank_info.player_id_] = nil
    end

    -- 玩家在榜内则说明排行榜有变化
    local new_rank_info = g_player_to_rank_info[player_id]
    if new_rank_info then
        g_rank_version = os.time()
        c_log( "[tower_rank_mng](update_player_tower_rank) rank list updated by player: %s, version: %d", _player.tostr_, g_rank_version )

        local new_rank = new_rank_info.rank_
        if new_rank <= 10 and new_rank < old_rank then
            local player_name = _player.player_name_
            bulletin_mng.send_template_bulletin( BULLETIN.TOWER_RANK_LIST_TOP10_UPDATE, { player_name, new_rank })
            chat_mng.send_world_system_msg(
                MESSAGES.TOWER_TOP10_CHAT_MSG,
                chat_mng.CHAT_URL_TYPE_TOWER_TOP10,
                player_id,
                _player.server_id_,
                player_name,
                _player.level_,
                _player.total_gs_,
                _player.head_icon_id_,
                player_name,
                new_rank
            )
            c_log( "[tower_rank_mng](update_player_tower_rank) top 10 update, player: %s, old_rank: %d, new_rank: %d", _player.tostr_, old_rank, new_rank )
        end

        if new_rank <= share_const.TOWER_RANK_NUM and new_rank < old_rank then
            _player:add_tower_rank_promoted( new_rank )
            c_log( "[tower_rank_mng](update_player_tower_rank) top 50 update, player: %s, old_rank: %d, new_rank: %d", _player.tostr_, old_rank, new_rank )
        end

        -- 爬塔名人堂,只要第一名的
        if new_rank >= 1 and new_rank <= 3 then
            new_rank_info.hall_tower_info_ = hall_mng.player_to_hall_info( _player, hall_mng.HALL_TYPE_TOWER )
        else
            new_rank_info.hall_tower_info_ = nil
        end
    end
end

function compare_tower_rank( _rank_info1, _rank_info2 )
    if _rank_info1.max_pass_id_ > _rank_info2.max_pass_id_ then
        return true
    end
    if _rank_info1.max_pass_id_ < _rank_info2.max_pass_id_ then
        return false
    end
    if _rank_info1.min_cost_time_ < _rank_info2.min_cost_time_ then
        return true
    end
    if _rank_info1.min_cost_time_ > _rank_info2.min_cost_time_ then
        return false
    end
    if _rank_info1.pass_timestamp_ < _rank_info2.pass_timestamp_ then
        return true
    end
    if _rank_info1.pass_timestamp_ > _rank_info2.pass_timestamp_ then
        return false
    end
    return false
end

function send_tower_rank_list( _player, _client_rank_list_version )
    local has_update = _client_rank_list_version ~= g_rank_version
    _player:add_tower_rank_list( has_update, g_rank_version, g_tower_rank_list )
end

function give_award_and_reset_rank_list()
    if not g_is_rank_list_inited then
        g_need_give_award_after_init = true
        c_log( "[tower_rank_mng](give_award_and_reset_rank_list) rank list is not inited, give award later" )
        return
    end

    if g_need_give_award_after_init then
        g_need_give_award_after_init = false
        c_log( "[tower_rank_mng](give_award_and_reset_rank_list) give award after rank list init" )
    end

    -- give award
    for rank, rank_info in ipairs( g_tower_rank_list ) do
        local award_cfg = TOWER_RANK_AWARD_CONFIG[rank]
        if award_cfg then
            local item_list = {}
            local item_num_list = award_cfg.ItemNumList[1]
            for i, item_id in ipairs( award_cfg.AwardItemList[1] ) do
                local item_num = item_num_list[i]
                if item_num then
                    local item = item_t.new_by_system( item_id, item_num, "TOWER_RANK_AWARD" )
                    tinsert( item_list, item )
                else
                    c_err( "[tower_rank_mng](give_award_and_reset_rank_list) item num not found, rank: %d, item_id: %d, index: %d", rank, item_id, i )
                end
            end
            mail_box_t.add_mail( rank_info.player_id_, AWARD_MAIL_ID, item_list, { tostring( rank ) } )
            c_trace("[tower_rank_mng](give_award_and_reset_rank_list) rank:%d, player_id:%d", rank or 0, rank_info.player_id_ or 0)
        else
            c_err( "[tower_rank_mng](give_award_and_reset_rank_list) award cfg not found, rank: %d", rank )
        end
    end

    hall_on_reset_tower_rank_list()
    title_on_reset_tower_rank_list()

    -- reset rank list
    g_tower_rank_list = {}
    g_rank_version = 0
    g_player_to_rank_info = {}

    -- reset player data
    for _, mng in pairs( player_mng.g_player_mng ) do
        for _, player in pairs( mng ) do
            local max_pass = player.tower_.max_pass_
            max_pass.max_pass_id_ = 0
            max_pass.min_cost_time_ = 0
            max_pass.pass_timestamp_ = 0
        end
    end

    -- clear db data
    dbclient.remote_call_db( "db_tower.do_delete_tower_rank_list" )

    c_log( "[tower_rank_mng](give_award_and_reset_rank_list) give award and reset rank list" )
end

function remove_player_from_tower_rank_list( _player_id )
    local player_rank_info = g_player_to_rank_info[_player_id]
    if not player_rank_info then
        c_log( "[tower_rank_mng](remove_player_from_tower_rank_list) player not in rank list, player_id: %d", _player_id )
        return
    end

    g_player_to_rank_info[_player_id] = nil

    local player_rank = player_rank_info.rank_
    tremove( g_tower_rank_list, player_rank )

    for i = player_rank, #g_tower_rank_list, 1 do
        local rank_info = g_tower_rank_list[i]
        rank_info.rank_ = rank_info.rank_ - 1
    end

    g_rank_version = os.time()
    c_log( "[tower_rank_mng](remove_player_from_tower_rank_list) remove player from rank list, player_id: %d, rank: %d", _player_id, player_rank )
end

function on_player_rename( _player )
    local player_rank_info = g_player_to_rank_info[_player.player_id_]
    if player_rank_info then
        player_rank_info.player_name_ = _player.player_name_
        g_rank_version = os.time()
    end
end

function hall_on_reset_tower_rank_list()
    local server_id = g_gamesvr:c_get_server_id()
    local info_list = {}
    for i = 1, 3 do
        local rank_info = g_tower_rank_list[i]
        if rank_info then
            local hall_tower_info = rank_info.hall_tower_info_
            if hall_tower_info then
                tinsert( info_list, hall_tower_info )
            else
                -- 应该哪里出错了，没信息，先用sid和pid顶着
                tinsert(info_list, {server_id_ = server_id, player_id_ = rank_info.player_id_ } )
            end
        end
    end
    hall_mng.game_change_hall( hall_mng.HALL_TYPE_TOWER, server_id, info_list )
end

function title_on_reset_tower_rank_list()
    -- 前10名发称号
    local server_id = g_gamesvr:c_get_server_id()
    local info_list = {}
    local len = #g_tower_rank_list
    local max_rank = (len < 10) and len or 10
    for rank = 1, max_rank do
        local rank_info = g_tower_rank_list[rank]
        local info = {
            server_id_ = server_id,
            player_id_ = rank_info.player_id_,
        }
        tinsert(info_list, info )
    end
    rank_list_mng.on_get_tower_rank_list( info_list )
end

function init_for_hall( _hall_ranking )
    local rank_info = g_tower_rank_list[_hall_ranking]
    if not rank_info then
        return
    end
    local hall_tower_info = rank_info.hall_tower_info_
    if not hall_tower_info then
        return
    end
    hall_tower_info.server_id_ = g_gamesvr:c_get_server_id()
end
