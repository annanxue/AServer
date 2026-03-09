module( "test_monster", package.seeall )

local MONSTER_DATA = MONSTER_DATA 
local MODEL_INFO = MODEL_INFO
local MODELS = MODELS
local SCULPTS = SCULPTS
local CHAR_AI = require "luacommon/setting/ai/CHAR_AI"
local sformat= string.format
local NO_CHECK_TYPE =4

local function test_is_hit()
    local check_result = true

    for i,v in pairs(MONSTER_DATA) do
        local model_id = v.model_id_
        local is_hit = v.is_hit_
        local is_repulse  = v.is_repulse_
        local npc_type = v.npc_type_

        local is_has_hit = false

        if npc_type ~= NO_CHECK_TYPE then
            if  MODEL_INFO[model_id] and MODEL_INFO[model_id]["anim"]["hit"] then
                is_has_hit = true
            end

            if (is_hit == 1 or is_repulse == 1) and (not is_has_hit) then
                check_result = false
                c_err(sformat("monster_id: %d need motion :hit", i))
            end 
        end
    end

    return check_result
end

local function test_is_hit_fly()
    local check_result = true

    for i,v in pairs(MONSTER_DATA) do
        local model_id = v.model_id_
        local is_hit_fly = v.is_hit_fly_
        local npc_type = v.npc_type_
        local is_has_hit_fly = false

        if npc_type ~= NO_CHECK_TYPE then
            if MODEL_INFO[model_id] and MODEL_INFO[model_id]["anim"]["hitfly"] then
                is_has_hit_fly = true
            end

            if (is_hit_fly == 1) and (not is_has_hit_fly) then
                check_result = false
                c_err(sformat("monster_id: %d need motion :hitfly", i))
            end 
        end
    end

    return check_result
end

local function test_is_ko()
    local check_result = true

    for i,v in pairs(MONSTER_DATA) do
        local model_id = v.model_id_
        local is_ko = v.is_ko_
        local npc_type = v.npc_type_
        local is_has_down = false

        if npc_type ~= NO_CHECK_TYPE then
            if MODEL_INFO[model_id] and MODEL_INFO[model_id]["anim"]["down"]then
                is_has_down = true
            end
            
            if (is_ko == 1) and (not is_has_down) then
                check_result = false
                c_err(sformat("monster_id: %d need motion : down", i))
            end 
        end
    end

    return check_result
end

local function test_die_normal()
    local check_result = true

    for i,v in pairs(MONSTER_DATA) do
        local model_id = v.model_id_
        local npc_type = v.npc_type_  --怪物的类型（0：普通怪物; 1:NPC ;2:主角机器人；3：无需die动作的怪物）

        if npc_type ~= NO_CHECK_TYPE then 
            if not MODEL_INFO[model_id] then
                check_result = false
                c_err(sformat("monster_id: %d model_id: %d not int MODEL_INFO:", i, model_id))         
            end
            if npc_type == 0 then
                if MODEL_INFO[model_id] and ( not MODEL_INFO[model_id]["anim"]["die"]) then
                    check_result = false
                    c_err(sformat("monster_id: %d need motion : die", i))
                end
            end
        end
    end

    return check_result
end

local function test_die_fly_back()
    local check_result = true

    for i,v in pairs(MONSTER_DATA) do
        local model_id = v.model_id_
        local is_hit_fly_death = v.is_hit_fly_death_
        local npc_type = v.npc_type_
        local is_has_die_diefly = false

        if npc_type ~=NO_CHECK_TYPE then 
            if not MODEL_INFO[model_id] then
                check_result = false
                c_err(sformat("monster_id: %d model_id: %d not int MODEL_INFO:", i, model_id))
            end

            if MODEL_INFO[model_id] and MODEL_INFO[model_id]["anim"]["diefly"] then
                is_has_die_diefly = true
            end

            if (is_hit_fly_death == 1) and (not is_has_die_diefly) then
                check_result = false
                c_err(sformat("monster_id: %d model_id: %d need motion : diefly", i, model_id))
            end 
        end
    end

    return check_result
end

local function test_is_float()
    local  check_result =true
    for i,v in pairs(MONSTER_DATA) do
        local model_id = v.model_id_
        local is_hit_float = v.is_hit_float_
        local npc_type = v.npc_type_
        local is_has_float = false
        
        if npc_type ~=NO_CHECK_TYPE then

            if not MODEL_INFO[model_id] then
                check_result = false
                c_err(sformat("monster_id: %d model_id: %d not int MODEL_INFO:", i, model_id))
            end

            if MODEL_INFO[model_id] and (MODEL_INFO[model_id]["anim"]["hitfloat"])and (MODEL_INFO[model_id]["anim"]["diefloat"]) then
                is_has_float = true
            end
            
            if (is_hit_float == 1) and (not is_has_float) then
                check_result = false
                c_err(sformat("monster_id: %d model_id: %d need motion : [hitfloat] or [diefloat]", i, model_id))
            end 
        end
    end

    return check_result

end

local function test_ai()
    local check_result =true  
    for i,v in pairs(MONSTER_DATA) do
        if v.default_ai_ then
            for _, ai_id in ipairs( v.default_ai_[1] or {} ) do 
                local monster_id = v.type_id_
                local npc_type = v.npc_type_
                if npc_type ~=NO_CHECK_TYPE then 
                    if ai_id and not CHAR_AI[ai_id] then
                        check_result =false
                        c_err(sformat("monster_id: %d ai_id: %d can not find!!",monster_id,ai_id))
                    end
                end
            end 
        end 
    end
    return check_result
end 


local function test_model() 
    local check_result = true
    for  i,v in pairs(MONSTER_DATA) do
        local monster_id =v.type_id_
        local npc_type = v.npc_type_
        local model_id =v.model_id_  ---怪物表中造型id ==造型表（SCULPTS.lua）
        if npc_type ~=NO_CHECK_TYPE then 
            if not SCULPTS[model_id] then
                check_result =false
                c_err(sformat("monster_id: %d model_id: %d is nil!1",monster_id,model_id))
            else
                base_model =SCULPTS[model_id].base_model_id   ---造型表中基础模型 ==模型表（MODELS.lua）
                if not MODELS[base_model] then
                    check_result = false
                    c_err(sformat("monster_id: %d base_model: %d is nil!!",monster_id,base_model))
                end 
            end
        end
    end
    return check_result
end 

-------------------
local TEST_FUNC ={
       [1] = test_is_hit,
       [2] = test_is_hit_fly,
       [3] = test_is_ko,
       [4] = test_die_normal,
       [5] = test_die_fly_back,
       [6] = test_is_float,
       [7] = test_ai,
       [8] = test_model,
    }

function test()
    local check_result = true 
    c_safe(sformat("MONSTER_DATA_config check start !!"))

    for _,func in pairs(TEST_FUNC) do
        if func then
            if not func() then
                check_result = false
            end
        end
    end

    if not check_result then
        c_safe(sformat("check not pass!"))
    end
    c_safe(sformat("MONSTER_DATA_config check over !!"))

    return check_result
end

function open_ai_print( _id, _is_open ) 
    local obj = ctrl_mng.get_ctrl(  _id ) 
    if obj and obj.ai_ then 
        local ai = obj.ai_
	    ai.core_:c_set_rare_print( _is_open and 1 or 0 )
	    ai.core_:c_set_verbose_print( _is_open and 1 or 0 )
    end
end

function test_config( _id ) 
    local obj = ctrl_mng.get_ctrl( _id ) 
    local type_id = _id
    if obj and obj:is_monster() then 
        type_id = obj.type_id_
    end

    local monster_cfg = MONSTER_DATA[type_id] or MONSTER_DATA[_id]
    if not monster_cfg then 
        c_err( "[test_config] no config  type:%d", type_id  )
        return
    end
    local ai_ids = monster_cfg["default_ai_"][1]
    for _, id in pairs( ai_ids ) do 
        local cfg = CHAR_AI[id]
        local template = cfg and cfg["Template"] or "NONE"
        c_trace( "[test_config] type id:%d ai id:%d template:%s", type_id, id, template )
    end

end
