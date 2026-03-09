module( "test_gs", package.seeall )

local EQUIP_GS = EQUIP_GS 
local EQUIP_ATTRS = EQUIP_ATTRS
local SKILL_LEARN = SKILL_LEARN
local RUNES = RUNES
local PLAYER_TRANSFORM_SKILL_BUFF = PLAYER_TRANSFORM_SKILL_BUFF

local ipairs, random, tremove, sgsub, sformat = ipairs, math.random, table.remove, string.gsub, string.format

local function test_attr_exist()
    local check_result = true

    for _, v in pairs(EQUIP_ATTRS) do
        local attr_name =  v.AttrName

        if not EQUIP_GS[attr_name] then
            c_err(sformat("attr_id: %d attr_name: %s not exist in EQUIP_GS",v.ID,v.AttrName))
            check_result = false
        end
    end
    return check_result
end

local function test_EQUIP_GsParam()
  local check_result = true
  for attr_name, v in pairs(EQUIP_GS) do
    --  if not v.GsParam or v.GsParam == 0 then        // temp allow 0
        if not v.GsParam then
            c_err(sformat("attr_name: %s gs_param not exist or 0  in EQUIP_GS",attr_name))
            check_result = false
      end
  end
  return check_result
end


local function test_SKILL_LEARN_GsParam()
  local check_result = true
  for skill_id, v in pairs( SKILL_LEARN ) do
      if not v.GsParam or v.GsParam == 0 then
            c_err(sformat("skill_id: %d gs_param not exist or 0 in SKILL_LEARN", skill_id))
            check_result = false
      end
  end
  return check_result
end



local function test_RUNES_GsParam()
  local check_result = true
  for rune_id, v in pairs( RUNES ) do
      if not v.GsParam or v.GsParam == 0 then
            c_err(sformat("rune_id: %d gs_param not exist or 0 in RUNES", rune_id))
            check_result = false
      end
  end
  return check_result
end



local function test_RUNES_GsParam()
  local check_result = true
  for rune_id, v in pairs( RUNES ) do
      if not v.GsParam or v.GsParam == 0 then
            c_err(sformat("rune_id: %d gs_param not exist or 0 in RUNES", rune_id))
            check_result = false
      end
  end
  return check_result
end



local function test_RUNES_GsParam()
    local check_result = true

    for rune_id, v in pairs( RUNES ) do
        if not v.GsParam or v.GsParam == 0 then
            c_err(sformat("rune_id: %d gs_param not exist or 0 in RUNES", rune_id))
            check_result = false
        end
    end

    return check_result
end



local function test_PLAYER_TRANSFORM_SKILL_BUFF_GsParam()
    local check_result = true

    for skill_id, v in pairs( PLAYER_TRANSFORM_SKILL_BUFF) do
        if not v.SkillGS or v.SkillGS == 0 then
            c_err(sformat("skill_id: %d gs_param not exist or 0 in PLAYER_TRANSFORM_SKILL_BUFF", skill_id))
            check_result = false
        end
    end

    return check_result
end


-------------------
local TEST_FUNC ={
        [1] = test_attr_exist,
        [2] = test_EQUIP_GsParam,
        [3] = test_SKILL_LEARN_GsParam,
        [4] = test_RUNES_GsParam,
        [5] = test_PLAYER_TRANSFORM_SKILL_BUFF_GsParam
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

