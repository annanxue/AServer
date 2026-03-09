
module( "test_player_t_rune", package.seeall )

local sformat = string.format
local PLAYER_RUNES = PLAYER_RUNES 
local RUNE_NUM = 540
local LEVEL_MIN,LEVEL_MAX = 15,90 
local ATTR_ID_MIN,ATTR_ID_MAX =150,330
local ATTR_VALUE_MIN,ATTR_VALUE_MAX = 3,400
local JOB_TYPES = 3
local MATERIAL_TYPES = 5
local MATERIAL_ID_MIN,MATERIAL_ID_MAX = 500,545
local ITEMS = ITEMS

local function test_PLAYER_RUNES()
    local check_result = true
         
     for i, v in pairs( PLAYER_RUNES ) do

        if  #(PLAYER_RUNES) ~= RUNE_NUM  then 
            c_err(sformat("attr_id: %d RuneID is unaceptable values in PLAYER_RUNES",i))
            check_result = false
        end
        if (v.PlayerLevel < LEVEL_MIN ) or (v.PlayerLevel > LEVEL_MAX) then
             c_err(sformat("attr_id: %d PlayerLevel is unaceptable values in PLAYER_RUNES",i))
            check_result = false
        end

        local JobAttrID = {v.Job1AttrID , v.Job2AttrID , v.Job3AttrID }
        local JobAttrValue= {v.Job1AttrValue , v.Job2AttrValue , v.Job3AttrValue }

        for i=1,JOB_TYPES do

            if  (JobAttrID[i] < ATTR_ID_MIN ) or (JobAttrID[i] > ATTR_ID_MAX) then
                c_err(sformat("attr_id: %d AttrID is unaceptable values in PLAYER_RUNES",i))
                check_result = false
            end
            if (JobAttrValue[1] < ATTR_VALUE_MIN) or (JobAttrValue[1] > ATTR_VALUE_MAX) then
                c_err(sformat("attr_id: %d AttrValue is unaceptable values in PLAYER_RUNES",i))
                check_result = false
            end
        end
    end
    --print ("[1]",check_result)
    return check_result
end


local function test_materiallist()
    local check_result = true

    for i,v in pairs ( PLAYER_RUNES ) do
        local  materiallist = v.MaterialList[1]

        if  #(materiallist) ~= MATERIAL_TYPES then
            c_err(sformat("attr_id: %d MaterialList is not correct numbers in PLAYER_RUNES", i))
            check_result = false
        end

        for ii,vv in pairs (materiallist)do
            local item_id = vv

            if ( vv < MATERIAL_ID_MIN )  or ( vv > MATERIAL_ID_MAX ) then
                c_err(sformat("attr_id: %d attr_id: %d MaterialList is unaceptable values in PLAYER_RUNES",i,ii))
                check_result = false
            end

            if  ITEMS[item_id]  then
            else
                c_err(sformat("attr_id: %d attr_id: %d MaterialList is unaceptable values in PLAYER_RUNES for ITEMS",i,ii))
                check_result = false
            end

        end

    end
    --print ("[2]",check_result)
    return check_result
end


-------------------
local TEST_FUNC ={
        [1] = test_PLAYER_RUNES,
        [2] = test_materiallist
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

