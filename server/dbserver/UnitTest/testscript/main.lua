--------------------------------------------------
-- requires
--------------------------------------------------
g_db_test = g_db_test or DataBase:c_new() 
g_pack_ar = LAr:c_new()
g_unpack_ar = LAr:c_new()

--------------------------------------------------
-- constants
--------------------------------------------------

--------------------------------------------------
-- functions
--------------------------------------------------

function on_exit()
    if (g_db_test) then
        g_db_test:c_delete()
    end
end

function set_frame( _frame )
    g_frame = _frame
end

function message_handler( _buffer, _packet_type, _dpid )
    local func = client.func_map[_packet_type]
    if func then
        func( _buffer, _dpid )
    end
end

function acount_g_call_d( _robj, _str )
    --delete this func later!
    print( "acount_g_call_d~~~~~~", _str )
    g_robj_game.acount_d_call_g( "lalala" )
end

function main()
    --init db_test
    local host, user, passwd, port, dbname = "127.0.0.1", "root", "root", 3306, "test"
    if( g_db_test:c_connect( host, user, passwd, port ) < 0 ) then
        c_err( "[DB] (main) connect database test failed.".. "[" .. host .. ":" .. port .."]".. user )
        return false
    end
    if g_db_test:c_select_db( dbname ) ~= 0 then
        c_err( "[DB] (main) select database test failed" )
        return false
    end

    if( g_db_test:c_modify([[
        drop table if exists player;
        ]]  ) < 0 ) then
        c_err( "drop table player failed" )
        return false
    end
    if( g_db_test:c_modify([[
        create table player
        (
            id int(3) auto_increment not null primary key,
            name char(10) not null,
            test_char TINYINT,
            test_long int(32),
            test_ll bigint,
            test_ull bigint unsigned,
            test_float float,
            test_double double,
            test_str char(255),
            test_blog blob
        );
        ]]  ) < 0 ) then
        c_err( "create player table failed" )
        return false
    end

    if( g_db_test:c_modify([[
        insert into player (name) values('name1');
        ]]  ) < 0 ) then
        c_err( "create player table failed" )
        return false
    end

    if( g_db_test:c_query([[
        select id, name from player;
        ]]  ) < 0 ) then
        c_err( "create player table failed" )
        return false
    end
    local player_count = g_db_test:c_row_num()
    for i=1, player_count do
        if( g_db_test:c_fetch() == 0 ) then
            local player_id, player_name =  
                g_db_test:c_get_format( "is", "id", "name")
            c_err( "id:"..player_id..", name:"..player_name )
        end
    end

    ----------------------------------statement example below----------------------------
    g_pack_ar:flush()
    g_pack_ar:write_ulong(0)
    g_pack_ar:write_ulong(9)
    -- b: char, int8
    -- i: int, int32
    -- l: long, int64
    -- L: qword, uint64
    -- f: float
    -- d: double
    -- s: string
    -- a: ar: blob
    if( g_db_test:c_stmt_format_modify([[
        insert into player (name, test_char, test_long, test_ll, test_ull, test_float, test_double, test_str, test_blog) 
        values(?, ?, ?, ?, ?, ?, ?, ?, ?);
        ]], "sbilLfdsa", "name_1", "2", 3, 4, 5, 6.6, 7.7, "8", g_pack_ar:get_buffer() ) < 0 ) then
        c_err( "create player table failed" )
        return false
    end
    c_log(string.format("insert id:%d", g_db_test:c_stmt_get_insert_id()))


    if( g_db_test:c_stmt_format_query([[
        select id, name, test_char, test_long, test_ll, test_ull, 
        test_float, test_double, test_str, test_blog from player where name = ?;
        ]], "s", "name_1", "isbilLfdsa"  ) < 0 ) then
        c_err( "c_stmt_format_query failed" )
        return false
    end
    local player_count = g_db_test:c_stmt_row_num()
    c_log( "player count:" .. player_count )
    for i=1, player_count do
        if( g_db_test:c_stmt_fetch() == 0 ) then
            local id, name, test_char, test_long, test_ll, test_ull, test_float, test_double, test_str, test_blog =  
                g_db_test:c_stmt_get("id", "name", "test_char", "test_long", "test_ll", "test_ull", "test_float", "test_double", "test_str", "test_blog")
            
            g_unpack_ar:reuse( test_blog )
            c_log( 
                "id:" .. id..
                ", name:" .. name..
                ", test_char:" ..    test_char ..
                ", test_long:" ..    test_long ..
                ", test_ll:" ..      test_ll .. 
                ", test_ull:" ..     test_ull .. 
                ", test_float:" ..   test_float  ..
                ", test_double:" ..  test_double .. 
                ", test_str:" ..     test_str .. 
                ", test_blog:" ..    g_unpack_ar:read_ulong() .. "," .. g_unpack_ar:read_ulong()
                )
        end
    end

    return true 
end

assert( main() )

