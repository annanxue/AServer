module( "test_model", package.seeall )

local MODEL_INFO = MODEL_INFO  

local function test_skill()
    local check_result = true

    for i, v in pairs(MODEL_INFO) do
        for j, k in pairs(v.anim) do
            if( string.find(k,"skill") ) then
                print (k)
            end
        end


    end
        
    return check_result 
end

-------------------

local TEST_FUNC ={
        [1] = test_skill
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

