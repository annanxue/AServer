module( "test_drop", package.seeall )

local DROP_EQUIPS = DROP_EQUIPS 
local PLANE_DROP = PLANE_DROP
local ACTIVITY_INSTANCE_CONFIG = ACTIVITY_INSTANCE_CONFIG 
local ITEMS = ITEMS
local ipairs, random, tremove, sgsub, sformat, sfind = ipairs, math.random, table.remove, string.gsub, string.format ,string.find


local function test_drop()
    local check_result = true

    for inst_id, v in pairs( ACTIVITY_INSTANCE_CONFIG ) do
        local drop_cfg = v.Drop[1]

        for job_id = 1, 3 do
            local drop_id = drop_cfg[ job_id ] 

            for drop_id_detail , _ in pairs( PLANE_DROP ) do
                local drop_id_str = tostring( drop_id )
                local drop_id_detail_str = tostring( drop_id_detail )
                local filter_str = string.sub( drop_id_detail_str, 1, #drop_id_detail_str - 2) 

                if filter_str == drop_id_str and PLANE_DROP[ drop_id_detail ].DropEquip then
                    local drop_equip = PLANE_DROP[ drop_id_detail ].DropEquip[1]

                    for level_index = 1, 4 do
                        local drop_lib_id = drop_equip[ level_index ]
                        local drop_lib_tbl = DROP_EQUIPS[ drop_lib_id ]

                        for index, item_id_t in pairs( drop_lib_tbl ) do
                            if type( item_id_t ) == "table" then
                                for i = 2, #item_id_t[1], 2  do
                                    if item_id_t[1][ i-1 ] ~= 0 then
                                        for _, item_id in ipairs( item_id_t[1][i]) do
                                            if ITEMS[ item_id ].JobLimit > 0 and ITEMS[ item_id ].JobLimit ~= job_id then
                                                c_err(sformat("inst_id:%d, job_id:%d, item_id:%d, item's job_id:%d", inst_id, job_id, item_id, ITEMS[ item_id ].JobLimit))
                                                check_result = false
                                            end
                                        end
                                    end
                                end
                            end
                        end
                    end

                end
            end

        end
    end

    return check_result
end


-------------------

local TEST_FUNC ={
        [1] = test_drop,
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

