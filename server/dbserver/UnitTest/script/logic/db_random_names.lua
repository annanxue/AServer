module( "db_random_names", package.seeall )

GENDER_MALE = 1
GENDER_FEMALE = 2
GENDER_CNT = 2

local sformat, tinsert, tremove, mrandom = string.format, table.insert, table.remove, math.random

local DELETE_RANDOM_NAME_SQL = [[
    DELETE FROM %s WHERE player_name = '%s'
]]

local function get_table_name( _gender )
    if _gender == GENDER_MALE then
        return "RANDOM_MALE_NAME_TBL"
    end
    return "RANDOM_FEMALE_NAME_TBL"
end

function add_random_name( _gender, _player_name )
    local name_array = g_name_array[_gender]
    local name2idx = g_name2idx[_gender]

    if name2idx[_player_name] then
        c_err( "[db_random_name](add_random_name) '%s' is repeatedly added", _player_name )
        return
    end

    tinsert( name_array, _player_name )
    name2idx[_player_name] = #name_array
end

-- 初始化，将可用的随机名字读入内存。因为随机名字的数据库导入和初始化开销很大，因此不支持热更新。
if not g_name2idx then
    c_log( "[db_random_names] Initializing random names begin" )

    g_name2idx = { {}, {} }
    g_name_array = { {}, {} }

    local SELECT_RANDOM_NAME_TBL_SQL = "SELECT player_name FROM %s"

    local db_character = dbserver.g_db_character

    local function init_random_names_for_gender( _gender )
        if db_character:c_query( sformat( SELECT_RANDOM_NAME_TBL_SQL, get_table_name( _gender ) ) ) < 0 then
            c_err( "[db_random_names](init_random_names_for_gender) sql err" )
            return
        end

        local row_num = db_character:c_row_num()

        for i = 1, row_num do
            if db_character:c_fetch() ~= 0 then
                c_err( "[db_random_names](init_random_names_for_gender) sql err" )
                return
            end
            local r1, player_name = db_character:c_get_str( "player_name", 64 )
            if r1 < 0 then
                c_err( "[db_random_names](init_random_names_for_gender) sql err" )
                return
            end
            add_random_name( _gender, player_name )
        end
    end

    init_random_names_for_gender( GENDER_MALE )
    init_random_names_for_gender( GENDER_FEMALE )

    c_log( "[db_random_names] Initializing random names end" )
end

local function remove_random_name_for_gender( _gender, _random_name, _remove_from_db )
    local name2idx = g_name2idx[_gender]
    local idx = name2idx[_random_name]
    if not idx then
        return true
    end

    -- 把要剔除的名字和name_array末尾的名字交换，然后移除末尾元素，避免移动整个name_array
    local name_array = g_name_array[_gender]
    local tmp_name = name_array[#name_array]
    name_array[idx] = tmp_name
    tremove( name_array )
    name2idx[tmp_name] = idx
    name2idx[_random_name] = nil

    -- 将名字从数据库中删除
    if _remove_from_db then
        local db_character = dbserver.g_db_character
        if db_character:c_modify( sformat( DELETE_RANDOM_NAME_SQL, get_table_name( _gender ), _random_name ) ) < 0 then
            c_err( "[db_random_names](remove_random_name_for_gender) gender: %d, random_name: %s; sql err", _gender, _random_name )
            return false
        end
    end
    return true
end

function remove_random_name( _random_name, _remove_from_db )
    local ret_male = remove_random_name_for_gender( GENDER_MALE, _random_name, _remove_from_db )
    local ret_female = remove_random_name_for_gender( GENDER_FEMALE, _random_name, _remove_from_db )
    return ret_male and ret_female
end

function is_male_random_name( _name )
    return g_name2idx[GENDER_MALE][_name] ~= nil
end

function is_female_random_name( _name )
    return g_name2idx[GENDER_FEMALE][_name] ~= nil
end

function get_random_name( _account_id, _gender )
    if _gender ~= GENDER_MALE and _gender ~= GENDER_FEMALE then
        return ""
    end

    db_login.unlock_name_of_account( _account_id )

    local name_array = g_name_array[_gender]
    if #name_array == 0 then
        return ""
    end

    local idx = mrandom( #name_array )
    local ret_name = name_array[idx]

    db_login.lock_name_for_account( _account_id, ret_name )

    return ret_name
end

-- 机器人随机名字
function get_robot_random_name( _gender )
    if _gender ~= GENDER_MALE and _gender ~= GENDER_FEMALE then
        return ""
    end

    local name_array = g_name_array[_gender]
    if #name_array == 0 then
        return ""
    end

    local idx = mrandom( #name_array )
    local ret_name = name_array[idx]

    -- 立即删除 
    remove_random_name( ret_name )

    return ret_name
end 