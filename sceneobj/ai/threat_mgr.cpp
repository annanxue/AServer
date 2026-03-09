#include "threat_mgr.h"
#include "component_ai.h"
#include "world_mng.h"
#include "timer.h"
#include <sstream>
#include "float.h"
#include "utils.h"
#include "misc.h"

namespace SGame
{

ThreatItem::ThreatItem()
{
    base_threat_ = BASE_THREAT;  
    hurt_threat_ = recover_threat_ = skill_threat_ = 0;  
}

float ThreatItem::get_threat()
{
    return base_threat_ + hurt_threat_ + recover_threat_ + skill_threat_;
}

void ThreatItem::set( ThreatType _threat_type,  float _threat )
{
    switch( _threat_type )
    {
        case THREAT_TYPE_BASE: base_threat_ = _threat; break;
        case THREAT_TYPE_HURT: hurt_threat_ = _threat; break;
        case THREAT_TYPE_RECOVER: recover_threat_ = _threat; break;
        case THREAT_TYPE_SKILL: skill_threat_ = _threat; break;
        default: break;
    }
}


void ThreatItem::add( ThreatType _threat_type, float _add_threat )
{
    switch( _threat_type )
    {
        case THREAT_TYPE_BASE: base_threat_ += _add_threat; break;
        case THREAT_TYPE_HURT: hurt_threat_ += _add_threat; break;
        case THREAT_TYPE_RECOVER: recover_threat_ += _add_threat; break;
        case THREAT_TYPE_SKILL: skill_threat_ += _add_threat; break;
        default: break;
    }
}

void ThreatItem::reset()
{
    base_threat_ = BASE_THREAT;
    hurt_threat_ = 0;
    recover_threat_ = 0;
    skill_threat_ = 0;
}

ThreatMgr::ThreatMgr()
{
	next_change_target_time_ = 0;
	melee_ot_ = 1.1f;
	range_ot_ = 1.3f;
	change_target_interval_ = 6.f;
	ai_ = NULL;
	ai_debug_ = false;
}

void ThreatMgr::set_owner( AI* _ai )
{
	ai_ = _ai;
}

// function ThreatMgr:get_num()
// 	return #Ranks_
// end

bool ThreatMgr::is_enemy(Obj* _target)
{
	return ai_->is_enemy(_target);
}

bool ThreatMgr::in_threat(uint32_t _target_id)
{
	return threats_.find(_target_id) != threats_.end();
}

// 
// function ThreatMgr:force_first(targetPid) --a
// 	local firstThreat = threats_[Ranks_[1]]
// 	local myThreat
// 	local bigTheat = 1000000
// 
// 	if !firstThreat or firstThreat < bigTheat then
// 		myThreat = bigTheat
// 	else
// 		myThreat = firstThreat * 10
// 	end
// 	add_threat(get_obj(targetPid), myThreat, false)
// end

void ThreatMgr::on_obj_enter( Ctrl* _obj )
{
	if(ai_->is_positive() && !ai_->is_in_battle() && is_enemy(_obj))
	{
		if(ai_debug_)
		{
			TRACE(2)("%lu set threat target %lu for aoi", ai_->get_id(), _obj->get_id());
		}
		add_threat(_obj, VIEW_THREAT, false, true);
	}
	else if(ai_->is_in_battle() && is_enemy(_obj))
	{
		add_threat(_obj, VIEW_THREAT, false, true);
	}
}

void ThreatMgr::on_obj_leave(uint32_t _target_id)
{
    if (ai_->has_master())
    {
		remove_threat_with_entity(_target_id, true);
        return;
    }

	out_of_sight_.erase(_target_id);

	if(ai_->is_positive() && !ai_->is_in_battle())
	{
		remove_threat_with_entity(_target_id, true);
	}
}

void ThreatMgr::remove_invalid_target(bool _remove_dead)
{
	std::vector<uint32_t> out_target_id;
	for(uint32_t i = 0; i < ranks_.size(); ++i)
	{
		uint32_t target_id = ranks_[i];
		if(_remove_dead)
		{
			if(ai_->is_invalid_target(target_id) || ai_->is_friend(ai_->get_obj(target_id)))
			{
				out_target_id.push_back(target_id);
			}
		}
		else
		{
			if(ai_->obj_out_chase_radius(target_id))
			{
				out_target_id.push_back(target_id);
				out_of_sight_.insert(target_id);
			}
		}
	}
	remove_targets(out_target_id);
}

void ThreatMgr::clear_all_threats()
{
	remove_targets(std::vector<uint32_t>(ranks_));
}

void ThreatMgr::remove_targets(const std::vector<uint32_t>& _out_target_ids)
{
	for(uint32_t i = 0; i < _out_target_ids.size(); ++i)
	{
		remove_threat_with_entity(_out_target_ids[i]);
	}

	if(ai_->is_in_battle() && !in_threat(ai_->tgt_get_id(NULL)))
	{
		//todo findnearest 0 threat targtet
		if(ranks_.size() > 0)
		{
			if(threats_[ranks_[0]]->get_threat() < 1)
			{
				Obj* target = NULL;
				float min_dist = FLT_MAX;
				for (size_t i = 0; i < ranks_.size(); ++i)
				{
					Obj *obj = ai_->get_obj(ranks_[i]);
					float dist = ai_->dist_of_obj(obj);
					if (dist < min_dist)
					{
						min_dist = dist;
						target = obj;
					}
				}
				ai_->set_target(target);
			}
			else
			{
				ai_->set_target(ai_->get_obj(ranks_[0]));
			}
			//on_change();
		}
		else
		{
			ai_->set_target(NULL);
		}
	}
}

void ThreatMgr::remove_threat_with_entity(uint32_t _target_id, bool _not_out)
{
	std::map<uint32_t, ThreatItem*>::iterator iter = threats_.find(_target_id);
	if (iter != threats_.end())
	{
		if(ai_debug_)
		{
			TRACE(2)("%lu remove threat %lu", ai_->get_id(), _target_id);
		}
		
        SAFE_DELETE( iter->second );
		threats_.erase(iter);
	}

	std::vector<uint32_t>::iterator it = find(ranks_.begin(), ranks_.end(), _target_id);
	if (it != ranks_.end())
	{
		ranks_.erase(it);
	}
	//on_change();
}


void ThreatMgr::execute(bool _in_flag, bool _out_flag)
{
    Ctrl* master = ai_->get_master();
    if (master != NULL)
    {
	    remove_invalid_target(true);
        set_combat_npc_target(master, _in_flag);
        return;
    }

    if (ai_->is_returning_home())
    {
        return;
    }

    if (ai_->need_return_home())
    {
		out_of_sight_.clear();
        clear_all_threats();
        return;
    }

	outsight_then_insight();
	remove_invalid_target(true);

	if(_out_flag && ai_->can_leave_fight())
	{
        remove_invalid_target(false);
	}

	if(_in_flag && ai_->is_positive() && !ai_->is_in_battle())
	{
        if( !find_nearest_tgt() )
        {
            for(uint32_t i = 0; i < ranks_.size(); ++i)
            {
                Obj *target = ai_->get_obj(ranks_[i]);
                if(is_valid_obj(target) && ai_->obj_in_sight_radius(target))
                {
                    ai_->set_target(target);
                    on_change();
                    ai_->find_chase_aoi_enemy();
                    break;
                }
            }
        }
    }
}

void ThreatMgr::set_combat_npc_target(Ctrl* _master, bool _in_flag)
{
    if ((ai_->is_in_battle() && obj_out_combat_npc_reach(_master, ai_->tgt_get_id(NULL))) || (_in_flag && !ai_->is_in_battle()))
    {
        if( !find_nearest_tgt() )
        {
            uint32_t target_id;
            for(uint32_t i = 0; i < ranks_.size(); ++i)
            {
                target_id = ranks_[i];
                if (!obj_out_combat_npc_reach(_master, target_id))
                {
                    Obj *target = ai_->get_obj(target_id);
                    ai_->set_target(target);
                    return;
                }
            }
            ai_->set_target(NULL);
        }
    }
}

bool ThreatMgr::find_nearest_tgt()
{
    if (ranks_.size() == 0)
    {
        return false;
    }

    uint32_t target_id;

    float min_dis = 0; 
    Obj* new_tgt = NULL;
    Ctrl* master = ai_->get_master();
    for(uint32_t i = 0; i < ranks_.size(); ++i)
    {
        target_id = ranks_[i];
        Obj* target = ai_->get_obj(target_id);

        if (master != NULL)
        {
            if (obj_out_combat_npc_reach(master, target_id))
            {
                continue;
            }
        }
        else
        {
            if (target == NULL || !ai_->obj_in_sight_radius(target))
            {
                continue;
            }
        }

        float dis = ai_->dist_of_obj(target);
        if (new_tgt == NULL)
        {
            min_dis = dis;
            new_tgt = target;
        }
        else if (dis < min_dis)
        {
            min_dis = dis;  
            new_tgt = target;
        }
    }
    if( new_tgt ) 
    {
        ai_->set_target(new_tgt);
        return true;
    }
        return false;
}

bool ThreatMgr::obj_out_combat_npc_reach(Ctrl* _master, uint32_t _tgt_id)
{
    if (_tgt_id == 0)
    {
        return true;
    }

    Obj* tgt = g_worldmng->get_ctrl(_tgt_id);
    if (is_invalid_obj(tgt))
    {
        return true;
    }

    return ai_->obj_dist_of_master(_master, tgt) > ai_->get_out_range() + ai_->get_remote_fix_rad(); 
}

float ThreatMgr::calc_threat( int32_t _damage_type, int32_t _damage_value, int32_t _skill_id )
{
	return _damage_value;
}

void ThreatMgr::try_replace_target(Obj* _target, float _threat)
{
	uint32_t target_id = static_cast<Ctrl*>(_target)->get_id();
	uint32_t ai_id = ai_->tgt_get_id(NULL);
	if(ai_id == target_id)return;

	uint32_t time = g_timermng->get_ms();

	if(ai_debug_)
	{
		TRACE(2)("try replace %lf %lf", next_change_target_time_, time);
	}

	if(next_change_target_time_ > time) return;

	float change_target_coe_ = melee_ot_;
	if(ai_->dist_of_obj(_target) > 3)
	{
	 	change_target_coe_ = range_ot_;
	}

	std::map<uint32_t, ThreatItem*>::iterator iter = threats_.find(ai_id);
	if (iter == threats_.end() || _threat > iter->second->get_threat() * change_target_coe_)
	{
	 	if(!ai_->is_in_battle())
		{
	 		ai_->find_chase_aoi_enemy();
		}

		ai_->set_target(_target);
		next_change_target_time_ = time + change_target_interval_;
		
		if(ai_debug_)
		{
			TRACE(2)("%lu replace threat target %lu, %lu", \
				ai_->get_id(), target_id, next_change_target_time_);
		}
	}
}

void ThreatMgr::add_threat(Obj* _target, float _threat, bool _default_aoi, bool _not_ot)
{
	if (!can_threat(_target, _threat))
	{
		return;
	}

	uint32_t target_id = static_cast<Ctrl *>(_target)->get_id();

 	std::map<uint32_t, ThreatItem*>::iterator iter = threats_.find(target_id);
 	if(iter != threats_.end())
	{
		std::vector<uint32_t>::iterator old_iter = ranks_.begin();
		while (old_iter != ranks_.end() && *old_iter != target_id)
		{
			old_iter++;
		}

		if (old_iter == ranks_.end())
		{
			if(ai_debug_)
			{
				TRACE(2)("%lu !in %lu 's threat ranks", target_id, ai_->get_id());
			}
			return;
		}
		else
		{
			ranks_.erase(old_iter);
		}

 		if( _threat < 1 && threats_[target_id]->get_threat() > 0)
		{
 
		}
		else
		{
 			threats_[target_id]->add( THREAT_TYPE_BASE, _threat );
		}
	}
    else
    {
        ThreatItem *ptr = new ThreatItem();
        threats_[target_id] =  ptr;
    }
	
	if(ai_debug_)
	{
		TRACE(2)("%lu add threat %lu %d", ai_->get_id(), target_id, _threat);
	}
 
 	threats_[target_id]->set( THREAT_TYPE_BASE, _threat );

	std::vector<uint32_t>::iterator list_iter = ranks_.begin();
	while (list_iter != ranks_.end() && threats_[*list_iter]->get_threat() > _threat)
	{
		list_iter++;
	}
	
	if (list_iter == ranks_.end())
	{
		ranks_.push_back(target_id);
	}
	else
	{
		ranks_.insert(list_iter, target_id);
	}

 	if(!_not_ot && !ai_->not_ot_)
	{
 		try_replace_target(_target, _threat);
 		ai_->try_enlarge_chase(_target, _default_aoi); //enlarge chase dist
	}
	on_change();
}

bool ThreatMgr::can_threat( Obj* _target, float _threat)
{
	if(ranks_.size() >=40 ) return false;

	if(_threat == 0) return false;

	if(!ai_->valid_target(_target)) return false;
	if(ai_->is_friend(_target)) return false;
	return true;
}


// function is_user(obj) --a
// 	return obj and obj:is_player()
// end
// 
// function get_ai(obj) --b
// 	return obj and obj.ai_ 
// end
// 
// function get_master(obj) --12
// 	return get_ai(obj) and obj.ai_:has_master()
// end
// 
// function is_char(obj)--3k
// 	return obj and (obj:is_player() or obj:is_monster())
// end

void ThreatMgr::add_damage(uint32_t _attacker_id, int32_t _damage_type, int32_t _damage_value, int32_t _skill_id)
{
	Obj *attacker = ai_->get_obj(_attacker_id);
	if(is_invalid_obj(attacker)) return;

	float threat = calc_threat(_damage_type, _damage_value, _skill_id);
	add_threat(attacker, threat, false);
}

std::string ThreatMgr::dump_info() const
{
	std::ostringstream oss;

	oss << "threat:{";
	for (std::vector<uint32_t>::const_iterator list_iter = ranks_.begin();
		list_iter != ranks_.end();
		list_iter++)
	{
		std::map<uint32_t, ThreatItem*>::const_iterator map_iter = threats_.find(*list_iter);
		oss << map_iter->first << ":" << map_iter->second->get_threat() << ", ";
	}
	oss << "}";

	return oss.str();
}

void ThreatMgr::on_change()
{
	// 	if AI_THREAT_LIST and AI_.Owner_ then
	// 		local threatList = {}
	// 		for _, target_id in ipairs(Ranks_) do
	// 			local target = get_obj(target_id)
	// 			if target then
	// 				threatTbl = {}
	// 				--threatTbl.name = target:get_name()
	// 				threatTbl.pid = target.ctrl_id_
	// 				threatTbl.threat = threats_[threatTbl.pid] * 10
	// 				local battle = true --COMBATMGR:check_is_has_combat_relation(AI_->get_pid(), threatTbl.pid) or AI_->tgt_get_pid() == target.ctrl_id_ or threats_[threatTbl.pid] > VIEW_THREAT
	// 				threatTbl.battle = battle and 1 or 0
	// 				table.insert(threatList, threatTbl)
	// 			end
	// 		end
	// 		print("ThreatMgr:on_change()", AI_->get_pid())
	// 		table.print(threatList)
	// 		--for_caller.c_threat_list(AI_.Owner_:get_viewer_user_vfds(), AI_->get_pid(), threatList)
	// 	end
	// end

	return ;

	std::stringstream ss;

	for (size_t i = 0; i < ranks_.size(); ++i)
	{
		int id = ranks_[i];
		ss<<id <<" -> "<< threats_[id]->get_threat() <<"  |  ";
	}
	LOG(2)("%d  %s",ai_->get_id(),ss.str().c_str());
}

void ThreatMgr::outsight_then_insight()
{
	if(out_of_sight_.empty())
	{
		return;
	}

	std::vector<int> re;

	std::set<uint32_t>::iterator it =  out_of_sight_.begin();
	for ( ; it != out_of_sight_.end() ; ++it)
	{
		if(ai_->is_invalid_target(*it) || ai_->is_friend(ai_->get_obj(*it)))
		{
			re.push_back(*it);
			continue;
		}

		Obj *target = ai_->get_obj( *it );
		if(is_valid_obj(target) && ai_->obj_in_sight_radius(target))
		{
			add_threat(target, VIEW_THREAT, false, true);
			re.push_back(*it);
		}
	}

	for (size_t i = 0; i < re.size(); ++i)
	{
		out_of_sight_.erase(re[i]);
	}
}

bool ThreatMgr::is_fake_battle() const
{
    return ranks_.size() > 0;
}

void ThreatMgr::select_ai_target( SELECT_AI_TARGET_TYPE _select_type  )
{
    int size = ranks_.size();
    if( size < 1) return; //no enemy
    int index = 0;
    switch( _select_type ) 
    {
        case SELECT_TYPE_HIGHEST_THREAT:
            index = 0;
        case SELECT_TYPE_SECOND_THREAT :
            index = 1;
            break;
        case SELECT_TYPE_LOWEST_THREAT:
            index = size - 1;
            break;

        case SELECT_TYPE_RANDOM_THREAT: 
            index = rand_int( 0, size -1 );            
            break;
    }
    if( index < 0 || index > size ) { return; }

    Obj *target = ai_->get_obj( ranks_[index] );
    if( target ) { ai_->set_target( target ); }
}


int ThreatMgr::get_threat_list_to_lua( lua_State* _state ) 
{
    lua_newtable( _state );
	for(uint32_t i = 0; i < ranks_.size(); ++i)
    {
        lua_pushinteger( _state, ranks_[i] );
        lua_rawseti( _state, -2, i+1 );
    }
      
    return 1;
}

void ThreatMgr::set_threat( Ctrl* _target, ThreatType _threat_type, float _threat )
{
    uint32_t target_id = _target->get_id();
	std::map<uint32_t, ThreatItem*>::iterator iter = threats_.find(target_id);
    ThreatItem * threat_record = NULL;
    if( iter == threats_.end() )
    {
        //LOG(2)("[ThreatMgr](set_threat)ctrl_id:%d is not in threat",  _ctrl_id)
        threat_record = new ThreatItem();
        threats_[ target_id ] = threat_record; 
    }
    else
    {
        threat_record = iter->second;
    }
    
    threat_record->set( _threat_type, _threat );
    
	std::vector<uint32_t>::iterator old_iter = ranks_.begin();
    while (old_iter != ranks_.end() && *old_iter != target_id)
    {
        old_iter++;
    }

    if (old_iter != ranks_.end())
    {
        ranks_.erase(old_iter);
    }

    float obj_threat = threat_record->get_threat();
    std::vector<uint32_t>::iterator list_iter = ranks_.begin();
    while (list_iter != ranks_.end() && threats_[*list_iter]->get_threat() > obj_threat)
    {
        list_iter++;
    }

    if (list_iter == ranks_.end())
    {
        ranks_.push_back( target_id );
    }
    else
    {
        ranks_.insert(list_iter, target_id);
    }

    ai_->set_target(ai_->get_obj(ranks_[0]));
}

void ThreatMgr::add_target_threat( Ctrl* _target, ThreatType _threat_type, float _threat )
{
    uint32_t target_id = _target->get_id();
	std::map<uint32_t, ThreatItem*>::iterator iter = threats_.find(target_id);
    ThreatItem * threat_record = NULL;
    if( iter == threats_.end() )
    {
        threat_record = new ThreatItem();
        threats_[ target_id ] = threat_record; 
    }
    else
    {
        threat_record = iter->second;
    }
    
    threat_record->add( _threat_type, _threat );
	std::vector<uint32_t>::iterator old_iter = ranks_.begin();
    while (old_iter != ranks_.end() && *old_iter != target_id)
    {
        old_iter++;
    }

    if (old_iter != ranks_.end())
    {
        ranks_.erase(old_iter);
    }

    float obj_threat = threat_record->get_threat();
    std::vector<uint32_t>::iterator list_iter = ranks_.begin();
    while (list_iter != ranks_.end() && threats_[*list_iter]->get_threat() > obj_threat)
    {
        list_iter++;
    }

    if (list_iter == ranks_.end())
    {
        ranks_.push_back( target_id );
    }
    else
    {
        ranks_.insert(list_iter, target_id);
    }

    ai_->set_target(ai_->get_obj(ranks_[0]));
}

void ThreatMgr::reset_threat( Ctrl* _target )
{
    uint32_t target_id = _target->get_id();
	std::map<uint32_t, ThreatItem*>::iterator iter = threats_.find(target_id);
    ThreatItem * threat_record = NULL;
    if( iter != threats_.end() )
    {
        threat_record = iter->second;
        threat_record->reset();
        
		std::vector<uint32_t>::iterator old_iter = ranks_.begin();
		while (old_iter != ranks_.end() && *old_iter != target_id)
		{
			old_iter++;
		}

		if (old_iter != ranks_.end())
		{
			ranks_.erase(old_iter);
		}

        float obj_threat = threat_record->get_threat();
        std::vector<uint32_t>::iterator list_iter = ranks_.begin();
        while (list_iter != ranks_.end() && threats_[*list_iter]->get_threat() > obj_threat)
        {
            list_iter++;
        }

        if (list_iter == ranks_.end())
        {
            ranks_.push_back( target_id );
        }
        else
        {
            ranks_.insert(list_iter, target_id);
        }

		ai_->set_target(ai_->get_obj(ranks_[0]));
    }
}

}
