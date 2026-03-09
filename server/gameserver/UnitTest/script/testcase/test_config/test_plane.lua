module("test_plane",package.seeall)

local INSTANCE_CFG = INSTANCE_CFG
local MONSTER_DATA = MONSTER_DATA
local ACTIVITY_INSTANCE_CONFIG = ACTIVITY_INSTANCE_CONFIG
local WORLD_CONFIG = require "luacommon/setting/resource/world_config"
local sformat = string.format 
local PASS_TYPE_BOSS =1
local PASS_TYPE_TRIGGER =2
local TEST_PLANE = 1
local BATTLE_PLANE = 132001

local function test_sceneid()
    local check_result = true

    for i ,v in pairs(INSTANCE_CFG) do
        local  ins_id = v.InstanceId
        local  scene_id = v.SceneId 

        if ins_id and scene_id and not WORLD_CONFIG[scene_id]	then
            check_result =false
            c_err(sformat("ins_id: "..ins_id.." don't have scene_id "..scene_id))
        end
    end
    return check_result
end 

local function test_instance_id()
    local check_result =true
    for i ,v in pairs (ACTIVITY_INSTANCE_CONFIG) do
        local ins_id =v.InstanceId
        if ins_id and not INSTANCE_CFG[ins_id] then
            check_result =false
            c_err(sformat("activity ins_id: "..ins_id.." don't have ins_id "..ins_id))
        end
    end
    return check_result
end

local  function test_pass()
    local check_result = true
    for i ,v in pairs(ACTIVITY_INSTANCE_CONFIG) do
        local  ins_id = v.InstanceId
        local isopen = v.IsOpen   ---是否开启
        if ins_id and INSTANCE_CFG[ins_id] then
            local finish = INSTANCE_CFG[ins_id].FinishCondition  ---通关类型（1：杀死boss ;2: 机关触发）
            local pass  = INSTANCE_CFG[ins_id].PassCondition     ---通关条件（1：boss_id； 2：可以不填写）  
            if isopen == 1 and finish then
                if finish < PASS_TYPE_BOSS or finish > PASS_TYPE_TRIGGER then
                    check_result =false
                    c_err(sformat("Instid: %d FinishCondition: %d is ERRO!!!",ins_id,finish))
                end
                if finish == PASS_TYPE_BOSS and  not pass then
                    check_result =false
                    c_err(sformat("Instid: %d PassCondition  is nil!!!",ins_id))
                end

                if finish == PASS_TYPE_BOSS and pass and  ins_id ~= BATTLE_PLANE and ins_id ~= TEST_PLANE  then
                    for _, v in pairs (pass) do
                        if not MONSTER_DATA[v]  then
                            check_result =false    
                            c_err(sformat("InstId: %d pass_boss_id: %d not found!",ins_id,v))
                        end
                    end  
                end
            end
        end
    end
    return check_result 
end 

local function test_inst_open()
    local check_result = true
    for i ,v in pairs(ACTIVITY_INSTANCE_CONFIG) do
        local  ins_id = v.InstanceId
        local  isopen = v.IsOpen   ---是否开放(活动副本表中)
        if INSTANCE_CFG[ins_id] then
            local  scene_id = INSTANCE_CFG[ins_id].SceneId  
            if not scene_id then
                check_result =false
                c_err(sformat("Instid: %d scene_id is nil!!",ins_id )) 
            else 
                local scene = WORLD_CONFIG[scene_id] 
                if scene then 
                    local isOk = scene.valid   ---场景是否加入了安装包
                    if isopen and isopen ==1 and isOk ~= 1 then
                        check_result =false
                        c_err(sformat("[scene not packaged in bundle!! ]Instid: %d scene_id: %d",ins_id,scene_id))
                    end
                end
            end
        end
    end
    return check_result
end 

local function  test_spawn()
    local check_result =true
    local SPAWN_DATA_PATH_FORMAT = "luacommon/setting/spawn/editSPAWN_DATA_%d"
    for i, v in pairs(ACTIVITY_INSTANCE_CONFIG) do
        local ins_id =v.InstanceId
        local  isopen = v.IsOpen   ---是否开放(活动副本表中)
        if ins_id and INSTANCE_CFG[ins_id] then
            local spawnid = INSTANCE_CFG[ins_id].SpawndataId
            if spawnid and isopen == 1 and ins_id  ~=1  then
                local dataPath = sformat(SPAWN_DATA_PATH_FORMAT, spawnid)
                if not dataPath then
                    check_result=false
                    c_err(sformat("Instid: %d spawn_data is nil!!",ins_id))
                end
                if dataPath then
                    if g_debug then
                        f, err = io.open(sformat("%s.lua", dataPath))
                    else
                        f, err = io.open(sformat("%d.lc", dataPath))
                    end
                    if err then
                        check_result=false
                        c_err( sformat("[spawn_data not exist] inst_id: %d, data_id: %d", i, spawnid))
                    end
                end
            end
        end
    end
    return check_result
end
local TEST_FUNC ={
        [1] = test_sceneid,
        [2] = test_pass,
        [3] = test_inst_open,
        [4] = test_instance_id,
       -- [5] = test_spawn,

    }

function test()
    local check_result = true 
    c_safe(sformat("INSTANCE_config check start !!"))
    
    for _,func in pairs(TEST_FUNC) do
        if func then
            if not func() then
                check_result = false
            end
        end
    end
    if not check_result then
       c_safe(sformat("check not pass !!"))
    end
    c_safe(sformat("INSTANCE_config check over !!"))
    
    return check_result
end

