module( "test_time_event", package.seeall )

local TIME_EVENT = TIME_EVENT 

local ipairs, random, tremove, sgsub, sformat = ipairs, math.random, table.remove, string.gsub, string.format

 function test_FormatDesc()
    local check_result = true

    for _, v in pairs( TIME_EVENT ) do 
        if v.RepeatType == "weekly"  then
            if not v.FormatDesc then
                c_err(sformat("TIME_EVENT formatdesc not exist for type weekly,event_id:%d",v.ID))
                check_result = false
            else
                if #v.FormatDesc[1] ~= 7 then
                    c_err(sformat("TIME_EVENT formatdesc lenth ~= 7 for type weekly,event_id:%d",v.ID ))
                    check_result = false
                end

                for _, switch in ipairs( v.FormatDesc[1] ) do
                    if switch ~= 1 and switch ~= 0 then
                        c_err(sformat("TIME_EVENT formatdesc elem ~= 0/1  for type weekly,event_id:%d",v.ID ))
                        check_result = false
                    end
                end
            end
        elseif v.RepeatType == "monthly" then
             if not v.FormatDesc then
                c_err(sformat("TIME_EVENT formatdesc not exist for type monthly,event_id:%d",v.ID))
                check_result = false
             else
                if #v.FormatDesc[1] < 1 then
                    c_err(sformat("TIME_EVENT formatdesc lenth < 1 for type monthly,event_id:%d",v.ID ))
                    check_result = false
                end

                for _, open_day in ipairs( v.FormatDesc[1] ) do
                    if open_day < 1 or open_day > 31 then
                        c_err(sformat("TIME_EVENT formatdesc elem not 1 ~ 31 for type monthly,event_id:%d",v.ID ))
                        check_result = false
                    end
                end
            end
        elseif v.RepeatType == "daily" then
             daily_time = 0
             check_daily = true
             local Desc = v.FormatDesc and v.FormatDesc[1]
             for _, v1 in ipairs (Desc or {}) do 
                 if daily_time <= v1 then                           --重复时间应该按照从小到大顺序排列
                     daily_time = v1
                 else
                     check_daily = false
                 end
             end
             if  check_daily == false then
                 c_err(sformat("TIME_EVENT formatdesc is not in sequence,event_id:%d",v.ID))
                 check_result = false
             end
        end
    end
    return check_result

 end

-- 资源大亨[201-250都是资源大亨id]
function test_ResourceMagnate()

    local check_result = true

    for _, config in pairs( TIME_EVENT ) do

        if time_event.EVENT_RESOURCE_MAGNATE_BEGIN <= config.ID and config.ID <= time_event.EVENT_RESOURCE_MAGNATE_END then
            if not string.find(config.EventName, 'EVENT_RESOURCE_MAGNATE%d*') and not string.find(config.EventName, 'EVENT_RESOURCE_MAGNATE_RANK_TODAY%d*') and
               not string.find(config.EventName, 'EVENT_RESOURCE_MAGNATE_RANK_TOTAL%d*') and not string.find(config.EventName, 'EVENT_RESOURCE_MAGNATE_REWARD_TODAY%d*')
            then
                check_result = false
                c_err("test_time_event.test_ResourceMagnate, id:%d error, %d to %d can only be used with resource magnate",
                    config.ID, time_event.EVENT_RESOURCE_MAGNATE_BEGIN, time_event.EVENT_RESOURCE_MAGNATE_END)
                break
            end
        end

    end

    return check_result

end

--团购狂欢[251-300都是团购id]
function test_GroupBuy()

    local check_result = true

    for _, config in pairs( TIME_EVENT ) do

        if time_event.EVENT_GROUP_BUY_BEGIN <= config.ID and config.ID <= time_event.EVENT_GROUP_BUY_END then
            if not string.find(config.EventName, 'EVENT_GROUP_BUY%d*') then
                check_result = false
                c_err("test_time_event.test_GroupBuy, id:%d error, %d to %d can only be used with group buy",
                    config.ID, time_event.EVENT_GROUP_BUY_BEGIN, time_event.EVENT_GROUP_BUY_END)
                break
            end
        end

    end

    return check_result

end

-------------------
local TEST_FUNC ={
        [1] = test_FormatDesc,
        [2] = test_ResourceMagnate,
        [3] = test_GroupBuy,
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

