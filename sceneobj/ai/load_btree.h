#ifndef __LOAD_BTREE_H__
#define __LOAD_BTREE_H__

#include <cmath>
#include <map>
#include <string>
#include <sstream>
#include "btree.h"
#include "composite.h"
#include "tinyxml.h"
#include "common.h"
#include "component_ai.h"

namespace SGame
{

class AI;

bool load_bt_tree(const string& _from_file, BehaviorTree* _bt, BTRootNode* _bt_root_node, AI* _ai);

typedef std::map<const string, TiXmlDocument*> DocMap;

class DocCache
{
public:
	explicit DocCache();
	virtual ~DocCache();
    void set_doc(const char* _xml_file_name, const char* _prop_str);
	TiXmlDocument* get_doc(const string& _file_name);
	void clear();

private:
	DocMap doc_cache;
};

void add_doc_cache(const char* _xml_file_name, const char* _prop_str);
void clear_doc_cache();

}

#endif  // __LOAD_BTREE_H__
