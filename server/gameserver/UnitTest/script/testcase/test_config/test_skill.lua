module( "test_skill", package.seeall )

local SKILLS = SKILLS 
local sformat, type = string.format, type

function check_search_func( _index_chain, _table, _index )
    local index_chain = sformat( "%s-%s", _index_chain, _index )
    local search_def  = _table[_index]
    if type( search_def ) ~= "table" then
        c_err( "[test_skill](check_search_func) not a table, index_chain: [%s]", index_chain )
        return
    end
    local search_func_name = search_def[1]
    local search_func = skill.g_search_func_map[search_func_name]
    if not search_func then
        c_err( "[test_skill](check_search_func) search function not found, func_name: %s, index_chain: [%s]", search_func_name, index_chain )
        return
    end
end

function check_sort_func( _index_chain, _table, _index )
    local index_chain = sformat( "%s-%s", _index_chain, _index )
    local sort_def  = _table[_index]
    if type( sort_def ) ~= "table" then
        c_err( "[test_skill](check_sort_func) not a table, index_chain: [%s]", index_chain )
        return
    end
    local sort_func_name = sort_def[1]
    local sort_func = skill.g_sort_func_map[sort_func_name]
    if not sort_func then
        c_err( "[test_skill](check_sort_func) sort function not found, func_name: %s, index_chain: [%s]", sort_func_name, index_chain )
        return
    end
end

function check_factor_func( _index_chain, _table, _index )
    local index_chain = sformat( "%s-%s", _index_chain, _index )
    local factor_def  = _table[_index]
    if type( factor_def ) ~= "table" then
        c_err( "[test_skill](check_factor_func) not a table, index_chain: [%s]", index_chain )
        return
    end
    local factor_func_name = factor_def[1]
    local factor_func = skill.g_factor_func_map[factor_func_name]
    if not factor_func then
        c_err( "[test_skill](check_factor_func) factor function not found, func_name: %s, index_chain: [%s]", factor_func_name, index_chain )
        return
    end
    if factor_func_name == "SkillFactor" then
        check_number( index_chain, factor_def, 2, 1, 5 )
        if factor_def[3] then
            check_formula( index_chain, factor_def, 3 )
        end
        if factor_def[4] then
            check_formula( index_chain, factor_def, 4 )
        end
    elseif factor_func_name == "SkillParam" or factor_func_name == "ElementSkillParam" then
        check_formula( index_chain, factor_def, 2 )
        check_formula( index_chain, factor_def, 3 )
    else
        check_formula( index_chain, factor_def, 2 )
    end
end

function check_attack_func( _index_chain, _table, _index )
    local index_chain = sformat( "%s-%s", _index_chain, _index )
    local attack_def  = _table[_index]
    if type( attack_def ) ~= "table" then
        c_err( "[test_skill](check_attack_func) not a table, index_chain: [%s]", index_chain )
        return
    end
    local attack_func_name = attack_def[1]
    local attack_func = skill.g_attack_func_map[attack_func_name]
    if not attack_func then
        c_err( "[test_skill](check_attack_func) attack function not found, func_name: %s, index_chain: [%s]", attack_func_name, index_chain )
        return
    end
end

function check_formula( _index_chain, _table, _index )
    local index_chain = sformat( "%s-%s", _index_chain, _index )
    local formula = _table[_index]
    local ttype = type( formula )
    if ttype == "nil" then
        c_err( "[test_skill](check_formula) formula is nil, index_chain: %s", index_chain )
        return
    end
    if ttype ~= "function" then
        local succ, func = skill.str2func( formula )
        if not succ then
            c_err( "[test_skill](check_formula) wrong formula: %s, index_chain: %s", formula, index_chain )
            return
        end
    end
end

function check_wave_list( _index_chain, _table, _index )
    local index_chain = sformat( "%s-%s", _index_chain, _index )
    local list = _table[_index]
    local ttype = type( list )
    if ttype ~= "table" then
        c_err( "[test_skill](check_wave_list) table expected, got %s, index_chain: %s", ttype, index_chain )
        return
    end
    if #list == 0 then
        c_err( "[test_skill](check_wave_list) empty list, index_chain: %s", ttype, index_chain )
        return
    end
    for _, cfg_id in ipairs( list ) do
        if not WAVE_PATHS[cfg_id] then
            c_err( "[test_skill](check_wave_list) wave cfg not found, cfg_id: %d, index_chain: %s", cfg_id, index_chain )
        end
    end
end

function check_buff_id( _index_chain, _table, _index )
    local index_chain = sformat( "%s-%s", _index_chain, _index )
    local buff_id = _table[_index]
    local ttype = type( buff_id )
    if ttype ~= "number" then
        c_err( "[test_skill](check_buff_id) number expected, got %s, index_chain: %s", ttype, index_chain )
        return
    end
    if not BUFFS[buff_id] then
        c_err( "[test_skill](check_buff_id) buff cfg not found, buff_id: %d, index_chain: %s", buff_id, index_chain )
        return
    end
end

function check_skill_id( _index_chain, _table, _index )
    local index_chain = sformat( "%s-%s", _index_chain, _index )
    local skill_id = _table[_index]
    local ttype = type( skill_id )
    if ttype ~= "number" then
        c_err( "[test_skill](check_skill_id) number expected, got %s, index_chain: %s", ttype, index_chain )
        return
    end
    if not SKILLS[skill_id] then
        c_err( "[test_skill](check_skill_id) skill cfg not found, skill_id: %d, index_chain: %s", skill_id, index_chain )
        return
    end
end

function check_magic_area_id( _index_chain, _table, _index )
    local index_chain = sformat( "%s-%s", _index_chain, _index )
    local id = _table[_index]
    local ttype = type( id )
    if ttype ~= "number" then
        c_err( "[test_skill](check_magic_area_id) number expected, got %s, index_chain: %s", ttype, index_chain )
        return
    end
    if not MAGIC_AREA[id] then
        c_err( "[test_skill](check_magic_area_id) magic area cfg not found, cfg_id: %d, index_chain: %s", id, index_chain )
        return
    end
end

function check_summon_cfg_id( _index_chain, _table, _index )
    local index_chain = sformat( "%s-%s", _index_chain, _index )
    local cfg_id = _table[_index]
    local ttype = type( cfg_id )
    if ttype ~= "number" then
        c_err( "[test_skill](check_summon_cfg_id) number expected, got %s, index_chain: %s", ttype, index_chain )
        return
    end
    if not SUMMON_FORMATIONS.SummonInfo[cfg_id] then
        c_err( "[test_skill](check_summon_cfg_id) summon cfg not found, cfg_id: %d, index_chain: %s", cfg_id, index_chain )
        return
    end
end


function check_bullet_list( _index_chain, _table, _index )
    local index_chain = sformat( "%s-%s", _index_chain, _index )
    local bullet_list = _table[_index]
    local ttype = type( bullet_list )
    if ttype ~= "table" then
        c_err( "[test_skill](check_bullet_list) table expected, got %s, index_chain: %s", ttype, index_chain )
        return
    end
    if #bullet_list == 0 then
        c_err( "[test_skill](check_bullet_list) empty bullet list, index_chain: %s", ttype, index_chain )
        return
    end
    for _, bullet_id in ipairs( bullet_list ) do
        if not BULLETS[bullet_id] then
            c_err( "[test_skill](check_bullet_list) bullet cfg not found, bullet_id: %d, index_chain: %s", bullet_id, index_chain )
        end
    end
end

function check_bullet_list_list( _index_chain, _table, _index )
    local index_chain = sformat( "%s-%s", _index_chain, _index )
    local bullet_list_list = _table[_index]
    local ttype = type( bullet_list_list )
    if ttype ~= "table" then
        c_err( "[test_skill](check_bullet_list_list) table expected, got %s, index_chain: %s", ttype, index_chain )
        return
    end
    if #bullet_list_list == 0 then
        c_err( "[test_skill](check_bullet_list_list) empty bullet list, index_chain: %s", ttype, index_chain )
        return
    end
    for i = 1, #bullet_list_list, 1 do
        check_bullet_list( index_chain, bullet_list_list, i )
    end
end

function check_number_list( _index_chain, _table, _index, _can_be_empty )
    local index_chain = sformat( "%s-%s", _index_chain, _index )
    local param = _table[_index]
    local ttype = type( param )
    if ttype ~= "table" then
        c_err( "[test_skill](check_number_list) table expected, got %s, index_chain: [%s]", ttype, index_chain )
        return
    end
    if #param == 0 and not _can_be_empty then
        c_err( "[test_skill](check_number_list) empty list, index_chain: [%s]", index_chain )
        return
    end
    for i = 1, #param, 1 do
        check_number( index_chain, param, i )
    end
end

function check_number( _index_chain, _table, _index, _min, _max )
    local index_chain = sformat( "%s-%s", _index_chain, _index )
    local param = _table[_index]
    local ttype = type( param )
    if ttype ~= "number" then
        c_err( "[test_skill](check_number) number expected, got %s, index_chain: [%s]", ttype, index_chain )
        return
    end
    if _min and _max then
        if param < _min or param > _max then
            c_err( "[test_skill](check_number) %d out of range[%d-%d], index_chain: [%s]", param, _min, _max, index_chain )
            return
        end
    end
end

function check_rate( _index_chain, _table, _index )
    return check_number( _index_chain, _table, _index, 1, 10000 )
end

function check_relation( _index_chain, _table, _index )
    return check_number( _index_chain, _table, _index, 1, 7 )
end

function check_max_count( _index_chain, _table, _index )
    return check_number( _index_chain, _table, _index, 1, 100 )
end

function check_apply_type( _index_chain, _table, _index )
    return check_number( _index_chain, _table, _index, 1, 7 )
end

function check_degree( _index_chain, _table, _index, _min, _max )
    _min = _min or 1
    _max = _max or 360
    return check_number( _index_chain, _table, _index, _min, _max )
end

function check_length( _index_chain, _table, _index )
    return check_number( _index_chain, _table, _index, 0.0001, limit.INT_MAX )
end

function check_COMMON_EFFECT( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_search_func( _index_chain, effect_def, 2 )
    check_sort_func( _index_chain, effect_def, 3 )
    check_max_count( _index_chain, effect_def, 4 )
    check_relation( _index_chain, effect_def, 5 )
    check_effect_list( _index_chain, effect_def, 6 )
end

function check_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_search_func( _index_chain, effect_def, 2 )
    check_sort_func( _index_chain, effect_def, 3 )
    check_max_count( _index_chain, effect_def, 4 )
    check_relation( _index_chain, effect_def, 5 )
    check_factor_func( _index_chain, effect_def, 6 )
    check_attack_func( _index_chain, effect_def, 7 )
    for i = 8, #effect_def, 1 do
        check_effect( _index_chain, effect_def, i )
    end
end

function check_SIMPLE_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_factor_func( _index_chain, effect_def, 2 )
    check_attack_func( _index_chain, effect_def, 3 )
    for i = 4, #effect_def, 1 do
        check_effect( _index_chain, effect_def, i )
    end
end

function check_DEGREE_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_degree( _index_chain, effect_def, 2, 0, 360 )
    check_length( _index_chain, effect_def, 3 )
    check_max_count( _index_chain, effect_def, 4 )
    check_apply_type( _index_chain, effect_def, 5 )
    check_factor_func( _index_chain, effect_def, 6 )
    check_attack_func( _index_chain, effect_def, 7 )
    for i = 8, #effect_def, 1 do
        check_effect( _index_chain, effect_def, i )
    end
end

function check_DEGREE_RANDOM_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_degree( _index_chain, effect_def, 2 )
    check_length( _index_chain, effect_def, 3 )
    check_max_count( _index_chain, effect_def, 4 )
    check_apply_type( _index_chain, effect_def, 5 )
    check_factor_func( _index_chain, effect_def, 6 )
    check_attack_func( _index_chain, effect_def, 7 )
    for i = 8, #effect_def, 1 do
        check_effect( _index_chain, effect_def, i )
    end
end

function check_RECT_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_length( _index_chain, effect_def, 2 )
    check_length( _index_chain, effect_def, 3 )
    check_max_count( _index_chain, effect_def, 4 )
    check_apply_type( _index_chain, effect_def, 5 )
    check_factor_func( _index_chain, effect_def, 6 )
    check_attack_func( _index_chain, effect_def, 7 )
    for i = 8, #effect_def, 1 do
        check_effect( _index_chain, effect_def, i )
    end
end

function check_RUSH_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_number( _index_chain, effect_def, 2 )
    check_length( _index_chain, effect_def, 3 )
    check_length( _index_chain, effect_def, 4 )
    check_apply_type( _index_chain, effect_def, 5 )
    check_number( _index_chain, effect_def, 6 )
    check_factor_func( _index_chain, effect_def, 7 )
    check_attack_func( _index_chain, effect_def, 8 )
    for i = 9, #effect_def, 1 do
        check_effect( _index_chain, effect_def, i )
    end
end

function check_RUSHEX_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_number( _index_chain, effect_def, 2 )
    check_length( _index_chain, effect_def, 3 )
    check_length( _index_chain, effect_def, 4 )
    check_apply_type( _index_chain, effect_def, 5 )
    check_number( _index_chain, effect_def, 6 )
    check_factor_func( _index_chain, effect_def, 7 )
    check_attack_func( _index_chain, effect_def, 8 )
    for i = 9, #effect_def, 1 do
        check_effect( _index_chain, effect_def, i )
    end
end

function check_PERSIST_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_degree( _index_chain, effect_def, 2 )
    check_max_count( _index_chain, effect_def, 3 )
    check_apply_type( _index_chain, effect_def, 4 )
    check_length( _index_chain, effect_def, 5 )
    check_number( _index_chain, effect_def, 6 )
    check_number( _index_chain, effect_def, 7 )
    check_number( _index_chain, effect_def, 8 )
    check_factor_func( _index_chain, effect_def, 9 )
    check_attack_func( _index_chain, effect_def, 10 )
    for i = 11, #effect_def, 1 do
        check_effect( _index_chain, effect_def, i )
    end
end

function check_CHARGE_ATTACK( _index_chain, _table, _index )
end

function check_BULLET_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_bullet_list( _index_chain, effect_def, 2 )
    check_degree( _index_chain, effect_def, 3, 0, 360 )
    check_apply_type( _index_chain, effect_def, 4 )
    check_factor_func( _index_chain, effect_def, 5 )
    check_attack_func( _index_chain, effect_def, 6 )
    for i = 7, #effect_def, 1 do
        check_effect( _index_chain, effect_def, i )
    end
end

function check_MULTI_BULLET_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_bullet_list_list( _index_chain, effect_def, 2 )
    check_degree( _index_chain, effect_def, 3, 0, 360 )
    check_apply_type( _index_chain, effect_def, 4 )
    check_factor_func( _index_chain, effect_def, 5 )
    check_attack_func( _index_chain, effect_def, 6 )
    for i = 7, #effect_def, 1 do
        check_effect( _index_chain, effect_def, i )
    end
end

function check_JUMP_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_number( _index_chain, effect_def, 2 )
    check_length( _index_chain, effect_def, 3 )
    check_effect_list( _index_chain, effect_def, 4 )
end

function check_BUFF( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_degree( _index_chain, effect_def, 2 )
    check_max_count( _index_chain, effect_def, 3 )
    check_apply_type( _index_chain, effect_def, 4 )
    check_buff_id( _index_chain, effect_def, 5 )
end

function check_SELF_BUFF( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_formula( _index_chain, effect_def, 2 )
    check_buff_id( _index_chain, effect_def, 3 )
end

function check_TARGET_BUFF( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_formula( _index_chain, effect_def, 2 )
    check_buff_id( _index_chain, effect_def, 3 )
end

function check_SKILLING_BUFF( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_formula( _index_chain, effect_def, 2 )
    check_buff_id( _index_chain, effect_def, 3 )
    if effect_def[4] then
        check_number( _index_chain, effect_def, 4, 1, 2 )
    end
end

function check_DEL_BUFF_GROUP( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_degree( _index_chain, effect_def, 2 )
    check_max_count( _index_chain, effect_def, 3 )
    check_apply_type( _index_chain, effect_def, 4 )
    check_number_list( _index_chain, effect_def, 5 )
    check_number_list( _index_chain, effect_def, 6, true )
end

function check_TARGET_DEL_BUFF_GROUP( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_rate( _index_chain, effect_def, 2 )
    check_number_list( _index_chain, effect_def, 3 )
    check_number_list( _index_chain, effect_def, 4, true )
end

function check_SELF_DEL_BUFF_GROUP( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_rate( _index_chain, effect_def, 2 )
    check_number_list( _index_chain, effect_def, 3 )
    check_number_list( _index_chain, effect_def, 4, true )
end

function check_SUMMON_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_summon_cfg_id( _index_chain, effect_def, 2 )
    check_number( _index_chain, effect_def, 3, 1, 100 )
    check_number( _index_chain, effect_def, 4, 0, 1 )
end

function check_POSITION_CIRCLE_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_length( _index_chain, effect_def, 2 )
    check_max_count( _index_chain, effect_def, 3 )
    check_apply_type( _index_chain, effect_def, 4 )
    check_number( _index_chain, effect_def, 5 )
    check_factor_func( _index_chain, effect_def, 6 )
    check_attack_func( _index_chain, effect_def, 7 )
    for i = 8, #effect_def, 1 do
        check_effect( _index_chain, effect_def, i )
    end
end

function check_WAVE_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_wave_list( _index_chain, effect_def, 2 )
    check_number( _index_chain, effect_def, 3 )
    check_length( _index_chain, effect_def, 4 )
    check_max_count( _index_chain, effect_def, 5 )
    check_apply_type( _index_chain, effect_def, 6 )
    check_factor_func( _index_chain, effect_def, 7 )
    check_attack_func( _index_chain, effect_def, 8 )
    for i = 9, #effect_def, 1 do
        check_effect( _index_chain, effect_def, i )
    end
end

function check_MOVE_DEGREE_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_number( _index_chain, effect_def, 2 )
    check_degree( _index_chain, effect_def, 3 )
    check_number( _index_chain, effect_def, 4 )
    check_number( _index_chain, effect_def, 5, 0, 1 )
    check_max_count( _index_chain, effect_def, 6 )
    check_apply_type( _index_chain, effect_def, 7 )
    check_factor_func( _index_chain, effect_def, 8 )
    check_attack_func( _index_chain, effect_def, 9 )
    for i = 10, #effect_def, 1 do
        check_effect( _index_chain, effect_def, i )
    end
end

function check_MAGIC_AREA_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_magic_area_id( _index_chain, effect_def, 2 )
    check_apply_type( _index_chain, effect_def, 3 )
    check_number( _index_chain, effect_def, 4, 0, 1 )
    check_number( _index_chain, effect_def, 5, 0, 1 )
    check_number( _index_chain, effect_def, 6 )
    check_length( _index_chain, effect_def, 7 )
    check_number( _index_chain, effect_def, 8 )
end

function check_TELEPORT( _index_chain, _table, _index )
end

function check_ACTIVATE_TRIGGER( _index_chain, _table, _index )
end

function check_CONDEFX( _index_chain, _table, _index )
    local effect_def = _table[_index]
    --check_cond_effect( _index_chain, effect_def, 2 )
    check_effect_list( _index_chain, effect_def, 3 )
    check_effect_list( _index_chain, effect_def, 4 )
end

function check_SELF_DEAD( _index_chain, _table, _index )
end

function check_CORPSE_REVIVE( _index_chain, _table, _index )
end

function check_LEADING_ATTACK( _index_chain, _table, _index )
end

function check_BLOOD_ATTACK( _index_chain, _table, _index )
end

function check_MULTI_HIT_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_number_list( _index_chain, effect_def, 2 )
    check_effect_list_list( _index_chain, effect_def, 3 )
end

function check_RAGE_ADD( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_formula( _index_chain, effect_def, 2 )
end

function check_RAGE_COST( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_formula( _index_chain, effect_def, 2 )
end

function check_IMMEDIATE_CD( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_rate( _index_chain, effect_def, 2 )
end

function check_PULL_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_degree( _index_chain, effect_def, 2 )
    check_max_count( _index_chain, effect_def, 3 )
    check_apply_type( _index_chain, effect_def, 4 )
    check_number( _index_chain, effect_def, 5 )
    check_factor_func( _index_chain, effect_def, 6 )
    check_attack_func( _index_chain, effect_def, 7 )
    for i = 8, #effect_def, 1 do
        check_effect( _index_chain, effect_def, i )
    end
end

function check_DRAG_ATTACK( _index_chain, _table, _index )
end

function check_PUSH_ATTACK( _index_chain, _table, _index )
end

function check_RENEWAL_BUFF( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_buff_id( _index_chain, effect_def, 2 )
    check_number( _index_chain, effect_def, 3, 1, 2 )
    check_number( _index_chain, effect_def, 4, 1, 2 )
end

function check_SHORTEN_CD( _index_chain, _table, _index )
end

function check_SKILL_CHASE( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_skill_id( _index_chain, effect_def, 2 )
    check_number( _index_chain, effect_def, 3 )
end

function check_AIRDROP_ATTACK( _index_chain, _table, _index )
end

function check_FOLLOW_STRAFE_ATTACK( _index_chain, _table, _index )
end

function check_CIRCLE_OFFSET_ATTACK( _index_chain, _table, _index )
end

function check_MULTI_ZONE_STRAFE_ATTACK( _index_chain, _table, _index )
end

function check_SELF_CORPSE_EXPLOSION( _index_chain, _table, _index )
end

function check_DYNAMIC_CREATE_MONSTER( _index_chain, _table, _index )
end

function check_RANDOM_SELECT_ADD_BUFF( _index_chain, _table, _index )
end

function check_RESET_THREAT( _index_chain, _table, _index )
end

function check_DROP_LEVEL_CHANGE( _index_chain, _table, _index )
end

function check_REMOVE_SELF_DAZED( _index_chain, _table, _index )
end

function check_BOUNCE_ATTACK( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_search_func( _index_chain, effect_def, 2 )
    check_number( _index_chain, effect_def, 3, 0, 1 )
    check_number( _index_chain, effect_def, 4 )
    check_number( _index_chain, effect_def, 5 )
    check_number( _index_chain, effect_def, 6 )
    check_number( _index_chain, effect_def, 7 )
    check_effect_list( _index_chain, effect_def, 8 )
end

function check_RANDOM_DEL_BUFF( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_number( _index_chain, effect_def, 2 )
end

function check_RANDOM_DEL_DEBUFF( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_number( _index_chain, effect_def, 2 )
end

function check_MAX_ATTRIBUTE_BUFF( _index_chain, _table, _index )
    local effect_def = _table[_index]
    --check_buff_id_list( _index_chain, effect_def, 2 )
    check_number( _index_chain, effect_def, 3 )
end

function check_RANDOM_TRANSFER( _index_chain, _table, _index )
    local effect_def = _table[_index]
    check_number( _index_chain, effect_def, 2 )
    check_number( _index_chain, effect_def, 3 )
    check_degree( _index_chain, effect_def, 4, 30, 360 )
end

function check_DECONTROL( _index_chain, _table, _index )
end

function check_DEL_STATE_SKILLING( _index_chain, _table, _index )
end

function check_effect( _index_chain, _table, _index )
    local index_chain = sformat( "%s-%s", _index_chain, _index )
    local effect_def = _table[_index]
    if type( effect_def ) ~= "table" then
        c_err( "[test_skill](check_effect) effect must be table, index_chain: [%s]", index_chain )
        return false
    end
    local effect_name = effect_def[1]
    if effect_name then
        local func = rawget( skill, effect_name )
        if func then
            local check_func = rawget( test_skill, sformat( "check_%s", effect_name ) )
            if check_func then
                index_chain = sformat( "%s(%s)", index_chain, effect_name )
                check_func( index_chain, _table, _index )
            end
        else
            c_err( "[test_skill](check_effect) effect func not found, effect_name: %s, index_chain: %s", effect_name, index_chain )
        end
    end
end

function check_effect_list( _index_chain, _table, _index )
    local index_chain = sformat( "%s-%s", _index_chain, _index )
    local effect_list = _table[_index]
    if type( effect_list ) ~= "table" then
        c_err( "[test_skill](check_effect_list) not a table, index_chain: [%s]", index_chain )
        return false
    end
    for i, effect_def in ipairs( effect_list ) do
        check_effect( index_chain, effect_list, i )
    end
end

function check_effect_list_list( _index_chain, _table, _index )
    local index_chain = sformat( "%s-%s", _index_chain, _index )
    local effect_list_list = _table[_index]
    if type( effect_list_list ) ~= "table" then
        c_err( "[test_skill](check_effect_list_list) not a table, index_chain: [%s]", index_chain )
        return false
    end
    for i, effect_list in ipairs( effect_list_list ) do
        check_effect_list( index_chain, effect_list_list, i )
    end
end

function check_skill_effects()
    for skill_id, skill_def in pairs( SKILLS ) do
        local index_chain = sformat( "%s", skill_id )
        check_effect_list( index_chain, skill_def, "start_action" )
        check_effect_list( index_chain, skill_def, "action" )
    end
end

local function test_attack_num()
    local check_result = true

    local check_num_func = function( skill_id, effect, effect_type )
        local attack_num = 0

        if effect[1] == "DEGREE_ATTACK" then
            attack_num = effect[3]
        elseif effect[1] == "PERSIST_ATTACK" then
            attack_num = effect[3]
        elseif effect[1] == "MOVE_DEGREE_ATTACK" then 
            attack_num = effect[6]
        end

        if attack_num > 8 then
            c_err(sformat("skill_id:%d's %s ,effect: %s attack num is:%d > 8 !", skill_id, effect_type, effect[1], attack_num ))
            check_result = false
        end
    end

    for skill_id , v in pairs( SKILLS ) do
        local action = v.action
        local start_action = v.start_action

        if v.action then
            for _ , effect in ipairs( v.action ) do
                check_num_func( skill_id, effect ,"action" )
            end
        end

        if v.start_action then
            for _ , effect in ipairs( v.start_action ) do
                check_num_func( skill_id, effect ,"start_action" )
            end
        end
    end

    return check_result 
end

local function test_magic_area()
    local has_error = false
    for id, cfg in pairs( MAGIC_AREA ) do
        local skill_id = cfg.skill_id
        if skill_id ~= 0 then
            local skill_cfg = SKILLS[skill_id]
            if not skill_cfg then
                c_err( "MAGIC_AREA skill not found, magic_area_id: %d, skill_id: %d", id, skill_id )
                has_error = true
            end
        end
    end
    return not has_error
end

function check_skill_anim()
    local skill_anim_map = {}
    for skill_id, skill_def in pairs( SKILLS ) do
        if skill_def.anim_section == 1 then
            local anim_name = skill_def.anim_list[1][1]
            skill_anim_map[anim_name] = true
        end
    end
    skill_anim_map["open"] = nil
    skill_anim_map["close"] = nil
    skill_anim_map["act"] = nil
    skill_anim_map["hit"] = nil
    skill_anim_map["run_fast"] = nil

    for model_id, model_info in pairs( MODEL_INFO ) do
        for anim_name, anim_info in pairs( model_info.anim ) do
            if skill_anim_map[anim_name] then
                if not anim_info["start"] then
                    c_err( "[test_skill](check_skill_anim) no start, model_id: %d, anim_name: %s", model_id, anim_name )
                end
                if not anim_info["hj"] then
                    c_err( "[test_skill](check_skill_anim) no hj, model_id: %d, anim_name: %s", model_id, anim_name )
                end
                if not anim_info["hit"] then
                    c_err( "[test_skill](check_skill_anim) no hit, model_id: %d, anim_name: %s", model_id, anim_name )
                end
                if not anim_info["bs"] then
                    c_err( "[test_skill](check_skill_anim) no bs, model_id: %d, anim_name: %s", model_id, anim_name )
                end
                if not anim_info["end"] then
                    c_err( "[test_skill](check_skill_anim) no end, model_id: %d, anim_name: %s", model_id, anim_name )
                end
            end
        end
    end
end

-------------------

local TEST_FUNC ={
        --[1] = test_attack_num
        [1] = test_magic_area,
        [2] = check_skill_effects,
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

