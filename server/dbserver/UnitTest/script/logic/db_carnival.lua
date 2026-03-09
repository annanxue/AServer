module( "db_carnival", package.seeall )

-- ------------------------------------
-- SQL STRING FORMAT
-- ------------------------------------

local SELECT_CARNIVAL_SQL = [[
    SELECT 
    `online_time`,
    `online_award`,
    `online_apply`,
    `boxes`,
    `titles`
    from PLAYER_CARNIVAL_TBL where player_id = ?;
]]

local SELECT_CARNIVAL_RESULT_TYPE = "iiiss"

local MODIFY_CARNIVAL_SQL = [[
    REPLACE INTO PLAYER_CARNIVAL_TBL 
    ( `player_id`, `online_time`, `online_award`, `online_apply`, `boxes`, `titles` ) VALUES 
    ( ?, ?, ?, ?, ?, ? );
]]

local MODIFY_CARNIVAL_PARAM_TYPE = "iiiiss"

-- ------------------------------------
-- Logic Functions
-- ------------------------------------

function do_get_player_carnival( _player_id )
    local database = dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_CARNIVAL_SQL, "i", _player_id, SELECT_CARNIVAL_RESULT_TYPE ) < 0 then
        c_err( "[db_rune](do_get_player_carnival) c_stmt_format_query on SELECT_RUNE_SQL failed, player_id: %d", _player_id )
        return 
    end
    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return { online_ = 0, award_ = 0, apply_ = 0, boxes_ = "0,0,0,0,0", titles_ = "" }
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local online, award, apply, boxes, titles = database:c_stmt_get( "online_time", "online_award", "online_apply", "boxes", "titles" )
            return { online_ = online, award_ = award, apply_ = apply, boxes_ = boxes, titles_ = titles }
        end
    end
end

function do_save_player_carnival( _player )
    local player_id = _player.player_id_
    local carnival = _player.carnival_
    local online = carnival.online_
    local award = carnival.award_
    local apply = carnival.apply_
    local boxes = carnival.boxes_
    local titles = carnival.titles_
    return dbserver.g_db_character:c_stmt_format_modify( MODIFY_CARNIVAL_SQL, MODIFY_CARNIVAL_PARAM_TYPE, player_id, online, award, apply, boxes, titles )
end

