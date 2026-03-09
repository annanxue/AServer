module( "test_energy", package.seeall )

local TIME_EVENT = TIME_EVENT 

local ipairs, random, tremove, sgsub, sformat = ipairs, math.random, table.remove, string.gsub, string.format

function test_Time()

    local check_result = true

    local time_ctrl_config = TIME_CTRL[time_ctrl.TIME_CTRL_DAILY_BUY_ENERGY]
    local diamond_list = ENERGY_CONFIG.EnergyCostDiamondList

    if time_ctrl_config.InitPoint ~= #diamond_list then
        check_result = false
        c_err("test_energy.test_Time, ENERGY_CONFIG and TIME_CTRL config not match")
    end

    return  check_result

end

-------------------
local TEST_FUNC ={
        [1] = test_Time,
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

