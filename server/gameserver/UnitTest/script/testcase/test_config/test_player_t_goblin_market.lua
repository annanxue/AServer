
module( "test_player_t_goblin_market", package.seeall )

local sformat = string.format

local GOBLIN_MARKET_CONFIG = GOBLIN_MARKET_CONFIG
local RANDOM_ITEM_LIBS = RANDOM_ITEM_LIBS
local ITEMS = ITEMS
local ADAPT_LEVEL = 12
local SYSTEM_OPEN_LEVEL = 1
local RATE_MIN,RATE_MAX = 1,10000
local ITEM_RATE_NUM = 2
local REFRESH_TIME = 6

local function test_GOBLIN_MARKET_CONFIG()
    local check_result = true

    if ( #(GOBLIN_MARKET_CONFIG.NormalItemLibs) ~= ADAPT_LEVEL )  then
        c_err(sformat("NormalItemLibs is not right numbers in GOBLIN_MARKET_CONFIG"))
        check_result = false
    end

    if ( #(GOBLIN_MARKET_CONFIG.RareItemLibs) ~=  ADAPT_LEVEL ) then
        c_err(sformat("#RareItemLibs is not right numbers in GOBLIN_MARKET_CONFIG"))
        check_result = false

        for i=1 , ADAPT_LEVEL do
            local rare_item_id = GOBLIN_MARKET_CONFIG.RareItemLibs[i] 
            if ITEMS[rare_item_id] then
                if ITEMS[rare_item_id].Quality < 4 then
                    c_err(sformat("RareItemLibs : d% in GOBLIN_MARKET_CONFIG is not right Quality for ITEMS",i))
                    check_result = false
                end
            else
                c_err(sformat("RareItemLibs : d%  in GOBLIN_MARKET_CONFIG is nothingness for ITEMS ",i))
                check_result = false
            end
        end
    end

    if ( #(GOBLIN_MARKET_CONFIG.RedrawAllConsumingHonor) ~=  ADAPT_LEVEL ) then
        c_err(sformat("#RedrawAllConsumingHonor is not right numbers in GOBLIN_MARKET_CONFIG"))
        check_result = false
    end

    if ( GOBLIN_MARKET_CONFIG.SystemOpenLevel[1] ~= SYSTEM_OPEN_LEVEL ) then
        c_err(sformat("SystemOpenLevel is not right in GOBLIN_MARKET_CONFIG"))
        check_result = false
    end

    if GOBLIN_MARKET_CONFIG.RefreshRareItemInterval[1] ~= REFRESH_TIME then
        c_err(sformat("RefreshRareItemInterval is not right in GOBLIN_MARKET_CONFIG"))
        check_result = false
    end

    if #(GOBLIN_MARKET_CONFIG.RareItemRates) ~= ITEM_RATE_NUM then
        c_err(sformat("#RareItemRates is not right numbers in GOBLIN_MARKET_CONFIG"))
        check_result = false
    end

    for i = 1 ,ITEM_RATE_NUM do
        if ( GOBLIN_MARKET_CONFIG.RareItemRates[i] < RATE_MIN ) or ( GOBLIN_MARKET_CONFIG.RareItemRates[i] > RATE_MAX ) then
            c_err(sformat("RareItemRates is unacceptable value in GOBLIN_MARKET_CONFIG"))
            check_result = false
        end
    end

    --print ("[1]",check_result)
    return check_result
end

-------------------
local TEST_FUNC ={
        [1] = test_GOBLIN_MARKET_CONFIG
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
