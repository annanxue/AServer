--
--  测试翅膀配置相关内容
--

module("test_wing_system", package.seeall)


local sformat = string.format


local function test_wing_skill_config()
	local wing_skills = WING_SKILLS
	local wing_skill_ranks = WING_SKILL_RANKS
	local buffs = BUFFS
	for _, v in ipairs(wing_skill_ranks) do
		local skill_id = math.floor(v.Id / 100)
		if not wing_skills[skill_id] then
			c_err("test_wing_skill_config error: skill id not found %d", v.Id)
			return false
		end
		if not buffs[v.BuffId] then
			c_err("test_wing_skill_config error: BuffId not found %d", v.BuffId)
			return false
		end
		if (not v.UpgradeCondition) or (#v.UpgradeCondition < 1) then
			c_err("test_wing_skill_config error: UpgradeCondition is nil")
			return false
		end
		local condition_proc = rawget(player_t, v.UpgradeCondition[1])
		if not condition_proc then
			c_err("test_wing_skill_config error: UpgradeCondition name error, %s", v.UpgradeCondition[1])
			return false
		end
	end
	return true
end

local function test_wing_core_config()
	local wing_core_upgrade_star = WING_CORE_UPGRADE_STAR
	local wing_core_attrs = WING_CORE_ATTRS
	local wing_cores = WING_CORES	
	local items = ITEMS
	for _, v in ipairs( wing_core_upgrade_star ) do
		
		local core_id = math.floor((v.Id % 10000) / 100)
		if not wing_cores[core_id] then
			c_err("test_wing_core_config: wing_core_upgrade_star.coreid error, id = %d", core_id);
			return false
		end
				
		if (#v.AttrId ~= #AttrValue) or (#MaterialList ~= #MaterialNum) then
			c_err("test_wing_core_config: not same length, %d", v.Id);
			return false
		end
		
		for _, id in ipairs(v.AttrId) do
			if not wing_core_attrs[id] then
				c_err("test_wing_core_config: attr id not exists, %d, %d", v.Id, id);
				return false
			end
		end
		
		for _, id in ipairs(v.MaterialList) do
			if not items[id] then
				c_err("test_wing_core_config: attr id not exists, %d, %d", v.Id, id);
				return false
			end
		end
	end
	return true
end

local TEST_FUNC = {
	test_wing_skill_config,
	test_wing_core_config,
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
    if not check_result then
		c_err( "wing_system Check not pass!!" )
    end
    return check_result 
end