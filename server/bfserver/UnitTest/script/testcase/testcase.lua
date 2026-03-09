module( "testcase", package.seeall )

require_ex("luacommon/testcase/test_common/test_common")

function print_hello( _str )
    c_log( "hello world! " .. _str  )
end

function back_to_gs()
    for _, target in pairs(ctrl_mng.g_ctrl_mng) do
        if target:is_player() then
            bfsvr.send_player_back_to_game( target )
            c_log( "player_id:%d back to gs over!", target.player_id_ )
        end
    end
end

function sv( index, value )
    for _, target in pairs ( ctrl_mng.g_ctrl_mng ) do   
        if target:is_player() then 
            if target and target[index] then 
                target[index] = value 
                target:broadcast_value( index )
                c_log(string.format("%s has changed!",index))
            else 
                c_err(string.format("%s not exits",index))
            end  
        end  
    end  
end
