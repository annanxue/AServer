module( "ctrl_mng", package.seeall )

local sformat, tinsert = string.format, table.insert

g_ctrl_mng = g_ctrl_mng or {}

function get_ctrl( _dpid, _ctrl_id )
    local ctrl_mng_of_one_dpid = g_ctrl_mng[_dpid]
    if not ctrl_mng_of_one_dpid then
        return nil
    end
    return ctrl_mng_of_one_dpid[_ctrl_id]
end

function add_dpid( _dpid )
    g_ctrl_mng[_dpid] = {}
end

function remove_dpid( _dpid )
    local ctrl_mng_of_one_dpid = g_ctrl_mng[_dpid]
    if ctrl_mng_of_one_dpid then
        for ctrl_id, ctrl in pairs(ctrl_mng_of_one_dpid) do
            ctrl.core_:c_client_delete()
        end
        g_ctrl_mng[_dpid] = nil
    end
end

function add_ctrl( _dpid, _ctrl_id, _ctrl )
    local ctrl_mng_of_one_dpid = g_ctrl_mng[_dpid]
    if ctrl_mng_of_one_dpid[_ctrl_id] then
        c_err( "[ctrl_mng](add_ctrl) ctrl id %d of dpid 0x%X already exists!", _ctrl_id, _dpid )
        return
    end
    ctrl_mng_of_one_dpid[_ctrl_id] = _ctrl
end

function remove_ctrl( _dpid, _ctrl_id )
    local ctrl_mng_of_one_dpid = g_ctrl_mng[_dpid]
    if not ctrl_mng_of_one_dpid then return end
    local ctrl = ctrl_mng_of_one_dpid[_ctrl_id]
    if ctrl then
        ctrl.core_:c_client_delete()
        ctrl_mng_of_one_dpid[_ctrl_id] = nil
    end
end

function remove_all_ctrls_except_hero( _dpid )
    local hero_ctrl_id = 0
    local hero = robotclient.get_hero( _dpid )
    if hero then
        hero_ctrl_id = hero.ctrl_id_
    end
    local ctrl_mng_of_one_dpid = g_ctrl_mng[_dpid]
    if ctrl_mng_of_one_dpid then
        local ctrl_id_list = {}
        for ctrl_id, _ in pairs( ctrl_mng_of_one_dpid ) do
            if ctrl_id ~= hero_ctrl_id then
                tinsert( ctrl_id_list, ctrl_id )
            end
        end
        for _, ctrl_id in ipairs( ctrl_id_list ) do
            remove_ctrl( _dpid, ctrl_id )
        end
    end
end

func_map =
{
}

function ctrl_lua_handler( _func_name, ... )
    local func = func_map[_func_name]
    if not func then
        c_err( sformat( "[ctrl_mng](ctrl_call)func %s not found", _func_name ) )
        return
    end
    return func( ... )
end




function show_all_player() 
    for dpid, list in pairs( g_ctrl_mng ) do 
        print( string.format( "dpid:0x%x", dpid ) ) 
        for ctrl_id, ctrl in pairs( list ) do 
            if ctrl:is_player() then 
                print( string.format( "\t\tctrlid:%d, name:%s, server_id:%d, player_id:%d", ctrl_id, ctrl.player_name_, ctrl.server_id_, ctrl.player_id_ ) )
            end
        end
    end
end

