module( "test_main_quest", package.seeall )

local TARGETS,QUESTS,ITEMS,ACTIVITY_INSTANCE_CONFIG =TARGETS,QUESTS,ITEMS,ACTIVITY_INSTANCE_CONFIG 
local ipairs, random, tremove, sgsub, sformat = ipairs, math.random, table.remove, string.gsub, string.format

local function test_main_quest()
    local check_result = true
    
    for quest_id,quest_v in pairs( QUESTS ) do
        
-----check targets  
      if not quest_v.Targets then
          check_result =false
          c_err("the target of main quest %d is nil",quest_v.ID)
      else
        for i,target_id in pairs(quest_v.Targets) do
            check_target =false
            for ii,target_v in pairs(TARGETS) do
                if target_id == target_v.ID then
                    check_target =true
                end
            end
            if not check_target then 
               check_result =false
               c_err("the target of main quest %d is not in the TARGETS",quest_v.ID)
            end
        end
      end

-----check PreQuest
      if quest_v.ID > 1 and not quest_v.PreQuests then
          check_result =false
          c_err("the PreQuests of the main quest %d is nil",quest_v.ID)
      elseif quest_v.ID > 1 and quest_v.PreQuests[1][1]+1  ~=quest_v.ID then
          check_result =false
          c_err("the line of main quest is interupted,check the PreQuests of Quest %d",quest_v.ID)
      end

-----check PrizeId and PrizeNum
      if quest_v.PrizeIDs then
         for i,prize_id in pairs (quest_v.PrizeIDs[1]) do
             check_prize =false
             for ii, items in pairs(ITEMS) do
                 if items.ID == prize_id then
                     check_prize =true
                 end
             end
             if not check_prize then 
             check_result =false
             c_err("the PrizeID of the main quest %d is not in the ITEMS",quest_v.ID)
             end
         end
      end

    end
    return check_result
end

 function test_target() 
    local check_result =true
    for target_i,target_v in pairs( TARGETS ) do

-----check the activity instance id while the TargetType is 1        
        if target_v.TargetType == 1 then
           check_activity = false
           for activity_id,activity_v in pairs (ACTIVITY_INSTANCE_CONFIG) do
               if target_v.Param[1][1] == activity_id then 
                  check_activity =true
               end 
           end
           if not check_activity then 
              check_result =false
              c_err("the Para of the target %d is not in the ACTIVITY_INSTANCE_CONFIG",target_v.ID)
           end
        end

    end
    return check_result
end
    
-------------------
local TEST_FUNC ={
      [1] = test_main_quest,
      [2] = test_target
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

