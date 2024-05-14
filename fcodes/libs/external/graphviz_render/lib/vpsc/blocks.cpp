/**
 * \brief A block structure defined over the variables
 *
 * A block structure defined over the variables such that each block contains
 * 1 or more variables, with the invariant that all constraints inside a block
 * are satisfied by keeping the variables fixed relative to one another
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

#include <vpsc/blocks.h>
#include <vpsc/block.h>
#include <vpsc/constraint.h>
#include <fstream>
using std::ios;
using std::ofstream;
using std::set;
using std::list;

#ifndef RECTANGLE_OVERLAP_LOGGING
	#define RECTANGLE_OVERLAP_LOGGING 0
#endif

long blockTimeCtr;

Blocks::Blocks(const int n, Variable *vs[]) : vs(vs),nvs(n) {
	blockTimeCtr=0;
	for(int i=0;i<nvs;i++) {
		insert(new Block(vs[i]));
	}
}
Blocks::~Blocks()
{
	blockTimeCtr=0;
	for (Block *b : *this) {
		delete b;
	}
}

/**
 * returns a list of variables with total ordering determined by the constraint 
 * DAG
 */
list<Variable*> Blocks::totalOrder() {
	list<Variable*> order;
	for(int i=0;i<nvs;i++) {
		vs[i]->visited=false;
	}
	for(int i=0;i<nvs;i++) {
		if(vs[i]->in.empty()) {
			dfsVisit(vs[i],order);
		}
	}
	return order;
}
// Recursive depth first search giving total order by pushing nodes in the DAG
// onto the front of the list when we finish searching them
void Blocks::dfsVisit(Variable *v, list<Variable*> &order) {
	v->visited=true;
	for (Constraint *c : v->out) {
		if(!c->right->visited) {
			dfsVisit(c->right, order);
		}
	}	
	if (RECTANGLE_OVERLAP_LOGGING) {
		ofstream f(LOGFILE,ios::app);
		f<<"  order="<<*v<<"\n";
	}
	order.push_front(v);
}
/**
 * Processes incoming constraints, most violated to least, merging with the
 * neighbouring (left) block until no more violated constraints are found
 */
void Blocks::mergeLeft(Block *r) {	
	if (RECTANGLE_OVERLAP_LOGGING) {
		ofstream f(LOGFILE,ios::app);
		f<<"mergeLeft called on "<<*r<<"\n";
	}
	r->timeStamp=++blockTimeCtr;
	r->setUpInConstraints();
	Constraint *c=r->findMinInConstraint();
	while (c != nullptr && c->slack()<0) {
		if (RECTANGLE_OVERLAP_LOGGING) {
			ofstream f(LOGFILE,ios::app);
			f<<"mergeLeft on constraint: "<<*c<<"\n";
		}
		r->deleteMinInConstraint();
		Block *l = c->left->block;		
		if (l->in==nullptr) l->setUpInConstraints();
		double dist = c->right->offset - c->left->offset - c->gap;
		if (r->vars.size() < l->vars.size()) {
			dist=-dist;
			std::swap(l, r);
		}
		blockTimeCtr++;
		r->merge(l, c, dist);
		r->mergeIn(l);
		r->timeStamp=blockTimeCtr;
		removeBlock(l);
		c=r->findMinInConstraint();
	}		
	if (RECTANGLE_OVERLAP_LOGGING) {
		ofstream f(LOGFILE,ios::app);
		f<<"merged "<<*r<<"\n";
	}
}	
/**
 * Symmetrical to mergeLeft
 */
void Blocks::mergeRight(Block *l) {	
	if (RECTANGLE_OVERLAP_LOGGING) {
		ofstream f(LOGFILE,ios::app);
		f<<"mergeRight called on "<<*l<<"\n";
	}
	l->setUpOutConstraints();
	Constraint *c = l->findMinOutConstraint();
	while (c != nullptr && c->slack()<0) {		
		if (RECTANGLE_OVERLAP_LOGGING) {
			ofstream f(LOGFILE,ios::app);
			f<<"mergeRight on constraint: "<<*c<<"\n";
		}
		l->deleteMinOutConstraint();
		Block *r = c->right->block;
		r->setUpOutConstraints();
		double dist = c->left->offset + c->gap - c->right->offset;
		if (l->vars.size() > r->vars.size()) {
			dist=-dist;
			std::swap(l, r);
		}
		l->merge(r, c, dist);
		l->mergeOut(r);
		removeBlock(r);
		c=l->findMinOutConstraint();
	}	
	if (RECTANGLE_OVERLAP_LOGGING) {
		ofstream f(LOGFILE,ios::app);
		f<<"merged "<<*l<<"\n";
	}
}
void Blocks::removeBlock(Block *doomed) {
	doomed->deleted=true;
	//erase(doomed);
}
void Blocks::cleanup() {
	for (auto i = begin(); i != end();) {
		Block *b=*i;
		if(b->deleted) {
			i = erase(i);
			delete b;
		} else {
			++i;
		}
	}
}
/**
 * Splits block b across constraint c into two new blocks, l and r (c's left
 * and right sides respectively)
 */
void Blocks::split(Block *b, Block *&l, Block *&r, Constraint *c) {
	b->split(l,r,c);
	if (RECTANGLE_OVERLAP_LOGGING) {
		ofstream f(LOGFILE,ios::app);
		f<<"Split left: "<<*l<<"\n";
		f<<"Split right: "<<*r<<"\n";
	}
	r->posn = b->posn;
	r->wposn = r->posn * r->weight;
	mergeLeft(l);
	// r may have been merged!
	r = c->right->block;
	r->wposn = r->desiredWeightedPosition();
	r->posn = r->wposn / r->weight;
	mergeRight(r);
	removeBlock(b);

	insert(l);
	insert(r);
}
/**
 * returns the cost total squared distance of variables from their desired
 * positions
 */
double Blocks::cost() {
	double c = 0;
	for (Block *b : *this) {
		c += b->cost();
	}
	return c;
}
