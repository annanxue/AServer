module( "hkc", package.seeall )

--[[ 
使用方法, do hkc.add_monster(10000) 在玩家旁边放只怪 

1.  add_monster( typeid )   放怪
2.  kill_monster( typeid )  杀指定怪
3.  clear()                 清附近怪
4.  fly( x , z )            飞过去
5.  level( level )          升级
6.  smode()                 单机版
7.  nmode()                 网络版

]]

local sformat, tinsert, tconcat, rand, tremove, floor, abs = string.format, table.insert, table.concat, math.random, table.remove, math.floor, math.abs
local LOG_HEAD = "[hkc]"

function get_player()
    for _, target in pairs ( ctrl_mng.g_ctrl_mng ) do  
        if target:is_player() then
            return target
        end
    end
    return nil
end 
 
--放怪
function add_monster( _monster_typeid )
   for id , target in pairs ( ctrl_mng.g_ctrl_mng ) do 
        if target:is_player() then
            local x, _, z = target.core_:c_getpos() 
            local scene_id = target.scene_id_
            local plane_id = target:get_plane_id()
            local angle = target.angle_
            local level = MONSTER_DATA[_monster_typeid].level_
            monster_t.add_monster( _monster_typeid, level, scene_id, x, z, angle_ , nil, plane_id ,0)
        end
    end
end

function add_monster_pos( _monster_typeid ,_x ,_z )
   for id , target in pairs ( ctrl_mng.g_ctrl_mng ) do 
        if target:is_player() then
            local x, _, z = target.core_:c_getpos() 
            local scene_id = target.scene_id_
            local plane_id = target:get_plane_id()
            local angle = target.angle_
            local level = MONSTER_DATA[_monster_typeid].level_
            monster_t.add_monster( _monster_typeid, level, scene_id, _x, _z, angle_ , nil, plane_id ,0)
        end
    end
end

--杀某只怪
function kill_monster( _monster_typeid )

    local player
    for _, ctrl in pairs(ctrl_mng.g_ctrl_mng) do
        if ctrl:is_player() then
            player = ctrl
            break
        end
    end
    local x, _, z = player.core_:c_getpos()
    local player_pos = {x, z}

    for _, ctrl in pairs(ctrl_mng.g_ctrl_mng) do
        if ctrl:is_monster() and ctrl.type_id_ ==  _monster_typeid then
            local x, _, z = ctrl.core_:c_getpos()
            ctrl:monster_kill(player)
            c_log(string.format("kill monster %d", ctrl.ctrl_id_))
            return 
        end 
    end

end

--清怪
function clear()
    test.del_mst()
end 

function add_client_monster( _monster_typeid )
    for id , target in pairs ( ctrl_mng.g_ctrl_mng ) do 
        if target:is_player() then
            target:add_client_create_monster( _monster_typeid )
        end
    end
end

--飞过去
function fly( _x , _z)
    for id , target in pairs ( ctrl_mng.g_ctrl_mng ) do 
        if target:is_player() then
            local x, _, z = target.core_:c_getpos() 
            local scene_id = target.scene_id_
            local plane_id = target:get_plane_id()
            target:replace_plane_safe(scene_id, plane_id, _x, 0, _z)
        end
    end
end

local function cal_exp ( lvl )
    if lvl < 1 then return 0 end
    local exp = -24 -- 一级那个不用的
    for k , v  in pairs( EXP )  do 
        if k > lvl then return exp end 
        exp = exp + v.levelup_exp
    end
    return exp
end 

--设置玩家等级
function level( val )
    test.level( val )
--[[
    local exp = cal_exp( val )

    for id , target in pairs ( ctrl_mng.g_ctrl_mng ) do 
        if target:is_player() then
            target.exp1_ = exp 
            target:on_level_up(val)
           c_log("通过gm命令设置玩家等级为:%d",val)
        end
    end
    ]]
end

function exp( val )
    for id , target in pairs ( ctrl_mng.g_ctrl_mng ) do 
        if target:is_player() then
            target:add_exp( val , "gm" )
           c_log("通过gm命令增加玩家经验:%d",val)
        end
    end
end 

--单机版
function smode()
    for id , target in pairs ( ctrl_mng.g_ctrl_mng ) do 
        if target:is_player() then
            target:add_set_game_mode(1)
            c_log("设为单机版罗")
        end
    end
end

--网络版
function nmode()
    for id , target in pairs ( ctrl_mng.g_ctrl_mng ) do 
        if target:is_player() then
            target:add_set_game_mode(2)
            c_log("设为网络版罗")
        end
    end
end


--调试机关
function debug_trigger()
    trigger.DEBUG_TRIGGER = true
    c_log("打开机关调试")
end

function debug_trigger_off()
    trigger.DEBUG_TRIGGER = false
    c_log("关闭机关调试")
end

--1、跟踪位置
--do g_playermng:c_set_trace_pos(true)



--放怪
function add_trigger( _typeid )
   for id , target in pairs ( ctrl_mng.g_ctrl_mng ) do 
        if target:is_player() then
            local x, _, z = target.core_:c_getpos() 
            local scene_id = target.scene_id_
            local plane_id = target:get_plane_id()
            local angle = target.angle_
            -- local level = MONSTER_DATA[_monster_typeid].level_
            local tg = trigger.create_trigger( _typeid, scene_id, plane_id, x, z, angle_)
            tg:set_angle()
        end
    end
end


function test_countdown()
    for _, target in pairs ( ctrl_mng.g_ctrl_mng ) do  
        if target:is_player() then
            target:add_plane_start_count_down(10 )
        end
    end
    print "test count down  success !"
end 

function join_honorhall()
    for _, target in pairs ( ctrl_mng.g_ctrl_mng ) do  
        if target:is_player() then
            local mode = honor_hall.HONOR_HALL_GAME_MODE_DEVELOP 
            honor_hall.honor_hall_join_match( target , mode )
            break
        end
    end
    print "join success !"
end 


function join_battlefield()
    for _, target in pairs ( ctrl_mng.g_ctrl_mng ) do  
        if target:is_player() then
            local mode = tamriel_relic.GAME_MODE_DEVELOP
            tamriel_relic.join_match( target , mode )
        end
    end
    print "join success !"
end 


function battlefield_win( win )
    local player = nil
    for _, target in pairs ( ctrl_mng.g_ctrl_mng ) do  
        if target:is_player() then
            player = target
            break
        end
    end
    local plane = planemng.get_player_plane( player )
    local lose_camp = player.core_:c_get_camp()
    if win then 
        for _ , camp in pairs( TAMRIEL_RELIC.TeamCamp ) do 
            if camp ~= lose_camp then
                lose_camp = camp 
                break
            end 
        end 
    end
    tamriel_relic.end_battle( plane , lose_camp)
    if win then
        print "win"
    else
        print "lose"
    end 
end 

function damage_tam_crystal()
    local player = get_player()
    local relation = 2
    local damage_type = 2
    local damage_value = 300
    tamriel_relic.damage_crystal( player, relation, damage_type, damage_value )
    c_log("damage!")
end 

function print_inst_error( )
    for k,v in pairs( INSTANCE_CFG ) do 
        local inst_id = v.SpawndataId 
        if inst_id ~= k then 
            c_safe( "not match %d", k )
        end 
    end
    c_safe(" done " ) 
end 

local function get_table_count( _table )
    local count  = 0
    for _, _ in pairs( _table ) do 
        count = count + 1
    end
    return count 
end 

-----------------------------------------------------------
-- 求有效副本　求有效怪物列表　求有效ai列表脚本
----------------------------------------------------------
local function get_open_instance_id()
    local log_head = sformat( "%s(get_open_instance_id)", LOG_HEAD )
    local instance_table = {}
    local INSTANCE_CFG = INSTANCE_CFG
    for k, v in pairs( ACTIVITY_INSTANCE_CONFIG ) do 
        local instance_id = v.InstanceId
        if not INSTANCE_CFG[ instance_id ] then 
            c_err( "%s instance id: %d not found, activity instance id: %d", log_head, instance_id, k )
        else
            if v.IsOpen == 1 then 
                instance_table[ instance_id ] = true
            end 
        end 
    end
    return instance_table
end 

------------------------------------------------------------
--　副本的怪物：预置物 + 召唤机关怪 + 手填的卡片怪，隐藏怪（战场是用代码加的，别外有些是召唤技能的怪)
----------------------------------------------------------
function get_instance_monster_list( _instance_id )
    local log_head = sformat( "%s(get_instance_monster_list)", LOG_HEAD )
    local cfg = INSTANCE_CFG[_instance_id]
    if not cfg then 
        c_err( "%s instance id: %d not found", log_head, _instance_id )
        return 
    end
    local monster_list = test.get_instance_monster_type_id_list( _instance_id ) or {}
    local monster_card_list = cfg.Monster and cfg.Monster[1] or {}
    for _, mst_type_id in pairs( monster_card_list ) do 
        monster_list[mst_type_id] = true       
    end 

    local hide_monster_card_list = cfg.HideMonster and cfg.HideMonster[1] or {}
    for _, mst_type_id in pairs( hide_monster_card_list ) do 
        monster_list[mst_type_id] = true 
    end 
    return monster_list
end

local function get_all_instance_monster_list()
    local monster_list = {}
    instance_list = INSTANCE_CFG
    for instance_id, _ in pairs( instance_list ) do 
        local instance_monster_list = get_instance_monster_list( instance_id )
        for type_id, _ in pairs( instance_monster_list or {} ) do 
            monster_list[ type_id ] = true
        end 
    end
    c_safe( "instance monster count: %d", get_table_count( monster_list ) )
    return monster_list
end 

-------------------------------------------------------------
-- 把召唤技能表上的怪算进来，其实和上面的隐藏怪重复了
----------------------------------------------------------
local function get_summon_skill_monster_list()
    local monster_list = {}
    
    local SummonInfo = SUMMON_FORMATIONS.SummonInfo
    local TriggerAttr = SUMMON_FORMATIONS.TriggerAttr
 
    for formation_id, _ in pairs( SummonInfo ) do 
        local form_cfg = SummonInfo[formation_id]["formation_info"]
        for _, record in pairs( form_cfg ) do
            local obj_type    = record[1]
            local type_id     = record[2]
            if skill.TYPE_MONSTER == obj_type then 
                monster_list[type_id] = true 
            end 
        end
   end
   for _, trigger_cfg in pairs( TriggerAttr ) do 
       local type_id = trigger_cfg.summon_npc_type 
       if type_id then 
           monster_list[type_id] = true
       end 
   end

   c_safe( "summon skill monster count: %d", get_table_count( monster_list ) )

   return monster_list
end

-------------------------------------------------------------
-- 剧情用到的 CreateMonster
-- 这部分怪可以不用到ai，只是客户端拿来作表现的
--
-- 在剧情配置表下面执行下面脚本
-- cinema_monster_list.sh
-- grep typeId *.txt | sed 's/.*"typeId": \([0-9]*\).*/\1,/g'  > cinema_monster.txt
-- 再把怪物拷到CINEMA_MONSTER_LIST里面即可
----------------------------------------------------------
local function get_cinema_monster_list()
    local monster_list = {}
 
    local CINEMA_MONSTER_LIST = testcase.CINEMA_MONSTER_LIST
    for _, type_id in ipairs( CINEMA_MONSTER_LIST ) do
        monster_list[ type_id ] = true
    end 

    c_safe( "cinema  monster count: %d", get_table_count( monster_list ) )

    return monster_list
end 

-------------------------------------------------------------
-- 怪物进行难度适配时会换怪物id，所以这里加上挑战噩梦本适配出来的怪物id
----------------------------------------------------------
local function get_all_expert_monster_list( _monster_list )
    local expert_monster_id = {}
    return expert_monster_id
end 

local function print_monster_list( _monster_map, _action )
    local monster_list = {}
    for type_id, _ in pairs( _monster_map ) do 
        tinsert( monster_list, type_id )
    end 
    _action = _action or ""
    local list_str = tconcat( monster_list, "," )
    c_safe( "after %s, count: %d , monster: %s", _action, #monster_list, list_str )
end 
-------------------------------------------------------------
-- 全部有效怪物, 删除怪物列表
----------------------------------------------------------
function get_all_monster()
    local monster_list = {}

    --1. 副本布怪，卡片怪，隐藏怪加的怪
    local all_instance_monster = get_all_instance_monster_list()
    for type_id, _ in pairs( all_instance_monster ) do 
        monster_list[ type_id ] = true
    end 

    --2. 召唤技能怪
    local summon_skill_monster = get_summon_skill_monster_list()
    for type_id, _ in pairs( summon_skill_monster ) do 
        monster_list[ type_id ] = true
    end 

    --3.剧情怪,虽然只能客户端用，但还是加进有效列表
    local cinema_monster_list = get_cinema_monster_list()
    for type_id, _ in pairs( cinema_monster_list ) do 
        monster_list[ type_id ] = true
    end 

    --4.难度适配产生的新怪
    local expert_monster_list = get_all_expert_monster_list( monster_list )
    for type_id, _ in pairs( expert_monster_list ) do 
        monster_list[ type_id ] = true
    end 

    c_safe( "valid monster count: %d", get_table_count( monster_list ) )

    local MONSTER_DATA = MONSTER_DATA
    local delete_monster_list = {}
    for type_id, _ in pairs(MONSTER_DATA) do
        if not monster_list[ type_id ] then 
            tinsert( delete_monster_list, type_id )
        end 
    end
    table.sort( delete_monster_list )
    local delete_list_str = tconcat( delete_monster_list, "," )
    c_safe( "need delete monster: %s", delete_list_str )
    c_safe( "total delete count: %d", #delete_monster_list )
end 

local function scan_valid_hide_instance( _instance_list )
    local hide_list = {}
    for inst_id , _ in pairs( _instance_list ) do 
        local cfg = INSTANCE_CFG[inst_id]
        if not cfg then 
            c_err( "not found instance: %d", inst_id )
        else
            if cfg.HideInstanceId then 
                hide_list[ cfg.HideInstanceId ] = true 
            end 
            local rand_inst = cfg.RandomInstId and cfg.RandomInstId[1]
            for rand_inst_id, _ in pairs( rand_inst or {} ) do 
                hide_list[ rand_inst_id ] = true
            end 
        end 
   end
   c_safe( "valid hide inst: %d", get_table_count( hide_list ) )

   return hide_list
end 

--
-- 1001 是主城，不能删
--
function scan_all_not_open_instance()
    local instance_list = get_open_instance_id()
    local hide_inst_list = scan_valid_hide_instance( instance_list )
    for inst_id, _ in pairs( hide_inst_list ) do 
        instance_list[ inst_id ] = true
    end 

    local delete_list = {}
    for inst_id, _ in pairs( INSTANCE_CFG ) do 
        if inst_id ~= 1001 and  not instance_list[inst_id] then 
            tinsert( delete_list, inst_id )
        end 
    end 

    c_safe( "INSTANCE_CFG count: %d", get_table_count( INSTANCE_CFG ) )

    table.sort( delete_list )

    local delete_list_str = tconcat( delete_list, "," )
    c_safe( "not open instance, need delete instance: %s", delete_list_str )
    c_safe( "total delete count: %d", #delete_list )
end

--
-- 扫出无效的角色ai
--
function scan_delete_ai_list()
    local valid_list = {}
    for _, cfg in pairs( MONSTER_DATA ) do 
        local ai_list = cfg.default_ai_ and cfg.default_ai_[1]
        for _, ai_id in pairs( ai_list or {} ) do 
            valid_list[ai_id] = true
        end 
    end
    c_safe( "valid ai count: %d", get_table_count( valid_list ) )

    local delete_list = {}
    for k, v  in pairs( CHAR_AI ) do 
        if not valid_list[k] then 
            tinsert( delete_list, k )
        end 
    end 

    table.sort( delete_list )

    local delete_list_str = tconcat( delete_list, "," )
    c_safe( "need delete char ai: %s", delete_list_str )
    c_safe( "total delete count: %d", #delete_list )
end


function get_ai_file_name( path, ai_path )
    local len = string.len(ai_path);
    local name = string.sub(path, len+2) --  remove "/"
    return string.sub(name, 1, -5 ) -- remove ".xml"
end

--
-- 扫出无效的ai xml 文件
--
function scan_delete_ai_xml_file()
    local ai_path = "../../luacommon/setting/ai" 
    local file_list = io.popen('find * ' .. ai_path)

    local all_xml_file = {}
    for file in file_list:lines() do 
        if string.find(file,"%.xml$") then
            local name = get_ai_file_name( file, ai_path )
            all_xml_file[name] = true
        end 
    end

    c_safe( "all xml file count: %d", get_table_count( all_xml_file ) )

    local valid_list = {}
    for k, v  in pairs( CHAR_AI ) do
        local name = v.Template
        if not all_xml_file[name] then 
            c_err("not found xml file for CHAR_AI key: %d, name: %s", k, name )
        else
            valid_list[ name ] = true 
        end 
    end 

    c_safe( "valid xml file count: %d", get_table_count( valid_list ) )

    local delete_list = {}
    for name, _ in pairs( all_xml_file ) do 
        if not valid_list[name] then 
            tinsert( delete_list, name )
        end 
    end 

    table.sort( delete_list )

    local delete_list_str = tconcat( delete_list, ",\n" )
    c_safe( "need delete ai xml :\n%s", delete_list_str )
    c_safe( "total delete count: %d", #delete_list )
end 


function trigger_action_skill_list()
    local skill_id_list = {}
    for k,v in pairs( TRIGGER_ACTION ) do 
        local t = v.action_class
        if t == 0 or t == 1 or t == 2 or t == 8 or t == 9 then
            if v.action_types then 
                skill_id_list[v.action_types] = true           
            end 
        end
        if t == 8 then 
            for _, v in pairs( v.action_extend_ or {} ) do 
                local skill_id = v[2]
                skill_id_list[skill_id] = true
            end 
        end 
    end
    
    local skill_list = {}
    for id, _ in pairs( skill_id_list ) do 
        tinsert( skill_list, id )
    end 

    table.sort( skill_list )

    local delete_list_str = tconcat( skill_list, "\n" )
    c_safe( "need save trigger action  skill id :\n%s", delete_list_str )
    c_safe( "total save count: %d", #skill_list )
end 

function start_section_gain_test()
    local player = get_player()
    if player then 
        player:start_record_player_section_gain()
        c_safe("start")
    end     
end 

function end_section_gain_test()
    local player = get_player()
    if player then 
        local gain = player.player_section_gain_ 
        if not gain then 
            c_err( "%s not start record, so the gain is nil", log_head )
            return 
        end 
        player:add_player_section_gain( gain.gold_ )
        player:clear_player_section_gain()
        c_safe("end")
    end     
end 

function test_time_limited_entrance( _num )
    local player = get_player()
    if not player then 
        c_safe( "no player " )
        return
    end 
    _num = _num or 0 
    local open_list = {}
    for i = 1, _num do 
        open_list [i] = true
    end
    time_limited_entrance_t.g_open_activity_list = open_list
    player_mng.broadcast_time_limited_entrance( )
    c_safe( "send num: %d", _num )
end 

function test_left_short_tips( _rate )
    local player = get_player()
    if not player then 
        c_safe( "no player " )
        return
    end 
    player:add_chess_short_tips_text( _rate )
    c_safe( "send rate: %d", _rate )
end 

function start_question()
    time_limited_entrance_t.g_open_activity_list[1] = true
    player_mng.broadcast_time_limited_entrance( )
    event_question_t.question_event_start()
end

function end_question()
    time_limited_entrance_t.g_open_activity_list[1] = nil
    player_mng.broadcast_time_limited_entrance( )
    event_question_t.question_event_end()
end 

function print_trigger( _chunk_idx )
    if not _chunk_idx then 
        c_safe( "chunk idx is nil" )
        return
    end
    local player = get_player()
    if not player then 
        c_safe( "no player " )
        return
    end 
    local inst = instance_mng.get_obj_instance( player )
    if not inst then 
        c_safe( "player not in instance " )
        return
    end 
    local trigger = inst.mgr_obj_:get_ctrl_by_idx( _chunk_idx )
    if not trigger then 
        c_safe( "not found trigger by chunk idx: %d", _chunk_idx )
        return
    end 

    local impl = trigger.impl_
    c_safe("------------------------------------------------------------")
    c_safe(" chunk idx: [%d] %s \n", impl.chunk_idx_, impl.spawn_name_ ) 
    c_safe( "自动复位: %s, is_end:  %s \n", tostring(impl:is_switchable()), tostring(impl:is_end()) ) 
    c_safe( " is_enable: %s \n", tostring(impl.is_enable_) )
    c_safe("------------------------------------------------------------")
end 

function insert_sort_tail_node( _tb )
    local tb_len = #_tb
    local val = _tb[ tb_len ] 
    for index = tb_len, 1, -1 do 
        local before_index = index - 1
        if before_index == 0 then 
            return
        end 
        local before_val = _tb[before_index]
        if val > before_val then 
            _tb[before_index] = val 
            _tb[index] = before_val
        else
            return
        end 
    end 
end 

function test_insert_sort( )
    local tb = { 2,4,8,4,2,7,8 }

    local ret = {}
    for _, v in ipairs( tb ) do 
        tinsert( ret, v )
        --insert_sort_tail_node( ret )     
        utils.insert_sort_tail_node( ret )
    end

    for _, v in ipairs( ret ) do 
        c_log( "-----------> %d", v )
    end 
end 

function test_snake_config()
    local i = 1
    local EQUIPS = EQUIPS  
    local ITEMS = ITEMS

    local equip_count = 0
    local purple_count = 0


    for k, item_id in ipairs( SNAKE_CONFIG ) do 
        if EQUIPS[item_id] then 
            equip_count = equip_count + 1
            local qua = ITEMS[item_id].Quality
            if qua == 4 then 
                purple_count = purple_count + 1
            end 
        end 
    end

    local item_count = #SNAKE_CONFIG

    c_safe( "item count: %d, equip count: %d, purple count: %d, purple rate: %f", item_count, equip_count, purple_count, purple_count/ equip_count ) 

end

----------------------------------------
-- 棋社模拟器
----------------------------------------
function chess_random_win_lose()
    local max_rate = 100
    local rate_table = { 50,50,50,50,100}
    local win_lose_table = {}
    for i = 1, 5 do 
        local index = rand( #rate_table )
        local weight = rate_table[ index ]
        tremove( rate_table, index )
        if weight == max_rate then 
            tinsert( win_lose_table, true )
        else
            local is_win = rand( max_rate ) <= weight
            tinsert( win_lose_table, is_win )
        end 
    end
    return win_lose_table
end

function chess_win_count( _win_lose_table )
    _win_lose_table = chess_random_win_lose()
    local count = 0
    for _, v in ipairs( _win_lose_table ) do 
        if v then 
            count = count + 1
        end 
    end
    return count
end 

function chess_gain_one_drop( _drop_data, _act_inst_id, drop_level )
    local drop_data = _drop_data[_act_inst_id] 
    if not drop_data then
        c_err("%s drop data is nil, player id: %d, act_inst_id: %d", log_head, 0, _act_inst_id)    
        return 
    end

    local gain = drop_data.drop_level_gain_[ drop_level ]
    if not gain then return end -- not drop for this level 

    local drop_level_result = drop_data.drop_level_result_[drop_level]
    if gain.monster_count_ >= #drop_level_result then 
        c_err("%s drop max, player id: %d, act_inst_id: %d, drop level: %d, count: %d", log_head, 0, _act_inst_id, drop_level, gain.monster_count_)    
        return 
    end 
    gain.monster_count_ = gain.monster_count_ + 1
    local one_drop_result = drop_level_result[gain.monster_count_]
  
    for _, gold in ipairs( one_drop_result.gold_list_ ) do 
        gain.total_gold_ = gain.total_gold_ + gold
    end 
    for _, exp in ipairs( one_drop_result.exp_list_ ) do 
        gain.total_exp_ = gain.total_exp_ + exp
    end 
    for _, honor in ipairs( one_drop_result.honor_list_ ) do 
        gain.total_honor_ = gain.total_honor_ + honor
    end 
    for _, diamond in ipairs( one_drop_result.diamond_list_ ) do 
        gain.total_diamond_ = gain.total_diamond_ + diamond 
    end 
    gain.item_count_ = gain.item_count_ + #one_drop_result.item_id_list_

    --c_safe( tconcat( one_drop_result.item_id_list_, "," ))
end 


function sweep_chess_new( act_inst_id, player, enter_type )
    local inst_id = ACTIVITY_INSTANCE_CONFIG[act_inst_id].InstanceId
    local load_drop_data_task = 
    {
        act_inst_id_    = act_inst_id,
        enter_type_     = enter_type,
        extra_param_    = {},
        x_              = 0,
        z_              = 0,
        player_id_      = player.player_id_, 
        inst_id_        = inst_id,
        unit_task_list_ = {},
        progress_        = 1,
        drop_data_      = {},
        drop_addition_  = {},
        load_success_   = false,
    }

    instance_t.calculate_instance_drop_data( player, load_drop_data_task )
    instance_t.unpack_sumary_to_every_monster( load_drop_data_task.drop_data_ )

    local drop_level_list = { 1,2,3,4,5 }
    -- 取品级物品
    local chess_win_count = chess_win_count()
    for i = 1, chess_win_count do
        local index = rand( #drop_level_list )
        local drop_level = drop_level_list[index]
        chess_gain_one_drop( load_drop_data_task.drop_data_, act_inst_id, drop_level )
    end 

    local player_drop = drop.calculate_player_instance_drop( load_drop_data_task.drop_data_, false )

    return player_drop
end 

function sweep_chess_plane_drop( _activity_instance_id )
    local count = 5000

    local total_gold = 0
    local items = {}
    local player = nil
    local ITEMS = ITEMS

    for _, target in pairs ( ctrl_mng.g_ctrl_mng ) do  
        if target:is_player() then
            player = target
            local enter_type = 6 

            for i = 1 , count do
                local player_drop = sweep_chess_new( _activity_instance_id, player, enter_type ) 
                
                local gold = player_drop.total_gold_
                local item_ids = player_drop.item_id_list_

                total_gold = total_gold + player_drop.total_gold_
                
                for  _, item_id in ipairs( item_ids ) do 
                    local item_count = items[item_id] or 0
                    items[item_id] = item_count + 1
                end
            end

            c_log( "total gold: %d", total_gold )

            local stat = {
                ["linghun"] = 0,
                ["equip_1"] = 0,
                ["equip_2"] = 0,
                ["equip_3"] = 0,
                ["equip_4"] = 0,
                ["equip_5"] = 0,
                ["other"] = 0
            }

            for item_id , item_count in pairs( items ) do
                if EQUIPS[ item_id ] then
                    if ITEMS[ item_id ].Quality == 2 then
                        stat.equip_2 = stat.equip_2 + item_count

                    elseif ITEMS[ item_id ].Quality == 3 then
                        stat.equip_3 = stat.equip_3 + item_count

                    elseif ITEMS[ item_id ].Quality == 4 then
                        stat.equip_4 = stat.equip_4 + item_count

                    elseif ITEMS[ item_id ].Quality == 5 then
                        stat.equip_5 = stat.equip_5 + item_count

                    elseif ITEMS[ item_id ].Quality == 1 then
                        stat.equip_1 = stat.equip_1 + item_count
                    end

                elseif ITEMS[ item_id ].UseType == 5 then
                    stat.linghun = stat.linghun + item_count

                end
            end

            for item_type, num in pairs( stat ) do
                c_log("%s, %d%%", item_type, num / count * 100 )
            end
        end 
    end 
end 

--
-- 扫出有用的掉落ID
--
function pass_activity_instance_config()
    local drop_id_list = {}

    for _, act_cfg in pairs( ACTIVITY_INSTANCE_CONFIG ) do 
        if act_cfg.Drop then 
            for _, drop_id in ipairs( act_cfg.Drop[1] ) do 
                drop_id_list[ drop_id ] = true
            end 
        end
        if act_cfg.HolidayDrop then 
            for _, drop_id in ipairs( act_cfg.HolidayDrop[1] ) do 
                drop_id_list[ drop_id ] = true
            end
        end
        if act_cfg.HeiAnZhiMenDrop then 
            for _, drop_id in ipairs( act_cfg.HeiAnZhiMenDrop[1] ) do 
                drop_id_list[ drop_id ] = true
            end 
        end
        if act_cfg.HeiAnZhiMenHolidayDrop then 
            for _, drop_id in ipairs( act_cfg.HeiAnZhiMenHolidayDrop[1] ) do 
                drop_id_list[ drop_id ] = true
            end 
        end
        if act_cfg.TeamDrop then 
            for _, drop_id in ipairs( act_cfg.TeamDrop[1] ) do 
                drop_id_list[ drop_id ] = true
            end 
        end
    end 

    --[[
    local list = {}
    for drop_id, _ in pairs( drop_id_list ) do 
        tinsert( list, drop_id )
    end 
    local list_str = tconcat( list, "," )
    c_safe(  "drop id: %s", list_str )
    --]]

    return drop_id_list
end

local function get_key_string_from_table( _key_value_table )
    local list = {}
    for key, _ in pairs( _key_value_table ) do 
        tinsert( list, key )
    end 
    return tconcat( list, "," ), #list
end 

local function print_use_and_not_use_result( _useful_list, _not_useful_list, _action_str )
    local list_str, num = get_key_string_from_table( _useful_list )
    c_safe( "after action: %s, usefull ist, num: %d, str: %s", _action_str, num, list_str )

    list_str, num  = get_key_string_from_table( _not_useful_list )
    c_safe(  "after action: %s, not usefull ist, num: %d, str: %s", _action_str, num, list_str )
end 

--
-- 掉落编号特殊映射 
--
function pass_drop_id_chane( _useful_list, _not_useful_list )
    for drop_id, cfg in pairs( DROP_ID_CHANGE_CONFIG ) do 
        -- 如果映射表中的ID出现在useful里面则把其映射的子集加进有效列表中
        if _useful_list[ drop_id ] then 
            for index, node in ipairs( cfg.DropIdChangeRule[1] ) do 
                if index % 2 == 0 then 
                    local another_drop_id = node[1] 
                    _useful_list[ another_drop_id ] = true 
                end 
            end 
        else -- 如果映射表中的ID不在useful出现，则把它加进废表中
            _not_useful_list[ drop_id ] = true
            --c_safe( " DROP_ID_CHANGE_CONFIG found not useful id: %d", drop_id )
            for index, node in ipairs( cfg.DropIdChangeRule[1] ) do 
                if index % 2 == 0 then 
                    local another_drop_id = node[1] 
                    if not _useful_list[ another_drop_id ] then 
                        _not_useful_list [ another_drop_id ] = true
                        --c_safe( " DROP_ID_CHANGE_CONFIG found not useful id: %d", another_drop_id )
                    end 
                end 
            end 
        end 
    end 
end 

--
-- 扫出所有的掉落ID
--
function find_all_drop_id()
    local drop_id_list = {}
    local PLANE_DROP = PLANE_DROP
    for drop_key, _ in pairs( PLANE_DROP ) do 
        local drop_id = floor( drop_key / 100 )
        drop_id_list [ drop_id ] = true
    end
    return drop_id_list
end 

--
-- 扫掉落表
--
function pass_plane_drop_config( _useful_list, _not_useful_list )
    local all_drop_id_list = find_all_drop_id()
    for drop_id, _ in pairs( all_drop_id_list ) do
        if not _useful_list[ drop_id ] then 
            _not_useful_list [ drop_id ] = true
        end 
    end 
end 

-- 
-- 扫废的掉落ID
--
function find_not_useful_drop_id()
    local file_name = "not_useful_drop_key.txt"
    local file = io.open( file_name , "w+")
    
    local useful_list = {}
    local not_useful_list = {}
    
    local useful_list = pass_activity_instance_config()
    
    --print_use_and_not_use_result( useful_list, not_useful_list, "pass_activity_instance_config" )

    pass_drop_id_chane( useful_list, not_useful_list ) 

    local file_content = sformat( "《掉落特殊映射》废的掉落ID: %s \n\n", get_key_string_from_table( not_useful_list ) ) 
    file:write( file_content ) 

    --print_use_and_not_use_result( useful_list, not_useful_list, "pass_drop_id_chane" )

    pass_plane_drop_config( useful_list, not_useful_list )

    file_content = sformat( "《掉落表》废的掉落ID: %s \n\n", get_key_string_from_table( not_useful_list ) ) 
    file:write( file_content ) 

    --print_use_and_not_use_result( useful_list, not_useful_list, "pass_plane_drop_config" )

    c_log( "正在算废的掉落key..." ) 
    local not_use_drop_key_list = {}
    for drop_key, _ in pairs( PLANE_DROP ) do 
        local drop_id = floor( drop_key / 100 )
        -- 已在废单里面的
        if not_useful_list [ drop_id ] then 
            not_use_drop_key_list [ drop_key ] = true
        end
        -- 在有用单里面也没找到
        if not useful_list[ drop_id ] then 
            not_use_drop_key_list [ drop_key ] = true
        end 
    end 

    local not_use_key_str, key_num = get_key_string_from_table( not_use_drop_key_list ) 
    file_content = sformat( "《掉落表》废的掉落KEY: %s \n\n", not_use_key_str ) 
    file:write( file_content ) 

    file:close()

    c_log( "计算完成, 废的key: %d 条 请查看%s", key_num, file_name )
end 


function test_find_match_pair_algorithm( _queue_len  )
    local wait_queue = {}
    local MIN = 1600
    local MAX = 2300

    _queue_len = _queue_len or 50

    for i = 1, _queue_len do
        local score = rand( 1600, 2300 )
        tinsert( wait_queue, score )
        c_safe( "----> score: %d", score )
    end 

    local range = 100

    local queue_len = #wait_queue
    local first_player_index = 1
    for i = 1, queue_len do 
        local first_player_score = wait_queue [first_player_index]  
        if not first_player_score then 
            c_safe( "----> done" )
            break
        end
    
        local find_second_player = false
        for second_player_index = first_player_index + 1, #wait_queue do 
            local second_player_score = wait_queue[ second_player_index ]
            if abs( first_player_score - second_player_score ) < range then 
                find_second_player = true
                c_safe( "-------> first_player_score: %d, second player score: %d", first_player_score, second_player_score )
                tremove( wait_queue, second_player_index )
                break
            end 
        end

        if find_second_player then 
            tremove( wait_queue,  first_player_index )
        else
            first_player_index = first_player_index + 1
        end 
    end

    for _, score in ipairs( wait_queue ) do 
        c_safe( "----> not match: %d", score )
    end 
        
end 

function time_to_date( _time )
    local _time = 1491397361
    local temp = os.date("*t", _time )

    c_debug()
    
    local aaa  = 1
end 

local function cmp_func( _a, _b )
    if _a.val_ == _b.val_ then 
        return _a.key_ < _b.key_ 
    end 
    return _a.val_ > _b.val_ 
end

function begin_test_bucket()
    local bucket = bucket_rank_t.bucket_rank_new() 
    
    local player_data = {}
    local answer = {}

    local SCORE_MAX = 5000
    local KEY_MAX = 500000

    for key = 1, KEY_MAX do 
        local val = random( SCORE_MAX )
        player_data [ key ] = val
        bucket:bucket_rank_set_player_score( key, val )
    end

    c_safe( "-------> begin random event " )

    -- random event
    for key = 1, KEY_MAX do 
        local event = random( 3 )
        if event == 1 then -- modify 
            local new_score  = random( SCORE_MAX )
            player_data [ key ] = new_score
            bucket:bucket_rank_set_player_score( key, new_score )
        elseif event == 2 then -- remove 
            player_data [ key ] = nil
            bucket:bucket_rank_remove_player_score( key )
        else -- insert new node
            local new_score  = random( SCORE_MAX )
            local new_key = key + KEY_MAX 
            player_data [ new_key ] = new_score
            bucket:bucket_rank_set_player_score( new_key, new_score )
        end 
    end 

    for key, val in pairs( player_data ) do 
        local sort_node = 
        {
            key_ = key,
            val_ = val,
        }
        tinsert( answer, sort_node )
    end 

    tsort( answer, cmp_func )

    for index, node in ipairs( answer ) do 
        --c_log( "rank %d , player key: %d, val: %d", index, node.key_, node.val_ )
    end 

    local check_key_list = bucket:bucket_rank_query_range( 1, #answer )   
    for rank, player_key in ipairs( check_key_list )do 
        local left_score = player_data [ player_key  ] 
  
        local right_score = answer [ rank ].val_ 
    
        if left_score ~= right_score then 
            c_safe( "error,  left score: %d right score: %d", left_score, right_score )
        end 
    end 

    c_safe( "pass !" )
end 


--
-- 加藏宝图纸
--
function add_treasure_hunt_map()
    for server_id, mng in pairs( player_mng.g_player_mng ) do
        for player_id, player in pairs( mng ) do
            local data = player:get_player_treasure_hunt_data()
            for map_type = 1, 3 do 
                local old_val = data.my_treasure_map_left_[ map_type ] or 0 
                data.my_treasure_map_left_[ map_type ] = old_val + 1000
            end
            player:send_my_treasure_hunt_map_left_num( data ) 
            c_safe( "add succ" )
        end
    end
end 

function add_treasure_hunt_fragment()
    for server_id, mng in pairs( player_mng.g_player_mng ) do
        for player_id, player in pairs( mng ) do
            local item_list = { } 
            for _,  item_id in pairs( TREASURE_HUNT_CONFIG.BestMapFragmentItemList ) do 
                local item = item_t.new_by_system( item_id , 5, "gm" )
                tinsert( item_list, item )                
            end 
            player:pick_item_list_safe( item_list, "gm" ) 
        end
    end
end 

-- BF刷榜
function bf_refresh_rank()
    rank_list_mng.begin_all_server_common_rank()
end 

-- 加羊皮纸
function add_parchemnt()
    test.add_parchment(1601,100)
    test.add_parchment(1602,100)
    test.add_parchment(1603,100)
end 

-- 通关副本
function finish_instance()
    test.finish_inst()
end 

function set_diamond( _val )
    local player = get_player()
    if player then 
        player.bind_diamond_ = _val
        player.non_bind_diamond_ = _val
        c_safe( "diamond=%d", _val)
    end 
end 

-- 进多人本
function join_multi()
    --do test.create_team() 
    --do test.test_enter_multi( 20001 )
    
    --次数 do test.clear_inst_count()
end 

function cal_pid()
    local key = 4294967299
    local sid = 1

    local sid , pid = honor_ladder_center.compute_player_sid_pid( key )

    c_safe( "%d pid", pid )
end 

function set_level( _level )
    local player = get_player()
    if not player then 
        c_safe( "not found player " )
        return
    end 
    test.set_level( player.player_id_, _level )
end 

function print_lucky_rate( _operation_id )
    local player = get_player()
    if not player then 
        c_safe( "not found player " )
        return
    end 
    local str = lucky_rate.format_lucky_rate_string( player, _operation_id or 1 )
    c_safe( str )
end 

function print_lucky_rate_for_logclient()
    local player = get_player()
    if not player then 
        c_safe( "not found player " )
        return
    end 
    lucky_rate.write_lucky_rate_log( player )   
end 

-- 觉醒材料，邮箱取
function add_awake()
    test.add_mail( 34, { 1121,1122,1123,1132,1133,1143}, {999,999,999,999,999,999,999 } )
end 

-- vip
function vip()
    test.recharge(1000000)
end 

-- 重置商城
function reset_mall()
    local player = get_player()
    if not player then 
        c_safe( "not found player " )
        return
    end 
    player:reset_shopping_mall()
end 

-- 怪物攻城 GM
--[[
-- 通过第一阶段，直接打BOSS, BF1执行
do guild_mng.finish_zone_inst(true)

-- 杀死BOSS
do guild_mng.finish_boss_inst(true)

-- 清公会次数 GAME
do guild_mng.clear_guild_pve()

-- 清个人次数 GAME
do player_mng.get_player(20,1).guild_pve_.enter_cnt_ = 0

--]]


-- 开服庆典要改开服时间
-- [[
--game_zone_mng.lua 在 on_server_list_response 
--...
--61     if g_server_list and g_zone_list then
--62         cache_server_list_in_my_zone_group()
--63         on_load_zone_info_finish()
--64     end
--　插入以下代码
--65     local server_id = g_gamesvr:c_get_server_id()
--66     local cfg = g_server_list[server_id]
--67     if cfg then
--68         cfg["ServerStartDate"] = "2017/4/10"
--69     end
--...
-- ]]


-- 公会赐福
-- guild_mng.refresh_donation(  guild_mng.g_guild_mng[ 公会id ] )
-- do guild_mng.add_diamond(  guild_mng.g_guild_mng[1], 1000000, "gm" )
-- do guild_mng.refresh_donation(  guild_mng.g_guild_mng[1] )    
--

-- tamriel
-- TAMRIEL playernum = 2
-- do tamriel_relic_game.start_event()  开启报名 
-- do tamriel_relic_game.end_event() 结束报名

-- 55 开启
-- test.start_five()
-- test.end_five()
-- test.win()


-- 打印当前卡斯尔天梯，玩家机器人索引
function print_ladder_robot_gs()
    local player = get_player()
    if not player then 
        c_safe( "not found player" )
        return
    end 
   
    local ladder_data = honor_ladder_game.get_player_ladder_data( player )
    if not ladder_data then 
        c_safe( "not found ladder data" )
        return
    end 

    c_safe( ladder_data.robot_gs_index_ )
end 

-- 设置玩家机器人索引
function set_ladder_robot_gs( _new_robot_gs_index )
    local player = get_player()
    if not player then 
        c_safe( "not found player" )
        return
    end 
   
    local ladder_data = honor_ladder_game.get_player_ladder_data( player )
    if not ladder_data then 
        c_safe( "not found ladder data" )
        return
    end 

    local player_id = player.player_id_
    local ladder_score = ladder_data.ladder_score_
    local match_count = ladder_data.match_count_
    local win_count = ladder_data.win_count_
    local robot_gs_index = _new_robot_gs_index

    honor_ladder_game.set_player_ladder_data( player_id, ladder_score, match_count, win_count, robot_gs_index )
    c_safe( "set robot gs index: %d", _new_robot_gs_index )
end 

function do_all()
    local player = get_player()
    if not player then 
        c_safe( "not found player" )
        return
    end 
    test.do_all()
    player:add_clear_system_unlock_tips()
end 

-- 不小心客户端点到充值
function clear_rechard()
    test.use_test_order_url(true)
end 

function awake()
    for ctrl_id, obj in pairs( ctrl_mng.g_ctrl_mng ) do  
        if obj:is_player() then 
            test.awake_all( obj.player_id_ ) 
        end 
    end 
end 

--[[
卡斯尔冠军赛GM

bf0刷日榜
do honor_ladder_center.on_center_timer_ladder_daily_match_end()\r

bf0报名
do honor_tournament_center.gm_start_sign_up()\r

bf0结束报名
do honor_tournament_center.gm_end_sign_up()\r

gs显示上周冠军
do honor_tour_game.on_timer_end_show_honor_tour_sign_up_count_down()\r

gs显示报名倒计时
do honor_tour_game.on_timer_start_show_honor_tour_sign_up_count_down()\r

gs显示本周冠军
do honor_tour_game.on_timer_start_show_honor_tour_battle_result()\r

--]]


--[[
-- five 55开关
-- 1、先把人数改成2人，时间改为60S
-- 2、然后开启报名，结束报名
-- do test.start_five()
--]]


