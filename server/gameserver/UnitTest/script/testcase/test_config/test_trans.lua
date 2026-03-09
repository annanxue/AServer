module( "test_trans", package.seeall )

local PLAYER_TRANSFORM_ENHANCE = PLAYER_TRANSFORM_ENHANCE 
local PLAYER_TRANSFORMS = PLAYER_TRANSFORMS
local PLAYER_TRANSFORM_SKILL_BUFF = PLAYER_TRANSFORM_SKILL_BUFF

local ipairs, random, tremove, sgsub, sformat = ipairs, math.random, table.remove, string.gsub, string.format

local function test_PlayerLevel()
    local check_result = true

    for i,v in pairs( PLAYER_TRANSFORM_ENHANCE ) do
        if ( not v.PlayerLevel ) or type(v.PlayerLevel) ~= "number" then
            check_result = false
            c_err(string.format("trans key:%s PlayerLevel not exist or not number!", i ))
        elseif v.PlayerLevel < 1  or v.PlayerLevel > 100 then
            check_result = false
            c_err(string.format("trans key:%s PlayerLevel not between 1 ~ 100!", i ))
        end
    end

    return check_result
end

local function test_MaterialCountList()
    local check_result = true

    for i,v in pairs( PLAYER_TRANSFORM_ENHANCE ) do
        if i ~= "1,0" then
            if ( not v.MaterialCountList ) or type(v.MaterialCountList) ~= "table" then
                check_result = false
                c_err(string.format("trans key:%s MaterialCountList not exist or not number!", i ))
            elseif #v.MaterialCountList[1] ~= 5 then
                check_result = false
                c_err(string.format("trans key:%s MaterialCountList count is not 5!", i ))
            end 

            for _, k in ipairs(v.MaterialCountList[1]) do
                if type(k) ~= "number" or k <= 0 then
                    check_result = false
                    c_err(string.format("trans key:%s some elem of MaterialCountList type not number or < 0", i ))
                end
            end
        end
    end

    return check_result
end


local function test_MaterialList()
    local check_result = true

    for i,v in pairs( PLAYER_TRANSFORM_ENHANCE ) do
        if i ~= "1,0" then
            if ( not v.MaterialList ) or type(v.MaterialList) ~= "table" then
                check_result = false
                c_err(string.format("trans key:%s MaterialList not exist or not number!", i ))
            elseif #v.MaterialList[1] ~= 5 then
                check_result = false
                c_err(string.format("trans key:%s MaterialList count is not 5!", i ))
            end 

            for _, k in ipairs(v.MaterialList[1]) do
                if type(k) ~= "number" or k <= 0 then
                    check_result = false
                    c_err(string.format("trans key:%s some elem of MaterialList type not number or < 0", i ))
                elseif not ITEMS[k] then 
                    check_result = false
                    c_err(string.format("trans key:%s some elem of MaterialList not exist in ITEMS, item_id:%d", i,k ))

                end
            end


        end
    end

    return check_result
end

--- PLAYER_TRANSFORMS

local function test_AttrList()
    local check_result = true

    for i,v in pairs( PLAYER_TRANSFORMS ) do
            if ( not v.AttrList ) or type(v.AttrList) ~= "table" then
                check_result = false
                c_err(string.format("trans key:%d AttrList not exist or not table!", i ))
            elseif #v.AttrList[1] ~= 6 then
                check_result = false
                c_err(string.format("trans key:%d AttrList count is not 6!", i ))
            end 

            for _, k in ipairs(v.AttrList[1]) do
                if type(k) ~= "number" or k <= 0 then
                    check_result = false
                    c_err(string.format("trans key:%d some elem of AttrList type not number or < 0", i ))
                elseif not EQUIP_ATTRS[k] then 
                    check_result = false
                    c_err(string.format("trans key:%d some elem of AttrList not exist in EQUIP_ATTR, attr_id:%d", i,k ))
                elseif EQUIP_ATTRS[k].ValueMax ~=  EQUIP_ATTRS[k].ValueMin then
                    check_result = false
                    c_err(string.format("trans key:%d some elem of AttrList min ~= max in EQUIP_ATTR, attr_id:%d", i,k ))
                end
            end
        end
    return check_result
end

local function test_SculptID()
    local check_result = true

    for i,v in pairs( PLAYER_TRANSFORMS ) do
            if ( not v.SculptID ) or type( v.SculptID ) ~= "number" or ( not SCULPTS[v.SculptID] ) then
                check_result = false
                c_err(string.format("trans key:%d SculptID not exist or not table or not exist in SCULPTS! sculpt_id: %d", i,v.SculptID ))
            end 
       end
    return check_result
end


--PLAYER_TRANSFORM_SKILL_BUFF

local function test_BuffID()
    local check_result = true

    for i,v in pairs( PLAYER_TRANSFORM_SKILL_BUFF ) do
        if ( not v.BuffID ) or type( v.BuffID ) ~= "number" or ( not BUFFS[v.BuffID] ) then
            check_result = false
            c_err(string.format("skill_id :%d element BuffID not exist or not table or not exist in BUFFS! buff_id: %d", i, v.BuffID ))
        elseif ( BUFFS[v.BuffID].Group ~= 999 ) then
            check_result = false
            c_err(string.format("skill_id :%d element group_id ~= 999! buff_id: %d", i, v.BuffID ))
        end
    end
    return check_result
end


local function test_others()
    local check_result = true

    for i,v in pairs( PLAYER_TRANSFORM_SKILL_BUFF ) do
        if ( not v.IsExclusive ) or (not v.SkillDesc) or ( not v.SkillIcon ) or ( not v.SkillName) then
            check_result = false
            c_err(string.format("skill_id :%d element IsExclusive or SkillDesc or SkillIcon  or SkillName not exist", i))
        end
    end
    return check_result
end



-------------------

local TEST_FUNC ={
        [1] = test_PlayerLevel,
        [2] = test_MaterialCountList,
        [3] = test_MaterialList,
        -- [4] = test_AttrList,
        [5] = test_SculptID,
        [6] = test_BuffID,
        [7] = test_others
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

