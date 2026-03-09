module( "login_mng",  package.seeall )

-- ------------------------------------
-- Local Lib Functions
-- ------------------------------------
local m_random, m_max, str_format = math.random, math.max, string.format

-- ------------------------------------
-- Global Constants & Var
-- ------------------------------------
local EMPTY_ACCOUNT_ID = 0

local MAX_SESSION_TIME_OUT_TIME_S  = 60 * 60
local MIN_SESSION_TIME_OUT_TIME_S  = 5 * 60

g_connection_map = g_connection_map or {}                           -- key: dpid, value: account_id
g_session_map = g_session_map or {}                                 -- key: account_id, value: session

g_migrated_waiting_rejoin_map = g_migrated_waiting_rejoin_map or {} -- key: account_id, value: { dpid_, login_token_ }

g_old_connection_timer_map = g_old_connection_timer_map or {}

local LOGIN_QUEUE_SIZE = 9999
local SAFE_MAX_USER_NUM = 100
local REFRESH_QUEUE_RANK_TIME_S = 5
local PRINT_ONLINE_COUNT_TIME_S = 60

g_max_user_num = g_max_user_num or SAFE_MAX_USER_NUM

g_login_queue = g_login_queue or login_queue_t.new( LOGIN_QUEUE_SIZE ) 
g_online_counter = g_online_counter or counter_t.new()
g_refresh_queue_rank_timer = g_refresh_queue_rank_timer or -1 
g_print_online_count_timer = g_print_online_count_timer or -1

-- ------------------------------------
-- Internal Functions
-- ------------------------------------

local function generate_certify_key()
    return m_random( 2147483647 ) -- 2 ^ 31 - 1
end

local function suspend_session( _session )
    if _session.suspend_ then
        c_err( "[login_mng](suspend_session) session is already suspended, account_id: %d", _session.account_id_ )
        return 
    end

    if _session.migrate_ then
        local player_id = _session.player_id_
        local bf_player = bfclient.g_bf_players[ player_id ]
        if bf_player and bf_player.state_ == bfclient.BF_PLAYER_STATE_GAME2BF then
            c_login( "[login_mng](suspend_session) session is migrate to bf and player is in game2bf state, do not suspend session this time, account_id: %d, player_id: %d", _session.account_id_, player_id )
            return
        end
    end

    local dpid = _session.dpid_
    local player = player_mng.get_player_by_dpid( dpid )
    if player then
        player_mng.on_save_player( player.server_id_, player.player_id_ ) 
        logclient.log_logout(player)
		logclient.log_user(player)
    end
    
    _session.suspend_ = true

    local waiting_time = m_max( MAX_SESSION_TIME_OUT_TIME_S - g_online_counter:count(), MIN_SESSION_TIME_OUT_TIME_S ) 
    _session.close_timer_ = g_timermng:c_add_timer_second( waiting_time, timers.CLOSE_SUSPEND_SESSION_TIMER, _session.account_id_, 0, 0 )
 
    c_login( "[login_mng](suspend_session) account_id: %d, dpid: 0x%X", _session.account_id_, dpid )
end

local function awake_session( _session, _dpid )
    _session.dpid_ = _dpid
    _session.suspend_ = false
    if _session.close_timer_ > 0 then
        g_timermng:c_del_timer( _session.close_timer_ ) 
        _session.close_timer_ = -1
    end
end

local function migrate_session_to_bf( _session, _player_id )
    if not _session.migrate_ then
        _session.migrate_ = true    
        _session.player_id_ = _player_id
        c_trace( "[login_mng](migrate_session_to_bf) session is migrated to bf, account_id: %d, player_id: %d", _session.account_id_, _player_id )
    else
        c_err( "[login_mng](migrate_session_to_bf) session is already migrated to bf, account_id: %d", _session.account_id_ )
    end
end

local function migrate_session_back_gs( _session )
    if _session.migrate_ then
        _session.migrate_ = false
        local player_id = _session.player_id_ 
        _session.player_id_ = -1
        c_trace( "[login_mng](migrate_session_back_gs) session is migrated back to gs, account_id: %d, player_id: %d", _session.account_id_, player_id )
    else
        c_err( "[login_mng](migrate_session_back_gs) session has not migrated to bf yet, account_id: %d", _session.account_id_ )
    end
end

local function join_game_server( _dpid, _account_id )
    dbclient.send_certify_to_db( _dpid, _account_id )
    g_online_counter:increase_by_one( _account_id )
end

local function join_login_queue( _dpid, _account_id )
    local account_info = { key_ = _account_id, dpid_ = _dpid }
    local rank = g_login_queue:enqueue( account_info )
    if rank then
        gamesvr.send_login_queue_enqueue( _dpid, rank )
    else
        c_err( "[login_mng](join_login_queue) dpid: 0x%X account_id: %d enqueue failed", _dpid, _account_id )
    end
end

local function advise_change_server( _dpid, _account_id )
    gamesvr.send_login_queue_is_full( _dpid )
    g_connection_map[ _dpid ] = nil
end

local function refresh_login_queue_rank( _account_info, _rank )
    gamesvr.send_login_queue_rank( _account_info.dpid_, _rank )
end

local function decrease_join_player_count( _account_id )
    g_online_counter:decrease_by_one( _account_id )
    if g_online_counter:count() < g_max_user_num and not g_login_queue:is_empty() then
        local account_info = g_login_queue:dequeue()
        if account_info then
            join_game_server( account_info.dpid_, account_info.key_ )
        end
    end
end

local function close_session( _session )
    local account_id = _session.account_id_
    local dpid  = _session.dpid_

    local close_timer = _session.close_timer_ 
    if close_timer > 0 then
        g_timermng:c_del_timer( close_timer )
    end

    if g_login_queue:is_in_queue( account_id ) then          -- in login queue
        g_login_queue:remove( account_id )      
    elseif _session.migrate_ then                            -- in bf
        local player_id = _session.player_id_
        channel_mng.on_migrate_player_offline( player_id )
        decrease_join_player_count( account_id )
    else                                                     -- in gs
        gamesvr.remove_joining_player_by_dpid( dpid )
        decrease_join_player_count( account_id )
    end
    
    gamesvr.clear_dpid_info( dpid )
    dbclient.send_session_closed_to_db( account_id )
    player_mng.del_player_by_dpid( dpid )
    
    g_session_map[ account_id ] = nil
    c_login( "[login_mng](close_session) account_id: %d dpid: 0x%X", account_id, dpid )
end

local function handle_login_other_place( _session, _dpid, _login_token )
    local old_dpid = _session.dpid_ 
    if g_login_queue:is_in_queue( old_dpid ) then            -- in login queue
        g_login_queue:remove( old_dpid )        
        gamesvr.send_login_other_place_to_client( old_dpid )
        return true
    elseif _session.migrate_ then                            -- in bf
        local player_id = _session.player_id_
        local bf_player = bfclient.g_bf_players[ player_id ]
        if bf_player then
            bfclient.notify_bf_player_login_other_place( bf_player )
            g_migrated_waiting_rejoin_map[ _session.account_id_ ] = { dpid_ = _dpid, login_token_ = _login_token } 
            return false
        else
            c_err( "[login_mng](handle_login_other_place) can not find bf_player in g_bf_players map dpid: 0x%X, account_id: %d, player_id: %d ", _dpid, _session.account_id_, player_id )
            return true
        end 
    else                                                     -- in gs
        -- kick the old user
        player_mng.del_player_by_dpid( old_dpid )
        if gamesvr.g_dpid_mng[ old_dpid ] then
            gamesvr.g_dpid_mng[ old_dpid ].account_id_ = nil 
        end
        g_connection_map[ old_dpid ] = nil

        -- TODO: Add the old_dpid to a pending delete list to ignore all the messages received from it

        g_old_connection_timer_map[ old_dpid ] = g_timermng:c_add_timer_second( 5, timers.CLOSE_OLD_CONNECTION, old_dpid, 0, 0 )
        gamesvr.send_login_other_place_to_client( old_dpid )
        return true
    end
end

local function reconnect_connection_session( _new_dpid, _session )
    local account_id = _session.account_id_
    local old_dpid = _session.dpid_

    if g_login_queue:is_in_queue( account_id ) then
        local elem = g_login_queue:get( account_id )
        if elem and elem.dpid_ == old_dpid then
            elem.dpid_ = _new_dpid 
        end
    elseif _session.migrate_ then
        local player_id = _session.player_id_
        local bf_player = bfclient.g_bf_players[ player_id ]
        if bf_player then
            migrate_session_back_gs( _session )
            gamesvr.do_rejoin( _new_dpid, player_id, bf_player )
            -- notify client to close waiting UI
            gamesvr.send_reconnect_from_bf( _new_dpid )
        else
            gamesvr.send_session_alive_ret( _new_dpid, false )
            c_log( "[login_mng](reconnect_connection_session) can not find bf_player by player_id: %d", player_id )
            return
        end
    else
        local player = player_mng.get_player_by_dpid( old_dpid )
        if player then
            player_mng.on_player_reconnect( player, _new_dpid, old_dpid ) 
            logclient.log_login(player)
        end

        local dpid_info = gamesvr.g_dpid_mng[ old_dpid ] 
        if dpid_info then
            gamesvr.g_dpid_mng[ _new_dpid ] = dpid_info
            gamesvr.g_dpid_mng[ old_dpid ] = nil 
            local account_info = dpid_info.account_info_
            if account_info then
                gamesvr.g_account_name_2_dpid[account_info.account_name_] = _new_dpid
            end
        end

        local timer = g_old_connection_timer_map[ old_dpid ] 
        if timer then
            g_old_connection_timer_map[ _new_dpid ] = timer
            g_old_connection_timer_map[ old_dpid ] = nil
        end
    end
    
    awake_session( _session, _new_dpid )
    g_connection_map[ _new_dpid ] = account_id
    c_login( "[login_mng](reconnect_connection_session) session account_id: %d, new_dpid: 0x%X, old_dpid: 0x%X", account_id, _new_dpid, old_dpid )
end

-- ------------------------------------
-- Timer Callback Functions
-- ------------------------------------

function close_old_connection_timer_callback( _dpid )
    g_old_connection_timer_map[ _dpid ] = nil
    g_gamesvr:c_kick_player( _dpid )
end

function refresh_queue_rank_timer_callback()
    if not g_login_queue:is_empty() then
        g_login_queue:map( refresh_login_queue_rank )  
    end
end

function close_session_timer_callback( _account_id )
    local session = g_session_map[ _account_id ]
    if session then
        close_session( session )
        c_login( "[login_mng](close_session_timer_callback) account_id: %d session is closed by system", _account_id )
    end
end

function print_online_count_timer_callback()
    order_mng.query_recharge_total()
   
    local gs_count, all_count = get_session_awake_player_count()
    c_login( "[login_mng](print_online_count_timer_callback) join player num: %d, gs player num: %d, all player num: %d, login queue count: %d", g_online_counter:count(), gs_count, all_count, g_login_queue:count() ) 

    local server_id = g_gamesvr:c_get_server_id()
    bfclient.remote_call_center( "bfsvr.on_game_server_report_online_count", server_id, g_online_counter:count() )
end

-- ------------------------------------
-- Interface Functions
-- ------------------------------------

function init()
    if g_refresh_queue_rank_timer >= 0 then
        g_timermng:c_del_timer( g_refresh_queue_rank_timer )
    end
    g_refresh_queue_rank_timer = g_timermng:c_add_timer_second( REFRESH_QUEUE_RANK_TIME_S, timers.LOGIN_QUEUE_RANK_TIMER, 0, 0, REFRESH_QUEUE_RANK_TIME_S )  

    if g_print_online_count_timer < 0 then
        g_print_online_count_timer = g_timermng:c_add_timer_second( PRINT_ONLINE_COUNT_TIME_S, timers.PRINT_ONLINE_COUNT_TIMER, 0, 0, PRINT_ONLINE_COUNT_TIME_S )
    end
end

function set_max_user_num( _max_user_num )
    local increment = _max_user_num - g_max_user_num
    g_max_user_num = _max_user_num
    if increment > 0 then
       while g_online_counter:count() < g_max_user_num and not g_login_queue:is_empty() do
           local account_info = g_login_queue:dequeue()
           if account_info then
               join_game_server( account_info.dpid_, account_info.key_ )
           end
       end
    end
    c_login( "[login_mng](set_max_user_num) g_max_user_num is set to %d", g_max_user_num )
    return true
end

function set_login_queue_size( _size )
    if g_login_queue then
        g_login_queue.size_ = _size
        c_login( "[login_mng](set_login_queue_size) g_login_queue is set to %d", _size )
        return true
    end
    return false
end

function create_connection( _dpid )
    if not g_connection_map[ _dpid ] then
        g_connection_map[ _dpid ] = EMPTY_ACCOUNT_ID
        c_login( "[login_mng](create_connection) create a connection dpid: 0x%X", _dpid )
    else 
        c_err( "[login_mng](create_connection) try to create a connection with a reduplicative dpid: 0x%X", _dpid ) 
    end
end

function create_session( _dpid, _account_id, _certify_key )
    local old_session = g_session_map[ _account_id ]
    if old_session then
        close_session( old_session ) 
        c_login( "[login_mng](create_session)close old session" )
    end
    
    local session = {}
    session.dpid_ = _dpid
    session.account_id_ = _account_id
    session.certify_key_ = _certify_key or generate_certify_key()
    session.suspend_ = false 
    session.migrate_ = false
    session.player_id_ = -1
    session.close_timer_ = -1

    g_connection_map[ _dpid ] = _account_id 
    g_session_map[ _account_id ] = session
    
    c_login( "[login_mng](create_session) account_id: %d, dpid: 0x%X", _account_id, _dpid )

    return session
end

function certify( _dpid, _account_id, _login_token )
    -- check if the connection is connected
    local account_id = g_connection_map[ _dpid ] 
    if account_id == EMPTY_ACCOUNT_ID then
        local verify_right_now = true
        local session = g_session_map[ _account_id ]
        if session then
            if not session.suspend_ then 
                verify_right_now = handle_login_other_place( session, _dpid, _login_token )   
                gamesvr.send_already_login_to_client( _dpid )
            end
            close_session( session )
        end
        if verify_right_now then
            loginclient.verify_account( _account_id, _login_token, _dpid )
        end
        g_connection_map[ _dpid ] = _account_id 
    else
        c_err( "[login_mng](certify) dpid: 0x%X, cache account_id: %d, send account_id: %d certify account info repeatly", _dpid, account_id, _account_id )
    end
end

function login( _dpid, _account_id, _queue_free )
    c_login( "[login_mng](login) dpid: 0x%X account_id: %d, _queue_free: %d login after certify", _dpid, _account_id, _queue_free )
    
    if _queue_free > 0 or g_online_counter:count() < g_max_user_num then
        local session = create_session( _dpid, _account_id )
        gamesvr.send_certify_code_to_client( _dpid, login_code.LOGIN_CERTIFY_OK, session.certify_key_ )
        join_game_server( _dpid, _account_id )
    elseif not g_login_queue:is_full() then
        local session = create_session( _dpid, _account_id )
        gamesvr.send_certify_code_to_client( _dpid, login_code.LOGIN_CERTIFY_OK, session.certify_key_ )
        join_login_queue( _dpid, _account_id )
    else
        advise_change_server( _dpid, _account_id )
    end
end

function account_quit_login_queue( _dpid )
    local account_id = g_connection_map[ _dpid ] 
    if account_id and account_id ~= EMPTY_ACCOUNT_ID then
        local account_info = g_login_queue:remove( account_id )
        if account_info then
            gamesvr.send_login_queue_quit_ret( _dpid, true )
            return
        end
    end
    gamesvr.send_login_queue_quit_ret( _dpid, false )
    c_err( "[login_mng](account_quit_login_queue) dpid: 0x%X quit login queue failed", _dpid )
end

function request_session_alive( _dpid, _account_id, _certify_key )
    local session = g_session_map[ _account_id ]
    if session then
        local old_dpid = session.dpid_
        local account_id = g_connection_map[ old_dpid ]
        if account_id then
            if account_id == _account_id then
                close_connection( old_dpid )
            end
            --[[
            gamesvr.send_session_alive_ret( _dpid, false )
            c_login( "[login_mng](request_session_alive) the old_dpid connection is alive, account_id: %d", _account_id )
            return
            --]]
        end
        
        if session.certify_key_ == _certify_key then
            gamesvr.send_session_alive_ret( _dpid, true )
            reconnect_connection_session( _dpid, session ) 
        else
            gamesvr.send_session_alive_ret( _dpid, false )
            c_login( "[login_mng](request_session_alive) request a alive session with a unmatched certify_key, account_id: %d", _account_id )
        end
    else
        gamesvr.send_session_alive_ret( _dpid, false )
    end
    c_login( "[login_mng](request_session_alive) dpid: 0x%X, account_id: %d, session alive: %s", _dpid, _account_id, session and "true" or "false " )
end

function close_connection( _dpid )
    local timer = g_old_connection_timer_map[ _dpid ]
    if timer then
        g_timermng:c_del_timer( timer )
        g_old_connection_timer_map[ _dpid ] = nil
    end
  
    local player = player_mng.get_player_by_dpid( _dpid )
    if player then
        player_mng.remove_player( player, true )
    end

    gamesvr.remove_joining_player_by_dpid( _dpid )

    local account_id = g_connection_map[ _dpid ]
    if account_id then
        if account_id ~= EMPTY_ACCOUNT_ID then
            local session = g_session_map[ account_id ]
            if session then
                if session.dpid_ == _dpid then
                    suspend_session( session )
                else
                    c_err( "[login_mng](close_connection) dpid: 0x%X in session and connection dpid: 0x%X aren't matched", session.dpid_, _dpid )
                end
            end
        end
        g_connection_map[ _dpid ] = nil
        
        if g_migrated_waiting_rejoin_map[ account_id ] then
            g_migrated_waiting_rejoin_map[ account_id ] = nil
        end
        c_login( "[login_mng](close_connection) account_id: %d, dpid: 0x%X", account_id, _dpid )
    end
end

function return_to_select_player( _dpid, _account_id, _certify_key, _player_id )
    local session = g_session_map[ _account_id ]
    if not session then
        c_err( "[login_mng](return_to_select_player) can not find session by account_id: %d", _account_id )
        return 
    end

    if session.dpid_ ~= _dpid then
        c_err( "[login_mng](return_to_select_player) dpid: %d in session and connect dpid: %d aren't matched, account_id: %d", session.dpid_, _dpid,  _account_id )
        return
    end

    if session.certify_key_ ~= _certify_key then
        c_err( "[login_mng](return_to_select_player) certify_key in session and _certify_key aren't matched, account_id: %d", _account_id )
        return
    end

    local player = player_mng.get_player_by_dpid( _dpid )
    if not player then
        c_err( "[login_mng](return_to_select_player) can not find player by dpid, account_id: %d", _account_id )
        return
    end

    if player.player_id_ ~= _player_id then
        c_err( "[login_mng](return_to_select_player) player_id: %d and _player_id: %d aren't matched, account_id: %d", player.player_id_, _player_id, _account_id )
        return
    end

    -- remove the old player
    player_mng.del_player_by_dpid( _dpid )

    player:add_reselect_player_ret()

    dbclient.send_get_playerlist_to_db( _dpid, _account_id ) 
    c_login( "[login_mng](return_to_select_player) player_id: %d, dpid: 0x%X", _player_id, _dpid )
end

function close_session_by_client( _dpid, _account_id )
    local account_id = g_connection_map[ _dpid ] 
    if account_id then
        if account_id == _account_id then
            local session = g_session_map[ account_id ]   
            if session then
                close_session( session )
                c_login( "[login_mng](close_session_by_client) account_id: %d connect session is closed by client", _account_id )
            end
        elseif account_id == EMPTY_ACCOUNT_ID then
            c_err( "[login_mng](close_session_by_client) dpid: 0x%X try to close a nonexistent session", _dpid )        
        else
            c_err( "[login_mng](close_session_by_client) dpid: 0x%X try to close a unmatched account_id session", _dpid ) 
        end
    else
        c_err( "[login_mng](close_session_by_client) dpid: 0x%X a nonexistent connect try to close session", _dpid )
    end

    gamesvr.send_close_session_ret( _dpid )
end

function close_session_before_kick_player( _account_id, _player_id, _is_migrated )
    local session = g_session_map[ _account_id ]
    if session then
        if session.migrate_ == _is_migrated then
            close_session( session )
            c_login( "[login_mng](close_session_before_kick_player) player_id: %d, account_id: %d migrate: %s session is closed by server before kick player", _player_id, _account_id, _is_migrated and "true" or "false" )
        else
            c_err( "[login_mng](close_session_before_kick_player) player_id: %d, account_id: %d migrate state don't matched, migrate_: %s, _is_migrate: %s", _player_id, _account_id, session.migrate_ and "true" or "false", _is_migrated and "true" or "false" )
        end
    else
        c_err( "[login_mng](close_session_before_kick_player) player_id: %d, account_id: %d can not find session", _player_id, _account_id )
    end
end

function before_player_migrate_bf( _account_id, _player_id )
    local session = g_session_map[ _account_id ]
    if session then
        migrate_session_to_bf( session, _player_id )
    else
        c_err( "[login_mng](before_player_migrate_bf) can not find session by account_id: %d, player_id: %d", _account_id, _player_id )
    end 
end

function before_player_migrate_game( _account_id )
    local session = g_session_map[ _account_id ] 
    if session then
        migrate_session_back_gs( session )
    else
        c_err( "[login_mng](before_player_migrate_game) can not find session by account_id: %d", _account_id )
    end
end

function migrated_player_certify( _account_id, _tips )
    local certify_info = g_migrated_waiting_rejoin_map[ _account_id ]
    if certify_info then
        local login_token = certify_info.login_token_
        local dpid = certify_info.dpid_
        loginclient.verify_account( _account_id, login_token, dpid )  

        g_migrated_waiting_rejoin_map[ _account_id ] = nil

        if _tips then
            gamesvr.send_already_login_to_client( dpid )
        end
        c_trace( "[login_mng](migrated_player_certify) id: %d, dpid: 0x%X", _account_id, dpid )
    end
end

function migrated_player_disconnect( _account_id )
    local session = g_session_map[ _account_id ]
    if session then
        if session.migrate_ then
            suspend_session( session )
        end
    end
end

function on_migrated_player_rejoin( _dpid, _account_id, _certify_key )
    create_session( _dpid, _account_id, _certify_key )
end

function on_bf_disconnect( _player_id_map )
    for account_id, session in pairs( g_session_map ) do
        if session.migrate_ and _player_id_map[ session.player_id_ ] then
            close_session( session )
        end
    end
end

function close_all_session()
    for account_id, session in pairs( g_session_map ) do
       close_session( session )  
    end
    c_login( "[login_mng](close_all_session)" )
end

function get_account_id_by_dpid( _dpid )
    return g_connection_map[ _dpid ]
end

function is_player_session_suspend( _player )
    local account_id = _player.account_id_
    local session = g_session_map[ account_id ]
    if session then
        return session.suspend_
    end
end

function get_session_awake_player_count() 
    local player_cnt = 0
    local suspend_cnt = 0
    for _, player_map in pairs( player_mng.g_player_mng ) do
        for id, player in pairs( player_map ) do
            player_cnt = player_cnt + 1
            if is_player_session_suspend( player ) then
                suspend_cnt = suspend_cnt + 1
            end
        end
    end

    local bf_cnt = 0
    for id in pairs( bfclient.g_bf_players ) do
        bf_cnt = bf_cnt + 1
    end
    
    local gs_cnt = player_cnt - suspend_cnt
    return gs_cnt, gs_cnt + bf_cnt
end

function gm_print_player_count()
    return true, str_format( "[login_mng](gm_print_player_count) join player num: %d; login queue count: %d", g_online_counter:count(), g_login_queue:count())
end

-- ------------------------------------
-- Test Cases
-- ------------------------------------
local function session_tostring( _session )
    return string.format( "session account_id: %d, dpid: 0x%X, suspend: %s migrate: %s", _session.account_id_, _session.dpid_, _session.suspend_ and "true" or "false", _session.migrate_ and "true" or "false" ) 
end

function print_state()
    local count = 0
    for dpid, account_id in pairs( g_connection_map ) do
        count = count + 1
        c_login( "[login_mng](print_state) connection dpid: 0x%X, account_id: %d", dpid, account_id )
    end
    if count == 0 then
        c_login( "[login_mng](print_state) g_connection_map is empty" )
    end

    count = 0
    for account_id, session in pairs( g_session_map ) do
        count = count + 1
        c_login( "[login_mng](print_state) session account_id: %d, seesion: %s", account_id, session_tostring( session ) ) 
    end
    if count == 0 then
        c_login( "[login_mng](print_state) g_session_map is empty" )
    end
end

function print_count()
    c_login( "[login_mng](print_count) join player num: %d", g_online_counter:count() )
    c_login( "[login_mng](print_count) login queue count: %d", g_login_queue:count() )
end

