module( "db_daily_goals", package.seeall )

SELECT_DAILY_GOALS_SQL = [[
	SELECT
  `last_reset_time`,
  `process`,
  `entice`
  from DAILY_GOALS_TBL where player_id = ?; 
]]

SELECT_DAILY_GOALS_SQL_RES_TYPE = "iss"

REPLACE_DAILY_GOALS_SQL = [[
  REPLACE INTO DAILY_GOALS_TBL
  ( `player_id`, `last_reset_time`, `process`, `entice` ) VALUES
  (
        ?,
        ?,
        ?,
        ?
  )
]]

REPLACE_DAILY_GOALS_PARAM_TYPE = "iiss"

function do_get_player_daily_goals_record( _player_id )
	local database = dbserver.g_db_character
	if database:c_stmt_format_query( SELECT_DAILY_GOALS_SQL, "i", _player_id, SELECT_DAILY_GOALS_SQL_RES_TYPE ) < 0 then
    c_err( "[db_daily_goals](do_get_daily_goals_record) c_stmt_format_query on SELECT_DAILY_GOALS_SQL failed, pid: %d.", _player_id )
    return nil
	end
	
	local rows = database:c_stmt_row_num()
	if rows == 0 then
	  return { last_reset_time_= 0, process_str_ = "{}", entice_str_ = "" }
	elseif rows == 1 then
    if database:c_stmt_fetch() == 0 then
  		local last_reset_time, process, entice = database:c_stmt_get( "last_reset_time", "process", "entice" )
      return { last_reset_time_ = last_reset_time, process_str_ = process, entice_str_ = entice }
    end
	end
end

function do_save_player_daily_goals_record( _player )
  local player_id = _player.player_id_ 
  local player_daily_goals = _player.daily_goals_
  return dbserver.g_db_character:c_stmt_format_modify( REPLACE_DAILY_GOALS_SQL, 
                                                        REPLACE_DAILY_GOALS_PARAM_TYPE, 
                                                        player_id, 
                                                        player_daily_goals.last_reset_time_, 
                                                        player_daily_goals.process_str_,
                                                        player_daily_goals.entice_str_)
end
