#include <string>
#include "load_btree.h"

namespace SGame
{

class AI;

static DocCache cache_;

void add_doc_cache(const char* _xml_file_name, const char* _prop_str)
{
    cache_.set_doc(_xml_file_name, _prop_str);
}

void clear_doc_cache()
{
	cache_.clear();
}

DocCache::DocCache()
{

}

DocCache::~DocCache()
{
	clear();
}

void DocCache::clear()
{
	DocMap::const_iterator itr = doc_cache.begin();
	for(;itr != doc_cache.end(); itr++)
	{
		delete (*itr).second;
	}
	doc_cache.clear();
}

void DocCache::set_doc(const char* _xml_file_name, const char* _prop_str)
{
    DocMap::const_iterator itr = doc_cache.find(_xml_file_name);

    if(itr != doc_cache.end())
    {
        delete (*itr).second;
    }

    TiXmlDocument* doc = new TiXmlDocument();
    doc->Parse(_prop_str, 0, TIXML_DEFAULT_ENCODING);
    bool loadOk = !doc->Error();

    if(!loadOk)
    {
        throw(runtime_error("Couldn't load XML document"));
    }

    doc_cache[_xml_file_name] = doc;
}

TiXmlDocument* DocCache::get_doc(const string& _file_name)
{
	DocMap::const_iterator itr = doc_cache.find(_file_name);
	if(itr != doc_cache.end())
	{
		return (*itr).second;
	}

	TiXmlDocument* doc = new TiXmlDocument();
	bool load_ok = doc->LoadFile(_file_name.c_str());

	if(!load_ok)
	{
		return doc;
	}

	doc_cache[_file_name] = doc;
	return doc;
}

static void read_attributes(TiXmlElement* _element, BTNode* _new_node, PropMap* _prop_map)
{
	if(!_element || !_new_node) return;

	const char* read = _element->Attribute("R_");
	if(NULL != read)
	{
		return;
	}

	const TiXmlAttribute* att = _element->FirstAttribute();
	while(att)
	{
		const char* attName = att->Name();
		const char* attVal = att->Value();
		_new_node->set_property_once(attName, attVal, _prop_map);

		att = att->Next();
	}
	_element->SetAttribute("R_", 1);
}

static bool load_recur(TiXmlElement* _element, BehaviorTree* _bt, BTNode* _dt_parent_element, AI* _ai, PropMap* _prop_map)
{
	if(!_ai || _ai->assert_fail(_element && _bt && _dt_parent_element))
	{
		return false;
	}

	int32_t nodeId = 0;
	_element->QueryIntAttribute("Node_id", &nodeId);

	BTNode* next_level_node = NULL;
	const char* myNodeType = _element->Attribute("sof-type");
	const char* nodeType = _element->Attribute("Type");

	if(myNodeType && strcmp(nodeType, "Selector") == 0)
	{
		nodeType = myNodeType;
	}

	if(nodeType)
	{
		if(strcmp(nodeType, "Root") == 0)
		{
			next_level_node = _dt_parent_element;
			_dt_parent_element->set_id(nodeId);
		}
		else if (strcmp(nodeType, "Selector") == 0)
		{
			BTSelecNode* new_node = new BTSelecNode(_dt_parent_element);
			new_node->set_behavior_tree(_bt);
			new_node->set_name(nodeType);
			new_node->set_id(nodeId);
			read_attributes(_element, new_node, _prop_map);
			_dt_parent_element->push_back(new_node);
			next_level_node = new_node;
		}
		else if (strcmp(nodeType, "rater") == 0)
		{
			BTRaterNode* new_node = new BTRaterNode(_dt_parent_element);
			new_node->set_behavior_tree(_bt);
			new_node->set_name(nodeType);
			new_node->set_id(nodeId);
			read_attributes(_element, new_node, _prop_map);
			_dt_parent_element->push_back(new_node);
			next_level_node = new_node;
		}
		else if (strcmp(nodeType, "Sequence") == 0)
		{
			BTSeqNode* new_node = new BTSeqNode(_dt_parent_element);
			new_node->set_behavior_tree(_bt);
			new_node->set_name(nodeType);
			new_node->set_id(nodeId);
			read_attributes(_element, new_node, _prop_map);
			_dt_parent_element->push_back(new_node);
			next_level_node = new_node;
		}
		else if (strcmp(nodeType, "Filter") == 0)
		{
			const char* filterType = _element->Attribute("Filter_Type");
			if(filterType)
			{
				BTNode* new_node = NULL;

				if(strcmp(filterType, "Logger") == 0)
				{
					const char* name = _element->Attribute("Name");
					if(name)
					{
						// new_node = new Filter_logger(DT_ParentElement, name);
					}
					else
					{
						return false;
					}
				}
				else if (strcmp(filterType, "Timer") == 0)
				{
					double time = -1;
					int32_t queryRes =  _element->QueryDoubleAttribute("Time", &time);
					if(queryRes == TIXML_SUCCESS)
					{
						// new_node = new Filter_timer(DT_ParentElement, (float)time);
					}
					else
					{
						return false;
					}
				}
				else if (strcmp(filterType, "Counter") == 0)
				{
					int32_t times = -1;
					int32_t queryRes =  _element->QueryIntAttribute("Times", &times);
					if(queryRes == TIXML_SUCCESS)
					{
						// new_node = new FilterCounter(DT_ParentElement, times);
					}
					else
					{
						return false;
					}
				}
				else if (strcmp(filterType, "Loop") == 0)
				{
					int32_t times = -1;
					int32_t queryRes = _element->QueryIntAttribute("Times", &times);
					if(queryRes == TIXML_SUCCESS)
					{
						// new_node = new Filter_loop(DT_ParentElement, times);
					}
					else
					{
						return false;
					}
				}
				else if (strcmp(filterType, "Until_Fails_Limited") == 0)
				{
					int32_t times = -1;
					int32_t query_res = _element->QueryIntAttribute("Times", &times);
					if(query_res == TIXML_SUCCESS)
					{
						new_node = new UntilFailsLimitedFilter(_dt_parent_element, times);
					}
					else
					{
						return false;
					}
				}

				else if (strcmp(filterType, "Until_Fails") == 0)
				{
					// new_node = new Filter_until_fails(DT_ParentElement);
				}
				else if (strcmp(filterType, "Non") == 0)
				{
					// new_node = new FilterNon(DT_ParentElement);
				}
				else
				{
					return false;
				}

				if(_ai->assert_fail(new_node))
				{
					return false;
				}

				new_node->set_behavior_tree(_bt);
				new_node->set_name(filterType);
				new_node->set_id(nodeId);
				read_attributes(_element, new_node, _prop_map);
				_dt_parent_element->push_back(new_node);
				next_level_node = new_node;
			}
			else
			{
				return false;
			}
		}
		else if ((strcmp(nodeType, "Action") == 0) || (strcmp(nodeType, "Condition") == 0))
		{
			BTLeafType leafType = (strcmp(nodeType, "Action") == 0) ? BTLEAF_ACTION : BTLEAF_CONDITION;

			const char* op = _element->Attribute("Operation");
			if(op)
			{
                bool is_skill_op = strcmp("skill", op) == 0;
                bool is_charge_skill_op = strcmp("chargeskill", op) == 0;
                bool is_chase_skill_op = strcmp("chaseskill", op) == 0;

				if (is_skill_op || is_charge_skill_op || is_chase_skill_op)
				{
					TiXmlElement new_element(*_element);

					int32_t new_node_id = nodeId * 10;
					const char* new_op = "skillseq";
					BTSeqNode* parent_node = new BTSeqNode(_dt_parent_element);
					parent_node->set_behavior_tree(_bt);
					parent_node->set_name(new_op);
					parent_node->set_id(new_node_id);
					read_attributes(&new_element, parent_node, _prop_map);
					_dt_parent_element->push_back(parent_node);

					new_op = "skillchase";
					new_node_id = nodeId * 100;
					new_element = *_element;

					if(_ai->assert_fail(BTLeafFactory::get_instance()->is_creator_registered(new_op)))
					{
						return false;
					}

					BTLeafNode *new_node = BTLeafFactory::get_instance()->get_creator(new_op)(
						parent_node, _ai);
					new_node->set_behavior_tree(_bt);
					new_node->set_name(new_op);
					new_node->set_leaf_type(leafType);
					new_node->set_id(new_node_id);
					read_attributes(&new_element, new_node, _prop_map);
					parent_node->push_back(new_node);

					if(_ai->assert_fail(BTLeafFactory::get_instance()->is_creator_registered(op)))
					{
						return false;
					}

					new_node = BTLeafFactory::get_instance()->get_creator(op)(
						parent_node, _ai);
					new_node->set_behavior_tree(_bt);
					new_node->set_name(op);
					new_node->set_leaf_type(leafType);
					new_node->set_id(nodeId);
					read_attributes(_element, new_node, _prop_map);
					parent_node->push_back(new_node);
				}
				else
				{
					BTLeafNode *new_node = BTLeafFactory::get_instance()->get_creator(op)(
						_dt_parent_element, _ai);
					new_node->set_behavior_tree(_bt);
					new_node->set_name(op);
					new_node->set_leaf_type(leafType);
					new_node->set_id(nodeId);
					read_attributes(_element, new_node, _prop_map);
					_dt_parent_element->push_back(new_node);
				}
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	if (next_level_node == NULL)
	{
		next_level_node = _dt_parent_element;
	}

	read_attributes(_element, next_level_node, _prop_map);

	TiXmlElement* connector = _element->FirstChildElement("Connector");
	if(!connector)
	{
		return false;
	}

	TiXmlElement* e = connector->FirstChildElement("Node");
	while(e)
	{
		if(!load_recur(e, _bt, next_level_node, _ai, _prop_map))
		{
			return false;
		}
		e = e->NextSiblingElement("Node");
	}
	return true;
}

bool load_bt_tree(const string& _from_file, BehaviorTree* _bt, BTRootNode* _node, AI* _ai)
{
	if(!_ai || _ai->assert_fail(_from_file.length() > 0 && _node != NULL))
	{
		return false;
	}

	bool load_ok = true;

	TiXmlDocument* doc = cache_.get_doc(_from_file);
	TiXmlElement* rootElement = doc->FirstChildElement("Behavior");
	if(!rootElement)
	{
		return false;
	}

	TiXmlElement* element = rootElement->FirstChildElement("Node");

	_node->set_behavior_tree(_bt);

    PropMap* prop_map = g_prop_cache->get_ai_prop( _ai->get_template_name() );

	if(!load_recur(element, _bt, _node, _ai, prop_map))
	{
		load_ok = false;
		//delete doc;
	}

	_bt->set_initialized(false);

	return load_ok;
}

}

