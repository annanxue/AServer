module( "db_rune", package.seeall )

-- ------------------------------------
-- SQL STRING FORMAT
-- ------------------------------------
local SELECT_RUNE_SQL = [[
    SELECT 
    `rune_cursor`,
    `next_rune_slots`
    from PLAYER_RUNE_TBL where player_id = ?;
]]

local SELECT_RUNE_RESULT_TYPE = "is"

local MODIFY_RUNE_SQL = [[
    REPLACE INTO PLAYER_RUNE_TBL 
    ( `player_id`, `rune_cursor`, `next_rune_slots` ) VALUES
    ( ?, ?, ? );
]]

local MODIFY_RUNE_PARAM_TYPE = "iis"

-- ------------------------------------
-- Logic Functions
-- ------------------------------------

function do_get_player_rune( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_RUNE_SQL, "i", _player_id, SELECT_RUNE_RESULT_TYPE ) < 0 then
        c_err( "[db_rune](do_get_player_rune) c_stmt_format_query on SELECT_RUNE_SQL failed, player_id: %d", _player_id )
        return
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return { rune_cursor_ = 1, rune_slot_str_ = "0,0,0,0,0,0,0,0" }
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local rune_cursor, rune_slot_str = database:c_stmt_get("rune_cursor", "next_rune_slots")
            return { rune_cursor_ = rune_cursor, rune_slot_str_ = rune_slot_str }
        end
    end
end

function do_save_player_rune( _player )
    local player_id = _player.player_id_
    local rune = _player.rune_
    local rune_cursor = rune.rune_cursor_
    local rune_slot_str = rune.rune_slot_str_
    return dbserver.g_db_character:c_stmt_format_modify( MODIFY_RUNE_SQL, MODIFY_RUNE_PARAM_TYPE, player_id, rune_cursor, rune_slot_str )
end
