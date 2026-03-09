#include "composite.h"
#include <cstdio>
#include <ctime>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cassert>
#include "btree.h"
#include "common.h"
#include "component_ai.h"
#include "limits.h"

namespace SGame
{

BTSeqNode::BTSeqNode(BTNode* _parent)
	: BTNode(_parent)
{
	child_cd_ = 0;
    can_reset_ = 1;
}

void BTSeqNode::init()
{
	BTNode::init();
	if ((parent_ && "rater" == parent_->get_name())|| "skillseq" == get_name())
	{
		child_cd_ = 1;
	}
	get_property_int("child-cd", child_cd_);
	get_property_int("reset", can_reset_);
}

void BTSeqNode::handle_disabled_child()
{
	this->update(SUCCESS);
}

void BTSeqNode::update(NodeStatus _result)
{
	tree_->set_current_node(this);

	if(_result == SUCCESS)
	{	
		if (it_child_ != children_.end())
		{
			++it_child_;
		}
		else
		{
			errorf("child node index over end!");
		}

		if(it_child_ != children_.end())
		{
			(*it_child_)->do_execute();
		}
		else
		{
			it_child_ = children_.begin();
			node_status_ = _result;
			log_node_result();
			parent_->update(node_status_);
		}
	}
	else if (_result == FAILURE)
	{
		node_status_ = _result;
		if ("skillseq" == node_name_ && it_child_ == children_.begin())
		{
			node_status_ = SUCCESS;
		}

		it_child_ = children_.begin();
		log_node_result();
		parent_->update(node_status_);
	}
}

void BTSeqNode::execute()
{
	if (!can_use())
	{
		it_child_ = children_.end();
		update(FAILURE);
		return;
	}

	it_child_ = children_.begin();
	(*it_child_)->do_execute();
}

void BTSeqNode::push_back(BTNode* _node)
{
	BTNode::push_back(_node);
	it_child_ = children_.begin();
}

bool BTSeqNode::can_use()
{
    if( children_.size() <= 0 )
    {
		errorf("BTSeqNode do not have any child");
        return false;
    }

	if( child_cd_ )
	{
		for(uint32_t i = 0; i < children_.size(); ++i)
		{
			if(!children_[i]->can_use())
			{
				return false;
			}
		}
	}

	return true;
}

BTSelecNode::BTSelecNode(BTNode* _parent)
	: BTNode(_parent)
{
}

void BTSelecNode::init()
{
	BTNode::init();
}

void BTSelecNode::handle_disabled_child()
{
	this->update(FAILURE);
}

void BTSelecNode::update(NodeStatus _result)
{
	tree_->set_current_node(this);

	if(_result != RUNNING)
	{
		if (it_child_ != children_.end())
		{
			++it_child_;
		}
		else
		{
			errorf("child node index over end!");
		}
	}

	if(_result == FAILURE)
	{
		if(it_child_ != children_.end())
		{
			node_status_ = RUNNING;
			log_node_result();
			(*it_child_)->do_execute();
		}
		else
		{
			it_child_ = children_.begin();
			node_status_ = _result;
			log_node_result();
			parent_->update(node_status_);
		}
	}
	else if (_result == SUCCESS)
	{
		it_child_ = children_.begin();
		node_status_ = _result;
		log_node_result();
		parent_->update(node_status_);
	}
}


void BTSelecNode::execute()
{
    if( !can_use() )
    {
	    update( FAILURE );
        return;
    }

	it_child_ = children_.begin();
	(*it_child_)->do_execute();
}

bool BTSelecNode::can_use()
{
    if( children_.size() <= 0 )
    {
		errorf("BTSelecNode do not have any child");
        return false;
    }

	return true;
}

BTRaterNode::BTRaterNode(BTNode* _parent)
	: BTNode(_parent)
{
}

void BTRaterNode::init()
{
	BTNode::init();
	init_cd();

	std::vector<string> strLst;
	get_property_string_list("skill", strLst);
	for (uint32_t i = 0; i< strLst.size(); i++)
	{
		std::vector<std::string> values;
		if(strLst[i].find(":") != string::npos)
		{
			str_split(strLst[i], values, ":");
		}
		int32_t skillId = 0;
		if (values.size() >= 1)
		{
			skillId = atoi(values[0].c_str());
		}
		float   rate = 1;
		if (values.size() >= 2)
		{
			rate = atof(values[1].c_str());
		}
		float   cdTime = 0;
		if (values.size() >= 3)
		{
			cdTime	= atof(values[2].c_str());
		}
		
		if (skillId != 0)
		{
			//LOG(2)("insert skill: %d, %f, %f, %d", skillId, cdTime, rate, values.size());
			insert_skill(skillId, cdTime, rate);
		}
	}
}

void BTRaterNode::handle_disabled_child()
{
	this->update(FAILURE);
}

void BTRaterNode::update(NodeStatus _result)
{
	tree_->set_current_node(this);

	if(FAILURE == _result || SUCCESS == _result)
	{
		it_child_ = children_.begin();
		node_status_ = _result;
		log_node_result();
		parent_->update(node_status_);
	}
}

void BTRaterNode::execute()
{
    if( !can_use() )
    {
        update( FAILURE );
        return;
    }

	map<BTNode*, int32_t> temp_map;
	int32_t total_sum = 0;

	for (map<BTNode*, int32_t>::const_iterator itr = weighting_map_.begin();
		itr != weighting_map_.end(); ++itr)
	{
		if(!(*itr).first->can_use())
		{
			continue;
		}

		int32_t rate = (*itr).first->get_rate();
		total_sum += rate;
		temp_map[(*itr).first] = rate;
	}

	if(0 == total_sum)
	{
		update(FAILURE);
		return;
	}

	int32_t chosen = rand_int(0, total_sum);
	int32_t sum = 0;

	for (map<BTNode*, int32_t>::iterator itr = temp_map.begin();
		itr != temp_map.end(); ++itr)
	{
		sum += (*itr).second;
		if (sum >= chosen)
		{
			(*itr).first->do_execute();
			return;
		}
	}
}

void BTRaterNode::push_back(BTNode* _node)
{
	BTNode::push_back(_node);

	bool ret = _node->get_property_int("rate", _node->rate);
	if(ai_->assert_fail(ret && _node->rate >= 0))
	{
        string err_msg = "no or error rater rate: "+_node->get_name();
		errorf( err_msg.c_str() );
		return;
	}
	_node->get_property_int("rate-acc", _node->rate_acc);

	_node->get_property_int("rate-min", _node->rate_min);
	_node->rate_min = max(_node->rate_min, 0);

	int32_t rate_max = _node->rate_max;
	_node->get_property_int("rate-max", rate_max);
	_node->rate_max = min(_node->rate_max, rate_max);

	weighting_map_[_node] = _node->rate;
	_node->set_use_checked(true);
}

bool BTRaterNode::can_use()
{
    if( children_.size() <= 0 )
    {
		errorf("BTRaterNode do not have any child");
        return false;
    }

	return true;
}

UntilFailsLimitedFilter::UntilFailsLimitedFilter( BTNode* _parent, int32_t _limit_usage ) 
	: BTNode(_parent)
{
	limit_usage_ = _limit_usage;
	if (limit_usage_ < 1)
	{
		limit_usage_ = INT_MAX;
	}
	current_usage_ = 0;
}

void  UntilFailsLimitedFilter::reset()
{
	it_child_ = children_.begin();
	current_usage_ = 1;
}

void UntilFailsLimitedFilter::update(NodeStatus _result)
{
	tree_->set_current_node(this);

	if(_result == FAILURE)
	{   
		node_status_ = _result;
		log_node_result();
		parent_->update(node_status_);
	}
	else if(_result == SUCCESS)
	{
		if(current_usage_ >= limit_usage_)
		{
			node_status_ = _result;
			log_node_result();
			parent_->update(node_status_);
		}
		else
		{
			node_status_ = RUNNING;
			log_node_result();
			current_usage_++;
			(*it_child_)->do_execute();
		}
	}
}

void UntilFailsLimitedFilter::execute()
{
	current_usage_ = 1;
	it_child_ = children_.begin();
	(*it_child_)->do_execute();
}

}
