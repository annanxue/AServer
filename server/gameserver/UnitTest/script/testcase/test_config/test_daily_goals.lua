module ("test_daily_goals", package.seeall)

local sformat = string.format
local DAILY_GOALS = DAILY_GOALS

local function test_daily_goals()
    local check_itemlist = true
    local check_randomcnt = true

    for i, v in pairs( DAILY_GOALS.cfg ) do
            if not v.ItemList[1][1] then
                check_itemlist = false
                c_err(sformat( "In DAILY_GOALS, ItemList is empty which id is :%d" ),id)
            end
            if not v.RandomCnt[1][1] then
                check_randomcnt = false
                c_err(sformat( "In DAILY_GOALS, RandomCnt is empty which id is :%d" ),id)
            end
    end
    
    check_result = check_itemlist and check_randomcnt 
    return check_result
end

local TEST_FUNC = {
    [1] = test_daily_goals,
}

function test()
    local check_result = true

    for _,func in pairs( TEST_FUNC ) do
        if func then
            if not func() then
                check_result = false
            end
        end
    end
    
    return check_result
end

