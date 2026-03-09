
module( "test_RANDOM_ITEM_LIBS", package.seeall )

local sformat = string.format
local RANDOM_ITEM_LIBS = RANDOM_ITEM_LIBS 
local ITEMS = ITEMS 
local RATE_MIN,RATE_MAX = 0,10000
local item_num_min,item_num_max = 1,2

local function test_random_item_libs()
    local check_result = true

    for i, v in pairs( RANDOM_ITEM_LIBS ) do

        for ii, vv in pairs( v.ItemLib ) do

            if  vv[1] < RATE_MIN  or  vv[1] >RATE_MAX  then
                c_err(sformat("attr_id: %d attr_id: %d ItemLib_item_rate is unaceptable values in RANDOM_ITEM_LIBS",i,ii))
                check_result = false
            end

            if #vv > 2  then

                if  vv[3] < RATE_MIN  or  vv[3] > RATE_MAX  then
                    c_err(sformat("attr_id: %d attr_id: %d ItemLib_rune_rate unaceptable values in RANDOM_ITEM_LIBS",i,ii))
                    check_result = false
                end
            end

            if not v.NotRealItemId then -- 表中并不是真正的物品id，比如神秘商人的，暂不检查
                for iii, vvv in pairs( vv[2] ) do
                    local item_id = vvv[1]
                    local item_num = vvv[2]

                    if not ITEMS[item_id] then
                        c_err(sformat("attr_id: %d attr_id: %d attr_id: %d item_id in RANDOM_ITEM_LIBS is unacceptable value for ITEMS",i,ii,iii))
                        check_result = false
                    end 

                    if item_num < item_num_min then
                        c_err(sformat("attr_id: %d attr_id: %d attr_id: %d item_num is unacceptable value in RANDOM_ITEM_LIBS",i,ii,iii))
                        check_result = false
                    end 

                    if ITEMS[item_id].UseType == 1 and item_num ~= item_num_min then
                        c_err(sformat("attr_id: %d attr_id: %d attr_id: %d item_num is unacceptable value in RANDOM_ITEM_LIBS for ITEMS.UseType",i,ii,iii))
                        check_result = false
                    end
                end
            end
        end
    end
    --print ("[1]",check_result)
    return check_result
end

-------------------
local TEST_FUNC ={
        [1] = test_random_item_libs
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

