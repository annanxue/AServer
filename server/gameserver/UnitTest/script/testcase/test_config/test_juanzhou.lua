module( "test_juanzhou", package.seeall )

local EQUIPS =EQUIPS 
local ipairs, random, tremove, sgsub, sformat = ipairs, math.random, table.remove, string.gsub, string.format

local function test_open()
    local check_result = true
    local sceneid_instid = {}

    for inst_id,v in pairs( INSTANCE_CFG ) do
        local scene_id = string.sub(tostring(inst_id),0,4)
        scene_id = tonumber(scene_id)

        if not sceneid_instid[ scene_id ] then
            sceneid_instid[ scene_id ] = {}
            table.insert(sceneid_instid[ scene_id ], inst_id )
        else
            table.insert(sceneid_instid[ scene_id ], inst_id )
        end

    end

    for scene_id, inst_id_tbl in pairs( sceneid_instid ) do
        local first_inst_id = inst_id_tbl[1]

        for _, inst_id in ipairs( inst_id_tbl ) do
            if inst_id == 210301 or inst_id == 210302 then 
                if INSTANCE_CFG[ inst_id ].IsOpen ~= INSTANCE_CFG[ first_inst_id ].IsOpen then
                    c_err(sformat("scene_id:%d different instance is_open not then same",scene_id ))
                    check_result = false 
                end
            end
        end
    end

    return check_result
end


-------------------
local TEST_FUNC ={
        test_open,
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

