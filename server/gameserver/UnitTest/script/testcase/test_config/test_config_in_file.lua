module( "test_config_in_file", package.seeall )

function in_loop( _index, _existed )
    _existed = _existed or {}
    if _existed[_index] then
        return true
    end
    _existed[_index] = true
    for i, v in ipairs( player_attr.LEVEL_UP_ATTR[_index].cascade ) do
        if in_loop( v, _existed ) then
            return true
        end
    end
    return false
end


local function test_LEVEL_UP_ATTR()
    local check_result = true

    for k, v in pairs( player_attr.LEVEL_UP_ATTR ) do
        if in_loop( k ) then
            check_result = false
            c_err(string.format("there is loop in LEVEL_UP_ATTR!",i))
        end
    end

    return check_result
end



-------------------
local TEST_FUNC ={
        [1] = test_LEVEL_UP_ATTR,
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

