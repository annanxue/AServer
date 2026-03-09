 module( "test_player_t_holiday", package.seeall )

 local sformat = string.format
 local HOLIDAY_EVENT_SYS_CONFIG = HOLIDAY_EVENT_SYS_CONFIG

 local function test_holiday_award()
     local check_result = true
     local check_exist_random = true
     local check_exist_item = true
     local check_result_job = true
     local check_level_adapt = true

     --职业1
     for i, v in pairs(HOLIDAY_EVENT_SYS_CONFIG.RandomPackIndexJob1) do
         random_lib_id = v
         if not RANDOM_ITEM_LIBS[random_lib_id] then
             check_exist_random = false
             c_err(sformat("Id:%d is not existing in RANDOM_ITEM_LIBS", i))
         end
         for j, w in pairs (RANDOM_ITEM_LIBS[random_lib_id].ItemLib) do
             for jj, ww in pairs (w[2]) do
                 local item_id = ww[1]
                 if not ITEMS[item_id] then
                     check_exist_item = false
                     c_err(sformat("id:%d is not existing in ITEMS",item_id))
                 end
                 if ITEMS[item_id].JobLimit ~= 0 and ITEMS[item_id].JobLimit ~= 1 then
                     check_result_job = false
                     c_err(sformat("id:%d don't have the same job type item",item_id))
                 end
                 --[[
                 if ITEMS[item_id].UseLevel > drop.DROP_ADAPT_PLAYER_LEVEL[i] then
                     check_level_adapt = false
                     c_err(sformat("id:%d don't have the right level adapt",item_id))
                 end
                 --]]
             end
         end
     end
     
     --职业2
     for i, v in pairs(HOLIDAY_EVENT_SYS_CONFIG.RandomPackIndexJob2) do
         random_lib_id = v
         if not RANDOM_ITEM_LIBS[random_lib_id] then
             check_exist_random = false
             c_err(sformat("Id:%d is not existing in RANDOM_ITEM_LIBS", i))
         end
         for j, w in pairs (RANDOM_ITEM_LIBS[random_lib_id].ItemLib) do
             for jj, ww in pairs (w[2]) do
                 local item_id = ww[1]
                 if not ITEMS[item_id] then
                     check_exist_item = false
                     c_err(sformat("id:%d is not existing in ITEMS",item_id))
                 end
                 if ITEMS[item_id].JobLimit ~= 0 and ITEMS[item_id].JobLimit ~= 2 then
                     check_result_job = false
                     c_err(sformat("id:%d don't have the same job type item",item_id))
                 end
                 --[[
                 if ITEMS[item_id].UseLevel > drop.DROP_ADAPT_PLAYER_LEVEL[i] then
                     check_level_adapt = false
                     c_err(sformat("id:%d don't have the right level adapt",item_id))
                 end
                 --]]
             end
         end
     end

     --职业3
     for i, v in pairs(HOLIDAY_EVENT_SYS_CONFIG.RandomPackIndexJob3) do
         random_lib_id = v
         if not RANDOM_ITEM_LIBS[random_lib_id] then
             check_exist_random = false
             c_err(sformat("Id:%d is not existing in RANDOM_ITEM_LIBS", i))
         end
         for j, w in pairs (RANDOM_ITEM_LIBS[random_lib_id].ItemLib) do
             for jj, ww in pairs (w[2]) do
                 local item_id = ww[1]
                 if not ITEMS[item_id] then
                     check_exist_item = false
                     c_err(sformat("id:%d is not existing in ITEMS",item_id))
                 end
                 if ITEMS[item_id].JobLimit ~= 0 and ITEMS[item_id].JobLimit ~= 3 then
                     check_result_job = false
                     c_err(sformat("id:%d don't have the same job type item",item_id))
                 end
                 --[[
                 if ITEMS[item_id].UseLevel > drop.DROP_ADAPT_PLAYER_LEVEL[i] then
                     check_level_adapt = false
                     c_err(sformat("id:%d don't have the right level adapt",item_id))
                 end
                 --]]
             end
         end
     end

     check_result = check_exist_random and check_exist_item and check_result_job and check_level_adapt
     return check_result
 end
     
 local TEST_FUNC ={
     [1] = test_holiday_award
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
