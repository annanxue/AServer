module( "test_item", package.seeall )

local ITEMS = ITEMS 

local function test_packmax()
    local check_result = true

    for i,v in pairs( ITEMS ) do
        if ( not v.PackMax ) or ( v.PackMax <= 0 ) then 
            check_result = false
            c_err(string.format("item_id:%d PackMax: %s is invalid!",i, tostring(v.PackMax )))
        end
    end

    return check_result
end

-- 物品_礼包39表.xls
local function test_gacha()
    local check_result = true
    for k,v in pairs(ITEMS) do
        if v.UseType == item_t.ITEM_USE_TYPE_RANDOM_GIFT_EQUIP and v.UseType2 and not GACHA_CFG[v.UseType2] then
            c_err("invalid 39 item.id:%d", k)
            check_result = false
        end
    end
    return check_result
end


-------------------
local TEST_FUNC ={
        [1] = test_packmax,
        [2] = test_gacha,
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

