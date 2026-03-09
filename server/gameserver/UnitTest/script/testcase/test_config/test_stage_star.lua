--
--  测试关卡星级相关内容
--

module("test_stage_star", package.seeall)


local sformat = string.format


local function test_STAGE_GET_STAR_CONFIG()
	local star_cfg = STAGE_GET_STAR_CONFIG
	local active_inst_cfg = ACTIVITY_INSTANCE_CONFIG
	local condition_type_cfg = player_t.CONDITION_CFG
	for inst_id, cfg in pairs( star_cfg ) do
		if not active_inst_cfg[inst_id] or not active_inst_cfg[inst_id].WorldMapPositionIndex then
			c_err( "CONFIG ERROR: instance id(%d) in STAGE_GET_STAR_CONFIG not in ACTIVITY_INSTANCE_CONFIG or instance not in wolrdmap", inst_id )
			return false
		end
		for _, v in pairs( cfg ) do
			if not v[1] or not condition_type_cfg[v[1]] then
				c_err( "CONFIG ERROR: condition_type(%s) not in player_t.CONDITION_CFG", v[1] )
				return false
			end
		end
	end 
	return true
end

local function test_STAGE_STAR_REWARD_CONFIG()
	local job_key = ""
	local item_list = {}
	local item_config = ITEMS
	local reward_cfg = STAGE_STAR_REWARD_CONFIG
	for key, cfg_job in pairs( reward_cfg ) do
		for i=1, 4 do
			job_key = sformat( "Job%dReward", i )
			item_list = cfg_job[job_key]
			if not item_list then
				c_err( "CONFIG ERROR: job reward(%s) not exist. id=%d", job_key, key )
				return false
			end
			for _, item in ipairs( item_list ) do
				if not item or not item[1] or not item_config[item[1]] or not item[2] or ( item[2] <= 0 ) then
					c_err( "CONFIG ERROR: job reward(%s) item not exist or count invalid. id=%d", job_key, key )
					return false
				end
			end
		end
	end
	return true
end

local TEST_FUNC = {
	test_STAGE_GET_STAR_CONFIG,
	test_STAGE_STAR_REWARD_CONFIG,
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
		c_err( "stage_star Check not pass!!" )
    end
    return check_result 
end