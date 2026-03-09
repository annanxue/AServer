module( "robot_nothing", package.seeall )

function after_hero_added( _dpid, _hero )
    local ctrl_id = _hero.ctrl_id_
    c_log( "[robot_nothing] after_hero_added, do nothing, dpid: 0x%X, ctrl_id: %d", _dpid, ctrl_id )
end
