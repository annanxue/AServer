module( "test_soulcards", package.seeall )

local sformat = string.format

function test_soulcard_config()
    local check_result = true

    for card_id, card_info in pairs( SOULCARDS ) do
        local item_cfg = ITEMS[card_id]
        if item_cfg then
            if item_cfg.ItemType ~= 7 or item_cfg.UseType ~= 54 then
                check_result = false
                c_err(sformat("Card_id:%d ItemType is:%d UseType is:%d!", card_id, item_cfg.ItemType, item_cfg.UseType))
            end
        else
        	check_result = false
            c_err(sformat("Card_id:%d not in ITEMS!",card_id))
        end

        if not card_info.BaseAttrLib then
        	check_result = false
            c_err(sformat("Card_id:%d not have BaseAttrLib!",card_id))
        end

        for _, lib_id in ipairs(card_info.BaseAttrLib[1] or {}) do
        	if not SOULCARD_ATTRS[lib_id] then
	        	check_result = false
	            c_err(sformat("card_id:%d, BaseAttrLib attr_lib_id:%d not in SOULCARD_ATTRS!", card_id, lib_id))
        	end
        end

        for _, lib_id in ipairs(card_info.StarAttrLib and card_info.StarAttrLib[1] or {}) do
        	if not SOULCARD_ATTRS[lib_id] then
	        	check_result = false
	            c_err(sformat("card_id:%d, StarAttrLib attr_lib_id:%d not in SOULCARD_ATTRS!", card_id, lib_id))
        	end
        end

        for _, lib_id in ipairs(card_info.RefineAttr1 and card_info.RefineAttr1[1] or {}) do
        	if not SOULCARD_ATTRS[lib_id] then
	        	check_result = false
	            c_err(sformat("card_id:%d, RefineAttr1 attr_lib_id:%d not in SOULCARD_ATTRS!", card_id, lib_id))
        	end
        end

        for _, lib_id in ipairs(card_info.RefineAttr2 and card_info.RefineAttr2[1] or {}) do
        	if not SOULCARD_ATTRS[lib_id] then
	        	check_result = false
	            c_err(sformat("card_id:%d, RefineAttr2 attr_lib_id:%d not in SOULCARD_ATTRS!", card_id, lib_id))
        	end
        end

        for _, lib_id in ipairs(card_info.RefineAttr3 and card_info.RefineAttr3[1] or {}) do
        	if not SOULCARD_ATTRS[lib_id] then
	        	check_result = false
	            c_err(sformat("card_id:%d, RefineAttr3 attr_lib_id:%d not in SOULCARD_ATTRS!", card_id, lib_id))
        	end
        end

        for _, lib_id in ipairs(card_info.RefineAttr4 and card_info.RefineAttr4[1] or {}) do
        	if not SOULCARD_ATTRS[lib_id] then
	        	check_result = false
	            c_err(sformat("card_id:%d, RefineAttr4 attr_lib_id:%d not in SOULCARD_ATTRS!", card_id, lib_id))
        	end
        end

        for _, lib_id in ipairs(card_info.RefineAttr5 and card_info.RefineAttr5[1] or {}) do
        	if not SOULCARD_ATTRS[lib_id] then
	        	check_result = false
	            c_err(sformat("card_id:%d, RefineAttr5 attr_lib_id:%d not in SOULCARD_ATTRS!", card_id, lib_id))
        	end
        end

        for _, lib_id in ipairs(card_info.RefineAttr6 and card_info.RefineAttr6[1] or {}) do
        	if not SOULCARD_ATTRS[lib_id] then
	        	check_result = false
	            c_err(sformat("card_id:%d, RefineAttr6 attr_lib_id:%d not in SOULCARD_ATTRS!", card_id, lib_id))
        	end
        end

        for _, lib_id in ipairs(card_info.RefineAttr7 and card_info.RefineAttr7[1] or {}) do
        	if not SOULCARD_ATTRS[lib_id] then
	        	check_result = false
	            c_err(sformat("card_id:%d, RefineAttr7 attr_lib_id:%d not in SOULCARD_ATTRS!", card_id, lib_id))
        	end
        end

        for _, lib_id in ipairs(card_info.RefineAttr8 and card_info.RefineAttr8[1] or {}) do
        	if not SOULCARD_ATTRS[lib_id] then
	        	check_result = false
	            c_err(sformat("card_id:%d, RefineAttr8 attr_lib_id:%d not in SOULCARD_ATTRS!", card_id, lib_id))
        	end
        end
    end
    return check_result
end

function test_soulcard_refine_config()
	local check_result = true

    for i, v in pairs( SOULCARD_REFINE_UNLOCK ) do
    	if v.AttrCountMax ~= #v.UnlockRefineLevel[1] then
    		c_err(sformat("soulcard refine attr max count:%d not match unlock table count:%d!",v.AttrCountMax, #v.UnlockRefineLevel[1]))
    		check_result = false
    	end
    end
    return check_result
end


function test_soulcard_refine_attr()
	local check_result = true

     for card_id, card_info in pairs( SOULCARDS ) do
     	local attr_name_list = {}
     	local quality = 0
     	local max_refine_num = 0

     	local item_cfg = ITEMS[card_id]
        if item_cfg then
        	quality = item_cfg.Quality
        else
        	check_result = false
            c_err(sformat("Card_id:%d not in ITEMS!",card_id))
        end

        local unlock_cfg = SOULCARD_REFINE_UNLOCK[quality]
        if unlock_cfg then
        	max_refine_num = unlock_cfg.RefineMax
        else
        	check_result = false
            c_err(sformat("quality:%d not in SOULCARD_REFINE_UNLOCK!", quality))
        end

     	for i, lib_id in ipairs(card_info.BaseAttrLib[1] or {}) do
     		local attr_cfg = SOULCARD_ATTRS[lib_id]
        	if attr_cfg then
	        	attr_name_list[i] = attr_cfg.AttrName
	        else
	        	check_result = false
	            c_err(sformat("BaseAttrLib attr_lib_id:%d, card_id:%d not in SOULCARD_ATTRS!",lib_id, card_id))
        	end
        end

        for i = 1, max_refine_num do
        	local refine_attr_id = card_id * 100 + i
        	local refine_attr_cfg = SOULCARD_REFINE[refine_attr_id]
        	if refine_attr_cfg then
        		local add_attrs = refine_attr_cfg.AddAttr
        		if add_attrs then
        			for j, lib_id in ipairs(add_attrs[1]) do
			     		local attr_cfg = SOULCARD_ATTRS[lib_id]
			        	if attr_cfg then
				        	local refine_name = attr_cfg.AttrName
				        	if attr_name_list[j] ~= refine_name then
					        	check_result = false
					            c_err(sformat("attr_name check,base attr_name not equal refine attr_name!, card_id:%d, refine_level:%d number:%d", card_id, i, j))
				        	end
				        else
				        	check_result = false
				            c_err(sformat("SOULCARD_REFINE AddAttr attr_lib_id:%d not in SOULCARD_ATTRS!",lib_id))
			        	end
        			end
        		else
		        	check_result = false
		            c_err(sformat("refine_attr_id:%d not have AddAttr in SOULCARD_REFINE!",refine_attr_id))
        		end
	        else
	        	check_result = false
	            c_err(sformat("refine_attr_id:%d not in SOULCARD_REFINE, card_id:%d, refine_level:%d!", refine_attr_id, card_id, i))
        	end
        end
    end

    return check_result
end

function test_soulcard_lib_attr()
	local check_result = true

     for idx, info in pairs( SOULCARD_ATTRS ) do
     	attr_type = info.AddType

     	if not attr_type then
        	check_result = false
            c_err(sformat("attr_idx:%d not have AddType in SOULCARD_ATTRS!", idx))
     	end

     	if attr_type < 1 or attr_type > 4 then
        	check_result = false
            c_err(sformat("attr_idx:%d AddType not in 1 ~ 4, SOULCARD_ATTRS!", idx))
     	end

     	if attr_type == 1 or attr_type == 4 then
     		if not info.DataZone or #info.DataZone < 1 then
	        	check_result = false
	            c_err(sformat("attr_idx:%d DataZone is error, SOULCARD_ATTRS!", idx))
     		end
     	end

     	if attr_type == 2 or attr_type == 3 then
     		if not info.BaseNumber or not info.ExtraScore then
	        	check_result = false
	            c_err(sformat("attr_idx:%d BaseNumber or ExtraScore is error, SOULCARD_ATTRS!", idx))
     		end
     	end
    end

    return check_result
end

-------------------
local TEST_FUNC ={
        [1] = test_soulcard_config,
        [2] = test_soulcard_refine_config,
        [3] = test_soulcard_refine_attr,
        [4] = test_soulcard_lib_attr,
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
