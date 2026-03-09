module( "db_avatar", package.seeall )

-- ------------------------------------
-- SQL STRING FORMAT
-- ------------------------------------
local SELECT_AVATAR_SQL = [[
    SELECT
    `use_model`,
    `light_avatar`,
    `avatar_str`,
    `destroy_str`,
    `artifact_str`,
    `astrolabe_level`,
    `astrolabe_avatar`,
    `artifact_fusion_str`
    from PLAYER_AVATAR_TBL where player_id = ?;
]]

local SELECT_AVATAR_RESULT_TYPE = "isssssss"

local MODIFY_AVATAR_SQL = [[
    REPLACE INTO PLAYER_AVATAR_TBL 
    ( `player_id`, `use_model`, `light_avatar`, `avatar_str`, `destroy_str`, `artifact_str`, `astrolabe_level`, `astrolabe_avatar`, `artifact_fusion_str` ) VALUES
    ( ?, ?, ?, ?, ?, ?, ?, ?, ? );
]]

local MODIFY_AVATAR_PARAM_TYPE = "iisssssss"

local DEFAULT_LIGHT_STR = "1,0,0"
local DEFAULT_AVTAR_STR = "" 
local DEFAULT_DESTROY_STR = "3,3,3,3,3,3,3,3,3"
local DEFAULT_ARTIFACT_STR = "0,0,0,0,0,0,0,0,0"
local DEFAULT_ASTROLABE_LEVEL = "0,0,0,0,0,0,0,0,0"
local DEFAULT_ASTROLABE_AVATAR = "0,0,0,0,0,0,0,0,0"
local DEFAULT_ARTIFACT_FUSION_STR = '0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0'

-- ------------------------------------
-- Logic Functions
-- ------------------------------------

function do_get_player_avatar( _player_id )
    local database =dbserver.g_db_character
    if database:c_stmt_format_query( SELECT_AVATAR_SQL, "i", _player_id, SELECT_AVATAR_RESULT_TYPE ) < 0 then
        c_err( "[db_avatar](do_get_player_avatar) c_stmt_format_query on SELECT_AVATAR_SQL failed, player_id: %d", _player_id )
        return
    end

    local rows = database:c_stmt_row_num()
    if rows == 0 then
        return { main_fight_ = 1, light_str_ = DEFAULT_LIGHT_STR, avatar_str_ = DEFAULT_AVTAR_STR, show_wings_ = 1, destroy_str_ = DEFAULT_DESTROY_STR, artifact_str_ = DEFAULT_ARTIFACT_STR, astrolabe_level_ = DEFAULT_ASTROLABE_LEVEL, astrolabe_avatar_ = DEFAULT_ASTROLABE_AVATAR, artifact_fusion_str_ = DEFAULT_ARTIFACT_FUSION_STR }
    elseif rows == 1 then
        if database:c_stmt_fetch() == 0 then
            local use_model, light_str, avatar_str, destroy_str, artifact_str, astrolabe_level, astrolabe_avatar, artifact_fusion_str = database:c_stmt_get( "use_model", "light_avatar", "avatar_str", "destroy_str", "artifact_str", "astrolabe_level", "astrolabe_avatar", "artifact_fusion_str" )
            return { main_fight_ = use_model, light_str_ = light_str, avatar_str_ = avatar_str, destroy_str_ = destroy_str, artifact_str_ = artifact_str, astrolabe_level_ = astrolabe_level, astrolabe_avatar_ = astrolabe_avatar, artifact_fusion_str_ = artifact_fusion_str }
        end
    end
end

function do_save_player_avatar( _player )
    local player_id = _player.player_id_
    local avatar = _player.avatar_
    local use_model = avatar.main_fight_
    local light_str = avatar.light_str_
    local avatar_str = avatar.avatar_str_
    local destroy_str = avatar.destroy_str_
    local artifact_str = avatar.artifact_str_
    local astrolabe_level = avatar.astrolabe_level_
    local astrolabe_avatar = avatar.astrolabe_avatar_
    local artifact_fusion_str = avatar.artifact_fusion_str_

    return dbserver.g_db_character:c_stmt_format_modify( MODIFY_AVATAR_SQL, MODIFY_AVATAR_PARAM_TYPE, player_id, use_model, light_str, avatar_str, destroy_str, artifact_str, astrolabe_level, astrolabe_avatar, artifact_fusion_str )
end
