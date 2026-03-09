module( "test_system_unlock", package.seeall )

local sformat = string.format
local SYSTEM_UNLOCK = SYSTEM_UNLOCK 
local QUESTS = QUESTS

local function test_system_unlock()
    local check_quest_task = true
    local check_level_fit = true
    local check_result = true

    for i, v in pairs (SYSTEM_UNLOCK) do
        if v.unlock_task then
            if v.unlock_task[1][1] ~= math.floor( v.unlock_task[1][2]/100 ) then
                check_quest_task = false
                c_err(sformat("SYSTEM_UNLOCK's id:%d with quset_id:%d is not fittable with SYSTEM_UNLOCK's task_id:%d",v.id,v.unlock_task[1][1],v.unlock_task[1][2]))
            end
            local unlock_system_level = v.unlock_level
            local quest_level = QUESTS[v.unlock_task[1][1]].Level
            if unlock_system_level > quest_level then
                check_level_fit = false
                c_err(sformat("SYSTEM_UNLOCK's id:%d with unlock_system_level:%d is not fittable with quest_level:%d",v.id,unlock_system_level,quest_level))
            end
        end
    end

    check_result = check_quest_task and check_level_fit
    return check_result

end

local TEST_FUNC ={
    [1] = test_system_unlock
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
    print (check_result)
    return check_result
end






