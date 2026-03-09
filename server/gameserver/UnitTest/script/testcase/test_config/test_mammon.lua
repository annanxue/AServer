module( "test_mammon", package.seeall )
local sformat = string.format
local QUEST_2_MAMMONS_TREASURE = QUEST_2_MAMMONS_TREASURE
local ACTIVITY_INSTANCE_CONFIG = ACTIVITY_INSTANCE_CONFIG

local function test_mammon()
    local check_result = true
    for green_id, green_value in pairs( QUEST_2_MAMMONS_TREASURE.GreenIDSet ) do
        if ( ACTIVITY_INSTANCE_CONFIG[green_value].IsOpen == 0 ) then
            check_result = false
            c_err("The activity instance id %d in GreenQuality is not open", green_value )
        end
    end
    for blue_id, blue_value in pairs( QUEST_2_MAMMONS_TREASURE.BlueIDSet ) do
        if ( ACTIVITY_INSTANCE_CONFIG[blue_value].IsOpen == 0 ) then
            check_result = false
            c_err("The activity instance id %d in BlueQuality is not open", blue_value )
        end
    end
    for purple_id, purple_value in pairs( QUEST_2_MAMMONS_TREASURE.PurpleIDSet ) do
        if ( ACTIVITY_INSTANCE_CONFIG[purple_value].IsOpen == 0 ) then
            check_result = false
            c_err("The activity instance id %d in PurpleQuality is not open", purple_value )
        end
    end
    for orange_id, orange_value in pairs( QUEST_2_MAMMONS_TREASURE.OrangeIDSet ) do
        if ( ACTIVITY_INSTANCE_CONFIG[orange_value].IsOpen == 0 ) then
            check_result = false
            c_err("The activity instance id %d in OrangeQuality is not open", orange_value )
        end
    end
end

local TEST_FUNC ={
    [1] = test_mammon
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
