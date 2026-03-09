module( "gm_cmd", package.seeall )

local sfind, sformat, sgmatch, ssub, tinsert, tsort, unpack, tconcat = string.find, string.format, string.gmatch, string.sub, table.insert,table.sort, unpack, table.concat

local MAX_PLAYER_LEVEL = 80

local g_max_skill_levels = {}

local function init_max_skill_levels()
    for key, cfg in pairs( SKILL_LEVELUP ) do
        if cfg.MinPlayerLevel <= MAX_PLAYER_LEVEL then
            local split_idx = sfind( key, ",", 1, true )
            local skill_id_str = ssub( key, 1, split_idx - 1 )
            local skill_level_str = ssub( key, split_idx + 1 )
            local skill_id = tonumber( skill_id_str )
            local skill_level = tonumber( skill_level_str )
            if skill_id and skill_level then
                local max_skill_level = g_max_skill_levels[skill_id]
                if not max_skill_level or skill_level > max_skill_level then
                    g_max_skill_levels[skill_id] = skill_level
                end
            else
                c_err( "[gm_cmd](init_max_skill_levels) SKILL_LEVELUP setting contains an invalid key '%s'", key )
            end
        end
    end
end

init_max_skill_levels()

-- 用于可操作离线玩家的gm命令，只能在game上使用，因为bf操纵数据库不方便；
-- 并且也只能用于可供客户端执行的gm命令，因为只有客户端的dpid和player_id有一对一的关系
local function get_player_id( _dpid, _player_id )
    local player_id

    if _player_id then
        player_id = tonumber( _player_id )
        if not player_id then
            c_err( "[gm_cmd](get_player_id) player id '%s' is not a number", _player_id )
            return nil
        end
        return player_id
    elseif _dpid == gmclient.g_gmserver_dpid then
        c_err( "[gm_cmd](get_player_id) player_id is not specified when running gm command with gm tool, _dpid: 0x%X", _dpid )
        return nil
    end

    player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[gm_cmd](get_player_id) cannot get player id by dpid 0x%X", _dpid )
        return nil
    end
    return player.player_id_
end

function set_authority( _dpid, _browser_client_id, _account_name, _authority )
    local authority = tonumber( _authority )

    if not _account_name or not authority then
        c_err( "[gm_cmd](set_authority) format error; correct format: set_authority account_name authority" )
        return
    end

    dbclient.send_set_account_authority_to_db( _account_name, authority )

    c_gm( "[gm_cmd](set_authority) _account_name: %s, _authority: %s; done", _account_name, _authority )

    -- 如果玩家在线，则更新线上数据

    local dpid = gamesvr.g_account_name_2_dpid[_account_name]
    if not dpid then
        return
    end

    local dpid_info = gamesvr.g_dpid_mng[dpid]
    if not dpid_info then
        return
    end

    local account_info = dpid_info.account_info_
    if not account_info then
        return
    end

    account_info.authority_ = _authority
end

function open_worldmap( _dpid, _browser_client_id, _player_id )
    local player = get_player( _dpid, _player_id )
    if not player then
        return false, err( "[gm_cmd](open_worldmap) player is not on game, player_id: %s", _player_id or "(nil)" )
    end
    local quest = player:get_quest_worldmap()
    quest.worldmap_position_ = quest_worldmap_t.WORLD_MAP_END_POSITION
    player:add_ack_quest_worldmap_info()
    c_gm( "[gm_cmd](open_worldmap) player_id: %d; done", player.player_id_ )
    return true
end

function add_batch_mail( _dpid, _browser_client_id, _mail_id, _item_id_list, _item_num_list, _tip_list, _player_id_list )
    local player_id_list = {}
    if _player_id_list then
        for player_id_str in sgmatch( _player_id_list, "[^,]+") do
            local p_id = tonumber( player_id_str )
            if not p_id then
                c_err("[gm_cmd](add_batch_mail) player id is not number:%s", player_id_str)
                return false, c_err("player id is not number:%s", player_id_str)
            end
            tinsert( player_id_list, p_id)
        end
    end
    for _, player_id in pairs( player_id_list ) do
        c_gm("[gm_cmd](add_batch_mail) processing, player_id:%d", player_id)
        local succ, info = add_mail(_dpid, _browser_client_id, _mail_id, _item_id_list, _item_num_list, _tip_list, player_id)
        if not succ then
            return succ, info
        end
    end

    c_gm("[gm_cmd]( add_batch_mail ) done")
    return true, "success"
end

function clear_mail( _dpid, _browser_client_id, _player_id )
    local player = get_player( _dpid, _player_id )

    if not player then
        return false, err( "[gm_cmd](clear_mail) player is nil. _dpid: 0x%X, pid: %s", _dpid, _player_id )
    end

    local pid = player.player_id_
    local mail_box = player.mail_box_
    if not mail_box then
        return false, err( "[gm_cmd](clear_mail) player has no mail box. pid: %d", pid )
    end

    local mail_num = #mail_box.mail_list_ 
    for i = 1, mail_num do
        player:remove_mail_by_index( 1 )
    end

    return true, log( "[gm_cmd](clear_mail) player_id: %d; mail box is cleared", pid )
end

function openall_tunlock_for_player( _player )
    local finish_quest = {}
    local last_id = 0
    for _, cfg in pairs( QUESTS ) do
        tinsert( finish_quest, cfg.ID )
        _player:add_send_finished_quest( cfg.ID )
        if cfg.ID > last_id then
            last_id = cfg.ID
        end
    end
    _player.quest_.main_:init_new_player_main_quest( last_id )
    _player.quest_.main_.finished_quest_ = finish_quest
    _player:add_ack_current_quest()
end

function openall_tunlock( _dpid, _browser_client_id, _player_id )
    local player = get_player( _dpid, _player_id )
    if not player then
        return false, err( "[gm_cmd](openall_tunlock) player is not on game, player_id: %s", _player_id or "(nil)" )
    end
    openall_tunlock_for_player( player )
    return true
end

function start_pvp()
    time_event.g_event_map[21].start_sec_ = os.time() +  15 
    return true, log( "[gm_cmd](start_pvp) wait for 15 seconds..." )
end

function add_juanzhou( _player )
    _player:add_item( 21011, 1, "gm_cmd" )
    _player:add_item( 21012, 1, "gm_cmd" )
    _player:add_item( 22011, 1, "gm_cmd" )
    _player:add_item( 22012, 1, "gm_cmd" )
end


function add_mat()
    for id, target in pairs (ctrl_mng.g_ctrl_mng) do
        if target:is_player() then
            local item_map = {}
            local honor, glory = 0, 0
            for model_index = 1, 9 do
                for model_level = 0, 36 do 
                    local key = target.job_id_ * 10000 + model_index * 100 + model_level
                    local cfg = TRANSFORM_ENHANCE_CONFIG[ key ]
                    local item_id = cfg.MaterialID
                    local count = cfg.MaterialCount
                    if item_map[ item_id ] then
                        item_map[ item_id ] = item_map[ item_id ] + count
                    else
                        item_map[ item_id ] = count
                    end
                    honor = honor + cfg.HonorCost
                    glory = glory + cfg.GloryCost
                end
            end
            for item_id, item_count in pairs( item_map ) do
                target:add_item( item_id, item_count, "test_case") 
            end
            target:add_honor( honor, "test_case" )
            target:add_glory( glory, "test_case" )
        end
    end
end

function do_all( _dpid, _browser_client_id, _player_id )
    local player = get_player( _dpid, _player_id )
    if not player then
        return
    end

    level( _dpid, _browser_client_id, sformat( "%d", MAX_PLAYER_LEVEL ), _player_id )

    -- 穿装备begin
    local part2id = {}

--[[    for equip_id, v in pairs( EQUIPS ) do
        if ITEMS[ equip_id ] and ( ITEMS[equip_id].JobLimit == player.job_id_ or ITEMS[equip_id].JobLimit == 0 ) and ITEMS[equip_id].UseLevel <= player.level_ then
            local part = v.EquipPart

            if not part2id[part] then
                part2id[part] = equip_id
            else
                local id_exist =  part2id[part]
                local quality_exist = ITEMS[id_exist].Quality
                local level_exist = ITEMS[id_exist].UseLevel 

                if ITEMS[equip_id].UseLevel >= level_exist and ITEMS[equip_id].Quality >= quality_exist and item_t.get_random_key_by_cfg( v.AttrNumMax )==6 then
                    part2id[part] = equip_id
                end
            end
        end
    end
--]]
    local desc_key_list={} 
    for key in pairs( ORANGE_EQUIP_EXCHANGE_CONFIG ) do
        tinsert( desc_key_list, key )
    end
    local function cmp_func( _a, _b ) return _a > _b end
    tsort( desc_key_list, cmp_func )

    local right_key
    for _, key in ipairs( desc_key_list ) do
        if player.level_>= key then
            right_key = key
            break
        end
    end

    if not right_key then
        c_err( "[player_t](exchange_orange_equip)can not find ORANGE_EQUIP_EXCHANGE_CONFIG key by player_id: %d player_level: %d", player_id, level )
        return
    end

    local cfg = ORANGE_EQUIP_EXCHANGE_CONFIG[ right_key ]
    if not cfg then
        c_err( "[player_t](exchange_orange_equip)can not find ORANGE_EQUIP_EXCHANGE_CONFIG cfg by player_id: %d key: %d", player_id, right_key )
        return
    end

    local job = player.job_id_
    local item_id_list 
    if job == JOBS.JOB_ID_JOB1 then
        item_id_list = cfg.Job1CommonOrange[ 1 ]    
    elseif job == JOBS.JOB_ID_JOB2 then
        item_id_list = cfg.Job2CommonOrange[ 1 ]    
    elseif job == JOBS.JOB_ID_JOB3 then
        item_id_list = cfg.Job3CommonOrange[ 1 ]    
    elseif job == JOBS.JOB_ID_JOB4 then
        item_id_list = cfg.Job4CommonOrange[ 1 ]    
    end
    clear( _dpid, _browser_client_id, _player_id )

    local equip_cnt = 0
    if item_id_list then
        for i=1, #(item_id_list) do
            player:add_item( item_id_list[i], 1, "gm" )
            equip_cnt = equip_cnt + 1
        end
    end

    for i = 1, equip_cnt do
        player:do_equip( i )
    end
    -- 穿装备end

    test.do_stone(tonumber(_player_id))
    for skill_id, v in pairs( player.job_skills_ ) do
        local max_skill_level = g_max_skill_levels[skill_id]
        if max_skill_level then
            player.job_skills_[skill_id].level_ = max_skill_level
            player:add_set_job_skill( skill_id , player.job_skills_[skill_id] )
        else
            c_err( "[gm_cmd](do_all) do not know the max skill level of %d, player_id: %d", skill_id, player.player_id_ )
        end
    end
    
    -- update Skill GS
    player:update_gear_score("skill")

    add_juanzhou( player )

    open_worldmap( _dpid, _browser_client_id, _player_id )
    openall_tunlock_for_player( player )

    add_non_bind_gold( _dpid, _browser_client_id, "1000000", _player_id )
    add_bind_diamond( _dpid, _browser_client_id, "1000000", _player_id )
    add_non_bind_diamond( _dpid, _browser_client_id, "1000000", _player_id )

    --add_mat( player )
    quest_mammons_treasure.check_and_refresh_mammons( player)
    event_price_board_t.price_board_try_reset( player )

    return true, ""
end


function add_all( _dpid, _browser_client_id, _player_id )
    
    local player = get_player( _dpid, _player_id )
    if not player then
        return
    end

    local m_id_map = {}
    

    for model_index = 1, 9 do 
        local start_level = model_index == 1 and 1 or 0
        for model_level = start_level, 12 do 
        local enhance_key = sformat( "%d,%d", model_index, model_level ) 

            local transform_enhance_cfg = PLAYER_TRANSFORM_ENHANCE[ enhance_key ]
            if not transform_enhance_cfg then return end

            local material_list = transform_enhance_cfg.MaterialList[ 1 ]
            local material_count_list = transform_enhance_cfg.MaterialCountList[ 1 ]

            for i, item_id in ipairs( material_list ) do
                local item_count = material_count_list[ i ]
                if m_id_map[ item_id ] then
                    m_id_map[ item_id ] = m_id_map[ item_id ] + item_count
                else
                    m_id_map[ item_id ] = item_count
                end
            end
        end
    end  

    for item_id, count in pairs( m_id_map ) do
        player:add_item( item_id, count, "gm" )
    end

    return true, log("[gm_cmd](add_all)player_id: %d add material succeed!", player.player_id_ )
end


function add_item_all( _dpid, _browser_client_id, item_id, item_num )
    for id, target in pairs (ctrl_mng.g_ctrl_mng) do
         if target:is_player() then
            target:add_item( tonumber(item_id), tonumber(item_num), "gm" )
        end
    end

    return true, log("[gm_cmd](add_item_all)every player add item: %d, item_num: %d succeed! ", item_id, item_num )
end

local function parse_add_mail_params( _mail_id, _item_id_list, _item_num_list, _tip_list )
    local mail_id = tonumber( _mail_id )
    if not mail_id then
        return nil, err( "[gm_cmd](parse_add_mail_params) mail_id '%s' is not a number", safe_log_str( _mail_id ) )
    end

    local mail_cfg = MAILS[mail_id] 
    if not mail_cfg then
        return nil, err( "[gm_cmd](parse_add_mail_params) mail_id:'%d' not have cfg", mail_id )
    end

    -- 物品ID
    local item_id_tbl = {}
    if _item_id_list then
        for item_id_str in sgmatch( _item_id_list,  "[^,]+" ) do
            local item_id = tonumber( item_id_str )
            if not item_id then
                return nil, err( "[gm_cmd](add_mail) item id '%s' is not a number; correct format: add_mail mail_id item1_id,item2_id,... item1_num,item2_num,... tip1,tip2... [player_id]", item_id_str )
            end
            tinsert( item_id_tbl, item_id )
        end
    end

    -- 物品数量
    local item_num_tbl = {}
    if _item_num_list then
        for item_num_str in sgmatch( _item_num_list, "[^,]+" ) do
            local item_num = tonumber( item_num_str )
            if not item_num then
                return nil, err( "[gm_cmd](add_mail) item number '%s' is not a number; correct format: add_mail mail_id item1_id,item2_id,... item1_num,item2_num,... tip1,tip2... [player_id]", item_num_str )
            end
            tinsert( item_num_tbl, item_num )
        end
    end

    if #item_id_tbl ~= #item_num_tbl then
        return nil, err( "[gm_cmd](add_mail) the length of item id list is %u, but the length of item number list is %u, they are not equal; correct format: add_mail mail_id item1_id,item2_id,... item1_num,item2_num,... tip1,tip2... [player_id]", #item_id_tbl, #item_num_tbl )
    end

    -- 物品列表
    local item_tbl = {}

    for i = 1, #item_id_tbl do
        local item_id = item_id_tbl[i]
        local item_num = item_num_tbl[i]
        local item = item_t.new_by_system( item_id, item_num, "gm" ) 

        if not item then
            return nil, err( "[gm_cmd](add_mail) item_id: %d, item_num: %d; cannot create item", item_id, item_num )
        end

        if  item.use_type_ == item_t.ITEM_USE_TYPE_NON_BIND_GOLD  or
            item.use_type_ == item_t.ITEM_USE_TYPE_BIND_DIAMOND or
            item.use_type_ == item_t.ITEM_USE_TYPE_NON_BIND_DIAMOND or
            item.use_type_ == item_t.ITEM_USE_TYPE_ENERGY or
            item.use_type_ == item_t.ITEM_USE_TYPE_EXP or
            item.use_type_ == item_t.ITEM_USE_TYPE_HONOR or
            item.use_type_ == item_t.ITEM_USE_TYPE_BIND_GOLD then
            item.item_attr_[1] = item_num
        end

        -- 运维发放装备默认绑定
        if item:can_equip() and item.is_bind_ == 0 then
            item.is_bind_ = 1
            c_item_c( sformat( "[ITEM_UPDATE]sys_log: %s, player id: %d, item_id: %d, item_sn: %u, item_num: %d, is_bind: %d", 
                     "gm_mail_item", 0, item.item_id_, item.item_sn_, item.item_num_, item.is_bind_ ) )
        end

        tinsert( item_tbl, item )
    end

    if #item_tbl == 0 then
        item_tbl = nil
    end

    -- 邮件内容参数

    -- gmsvr发送邮件，mail id 固定都是13，内容里面有可能有逗号
    local tip_tbl = {}
    if _mail_id ~= 13 then
        if _tip_list then
            for tip in sgmatch ( _tip_list, "[^,]+" ) do
                tinsert( tip_tbl, tip )
            end
        end
    else
        tinsert( tip_tbl, _tip_list)
    end

    local content = mail_cfg.Text
    local iterator = string.gmatch(content, "{%d+}")
    local count = 0
    for s in iterator do
        count = count + 1 
    end

    local tips_count = #tip_tbl
    if tips_count ~= count then
        return nil, err( "[mail_box_t](parse_add_mail_params) mail_id:%d connent need params count:%d but tips_count:%d", mail_id, count, tips_count)
    end

    local params =
    {
        mail_id_ = mail_id,
        item_list_ = item_tbl,
        tip_list_ = tip_tbl,
    }
    return params
end

function add_mail( _dpid, _browser_client_id, _mail_id, _item_id_list, _item_num_list, _tip_list, _player_id, _create_time, _gm_global_mail_id )
    if not _player_id then
        return false, err( "[gm_cmd](add_mail) player_id is nil" )
    end

    local player_id = tonumber( _player_id )
    if not player_id then
        return false, err( "[gm_cmd](add_mail) player_id is not a number: %s", _player_id )
    end

    local params, err_msg = parse_add_mail_params( _mail_id, _item_id_list, _item_num_list, _tip_list )
    if not params then
        return false, err_msg
    end

    mail_box_t.add_mail( player_id, params.mail_id_, params.item_list_, params.tip_list_, _create_time, _gm_global_mail_id )
    return true, log( "[gm_cmd](add_mail) player_id: %d, mail_id: %s, item_id_list: %s, item_num_list: %s, tip_list: %s; done",
           player_id, _mail_id, safe_log_str( _item_id_list ), safe_log_str( _item_num_list ), safe_log_str( _tip_list ) )
end

function add_mail_all( _dpid, _browser_client_id, _mail_id, _item_id_list, _item_num_list, _tip_list )
    local max_gm_global_mail_id = dbclient.g_max_gm_global_mail_id
    if max_gm_global_mail_id < 0 then
        return false, err( "[gm_cmd](add_mail_all) server initialization has not completed yet, please retry later" )
    end

    max_gm_global_mail_id = max_gm_global_mail_id + 1
    dbclient.g_max_gm_global_mail_id = max_gm_global_mail_id

    local params, err_msg = parse_add_mail_params( _mail_id, _item_id_list, _item_num_list, _tip_list )
    if not params then
        return false, err_msg
    end

    local mail_id = params.mail_id_
    local item_list = params.item_list_
    local tip_list = params.tip_list_

    for server_id, player_mng_one_server in pairs( player_mng.g_player_mng ) do
        for player_id, player in pairs( player_mng_one_server ) do
            mail_box_t.add_mail( player_id, mail_id, item_list, tip_list, nil, max_gm_global_mail_id )
        end
    end

    params.gm_global_mail_id_ = max_gm_global_mail_id

    for player_id, bf_player in pairs( bfclient.g_bf_players ) do
        if not player_mng.get_player_by_player_id( player_id ) then
            params.player_id_ = tostring( player_id )
            gamesvr.operate_player_data( player_id, "mail_box_t.add_mail_to_player_by_params", nil, params )
        end
    end

    for player_id, joining_player in pairs( gamesvr.g_joining_players ) do
        if not bfclient.g_bf_players[player_id] then
            params.player_id_ = tostring( player_id )
            gamesvr.operate_player_data( player_id, "mail_box_t.add_mail_to_player_by_params", nil, params )
        end
    end

    dbclient.remote_call_db( "db_mail.save_gm_global_mail", max_gm_global_mail_id, _mail_id, _item_id_list, _item_num_list, _tip_list )

    log("[gm_cmd](add_mail_all) succeed, mail_id: %d, gm_global_mail_id: %d", _mail_id, max_gm_global_mail_id )

    return true, "success"
end

function print_player_count( _dpid, _browser_client_id )
    local succ, ret_msg = login_mng.gm_print_player_count()
    return succ, ret_msg
end

function set_max_user_num( _dpid, _browser_client_id, _num )
    local num = tonumber( _num )
    if not num or num < 0 then
        return false, "invalid gm cmd param"
    end

    if login_mng.set_max_user_num( num ) then
        return true, sformat( "[gm_cmd](set_max_user_num) set to %d success", num )
    else
        return false, sformat( "[gm_cmd](set_max_user_num) set to %d failed", num )
    end
end

function show_max_user_num( _dpid, _browser_client_id )
    return true, sformat( "[gm_cmd](show_max_user_num) %d", login_mng.g_max_user_num )
end

function set_login_queue_size( _dpid, _browser_client_id, _size )
    local size = tonumber( _size ) 
    if not size or size < 0 then
        return false, "invalid gm cmd param"
    end

    if login_mng.set_login_queue_size(  _size ) then
        return true, sformat( "[gm_cmd](set_login_queue_size) set to %d success", _size )
    else
        return false, sformat( "[gm_cmd](set_login_queue_size) set to %d failed", _size )
    end
end

function show_login_queue_size( _dpid, _browser_client_id )
    return true, sformat( "[gm_cmd](show_login_queue_size) %d", login_mng.g_login_queue.size_ )
end

function start_time_event( _dpid, _browser_client_id, _event_id )
    local event_id = tonumber( _event_id ) 
    if not event_id then 
        return false, "invalid gm cmd param"
    end

    if time_event.gm_start_time_event( event_id ) then
        return true, sformat( "[gm_cmd](start_time_event) event_id: %d success", event_id )
    else
        return false, sformat( "[gm_cmd](start_time_event) event_id: %d failed", event_id )
    end
end

function finish_showing_newbie_guide( _dpid, _browser_client_id, _player_id )
    local player = get_player( _dpid, _player_id )
    if not player then
        c_err( "[gm_cmd](finish_showing_newbie_guide) player is nil. _dpid: 0x%X, pid: %s", _dpid, _player_id )
        return false, sformat( "[gm_cmd](finish_showing_newbie_guide) player is nil. _dpid: 0x%X, pid: %s", _dpid, _player_id )
    end
    player:add_finish_showing_newbie_guide()
    return true, sformat( "[gm_cmd](finish_showing_newbie_guide) execute succ. _dpid: 0x%X, pid: %s", _dpid, _player_id )  
end

function send_bulletin( _dpid, _browser_client_id, _begin_time, _bulletin_str, _interval, _show_count )
    local begin_time = tonumber( _begin_time )
    local interval = tonumber( _interval ) 
    local show_count = tonumber( _show_count )

    local str = _bulletin_str
    local succ, msg = bulletin_mng.send_system_bulletin(str, begin_time, interval, show_count )
    c_gm("[gm_cmd](send_bulletin) send_bulletin:%s", str)
    return succ, msg 
end

function query_player_info( _dpid, _browser_client_id, _player_id )
    if not _browser_client_id then
        return false, err( "[gm_cmd](query_player_info) browser_client_id not specified, "..
                           "this cmd must be executed by gmserver, dpid: 0x%X", _dpid )
    end
    if not _player_id then
        return false, err( "[gm_cmd](query_player_info) player_id not specified, dpid: 0x%X", _dpid )
    end

    local player_id = tonumber( _player_id )

    local player = player_mng.get_player_by_player_id( player_id )
    if player then
        local player_info_json = build_player_info_json( player )
        c_gm( "[gm_cmd](query_player_info) player_id: %d", player_id )
        if _dpid == 0 then  -- 函数调用是由db远程调用发起
            gmclient.send_gm_cmd_result( _browser_client_id, true, player_info_json )
            return
        else
            return true, player_info_json
        end
    end

    local bf_player = bfclient.g_bf_players[player_id]
    if bf_player then
        local bf_id = bf_player.bf_id_
        bfclient.remote_call_bf( bf_id, "gm_cmd.query_player_info",
                                 0, _browser_client_id, _player_id, tostring( g_gamesvr:c_get_server_id() ) )
        c_gm( "[gm_cmd](query_player_info) player is on bf, send query to bf %d, player_id: %d", bf_id, player_id )
        return
    end

    dbclient.remote_call_db( "db_gm.query_player_info", _browser_client_id, player_id )

    c_gm( "[gm_cmd](query_player_info) player is offline, send query to db, player_id: %d", player_id )
end

function query_player_info_by_player_name( _dpid, _browser_client_id, _player_name )
    if not _browser_client_id then
        return false, err( "[gm_cmd](query_player_info_by_player_name) browser_client_id not specified, "..
                           "this cmd must be executed by gmserver, dpid: 0x%X", _dpid )
    end
    if not _player_name then
        return false, err( "[gm_cmd](query_player_info_by_player_name) player_name not specified" )
    end
    dbclient.remote_call_db( "db_gm.query_player_info_by_player_name", _browser_client_id, _player_name )
end

function on_query_player_info_result( _browser_client_id, _player )
    player_t.unserialize_before_join( _player )  -- 这里反序列化是为了方便gm查询某些系统数据（如身上的装备）
    local player_info_json = build_player_info_json( _player )
    gmclient.send_gm_cmd_result( _browser_client_id, true, player_info_json )
end

function client_system_switch( _dpid, _browser_client_id, _sys_type, _on_off)   -- on 1, off 0
    local sys_type = tonumber( _sys_type )
    local on_off = tonumber( _on_off )

    if not CLIENT_ACCESS_SWITCH[sys_type] then
        c_err("[gm_cmd](client_system_switch) not config sys type:%d", sys_type)
        return false, sformat("[gm_cmd](client_system_switch) not config sys type")
    end

    return client_access_switch.switch_client_system_access( sys_type, on_off )
end

function finish_current_task( _dpid, _browser_client_id, _player_id )
    if not _browser_client_id then
        return false, err( "[gm_cmd](finish_task) browser_client_id not specified, "..
                           "this cmd must be executed by gmserver, dpid: 0x%X", _dpid )
    end
    if not _player_id then
        return false, err( "[gm_cmd](finish_task) player_id not specified, dpid: 0x%X", _dpid )
    end

    local player_id = tonumber( _player_id )

    local player = player_mng.get_player_by_player_id( player_id )
    if player then
        local main_task = player:get_quest_main()
        local current_task = main_task.current_quest_
        current_task.is_prized_ = 0
        local targets = current_task.targets_
        for _, target in pairs( targets ) do
            local target_cfg = TARGETS[target.id_]
            target.finish_cnt_ = target_cfg.TotalCnt 
        end
        player:add_ack_current_quest()
        return true, log("[gm_cmd](finish_current_task) success!")
    else
        return false, err("[gm_cmd](finish_current_task) can not find player by player id:%d", player_id)
    end
end

function jump_task( _dpid, _browser_client_id, _player_id, _task_id )
    if not _browser_client_id then
        return false, err( "[gm_cmd](jump_task) browser_client_id not specified, "..
                           "this cmd must be executed by gmserver, dpid: 0x%X", _dpid )
    end
    if not _player_id then
        return false, err( "[gm_cmd](jump_task) player_id not specified, dpid: 0x%X", _dpid )
    end

    if not _task_id then
        return false, err( "[gm_cmd](jump_task) task_id not specified, dpid: 0x%X", _dpid )
    end

    local player_id = tonumber( _player_id )
    local task_id = tonumber( _task_id )

    if not QUESTS[task_id] then
        return false, err( "[gm_cmd](jump_task) task_id:%d not valid", task_id )
    end

    local player = player_mng.get_player_by_player_id( player_id )
    if player then
        local main_task = player:get_quest_main()
        main_task:init_new_player_main_quest(task_id)
        player:add_ack_current_quest()
        return true, log("[gm_cmd](jump_task) success!")
    else
        return false, err("[gm_cmd](jump_task) can not find player by player id:%d", player_id)
    end
end

function reset_loginserver_connect_info( _dpid, _browser_client_id, _ip, _port )
    if not _browser_client_id then
        return false, err( "[gm_cmd](reset_loginserver_connect_info) browser_client_id not specified, "..
        "this cmd must be executed by gmserver, dpid: 0x%X", _dpid )
    end

    if not _ip then
        return false, err( "[gm_cmd](reset_loginserver_connect_info) ip not specified !" )
    end

    if not _port then
        return false, err( "[gm_cmd](reset_loginserver_connect_info) port not specified !" )
    end

    local ret = g_loginclient:c_reset_connect_info( _ip, _port )

    if ret == 1 then
        return false, err( "[gm_cmd](reset_loginserver_connect_info) invalid ip")
    end

    if ret == 2 then
        return false, err( "[gm_cmd](reset_loginserver_connect_info) invalid port")
    end

    return true, log("[gm_cmd](reset_loginserver_connect_info) success!")
end

function send_player_to_bf( _dpid, _browser_client_id, _player_id, _bf_id )
    local player_id = get_player_id( _dpid, _player_id )
    if not player_id then
        return false, err( "[gm_cmd](send_player_to_bf) player_id not specified or not valid" )
    end
    local bf_id = tonumber( _bf_id )
    if not bf_id then
        return false, err( "[gm_cmd](send_player_to_bf) bf_id not specified or not valid" )
    end
    test.send_player_to_bf( player_id, bf_id )
    return true
end

function do_recharge( _dpid, _browser_client_id, _recharge_count, _player_id )
    _recharge_count = tonumber( _recharge_count )
    _player_id = tonumber( _player_id )
    local player = get_player( _dpid, _player_id )
    if not player then
        return false, sformat( "[gm_cmd](do_recharge) can't find player: %d ", _player_id )
    end
    if _recharge_count <= 0 then
        return false, sformat( "[gm_cmd](do_recharge) recharge_count: %d invalid", _recharge_count )
    end
    player:add_ask_client_do_recharge( _recharge_count )
    return true
end

function finish_inst( _dpid, _browser_client_id, _player_id, _gamesvr_id )
    local player_id = tonumber( _player_id )
    if not player_id then
        return false, err( "[gm_cmd](finish_inst) can't convert _player_id param to number" )
    end
    
    local svr_id = 0
    if _gamesvr_id then
        svr_id = tonumber( _gamesvr_id ) 
        if not svr_id then
            return false, err( "[gm_cmd](finish_inst) can't convert _gamesvr_id param to number" )
        end
    end

    local player, err_msg = get_player( _dpid, player_id, svr_id )
    if not player then
        return false, err_msg
    end

    local inst  = player:get_instance()
    inst.finish_code_ = instance_t.INSTANCE_FINISH_SUCCESS
    inst:on_battle_finish()

    c_gm("[gm_cmd](finish_inst) player_id: %d, ctrl_id: %d", player_id, player.ctrl_id_ )
    return true
end

function friend_mng_lock( _dpid, _browser_client_id, _is_all_lock )
    local is_all_lock = tonumber( _is_all_lock ) or 0
    friend_mng.lock( is_all_lock )
    return true
end

function lock_player_shops( _dpid, _browser_client_id )
    shop_mng.g_locked_by_gm = true
    return true
end

function unlock_player_shops( _dpid, _browser_client_id )
    shop_mng.g_locked_by_gm = false
    return true
end

function lock_truncheon( _dpid, _browser_client_id, _is_lock )
    player_t.truncheon_lock_ = tonumber(_is_lock) or 0
    return true
end

function open_equip_exchange_shop( _dpid, _browser_client_id, _is_open )
    if _is_open == "true" then
        player_t.g_equip_exchange_shop_open = true
        c_gm("[gm_cmd](open_equip_exchange_shop)is_open: true")
        return true
    elseif _is_open == "false" then
        player_t.g_equip_exchange_shop_open = false
        c_gm("[gm_cmd](open_equip_exchange_shop)is_open: false")
        return true
    else
        return false, "check is_open: true/false"
    end
end

function close_all_shops( _dpid, _browser_client_id )
    shop_mng.remote_call_bf( "shop_mng.gm_close_all_shops_of_game", _browser_client_id, g_gamesvr:c_get_server_id() )
    c_gm( "[gm_cmd](close_all_shops) browser_client_id: %d", _browser_client_id )
end

function do_lua_on_miniserver( _dpid, _browser_client_id, _player_id, _func_str )
    local player = get_player( _dpid, _player_id )
    if not player then
        return false, sformat( "[gm_cmd](do_lua_on_miniserver) can't find player: %d ", _player_id )
    end

    gamesvr.game_server_remote_call_mini_server( player, "player_mng.on_miniserver_do_lua", _func_str )
    return true
end

function kick_account_internal( _account_id )
    local session = login_mng.g_session_map[_account_id]
    if not session then
        return true, log( "[gm_cmd](kick_account) account session not created yet, nothing need to do, account_id: %s", _account_id )
    end

    local account_dpid = session.dpid_
    local player_id = -1

    local player = player_mng.get_player_by_dpid( account_dpid )
    if player then
        player_id = player.player_id_
        player:add_kick_player()
        login_mng.close_session_before_kick_player( _account_id, player_id, false )
        g_timermng:c_add_timer_second( 1, timers.KICK_PLAYER_TIMER, account_dpid, 0, 0 )
        return true, log( "[gm_cmd](kick_account) player is on game, account_id: %d, player_id: %d", _account_id, player_id )
    end

    -- 不要用session.migrate_作为判断依据，因为玩家从bf回game但还未连上game的过程中，migrate_也为false
    for bf_player_id, bf_player in pairs( bfclient.g_bf_players ) do
        if bf_player.account_id_ == _account_id then
            if bf_player.state_ == bfclient.BF_PLAYER_STATE_BF2GAME then
                bfclient.remove_bf_player( bf_player_id )  -- on_rejoin中找不到对应的bf_player就会踢掉客户端
            else
                -- 不要remove_bf_player，不然可能会丢失玩家存盘包
                bfclient.remote_call_bf( bf_player.bf_id_, "player_mng.kick_player_offline", bf_player_id, g_gamesvr:c_get_server_id() )  
            end
            return true, log( "[gm_cmd](kick_account) player is on bf, account_id: %d, player_id: %d", _account_id, bf_player_id )
        end
    end

    local joining_player = gamesvr.g_dpid2joining_players[account_dpid]
    if joining_player then
        player_id = joining_player.player_id_
    end

    local ar = gamesvr.g_ar
    ar:flush_before_send( msg.ST_KICK_PLAYER )
    ar:send_one_ar( g_gamesvr, account_dpid )
    login_mng.close_session_before_kick_player( _account_id, player_id, false )
    g_timermng:c_add_timer_second( 1, timers.KICK_PLAYER_TIMER, account_dpid, 0, 0 )
    return true, log( "[gm_cmd](kick_account) account_id: %d, player_id: %d", _account_id, player_id )
end

function kick_account( _dpid, _browser_client_id, _account_id )
    local account_id = tonumber( _account_id )
    if not account_id then
        return false, err( "[gm_cmd](kick_account) can't convert _account_id param to number: %s", _account_id )
    end
    return kick_account_internal( account_id )
end

function forbid_accounts_by_player_ids( _dpid, _browser_client_id, _player_ids, _forbid_end_times, _is_forbid_machine )
    local player_forbid_tbl = {}

    local end_time_tbl = {}
    for end_time_str in sgmatch( _forbid_end_times, "[^,]+" ) do
        local forbid_end_time = tonumber( end_time_str )
        if not forbid_end_time then
            return false, err("[gm_cmd](forbid_accounts_by_player_ids) end time is not number: '%s'", end_time_str )
        end
        tinsert( end_time_tbl, forbid_end_time )
    end

    local i = 1
    for player_id_str in sgmatch( _player_ids, "[^,]+" ) do
        local player_id = tonumber( player_id_str )
        if not player_id then
            return false, err( "[gm_cmd](forbid_accounts_by_player_ids) player id is not number: '%s'", player_id_str )
        end
        player_forbid_tbl[player_id] = end_time_tbl[i] 
        i = i + 1
    end

    dbclient.remote_call_db( "db_gm.forbid_accounts_by_player_ids", _browser_client_id, player_forbid_tbl, _is_forbid_machine )
    return true
end

function forbid_accounts_by_machine_code( _dpid, _browser_client_id, _machine_code_list, _forbid_end_times )
    local machine_forbid_tbl = {}

    local end_time_tbl = {}
    for end_time_str in sgmatch( _forbid_end_times, "[^,]+" ) do
        local forbid_end_time = tonumber( end_time_str )
        if not forbid_end_time then
            return false, err("[gm_cmd](forbid_accounts_by_player_ids) end time is not number: '%s'", end_time_str )
        end
        tinsert( end_time_tbl, forbid_end_time )
    end

    for machine_code in sgmatch( _machine_code_list, "[^,]+" ) do
        tinsert( machine_forbid_tbl, machine_code )
    end

    if #machine_forbid_tbl > 0 then
        loginclient.remote_call_ls( "game_login_server.forbid_account_by_machine_code",
                                    _browser_client_id, 
                                    machine_forbid_tbl, 
                                    end_time_tbl, 
                                    "gm_cmd.forbid_accounts_by_machine_code_callback" )
    end
end

function forbid_accounts_by_machine_code_callback( _browser_client_id, _fail_list )
    local msg = "success"
    if #_fail_list > 0 then
        msg = tconcat( _fail_list, ";" )
    end
    gmclient.send_gm_cmd_result( _browser_client_id, true, msg )
end

function answer_player_question( _dpid, _client_id, _player_id, _idx, _answer, _state)
    local player = player_mng.get_player_by_player_id(_player_id)
    if player then
        player:on_answer_player_question(_idx, _answer, _state)
    else
        dbclient.remote_call_db( "db_customer_service.on_customer_service_answer", _player_id, _idx, _answer, _state )
    end

    c_log("[gmclient](on_customer_service_response) player_id:%d idx:%d answer:%s", _player_id, _idx, _answer)
    return true
end


-- 4. 根据账号id查找当前账号下所有角色信息
function get_all_player_info_by_account_id(_dpid, _browser_client_id, _account_id)

    local param = {}
    param.dpid_ = _dpid
    param.client_id_ = _browser_client_id
    param.account_id_ = _account_id

    dbclient.remote_call_db( "db_gm.query_account_info_by_account_id",
        _account_id, "gm_cmd.get_all_player_info_by_account_id_callback", param)

end

--(ldb) p _account_info
--{
--        ["account_first_player_time_"] = 1531724847.000000,
--        ["last_join_player_id_"] = 330.000000,
--        ["player_list_"] = {
--                [329.000000] = {
--                        ["player_name_"] = "布裏安娜·托雷斯",
--                        ["level_"] = 1.000000,
--                        ["total_gs_"] = 0.000000,
--                        ["slot_"] = 0.000000,
--                        ["use_model_"] = 1.000000,
--                        ["job_id_"] = 2.000000,
--                        ["wing_id_"] = 0.000000,
--                        ["need_download"] = 0.000000,
--                        ["active_weapon_sfxs_"] = "",
--                        ["model_id_"] = 2.000000,
--                },
--                [330.000000] = {
--                        ["player_name_"] = "吉爾·胡梅爾斯",
--                        ["level_"] = 1.000000,
--                        ["total_gs_"] = 0.000000,
--                        ["slot_"] = 1.000000,
--                        ["use_model_"] = 1.000000,
--                        ["job_id_"] = 2.000000,
--                        ["wing_id_"] = 0.000000,
--                        ["need_download"] = 0.000000,
--                        ["active_weapon_sfxs_"] = "",
--                        ["model_id_"] = 2.000000,
--                },
--        },
--        ["account_name_"] = "3094",
--        ["authority_"] = 0.000000,
--}

function get_all_player_info_by_account_id_callback(_param, _account_info)

    local player_list = {}
    for k,v in pairs(_account_info.player_list_) do
        local player = {}
        player.player_id_ = k
        player.player_name_ = v.player_name_
        player.job_id_ = v.job_id_

        table.insert(player_list, player)
    end

    gmclient.send_gm_cmd_result( _param.client_id_,
        utils.table.size(_account_info) == 0 and 0 or 1, cjson.encode(player_list) )
end


function do_get_player_info_by_player_name_rex(_param, _callback)
    dbclient.remote_call_db( "db_gm.get_player_info_by_player_name_rex", _callback, _param)
end
-- _player_name_rex 支持mysql正则表达式
function get_player_info_by_player_name_rex( _dpid, _browser_client_id, _player_name_rex )
    if not _browser_client_id then
        return false, err( "[gm_cmd](query_player_info_by_player_name) browser_client_id not specified, "..
                           "this cmd must be executed by gmserver, dpid: 0x%X", _dpid )
    end
    if not _player_name_rex then
        return false, err( "[gm_cmd](get_player_info_by_player_name_rex) player_name not specified" )
    end

    local param = {}
    param.dpid_ = _dpid
    param.client_id_ = _browser_client_id
    param.player_name_rex_ = '%'.._player_name_rex..'%'

    do_get_player_info_by_player_name_rex(param, "gm_cmd.get_player_info_by_player_name_rex_callback")
end


function get_player_info_by_player_name_rex_callback(_param, _player_list)

    -- gmclient.send_gm_cmd_result( _param.client_id_, 1, cjson.encode(_player_list) )

    if not _player_list then
        c_err("player list nil,param:%s", utils.serialize_table(_param))
        return
    end

    for _,v in ipairs(_player_list) do
        query_player_info(_param.dpid_, _param.client_id_, v.player_id_)
    end

end

function do_get_player_orders(_param, _callback)
    dbclient.remote_call_db( "dbserver.on_get_player_orders", _callback, _param)
end

-- 获取玩家所有充值订单
function get_player_orders(_dpid, _browser_client_id, _player_id)

    local param = {}
    param.dpid_ = _dpid
    param.client_id_ = _browser_client_id
    param.player_id_ = _player_id

    do_get_player_orders(param, "gm_cmd.get_player_orders_callback")

end

function get_player_orders_callback(_param, _orders)
    gmclient.send_gm_cmd_result(_param.client_id_, 1, cjson.encode(_orders))
end

function recharge_product(_dpid, _browser_client_id, _product_id, _player_id)
    
    local product_id = tonumber(_product_id)
    local player_id = tonumber(_player_id)
    
    local player = player_mng.get_player_by_player_id( player_id )
    if not player then
        local err_str = string.format("gm_cmd:recharge_product player id invalid:%s", tostring(_player_id))
        c_err(err_str)
        return false, err_str
    end

    local product_cfg = RECHARGE_ITEM_LIST[product_id]
    if not product_cfg then
        c_err( "[player_t](on_recharge_success)player_id: %d, order_id: %d, product_id: %d, product cfg not found", _player_id, _order_id, _product_id )
        return false
    end
    player.on_recharge_success(player.player_id_, 1, product_id, product_cfg.Price)

    return true, "success"

end

function do_get_item_log(_param, _callback)
    dbclient.remote_call_db("dbserver.on_get_item_log", _callback, _param)
end


function get_item_log(_dpid, _browser_client_id, _player_id, _item_id)

    local param = {}
    param.player_id_ = tonumber(_player_id)
    param.item_id_ = _item_id
    param.dpid_ = _dpid
    param.client_id_ = _browser_client_id

    do_get_item_log(param, "gm_cmd.get_item_log_callback")
    -- dbclient.remote_call_db("dbserver.on_get_item_log", "gm_cmd.get_item_log_callback", param)
end

function get_item_log_callback(_param, _item_log_list)
    gmclient.send_gm_cmd_result(_param.client_id_, 1, cjson.encode(_item_log_list))
end


function do_save_player_item_log(_item_log)
    dbclient.remote_call_db( "db_item_log.do_save_player_item_log", _item_log)
end