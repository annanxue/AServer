module( "testcase", package.seeall )

require_ex("luacommon/testcase/test_common/test_common")
require_ex("luacommon/testcase/test_common/test_property")
require_ex("testcase/test_config/test_config_in_file")
require_ex("testcase/test_hkc")
require_ex("testcase/code_generator")
CINEMA_MONSTER_LIST = require_ex( "testcase/CINEMA_MONSTER_LIST" )

local sformat = string.format
local PATH = "testcase/test_config/"
g_testcase_test = g_testcase_test or false

local MODULE_NAME = {
    "test_skill",
    "test_monster",
    "test_buff",
    "test_model",
    "test_item",
    "test_equip",
    -- "test_gs",
    "test_plane",
    --"test_plane_drop",
    -- "test_player_t_rune",
    --"test_player_t_pray",
    --"test_RANDOM_ITEM_LIBS",
    --"test_trans",
    --"test_player_t_goblin_market",
    "test_time_event",
    "test_juanzhou",
    "test_main_quest",
    --"test_daily",
    --"test_player_t_attend",
    --"test_drop",
    --"test_system_unlock",
	--"test_daily_goals",
    --"test_mystic",
    --"test_mammon",
    --"test_player_t_holiday",
	"test_stage_star",
	"test_wing_system",
	"test_energy",
    "test_soulcards",
}

for _, module_name in ipairs( MODULE_NAME ) do
    local module =  sformat("%s%s", PATH, module_name)
    require_ex( module )
end

---------------
--external func
---------------

function test()
    if not g_debug then return end 
    if not g_testcase_test then
        c_log("\nNow test start..")

        for _, module_name  in ipairs( MODULE_NAME ) do
            if  _G[ module_name ] then
                local test_func = _G[ module_name ].test
                local result = test_func()

                c_log("testing %s",module_name )
--                assert(result, "TestCase execute error!")
            end
        end

        g_testcase_test = true
    end
end


