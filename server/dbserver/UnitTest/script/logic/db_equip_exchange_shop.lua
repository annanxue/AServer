module( "db_equip_exchange_shop", package.seeall )

SELECT_EQUIP_EXCHANGE_SHOP_RECORD_SQL = [[
    SELECT
    `saint_equip_record`,
    `excellent_equip_record`
    from EQUIP_EXCHANGE_SHOP_RECORD_TBL where player_id = ?;

]]

SELECT_EQUIP_EXCHANGE_SHOP_RECORD_SQL_RES_TYPE = "ss"

REPLACE_EQUIP_EXCHANGE_SHOP_RECORD_SQL = [[
    REPLACE INTO EQUIP_EXCHANGE_SHOP_RECORD_TBL
    ( `player_id`, `saint_equip_record`, `excellent_equip_record` ) VALUES
    (
       ?,
       ?,
       ?
    )
]]

REPLACE_EQUIP_EXCHANGE_SHOP_RECORD_PARAM_TYPE = "iss"

local NEW_RECORD_STR = "{{},{},0,1}"

function do_get_player_equip_exchange_shop_record( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_EQUIP_EXCHANGE_SHOP_RECORD_SQL, "i", _player_id, SELECT_EQUIP_EXCHANGE_SHOP_RECORD_SQL_RES_TYPE ) < 0 then
        c_err( "[db_equip_exchange_shop](do_get_player_equip_exchange_shop_record) c_stmt_format_query on SELECT_EQUIP_EXCHANGE_SHOP_RECORD_SQL failed, player_id: %d", _player_id )
        return nil
    end

    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return { NEW_RECORD_STR, NEW_RECORD_STR }
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local saint_equip_rec_str = database:c_stmt_get( "saint_equip_record" )
            local excellent_equip_rec_str = database:c_stmt_get( "excellent_equip_record" )  
            local record_str_tbl = { saint_equip_rec_str, excellent_equip_rec_str } 
            for i, str in ipairs( record_str_tbl ) do 
                local record = loadstring( "return" .. str )()  
                if not record or type( record ) ~= "table" then
                    record_str_tbl[i] = NEW_RECORD_STR
                end
            end
            return record_str_tbl
        end
    end
end

function do_save_player_equip_exchange_shop_record( _player )
    local player_id = _player.player_id_ 
    local record_str_tbl = _player.equip_exchange_shop_rec_str_tbl_
    return dbserver.g_db_character:c_stmt_format_modify( REPLACE_EQUIP_EXCHANGE_SHOP_RECORD_SQL, 
           REPLACE_EQUIP_EXCHANGE_SHOP_RECORD_PARAM_TYPE, player_id, record_str_tbl[1], record_str_tbl[2] )
end

