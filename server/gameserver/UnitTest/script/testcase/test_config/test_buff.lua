module( "test_buff", package.seeall )

local BUFFS = BUFFS

local function test_ddelete()
    local check_result = true

    for i,v in pairs(BUFFS) do
        local Effect = v.Effect
        for _,k in pairs(Effect) do
            for _,n in pairs(k) do
                if n == "PERIOD_EFFECT" and v.DeadDelete ~= 1 then
                    check_result = false
                    c_err(string.format("buff_id:%d is PERIOD_EFFECT but DeadDelete is not 1!",i))
                end
            end
        end
    end

    return check_result
end

local function test_beneficialtype()
    local check_result = true
    local DEBUFF = {
        ["STATE_DAZED"] = true,
    }

    for i,v in pairs(BUFFS) do
        local Effect = v.Effect
        for _,k in pairs(Effect) do
            if k[1] and DEBUFF[ k[1] ] and v.BeneficialType == 1 then 
                check_result = false
                c_err(string.format("buff_id:%d is DEBUFF,but BeneficialType is 1",i))
            end
        end
    end

    return check_result
end


local function test_reflect()
    local check_result = true

    for i,v in pairs(BUFFS) do
        local Effect = v.Effect
        for _,k in pairs(Effect) do
            if k[1] and k[2] and k[1] == "BUFF_REFLECT" then
                for _, group_id in pairs(k[2]) do
                    for buff_id, buff  in pairs(BUFFS) do
                        if buff.Group == group_id and buff.BeneficialType == 1 then
                             check_result = false
                             c_err(string.format("buff_id:%d Effect is BUFF_REFLECT,but more then one reflect buff :%d  BeneficialType is 1",i, buff_id))
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
        [1] = test_ddelete,
        [2] = test_reflect,
        [3] = test_beneficialtype
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

