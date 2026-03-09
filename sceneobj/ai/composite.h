#ifndef __COMPOSITE_H__
#define __COMPOSITE_H__

#include <map>
#include "btnode.h"
#include "common.h"

namespace SGame
{

class BTSeqNode: public BTNode
{
public:
	explicit BTSeqNode(BTNode* parent);
	void init();
	void execute();
	bool can_use();
    bool can_reset() const;
	void push_back(BTNode* _node);
	void handle_disabled_child();
	void update(NodeStatus _result);
	
private:
	int32_t child_cd_;
    int32_t can_reset_;
};

inline bool BTSeqNode::can_reset() const
{
    return can_reset_ == 1;
}


class BTSelecNode: public BTNode
{
public:
	explicit BTSelecNode(BTNode* _parent);
	void init();
	void execute();
	bool can_use();
	void handle_disabled_child();
	void update(NodeStatus _result);
};


class BTRaterNode: public BTNode
{
public:
	explicit BTRaterNode(BTNode* _parent);
	void init();
	void execute();
	bool can_use();
	void update(NodeStatus _result);
	void handle_disabled_child();
	void push_back(BTNode* _node);

private:
	map<BTNode*, int32_t> weighting_map_;
};

class UntilFailsLimitedFilter : public BTNode
{
private:
	int32_t limit_usage_;

	int32_t current_usage_;

public:
	explicit UntilFailsLimitedFilter(BTNode* _parent, int32_t _limit_usage);

	void reset();

	void execute();

	void update(NodeStatus _result);
};

}
#endif  // __COMPOSITE_H__
