module( "test_player_t_attend", package.seeall )

local sformat = string.format
local GACHA_CONFIG = GACHA_CFG
local RANDOM_ITEM_LIBS = RANDOM_ITEM_LIBS 
local ITEMS = ITEMS
local LEVEL_REWARD_CONFIG = LEVEL_REWARD_CONFIG
local ATTEND_CFG = ATTEND_CFG

local function test_daily_attend()
    local check_result_ATTEND = true
    local check_result_LEVEL = true
    local check_result_GACHA = true
    local check_result_RANDOM_ITEM = true
    local check_result_JOB = true
    local check_result = true
    local check_result_LEVEL = true



    for i, v in pairs(ATTEND_CFG) do --测试每日登陆奖励里配置的礼包ID在礼包配置表中是否存在
        for ii, vv in pairs (v.ItemList[1])do
            if not GACHA_CONFIG[vv] then
                c_err(sformat("ATTEND_CFG attr_id: %d with ItemList %d is unacceptable value in GACHA_CONFIG",i,ii))
                check_result_ATTEND = false
            end
        end
    end


   for i, v in pairs(LEVEL_REWARD_CONFIG) do --测试等级礼包奖励配置表里对应的礼包ID在礼包配置表中是否存在
       for ii, vv in pairs (v.ItemList[1]) do
           if not GACHA_CONFIG[vv] then
               c_err(sformat("LEVEL_REWARD_CONFIG attr_id: %d with ItemList %d is unacceptable value in GACHA_CONFIG",i,ii))
               check_result_LEVEL = false
           end
       end
   end


    for i, v in pairs(GACHA_CONFIG) do --测试礼包奖励配置表里随机物品的ID在随机物品表里是否存在
       
        for ii, vv in pairs(v.PkgIdJob1) do --职业1
            random_lib_id = vv
            if not RANDOM_ITEM_LIBS[random_lib_id] then
                c_err(sformat("attr_id: %d with PkgIdJob1 with %d item_num is unacceptable value in RANDOM_ITEM_LIBS",i,ii))
                check_result_GACHA = false
            end
        end

        for ii, vv in pairs(v.PkgIdJob2) do --职业2
            random_lib_id = vv
            if not RANDOM_ITEM_LIBS[random_lib_id] then
                c_err(sformat("attr_id: %d with PkgIdJob2 with %d item_num is unacceptable value in RANDOM_ITEM_LIBS",i,ii))
                check_result_GACHA = false
            end 
        end

        for ii, vv in pairs(v.PkgIdJob3) do --职业3
            random_lib_id = vv
            if not RANDOM_ITEM_LIBS[random_lib_id] then
                c_err(sformat("attr_id: %d with PkgIdJob3 with %d item_num is unacceptable value in RANDOM_ITEM_LIBS",i,ii))
                check_result_GACHA = false
            end
        end
    end


     for i, v in pairs( RANDOM_ITEM_LIBS ) do --测试随机物品表中的物品ID在物品表中是否存在
         if not v.NotRealItemId then -- 表中并不是真正的物品id，比如神秘商人的，暂不检查
             for ii, vv in pairs( v.ItemLib ) do
                 for iii, vvv in pairs (vv[2]) do
                     local item_id = vvv[1]
                     local item_num = vvv[2]
                     if not ITEMS[item_id] then
                         c_err(sformat("attr_id: %d attr_id: %d attr_id: %d  item_id in RANDOM_ITEM_LIBS is unacceptable value for ITEMS",i,ii,iii))
                         check_result_RANDOM_ITEM = false
                     end
                 end
             end
         end
     end

    
     for i, v in pairs(GACHA_CONFIG) do --测试礼包配置表中发放给职业1的物品是否适用于职业1
         for ii, vv in pairs(v.PkgIdJob1) do
             for j, w in pairs (RANDOM_ITEM_LIBS[vv].ItemLib) do
                 for jj, ww in pairs (w[2]) do
                     local item_id = ww[1]
                     if ITEMS[item_id].JobLimit ~= 0 and ITEMS[item_id].JobLimit ~= 1 then
                         check_result_JOB = false
                         c_err(sformat("%d PkgIdJob1's %d don't have the same job type item",i,ii))
                     end
                 end
             end
         end
     end


     for  i, v in pairs(GACHA_CONFIG) do --测试礼包配置表中发放给职业1的物品是否适用于职业2
         for ii, vv in pairs(v.PkgIdJob2) do
             for j, w in pairs (RANDOM_ITEM_LIBS[vv].ItemLib) do
                 for jj, ww in pairs (w[2]) do
                     local item_id = ww[1]
                     if ITEMS[item_id].JobLimit ~= 0 and ITEMS[item_id].JobLimit ~= 2 then
                         check_result_JOB = false
                         c_err(sformat("%d PkgIdJob2's %d don't have the same job type item",i,ii))
                     end
                 end
             end
         end
     end

     for  i, v in pairs(GACHA_CONFIG) do --测试礼包配置表中发放给职业1的物品是否适用于职业3
         for ii, vv in pairs(v.PkgIdJob3) do
             for j, w in pairs (RANDOM_ITEM_LIBS[vv].ItemLib) do
                 for jj, ww in pairs (w[2]) do
                     local item_id = ww[1]
                     if ITEMS[item_id].JobLimit ~= 0 and ITEMS[item_id].JobLimit ~= 3 then
                         check_result_JOB = false
                         c_err(sformat("%d PkgIdJob3's %d don't have the same job type item",i,ii))
                     end
                 end
             end
         end
     end

    
   -- for attend_id, attend_item in pairs(ATTEND_CFG) do --测试登录奖励发放物品使用等级不超过玩家当前等级
         
     -- for gacha_id, gacha_item in pairs (GACHA_CONFIG[attend_item.ItemList[1][1]].PkgIdJob1) do 
       --[[      for i, v in pairs (RANDOM_ITEM_LIBS[gacha_item].ItemLib) do
                 for ii, vv in pairs (v[2]) do
                     local item_id = vv[1]
                     if drop.DROP_ADAPT_PLAYER_LEVEL[gacha_id] > 40 and ITEMS[item_id].UseLevel >= drop.DROP_ADAPT_PLAYER_LEVEL[gacha_id] then
                         check_result_LEVEL = false;
                         c_err(sformat("%d PkgIdJob1's %d don't have the correct award according to player's level",attend_id,gacha_id))
                     end
                 end
             end
         end --]]

       -- for gacha_id, gacha_item in pairs (GACHA_CONFIG[attend_item.ItemList[1][1]].PkgIdJob2) do 
        --[[    for i, v in pairs (RANDOM_ITEM_LIBS[gacha_item].ItemLib) do
                 for ii, vv in pairs (v[2]) do
                     local item_id = vv[1]
                     if drop.DROP_ADAPT_PLAYER_LEVEL[gacha_id] > 40 and ITEMS[item_id].UseLevel >= drop.DROP_ADAPT_PLAYER_LEVEL[gacha_id] then
                         check_result_LEVEL = false;
                         c_err(sformat("%d PkgIdJob2's %d don't have the correct award according to player's level",attend_id,gacha_id))
                     end
                 end
             end
         end --]]
 
       --  for gacha_id, gacha_item in pairs (GACHA_CONFIG[attend_item.ItemList[1][1]].PkgIdJob3) do 
         --[[    for i, v in pairs (RANDOM_ITEM_LIBS[gacha_item].ItemLib) do
                 for ii, vv in pairs (v[2]) do
                     local item_id = vv[1]
                     if drop.DROP_ADAPT_PLAYER_LEVEL[gacha_id] > 40 and ITEMS[item_id].UseLevel >= drop.DROP_ADAPT_PLAYER_LEVEL[gacha_id] then
                         check_result_LEVEL = false;
                         c_err(sformat("%d PkgIdJob3's %d don't have the correct award according to player's level",attend_id,gacha_id))
                     end
                 end
             end
         end

     end  --]]

     
     check_result = check_result_GACHA and check_result_RANDOM_ITEM and check_result_ATTEND and check_result_LEVEL and check_result_JOB and check_result_LEVEL
     return check_result

 end


local TEST_FUNC ={ 
    [1] = test_daily_attend
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








