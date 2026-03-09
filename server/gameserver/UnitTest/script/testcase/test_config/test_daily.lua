module("test_daily",package.seeall)

local sformat = string.format 
local DAILY_REWARD_EVENTS = DAILY_REWARD_EVENTS
local TIME_CTRL = TIME_CTRL
local TIME_EVENT = TIME_EVENT 
local ITEMS =ITEMS
local EQUIPS = EQUIPS

local function test_time_ctrl( )
	local check_result =true
	for i,v in pairs(DAILY_REWARD_EVENTS) do
	    local time_ctrl_id =v.TimeCtrlId 
	    if time_ctrl_id and not TIME_CTRL[time_ctrl_id] then
	    	check_result =false 
	    	c_err(sformat("DAILY_REWARD_EVENTS id :%d ,time_ctrl_id :%d err!",i,time_ctrl_id))
        end
	end
  return check_result
end

local function test_time_event( )
	local check_result =true
	for i,v in pairs(DAILY_REWARD_EVENTS) do
	    local time_event_id =v.TimeEventId
	    if time_event_id and not TIME_EVENT[time_event_id] then
	    	check_result =false 
	    	c_err(sformat("DAILY_REWARD_EVENTS id :%d ,time_event_id :%d err!",i,time_event_id))
        end
	end
  return check_result
end


local function  test_reward( )
    local check_result =true
    for i, v in pairs (DAILY_REWARD_EVENTS) do  
        local item_list = v.ItemIdList[1]
        local item_count =v.ItemCountList[1] 
  
      	if (item_list) and type(item_list) ~= "table" then
    		    check_result =false
    	      c_err(sformat("DAILY_REWARD_EVENTS id :%d item_list type is not table",i))
        end 
        if (item_count) and type(item_count) ~= "table" then
    		   check_result =false
    	     c_err(sformat("DAILY_REWARD_EVENTS id :%d item_count type is not table",i))
        end

        if item_list and item_count  and type(item_list) == "table" and type(item_count) =="table" then
           if  #item_count ~= #item_list  then
           	  check_result =false
              c_err(sformat("DAILY_REWARD_EVENTS id :%d ItemCountList length is not match ItemIdList length",i))
           end
           if item_list and item_count  then
               for j, k in ipairs(item_list) do 
                   if not ITEMS[k] then
                      check_result =false
                      c_err(sformat("DAILY_REWARD_EVENTS id :%d ItemIdList item_id : %d is nil",i,k))
                   end
                end
           end
         end
    end	
  return check_result
end

local TEST_FUNC ={
        [1] =test_time_ctrl,
        [2] =test_time_event,
        [3] =test_reward
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
