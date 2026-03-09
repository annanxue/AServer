module( "test_mystic", package.seeall )

local sformat = string.format
local MYSTIC_SYS_CONFIG = MYSTIC_SYS_CONFIG
local MYSTIC_GOODS_CONFIG = MYSTIC_GOODS_CONFIG

local function test_mystic()
    local check_exist_RIL = true
    local check_exist_ITEM = true
    local check_job = true
    local check_result = true
    local check_price_type = true
    local check_level_fittable = true
    local check_goods = true
    
    for mystic_inx, mystic_award in pairs(MYSTIC_SYS_CONFIG.ItemLibJob1) do
        for level_inx, level_award in pairs(mystic_award) do
            if not RANDOM_ITEM_LIBS[level_award] then
                check_exist_RIL = false
                c_err(sformat("mystic_inx: %d with PkgIdJob1 with %d award is unacceptable value in RANDOM_ITEM_LIBS",mystic_inx,level_inx))
            end
            for i, v in pairs(RANDOM_ITEM_LIBS[level_award].ItemLib) do
                for ii, vv in pairs(v[2]) do
                    local random_idx = vv[1]
                    local random_num = vv[2]
                    if not ITEMS[MYSTIC_GOODS_CONFIG[random_idx].ItemId] then
                        check_exist_ITEM = false
                        c_err(sformat("%d isn't in ITEM",MYSTIC_GOODS_CONFIG[random_idx].ItemId))
                    end
                    if ITEMS[MYSTIC_GOODS_CONFIG[random_idx].ItemId].JobLimit ~= 0 and ITEMS[MYSTIC_GOODS_CONFIG[random_idx].ItemId].JobLimit ~= 1 then
                        check_job = false
                        c_err(sformat("random_idx: %d in MYSTIC_GOODS_CONFIG isn't fittable with the job1",random_idx))
                    end
                end
            end
        end
        if mystic_inx == 3 then
            for level_inx, level_award in pairs(mystic_award) do
                for i, v in pairs(RANDOM_ITEM_LIBS[level_award].ItemLib) do
                    for ii, vv in pairs(v[2]) do
                        local random_idx = vv[1]
                        local random_num = vv[2]
                        if MYSTIC_GOODS_CONFIG[random_idx].PriceType ~= 83 then
                            check_price_type = false
                            c_err(sformat("random_idx: %d in JOB1's MYSTIC_GOODS_CONFIG isn't fittable with the glory dealer",random_idx))
                        end
                        if ITEMS[MYSTIC_GOODS_CONFIG[random_idx].ItemId].UseType ~= 1 and MYSTIC_GOODS_CONFIG[random_idx].ItemId ~= 1801 then
                            check_goods = false
                            c_err(sformat("random_idx: %d in JOB1's MYSTIC_GOODS_CONFIG isn't equipment",random_idx))
                        end
                        ---[[
                        if level_inx > 1 and ITEMS[MYSTIC_GOODS_CONFIG[random_idx].ItemId].UseType == 1 and ITEMS[MYSTIC_GOODS_CONFIG[random_idx].ItemId].UseLevel > drop.DROP_ADAPT_PLAYER_LEVEL[level_inx] then
                            check_level_fittable = false
                            c_err(sformat("random_idx: %d in JOB1's MYSTIC_GOODS_CONFIG's level is higher than player",random_idx))
                        end
                        --]]
                    end
                end
            end
        end
    end


    for mystic_inx, mystic_award in pairs(MYSTIC_SYS_CONFIG.ItemLibJob2) do
        for level_inx, level_award in pairs(mystic_award) do
            if not RANDOM_ITEM_LIBS[level_award] then
                check_exist_RIL = false
                c_err(sformat("mystic_inx: %d with PkgIdJob1 with %d award is unacceptable value in RANDOM_ITEM_LIBS",mystic_inx,level_inx))
            end
            for i, v in pairs(RANDOM_ITEM_LIBS[level_award].ItemLib) do
                for ii, vv in pairs(v[2]) do
                    local random_idx = vv[1]
                    local random_num = vv[2]
                    if not ITEMS[MYSTIC_GOODS_CONFIG[random_idx].ItemId] then
                        check_exist_ITEM = false
                        c_err(sformat("%d isn't in ITEM",MYSTIC_GOODS_CONFIG[random_idx].ItemId))
                    end
                    if ITEMS[MYSTIC_GOODS_CONFIG[random_idx].ItemId].JobLimit ~= 0 and ITEMS[MYSTIC_GOODS_CONFIG[random_idx].ItemId].JobLimit ~= 2 then
                        check_job = false
                        c_err(sformat("random_idx: %d in MYSTIC_GOODS_CONFIG isn't fittable with the job2",random_idx))
                    end
                end
            end
        end
        if mystic_inx == 3 then
            for level_inx, level_award in pairs(mystic_award) do
                for i, v in pairs(RANDOM_ITEM_LIBS[level_award].ItemLib) do
                    for ii, vv in pairs(v[2]) do
                        local random_idx = vv[1]
                        local random_num = vv[2]
                        if MYSTIC_GOODS_CONFIG[random_idx].PriceType ~= 83 then
                            check_price_type = false
                            c_err(sformat("random_idx: %d in JOB2's MYSTIC_GOODS_CONFIG isn't fittable with the glory dealer",random_idx))
                        end
                        if ITEMS[MYSTIC_GOODS_CONFIG[random_idx].ItemId].UseType ~= 1 and MYSTIC_GOODS_CONFIG[random_idx].ItemId ~= 1801 then
                            check_goods = false
                            c_err(sformat("random_idx: %d in JOB1's MYSTIC_GOODS_CONFIG isn't equipment",random_idx))
                        end 
                        ---[[
                        if level_inx > 1 and ITEMS[MYSTIC_GOODS_CONFIG[random_idx].ItemId].UseType == 1 and ITEMS[MYSTIC_GOODS_CONFIG[random_idx].ItemId].UseLevel > drop.DROP_ADAPT_PLAYER_LEVEL[level_inx] then
                            check_level_fittable = false
                            c_err(sformat("random_idx: %d in JOB2's MYSTIC_GOODS_CONFIG's level is higher than player",random_idx))
                        end
                        --]]
                    end
                end
            end
        end
    end

    for mystic_inx, mystic_award in pairs(MYSTIC_SYS_CONFIG.ItemLibJob3) do
        for level_inx, level_award in pairs(mystic_award) do
            if not RANDOM_ITEM_LIBS[level_award] then
                check_exist_RIL = false
                c_err(sformat("mystic_inx: %d with PkgIdJob1 with %d award is unacceptable value in RANDOM_ITEM_LIBS",mystic_inx,level_inx))
            end
            for i, v in pairs(RANDOM_ITEM_LIBS[level_award].ItemLib) do
                for ii, vv in pairs(v[2]) do
                    local random_idx = vv[1]
                    local random_num = vv[2]
                    if not ITEMS[MYSTIC_GOODS_CONFIG[random_idx].ItemId] then
                        check_exist_ITEM = false
                        c_err(sformat("%d isn't in ITEM",MYSTIC_GOODS_CONFIG[random_idx].ItemId))
                    end
                    if ITEMS[MYSTIC_GOODS_CONFIG[random_idx].ItemId].JobLimit ~= 0 and ITEMS[MYSTIC_GOODS_CONFIG[random_idx].ItemId].JobLimit ~= 3 then
                        check_job = false
                        c_err(sformat("random_idx: %d in MYSTIC_GOODS_CONFIG isn't fittable with the job3",random_idx))
                    end
                end
            end
        end
        if mystic_inx == 3 then
            for level_inx, level_award in pairs(mystic_award) do
                for i, v in pairs(RANDOM_ITEM_LIBS[level_award].ItemLib) do
                    for ii, vv in pairs(v[2]) do
                        local random_idx = vv[1]
                        local random_num = vv[2]
                        if MYSTIC_GOODS_CONFIG[random_idx].PriceType ~= 83 then
                            check_price_type = false
                            c_err(sformat("random_idx: %d in JOB3's MYSTIC_GOODS_CONFIG isn't fittable with the glory dealer",random_idx))
                        end
                        if ITEMS[MYSTIC_GOODS_CONFIG[random_idx].ItemId].UseType ~= 1 and MYSTIC_GOODS_CONFIG[random_idx].ItemId ~= 1801 then
                            check_goods = false
                            c_err(sformat("random_idx: %d in JOB1's MYSTIC_GOODS_CONFIG isn't equipment",random_idx))
                        end 
                        ---[[
                        if level_inx > 1 and ITEMS[MYSTIC_GOODS_CONFIG[random_idx].ItemId].UseType == 1 and ITEMS[MYSTIC_GOODS_CONFIG[random_idx].ItemId].UseLevel > drop.DROP_ADAPT_PLAYER_LEVEL[level_inx] then
                            check_level_fittable = false
                            c_err(sformat("random_idx: %d in JOB3's MYSTIC_GOODS_CONFIG's level is higher than player",random_idx))
                        end
                        --]]
                    end
                end
            end
        end
    end

    check_result = check_exist_RIL and check_exist_ITEM and check_job and check_price_type and check_level_fittable and check_goods
    return check_result
end

local TEST_FUNC ={
    [1] = test_mystic
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
