/**
 * \brief A block is a group of variables that must be moved together to improve
 * the goal function without violating already active constraints.
 * The variables in a block are spanned by a tree of active constraints.
 *
 * Authors:
 *   Tim Dwyer <tgdwyer@gmail.com>
 *
 * Copyright (C) 2005 Authors
 *
 * This version is released under the CPL (Common Public License) with
 * the Graphviz distribution.
 * A version is also available under the LGPL as part of the Adaptagrams
 * project: https://github.com/mjwybrow/adaptagrams.  
 * If you make improvements or bug fixes to this code it would be much
 * appreciated if you could also contribute those changes back to the
 * Adaptagrams repository.
 */

#pragma once

#include <memory>
#include <vector>
#include <iostream>
#include <vpsc/pairingheap/PairingHeap.h>
struct Variable;
struct Constraint;

class Block
{
	friend std::ostream& operator <<(std::ostream &os,const Block &b);
public:
	std::vector<Variable*> vars;
	double posn;
	double weight;
	double wposn;
	Block(Variable *v=nullptr);
	Block(const Block &) = delete;
	Constraint* findMinLM();
	Constraint* findMinLMBetween(Variable* lv, Variable* rv);
	Constraint* findMinInConstraint();
	Constraint* findMinOutConstraint();
	void deleteMinInConstraint();
	void deleteMinOutConstraint();
	double desiredWeightedPosition();
	void merge(Block *b, Constraint *c, double dist);
	void merge(Block *b, Constraint *c);
	void mergeIn(Block *b);
	void mergeOut(Block *b);
	void split(Block *&l, Block *&r, Constraint *c);
	Constraint* splitBetween(Variable* vl, Variable* vr, Block* &lb, Block* &rb);
	void setUpInConstraints();
	void setUpOutConstraints();
	double cost();
	bool deleted;
	long timeStamp;
	std::unique_ptr<PairingHeap<Constraint*>> in;
	std::unique_ptr<PairingHeap<Constraint*>> out;
private:
	typedef enum {NONE, LEFT, RIGHT} Direction;
	typedef std::pair<double, Constraint*> Pair;
	void reset_active_lm(Variable *v, Variable *u);
	double compute_dfdv(Variable *v, Variable *u, Constraint *&min_lm);
	Pair compute_dfdv_between(
			Variable*, Variable*, Variable*, Direction, bool);
	bool canFollowLeft(const Constraint *c, const Variable *last);
	bool canFollowRight(const Constraint *c, const Variable *last);
	void populateSplitBlock(Block *b, Variable *v, Variable *u);
	void addVariable(Variable *v);
	void setUpConstraintHeap(std::unique_ptr<PairingHeap<Constraint*>> &h, bool use_in);
};
