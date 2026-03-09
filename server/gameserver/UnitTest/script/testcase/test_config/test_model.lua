module( "test_model", package.seeall )

local MODEL_INFO = MODEL_INFO  
local SKILLS = SKILLS
local FILTER_MODEL_ID ={
        [2] = true,
        [3] = true,
        [4] = true,
        [5] = true
    }

local function test_hitfly()
    local check_result = true
    for i,v in pairs(MODEL_INFO) do
        for j,k in pairs(v.anim) do
            if FILTER_MODEL_ID[i] then
                break
            end

            if string.find(j,"hitfly") and not k["hj"] then
                check_result = false
                c_err(string.format("model_id: %d hitfly don't have hj", i))

            end

            if string.find(j,"hitfly") and not k["hit"] then
                check_result = false
                c_err(string.format("model_id: %d hitfly don't have hit", i))
            end

            if string.find(j,"hitfly") and not k["end"] then
                check_result =false
                c_err(string.format("model_id: %d hitfly don't have end",i))
            end
        end
    end

    return check_result 
end


local function test_diefly()
    local check_result = true
    for i,v in pairs(MODEL_INFO) do
        for j,k in pairs(v.anim) do
            if FILTER_MODEL_ID[i] then
                break
            end

            if string.find(j,"diefly") and not k["hj"] then
                check_result = false
                c_err(string.format("model_id: %d diefly don't have hj", i))

            end

            if string.find(j,"diefly") and not k["hit"] then
                check_result = false
                c_err(string.format("model_id: %d diefly don't have hit", i))
            end

            if string.find(j,"diefly") and not k["end"] then
                check_result =false
                c_err(string.format("model_id: %d diefly don't have end",i))
            end
        end
    end

    return check_result 
end

local function test_common_end()
    local check_result = true
    
    for i,v in pairs(MODEL_INFO) do
        for j,k in pairs(v.anim) do
            if FILTER_MODEL_ID[i] then
               break
            end 

            if not k["end"] then
               check_result = false
               c_err(string.format("model_id: %d , %s  error need end", i, j))
            end
        end
   end
   return check_result
end

local function jump_attack_check( check_result, j, k, skill_id) 
    if string.find(j,"pre") and not k["pre_end"] then
        check_result = false
        c_err(string.format("skill_id %d is JUMP_ATTACK,but skill_xxx_pre don't have pre_end", skill_id))

    elseif string.find(j,"idle") and not k["idle_end"] then
        check_result = false
        c_err(string.format("skill_id %d is JUMP_ATTACK,but skill_xxx_idle don't have idle_end", skill_id))

    elseif string.find(j,"cast") and (not k["hj"] or not k["hit"] or not k["end"]) then
        check_result = false
        c_err(string.format("skill_id %d is JUMP_ATTACK,but skill_xxx_cast don't have hj,hit,end", skill_id))
    end

    return check_result
end

local function rush_attack_check( check_result, j, k, skill_id) 
    if string.find(j,"pre") and not k["pre_end"] then
        check_result = false
        c_err(string.format("skill_id %d is RUSH_ATTACK,but skill_xxx_pre don't have pre_end", skill_id))

    elseif string.find(j,"idle") and not k["idle_end"] then
        check_result = false
        c_err(string.format("skill_id %d is RUSH_ATTACK,but skill_xxx_idle don't have idle_end", skill_id))

    elseif string.find(j,"cast") and not k["end"] then
        check_result = false
        c_err(string.format("skill_id %d is RUSH_ATTACK,but skill_xxx_cast don't have end", skill_id))
    end

   return check_result
end

local function persist_attack_check( check_result, j, k, skill_id) 
    if string.find(j,"pre") and not k["pre_end"] then
        check_result = false
        c_err(string.format("skill_id %d is PERSIST_ATTACK,but skill_xxx_pre don't have pre_end", skill_id))

    elseif string.find(j,"idle") and not k["loop"] then
        check_result = false
        c_err(string.format("skill_id %d is PERSIST_ATTACK,but skill_xxx_idle don't have loop", skill_id))

    elseif string.find(j,"cast") and not k["end"] then
        check_result = false
        c_err(string.format("skill_id %d is PERSIST_ATTACK,but skill_xxx_cast don't have end", skill_id))
    end

    return check_result
end


local function test_dots_3section()
  local check_result = true

    for i,v in pairs(MODEL_INFO) do
        for j,k in pairs(v.anim) do
            if string.find(j,"skill") and ( string.find(j,"pre") or string.find(j,"idle") or string.find(j,"cast") ) then
                local del_suffix = nil
                if string.find(j,"pre") then
                    del_suffix = string.sub(j, 1, string.len(j)-4) 
                else
                    del_suffix = string.sub(j, 1, string.len(j)-5)
                end

                local skills = {}
                for m,n in pairs(SKILLS) do
                    for _, anim in pairs( n.anim_list ) do
                        if (del_suffix == anim[1]) then
                            table.insert(skills, m)
                        end
                    end
                end

                for _ , skill_id in pairs( skills ) do
                     if not SKILLS[skill_id] then
                        check_result = false
                        c_err(string.format("skill_id %d not exit in the SKILLS.lua", skill_id))
                     else
                        local action = SKILLS[skill_id].action[1]
                        if not action then
                            c_err("skill action is nil, skill_id: %d", skill_id)
                        else
                            local skill_type = action[1]

                            if ( skill_type == "JUMP_ATTACK") then
                               check_result = jump_attack_check(check_result,j,k,skill_id)

                            elseif ( skill_type == "RUSH_ATTACK") then
                               check_result = rush_attack_check(check_result,j,k,skill_id)

                            elseif ( skill_type == "PERSIST_ATTACK") then
                               check_result = persist_attack_check(check_result,j,k,skill_id)
                            end
                        end
                    end
                end
            end
        end
    end

    return check_result
end


-------------------

local TEST_FUNC ={
        [1] = test_dots_3section,
        [2] = test_hitfly,
        [3] = test_diefly,
        [4] = test_common_end
    }

function test()
    local check_result = true 

    for _,func in pairs(TEST_FUNC) do
        if func then
            if not func() then
                check_result = false
            end
        end
    end

    return check_result
end

