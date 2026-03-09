
module( "test", package.seeall )

local PRAY_SYS_CONFIG = PRAY_SYS_CONFIG
local PRAY_PLAYER_EVENT_CONFIG = PRAY_PLAYER_EVENT_CONFIG 
local PRAY_GLOBAL_EVENT_CONFIG = PRAY_GLOBAL_EVENT_CONFIG
local RANDOM_ITEM_LIBS = RANDOM_ITEM_LIBS
local ITEMS = ITEMS
local ADAPT_LEVEL = 12
local JOB_KIND = 3 
local RATE_MIN , RATE_MAX =  0 , 10000
local SYSTEM_OPEN_LEVEL = 1
local PRAR_SPECIAL_ITEM_ID = 1002
local PRAY_ONCE_DIMAOND_CONSUMPTION = 18
local PRAY_TEN_DIAMOND_CONSUMPTION = 160
local TIMES_TO_RESET_COUNTER = 100
local NEWCOMER_TARGET_APPLIABLE_LEVEL = 40
local NEWCOMER_ITEMS_NUM = 9
local pray_showitem_id_min , pray_showitem_id_max = 4000,4051 

local player_level_adapt  = return_player_level_adapt()

local  sformat = string.format

local function test_pray_sys_config()
    local check_result = true

    if PRAY_SYS_CONFIG.SystemOpenLevel[1] ~= SYSTEM_OPEN_LEVEL then
        c_err(sformat("SystemOpenLevel is wrong in PRAY_SYS_CONFIG"))
        check_result = false
    end

    if PRAY_SYS_CONFIG.PrayOnceDiamondConsumption[1] ~= PRAY_ONCE_DIMAOND_CONSUMPTION then
        c_err(sformat("PrayOnceDiamondConsumption is wrong in PRAY_SYS_CONFIG"))
        check_result = false
    end

    if PrayTenDiamondConsumption[1] ~= PRAY_TEN_DIAMOND_CONSUMPTION then
        c_err(sformat("PrayTenDiamondConsumption is wrong in PRAY_SYS_CONFIG"))
        check_result = false
    end

    if PRAY_SYS_CONFIG.PrayOnceDiamondConsumption[1] * 10 < PRAY_SYS_CONFIG.PrayTenDiamondConsumption[1] then
        c_err(sformat("PrayOnceDiamondConsumption*10 is cheaper then PrayTenDiamondConsumption in PRAY_SYS_CONFIG"))
        check_result = false
    end

    if (PRAY_SYS_CONFIG.DailyGlobalPrayEventRate[1] < RATE_MIN) or (PRAY_SYS_CONFIG.DailyGlobalPrayEventRate[1] > RATE_MAX) then
        c_err(sformat("DailyGlobalPrayEventRate is unacceptable value in PRAY_SYS_CONFIG"))
        check_result = false
    end

    if PRAY_SYS_CONFIG.TimesToResetCounter ~= TIMES_TO_RESET_COUNTER then
        c_err(sformat("TimesToResetCounter is wrong in PRAY_SYS_CONFIG"))
        check_result = false
    end

    if  PRAY_SYS_CONFIG.PrayItemId[1] ~= PRAR_SPECIAL_ITEM_ID then
        c_err(sformat("PrayItemId is wrong in PRAY_SYS_CONFIG"))
        check_result = false 
    end

    if  PRAY_SYS_CONFIG.NewcomerTargetAppliableLevel ~= NEWCOMER_TARGET_APPLIABLE_LEVEL then
        c_err(sformat("NewcomerTargetAppliableLevel is wrong in PRAY_SYS_CONFIG")) 
        check_result = false 
    end
    print (check_result)

    local pray_item_libs_for_job = {PRAY_SYS_CONFIG.PrayItemLibsForJob1,PRAY_SYS_CONFIG.PrayItemLibsForJob2,PRAY_SYS_CONFIG.PrayItemLibsForJob3}
    for i,v in pairs (pray_item_libs_for_job) do
        if #v ~= ADAPT_LEVEL then
            c_err(sformat("#PrayItemLibsForJob: %d  is wrong in PRAY_SYS_CONFIG",i))
            check_result = false
        else
            for ii,vv in pairs (v) do
                for j,u in pairs (RANDOM_ITEM_LIBS) do
                    if u.Id == vv then
                        for jj,uu in pairs (u.ItemLib) do
                            for jjj,uuu in pairs (uu[2]) do
                                local item_id = uuu[1]
                                local item_num = uuu[2]

                                if ITEMS[item_id] then
                                    if ITEMS[item_id].JobLimit ~= i or ITEMS[item_id].JobLimit ~= 0 then
                                        c_err(sformat("attr_id: %d attr_id: %d attr_id: %d ITEMS is wrong_JobLimit in RANDOM_ITEM_LIBS",u.Id,jj,jjj))
                                        check_result = false
                                    end

                                    if ITEMS[item_id].ItemType < 5 and item_num ~= 1 then
                                        c_err(sformat("attr_id: %d attr_id: %d attr_id: %d ITEMS_num is wrong in RANDOM_ITEM_LIBS",u.Id,jj,jjj))
                                        check_result = false
                                    end

                                    if ii == 1 and ( ITEMS[item_id].UseLevel <= 0 or ITEMS[item_id].UseLevel > player_level_adapt[ii] ) then
                                        c_err(sformat("attr_id: %d attr_id: %d attr_id: %d ITEMS is wrong_UseLevel in RANDOM_ITEM_LIBS",u.Id,jj,jjj)) 
                                        check_result = false
                                    else
                                        if ITEMS[item_id].UseLevel <= player_level_adapt[ii - 1] or ITEMS[item_id].UseLevel > player_level_adapt[ii] then
                                            c_err(sformat("attr_id: %d attr_id: %d attr_id: %d ITEMS is wrong_UseLevel in RANDOM_ITEM_LIBS",u.Id,jj,jjj))
                                            check_result = false
                                        end
                                    end
                                else
                                    c_err(sformat("attr_id: %d attr_id: %d attr_id: %d in RANDOM_ITEM_LIBS is nothingness_id for ITEMS",u.Id,jj,jjj))
                                    check_result = false
                                end
                            end
                        end
                    else
                        c_err(sformat("PrayItemLibsForJob: %d attr_id: %d in PRAY_SYS_CONFIG is nothingness_id for RANDOM_ITEM_LIBS",i,ii))
                        check_result = false
                    end
                end
            end
        end
    end

    local rare_three_items_for_job = {PRAY_SYS_CONFIG.RareThreeItemsForJob1,PRAY_SYS_CONFIG.RareThreeItemsForJob2,PRAY_SYS_CONFIG.RareThreeItemsForJob3}
    for i ,v in pairs (rare_three_items_for_job) do
        if #v ~= ADAPT_LEVEL then
            c_err(sformat("#RareThreeItemsForJob: %d  is wrong~=ADAPT_LEVEL in PRAY_SYS_CONFIG",i)) 
            check_result = false
        else
            for ii,vv in pairs (v) do
                if #vv ~=3 then
                    c_err(sformat("#RareThreeItemsForJob: %d  is wrong~=3 in PRAY_SYS_CONFIG",i,ii)) 
                    check_result = false 
                else
                    for iii,vvv in pairs (vv) do
                        local item_id = vvv

                        if ITEMS[item_id] then 
                            if item_id < pray_showitem_id_min or item_id > pray_showitem_id_max then
                                c_err(sformat("RareThreeItemsForJob: %d attr_id: %d  attr_id: %d is unacceptable values in PRAY_SYS_CONFIG ",i,ii,iii))
                                check_result = false
                            end

                            if ITEMS[item_id].ItemType ~= 5 then
                                c_err(sformat("RareThreeItemsForJob: %d attr_id: %d attr_id: %d ITEMS is wrong_ItemType in PRAY_SYS_CONFIG",i,ii,iii)) 
                                check_result = false
                            end

                            if ITEMS[item_id].DropModel ~= 3011 then
                                c_err(sformat("RareThreeItemsForJob: %d attr_id: %d attr_id: %d ITEMS is wrong_DropModel in PRAY_SYS_CONFIG",i,ii,iii)) 
                                check_result = false
                            end
                            --
                            --if
                            --
                        else
                            c_err(sformat("RareThreeItemsForJob: %d attr_id: %d  attr_id: %d in PRAY_SYS_CONFIG is nothingness_id for ITEMS",i,ii,iii)) 
                            check_result = false 
                        end
                    end
                end

            end
        end
    end

    if #(PRAY_SYS_CONFIG.NewcomerTargetPrayTimes) ~= NEWCOMER_ITEMS_NUM then
        c_err(sformat("#NewcomerTargetPrayTimes is wrong in PRAY_SYS_CONFIG "))
        check_result = false
    else
        if PRAY_SYS_CONFIG.NewcomerTargetPrayTimes[1] < 1 then
            c_err(sformat("#NewcomerTargetPrayTimes[1] is unacceptable ualue in PRAY_SYS_CONFIG "))
            check_result = false
        end

        for i = 2 , NEWCOMER_ITEMS_NUM do
            if PRAY_SYS_CONFIG.NewcomerTargetPrayTimes[i] <= PRAY_SYS_CONFIG.NewcomerTargetPrayTimes[i-1] then
                c_err(sformat("NewcomerTargetPrayTimes: %d  not bigger then before in PRAY_SYS_CONFIG",i))
                check_result = false
            end
        end
    end

    local newcomer_target_lib_for_job = {PRAY_SYS_CONFIG.NewcomerTargetLibForJob1,PRAY_SYS_CONFIG.NewcomerTargetLibForJob2,PRAY_SYS_CONFIG.NewcomerTargetLibForJob3}
    for i,v in pairs (newcomer_target_lib_for_job) do
        if #v ~= NEWCOMER_ITEMS_NUM then
            c_err(sformat("#NewcomerTargetLibForJob: %d is wrong in PRAY_SYS_CONFIG ",i))
            check_result = false
        end
        for ii,vv in pairs (v) do
            local item_id = vv
            if ITEMS[item_id] then
                if ITEMS[item_id].UseLevel ~= NEWCOMER_TARGET_APPLIABLE_LEVEL then
                    c_err(sformat("NewcomerTargetLibForJob: %d attr_id: %d in PRAY_SYS_CONFIG is wrong_UseLevel for ITEMS",i,ii))
                    check_result = false
                end
                if ITEMS[item_id].JobLimit ~= i or ITEMS[item_id].JobLimit ~= 0 then
                    c_err(sformat("NewcomerTargetLibForJob: %d attr_id: %d in PRAY_SYS_CONFIG is wrong_JobLimit for ITEMS",i,ii))
                    check_result = false
                end
            else
                c_err(sformat("NewcomerTargetLibForJob: %d attr_id: %d in PRAY_SYS_CONFIG is nothingness_id for ITEMS",i,ii))
                check_result = false
            end

        end
    end


    return check_result
end



-------------------
local TEST_FUNC ={
    [1] = test_pray_sys_config;
   -- [2] = test_pray_player_event_config;
   -- [3] = test_pray_global_event_config;
}

function test()
    for _,func in pairs(TEST_FUNC) do
        local check_result = true 

        if func then
            check_result = func()
        end

        print (check_result)
    end
end
