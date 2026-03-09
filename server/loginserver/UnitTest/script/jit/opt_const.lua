
--[[
--  const type: nil, boolean, number, string
--]]

local registry = debug.getregistry()
local dgmt = debug.getmetatable

-- set cfg table empty to turn off opt_const 
-- cfg should not be changed by user(compiler only). 
local cfg =
{
    -- LAr
    [1] = 
    {
        const_keys = 
        {
            before_send = true,
            flush_before_send = true,
            send = true,
            send_one_ar  = true,
            write_byte = true,
            write_ubyte = true,
            write_char = true,
            write_short = true,
            write_ushort = true,
            write_int = true,
            write_uint = true,
            write_long = true,
            write_ulong = true,
            write_float = true,
            write_double = true,
            write_string = true,
            write_boolean = true,
            write_format = true,
            write_var_array_short = true,
            read_byte = true,
            read_ubyte = true,
            read_char = true,
            read_short = true,
            read_ushort = true,
            read_int = true,
            read_uint = true,
            read_long = true,
            read_ulong = true,
            read_float = true,
            read_double = true,
            read_string = true,
            read_boolean = true,
            read_format = true,
            read_var_array_short = true,
            bpack = true,
            flush = true,
            upack = true,
            get_this = true,
            is_loading = true,
            is_storing = true,
            get_offset = true,
            write_int_at = true,
            write_byte_at = true,
            c_delete = true,
            write_sntype = true,
            flush_write_sntype = true,
            check_ar = true,
            write_buffer = true,
        },
        is_parent = function( t )
            local mt = dgmt( t ) or {}
            return rawget(mt, "__index") == _G.LAr
        end,
    },
}


local opt_const = 
{
    -- const keys finder
    get_const_keys = function( t, opt_lv )
        if opt_lv >= 3 then
            for i, v in ipairs( cfg ) do
                if v.is_parent( t ) then
                    return i, v.const_keys
                end
            end
        end
        return nil, nil
    end,

    -- get cfg 
    get_const_cfg = function()
        return cfg
    end,

    -- opt begin
    -- create const_state 
    begin_const_state = function()
        local stmap = {}
        for i, v in ipairs( cfg ) do
            stmap[i] = {}
        end
        return stmap
    end,

    -- opt finish
    -- do nothing
}

return opt_const
