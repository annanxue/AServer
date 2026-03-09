module( "colossus_mng", package.seeall )

local table_size = utils.table.size

function on_recv_data( _server_id, _pmap, _colossus_refresh_id, _type_id )
    add2divide(false, _server_id, _pmap, _colossus_refresh_id, _type_id )
    --[[
    local count = table_size( _pmap )
    c_err("on_recv_data, pmap_count:%d", count)
    --]]
end



