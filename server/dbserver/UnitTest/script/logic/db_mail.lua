module( "db_mail", package.seeall )

local mmax, ssub, slen, sformat, tinsert = math.max, string.sub, string.len, string.format, table.insert

SELECT_MAIL_SQL = [[
    SELECT
    `gm_global_mail_id_read`,
    `mails`
    from MAIL_BOX_TBL where player_id = ?;
]]
SELECT_MAIL_SQL_RESULT_TYPE = "is"

REPLACE_MAIL_SQL = [[
    REPLACE INTO MAIL_BOX_TBL 
    ( `player_id`, `gm_global_mail_id_read`, `mails` ) VALUES
    (
        ?,
        ?,
        ?
    )
]]
REPLACE_MAIL_SQL_PARAM_TYPE = "iis"

UPDATE_OFFLINE_MAIL_SQL = [[
    UPDATE MAIL_BOX_TBL SET mails = ? WHERE player_id = ?;
]]
UPDATE_OFFLINE_MAIL_SQL_PARAM_TYPE = "si"

SELECT_MAX_GM_GLOBAL_MAIL_SQL = [[
    SELECT max(`gm_global_mail_id`) AS max_gm_global_mail_id FROM GM_GLOBAL_MAIL_TBL;
]]
SELECT_MAX_GM_GLOBAL_MAIL_SQL_PARAM_TYPE = ""
SELECT_MAX_GM_GLOBAL_MAIL_SQL_RESULT_TYPE = "i"

SELECT_UNREAD_GM_GLOBAL_MAIL_SQL = [[
    SELECT
        `gm_global_mail_id`,
        unix_timestamp(create_time) as create_time_int,
        `mail_id`,
        `item_id_list`,
        `item_num_list`,
        `tip_list`
    FROM GM_GLOBAL_MAIL_TBL WHERE `gm_global_mail_id` > ?
    ORDER BY `gm_global_mail_id`;
]]
SELECT_UNREAD_GM_GLOBAL_MAIL_SQL_PARAM_TYPE = "i"
SELECT_UNREAD_GM_GLOBAL_MAIL_SQL_RESULT_TYPE = "iiisss"

INSERT_GM_GLOBAL_MAIL_SQL = [[
    INSERT INTO GM_GLOBAL_MAIL_TBL
    (
        `gm_global_mail_id`,
        `mail_id`,
        `item_id_list`,
        `item_num_list`,
        `tip_list`
    )
    VALUES 
    ( 
        ?,
        ?,
        ?,
        ?,
        ?
    );
]]
INSERT_GM_GLOBAL_MAIL_SQL_PARAM_TYPE = "iisss"

local function get_player_mails( _player_id )
    local player = db_login.g_player_cache:get( _player_id ) 
    if player then
        return player.mail_box_ or init_mail_box()
    else
        return do_get_player_mails( _player_id )
    end
end

local function get_max_gm_global_mail_id()
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_MAX_GM_GLOBAL_MAIL_SQL
                                   , SELECT_MAX_GM_GLOBAL_MAIL_SQL_PARAM_TYPE
                                   , SELECT_MAX_GM_GLOBAL_MAIL_SQL_RESULT_TYPE
                                   ) < 0 then
        c_err( "[db_mail](get_max_gm_global_mail_id) SELECT_MAX_GM_GLOBAL_MAIL_SQL query err" )
        return 0
    end

    local rows = database:c_stmt_row_num()
    if rows <= 0 then
        return 0
    end

    if database:c_stmt_fetch() < 0 then
        c_err( "[db_mail](get_max_gm_global_mail_id) SELECT_MAX_GM_GLOBAL_MAIL_SQL fetch err, rows: %d", rows )
        return 0
    end

    return database:c_stmt_get( "max_gm_global_mail_id" )
end

function on_game_query_max_gm_global_mail_id()
    dbserver.remote_call_game( "dbclient.set_max_gm_global_mail_id", get_max_gm_global_mail_id() )
end

local function do_save_offline_player_mail( _player_id, _mails_str )
    -- refresh player cache of db
    local player = db_login.g_player_cache:get( _player_id ) 
    if player and player.mail_box_ then
        local mail_box = player.mail_box_
        local gm_global_mail_id_read = mail_box.gm_global_mail_id_read_ or get_max_gm_global_mail_id()
        player.mail_box_ = { gm_global_mail_id_read_ = gm_global_mail_id_read, mails_str_ = _mails_str }
        c_log("[db_mail](do_save_offline_player_mail)player_id: %d, gm_global_mail_id_read:%d", 
            _player_id, gm_global_mail_id_read)
    end

    -- update db data
    local ret_code = dbserver.g_db_character:c_stmt_format_modify( UPDATE_OFFLINE_MAIL_SQL
                                                                 , UPDATE_OFFLINE_MAIL_SQL_PARAM_TYPE
                                                                 , _mails_str
                                                                 , _player_id
                                                                 )
    if ret_code < 0 then
        c_err( "[db_mail](do_save_offline_player_mail)update player mail error! player_id:%d, db_ret_code:%d", _player_id, ret_code )
    else
        c_log( "[db_mail](do_save_offline_player_mail) save succ, player_id: %d", _player_id )
    end
end

local function try_merge_mail_str( _player_id, _origin_str, _new_str )
    local origin_mails_tbl = loadstring( "return" .. _origin_str )()
    local new_mails_tbl = loadstring( "return" .. _new_str )()

    local total_num = #origin_mails_tbl + #new_mails_tbl
    if total_num > mail_box_t.MAIL_NUM_MAX then
        return nil  
    end

    if next( origin_mails_tbl ) == nil then                     
        c_trace( "[db_mail](try_merge_mail_str) player_id: %d, origin_str: %s, new_str: %s", _player_id, _origin_str, _new_str ) 
        return _new_str
    end

    local origin_sub_len = slen( _origin_str ) - 1
    local origin_str = ssub( _origin_str, 1, origin_sub_len )   -- get " { mail1, mail2 " 
    local new_sub_len = slen( _new_str ) - 1 
    local new_str = ssub( _new_str, -new_sub_len )              -- get " mail3, mail4 } "
    return sformat( "%s,%s", origin_str, new_str )
end

function do_get_player_mails( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_MAIL_SQL, "i", _player_id, SELECT_MAIL_SQL_RESULT_TYPE ) < 0 then
        c_err( "[db_mail] c_stmt_format_query on SELECT_MAIL_SQL failed, player id: %d", _player_id )
        return nil
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return init_mail_box()
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local gm_global_mail_id_read = database:c_stmt_get( "gm_global_mail_id_read" )
            local mails_str = database:c_stmt_get( "mails" )
            local mail_box =
            {
                gm_global_mail_id_read_ = gm_global_mail_id_read,
                mails_str_ = mails_str,
            }
            return mail_box
        end
    end
    return nil
end

function init_mail_box()
    local mail_box =
    {
        gm_global_mail_id_read_ = get_max_gm_global_mail_id(),
        mails_str_ = "{}",
    }
    return mail_box
end

function do_save_player_mail( _player )
    local mail_box = _player.mail_box_
    return dbserver.g_db_character:c_stmt_format_modify( REPLACE_MAIL_SQL
                                                       , REPLACE_MAIL_SQL_PARAM_TYPE
                                                       , _player.player_id_
                                                       , mail_box.gm_global_mail_id_read_
                                                       , mail_box.mails_str_
                                                       )
end

function save_player_offline_mails( _player_id, _need_merge, _mails_str )
    -- check the validity of player
    if not db_login.is_valid_player( _player_id ) then
        c_log( "[db_mail](save_player_offline_mails)player is not valid, pid:%d deny to save mails", _player_id )
        return 
    end

    -- merge by game before, direct save
    if not _need_merge then
        do_save_offline_player_mail( _player_id, _mails_str )
        c_log( "[db_mail](save_player_offline_mails) no need merge, direct save, player_id: %d", _player_id )
        return
    end

    -- merge origin mails and new mails
    local player_mails = get_player_mails( _player_id )
    if not player_mails or not player_mails.mails_str_ then
        c_err( "[db_mail](save_player_offline_mails)get player mails string fail, pid:%d", _player_id )
        return
    end

    local merge_mails_str = try_merge_mail_str( _player_id, player_mails.mails_str_, _mails_str )
    if merge_mails_str then
        do_save_offline_player_mail( _player_id, merge_mails_str )
    else
        c_log( "[db_mail](save_player_offline_mails)send merge mail quest to game, pid:%d", _player_id )
        dbserver.send_merge_offline_mail_quest_to_game( _player_id, player_mails.mails_str_, _mails_str )
    end
end

function do_get_unread_gm_global_mails( _gm_global_mail_id_read )
    local max_gm_global_mail_id = get_max_gm_global_mail_id()
    local gm_global_mail_id_start = mmax( _gm_global_mail_id_read, max_gm_global_mail_id - mail_box_t.MAIL_NUM_MAX )

    local database = dbserver.g_db_character

    if database:c_stmt_format_query( SELECT_UNREAD_GM_GLOBAL_MAIL_SQL
                                   , SELECT_UNREAD_GM_GLOBAL_MAIL_SQL_PARAM_TYPE
                                   , gm_global_mail_id_start
                                   , SELECT_UNREAD_GM_GLOBAL_MAIL_SQL_RESULT_TYPE
                                   ) < 0 then
        c_err( "[db_mail](do_get_unread_gm_global_mails) SELECT_UNREAD_GM_GLOBAL_MAIL_SQL query err, "
             .."gm_global_mail_id_read: %d, max_gm_global_mail_id: %d, gm_global_mail_id_start: %d"
             , _gm_global_mail_id_read, max_gm_global_mail_id, gm_global_mail_id_start )
        return
    end

    local rows = database:c_stmt_row_num()
    if rows <= 0 then
        return
    end

    local unread_gm_global_mails = {}
    for i = 1, rows do
        if database:c_stmt_fetch() < 0 then
            c_err( "[db_mail](do_get_unread_gm_global_mails) SELECT_MAX_GM_GLOBAL_MAIL_SQL fetch err, _read: %d", _gm_global_mail_id_read )
            return unread_gm_global_mails
        end
        local gm_global_mail =
        {
            gm_global_mail_id_ = database:c_stmt_get( "gm_global_mail_id" ),
            create_time_ = database:c_stmt_get( "create_time_int" ),
            mail_id_ = database:c_stmt_get( "mail_id" ),
            item_id_list_ = database:c_stmt_get( "item_id_list" ),
            item_num_list_ = database:c_stmt_get( "item_num_list" ),
            tip_list_ = database:c_stmt_get( "tip_list" ),
        }
        tinsert( unread_gm_global_mails, gm_global_mail )
    end
    return unread_gm_global_mails 
end

function save_gm_global_mail( _gm_global_mail_id, _mail_id, _item_id_list, _item_num_list, _tip_list )
    local database = dbserver.g_db_character
    if database:c_stmt_format_modify( INSERT_GM_GLOBAL_MAIL_SQL
                                    , INSERT_GM_GLOBAL_MAIL_SQL_PARAM_TYPE
                                    , _gm_global_mail_id
                                    , _mail_id
                                    , _item_id_list
                                    , _item_num_list
                                    , _tip_list
                                    ) < 0 then
        c_err( "[db_mail](save_gm_global_mail) INSERT_GM_GLOBAL_MAIL_SQL err, gm_global_mail_id: %d", _gm_global_mail_id )
        return
    end
    c_log( "[db_mail](save_gm_global_mail) gm_global_mail_id: %d", _gm_global_mail_id )
end
