/**
 * \file
 * \brief Solve an instance of the "Variable Placement with Separation
 * Constraints" problem.
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

#include <cassert>
#include <vpsc/constraint.h>
#include <vpsc/block.h>
#include <vpsc/blocks.h>
#include <vpsc/solve_VPSC.h>
#include <math.h>
#include <memory>
#include <sstream>
#include <fstream>
using std::ios;
using std::ofstream;

using std::ostringstream;
using std::list;
using std::set;

#ifndef RECTANGLE_OVERLAP_LOGGING
	#define RECTANGLE_OVERLAP_LOGGING 0
#endif

IncVPSC::IncVPSC(const unsigned n, Variable *vs[], const unsigned m, Constraint *cs[]) 
	: VPSC(n,vs,m,cs) {
	inactive.assign(cs,cs+m);
	for(Constraint *c : inactive) {
		c->active=false;
	}
}
VPSC::VPSC(const unsigned n, Variable *vs[], const unsigned m, Constraint *cs[])
  : bs(n, vs), cs(cs), m(m) {
	if (RECTANGLE_OVERLAP_LOGGING) {
		printBlocks();
		assert(!constraintGraphIsCyclic(n,vs));
	}
}

// useful in debugging
void VPSC::printBlocks() {
	if (RECTANGLE_OVERLAP_LOGGING) {
		ofstream f(LOGFILE,ios::app);
		for(Block *b : bs) {
			f<<"  "<<*b<<"\n";
		}
		for(unsigned i=0;i<m;i++) {
			f<<"  "<<*cs[i]<<"\n";
		}
	}
}
/**
* Produces a feasible - though not necessarily optimal - solution by
* examining blocks in the partial order defined by the directed acyclic
* graph of constraints. For each block (when processing left to right) we
* maintain the invariant that all constraints to the left of the block
* (incoming constraints) are satisfied. This is done by repeatedly merging
* blocks into bigger blocks across violated constraints (most violated
* first) fixing the position of variables inside blocks relative to one
* another so that constraints internal to the block are satisfied.
*/
void VPSC::satisfy() {
	list<Variable*> vs=bs.totalOrder();
	for(Variable *v : vs) {
		if(!v->block->deleted) {
			bs.mergeLeft(v->block);
		}
	}
	bs.cleanup();
	for(unsigned i=0;i<m;i++) {
		if(cs[i]->slack()<-0.0000001) {
			if (RECTANGLE_OVERLAP_LOGGING) {
				ofstream f(LOGFILE,ios::app);
				f<<"Error: Unsatisfied constraint: "<<*cs[i]<<"\n";
			}
			//assert(cs[i]->slack()>-0.0000001);
			throw "Unsatisfied constraint";
		}
	}
}

void VPSC::refine() {
	bool solved=false;
	while(!solved) {
		solved=true;
		for(Block *b : bs) {
			b->setUpInConstraints();
			b->setUpOutConstraints();
		}
		for(Block *b : bs) {
			Constraint *c=b->findMinLM();
			if(c!=nullptr && c->lm<0) {
				if (RECTANGLE_OVERLAP_LOGGING) {
					ofstream f(LOGFILE,ios::app);
					f<<"Split on constraint: "<<*c<<"\n";
				}
				// Split on c
				Block *l=nullptr, *r=nullptr;
				bs.split(b,l,r,c);
				bs.cleanup();
				// split alters the block set so we have to restart
				solved=false;
				break;
			}
		}
	}
	for(unsigned i=0;i<m;i++) {
		if(cs[i]->slack()<-0.0000001) {
			assert(cs[i]->slack()>-0.0000001);
			throw "Unsatisfied constraint";
		}
	}
}
/**
 * Calculate the optimal solution. After using satisfy() to produce a
 * feasible solution, refine() examines each block to see if further
 * refinement is possible by splitting the block. This is done repeatedly
 * until no further improvement is possible.
 */
void VPSC::solve() {
	satisfy();
	refine();
}

void IncVPSC::solve() {
	if (RECTANGLE_OVERLAP_LOGGING) {
		ofstream f(LOGFILE,ios::app);
		f<<"solve_inc()...\n";
	}
	double lastcost,cost = bs.cost();
	do {
		lastcost=cost;
		satisfy();
		splitBlocks();
		cost = bs.cost();
		if (RECTANGLE_OVERLAP_LOGGING) {
			ofstream f(LOGFILE,ios::app);
			f<<"  cost="<<cost<<"\n";
		}
	} while(fabs(lastcost-cost)>0.0001);
}
/**
 * incremental version of satisfy that allows refinement after blocks are
 * moved.
 *
 *  - move blocks to new positions
 *  - repeatedly merge across most violated constraint until no more
 *    violated constraints exist
 *
 * Note: there is a special case to handle when the most violated constraint
 * is between two variables in the same block.  Then, we must split the block
 * over an active constraint between the two variables.  We choose the 
 * constraint with the most negative lagrangian multiplier. 
 */
void IncVPSC::satisfy() {
	if (RECTANGLE_OVERLAP_LOGGING) {
		ofstream f(LOGFILE,ios::app);
		f<<"satisfy_inc()...\n";
	}
	splitBlocks();
	long splitCtr = 0;
	Constraint* v = nullptr;
	while(mostViolated(inactive,v)<-0.0000001) {
		assert(!v->active);
		Block *lb = v->left->block, *rb = v->right->block;
		if(lb != rb) {
			lb->merge(rb,v);
		} else {
			if(splitCtr++>10000) {
				throw "Cycle Error!";
			}
			// constraint is within block, need to split first
			inactive.push_back(lb->splitBetween(v->left,v->right,lb,rb));
			lb->merge(rb,v);
			bs.insert(lb);
		}
	}
	if (RECTANGLE_OVERLAP_LOGGING) {
		ofstream f(LOGFILE,ios::app);
		f<<"  finished merges.\n";
	}
	bs.cleanup();
	for(unsigned i=0;i<m;i++) {
		v=cs[i];
		if(v->slack()<-0.0000001) {
			//assert(cs[i]->slack()>-0.0000001);
			ostringstream s;
			s<<"Unsatisfied constraint: "<<*v;
			throw s.str().c_str();
		}
	}
	if (RECTANGLE_OVERLAP_LOGGING) {
		ofstream f(LOGFILE,ios::app);
		f<<"  finished cleanup.\n";
		printBlocks();
	}
}
void IncVPSC::moveBlocks() {
	if (RECTANGLE_OVERLAP_LOGGING) {
		ofstream f(LOGFILE,ios::app);
		f<<"moveBlocks()...\n";
	}
	for(Block *b : bs) {
		b->wposn = b->desiredWeightedPosition();
		b->posn = b->wposn / b->weight;
	}
	if (RECTANGLE_OVERLAP_LOGGING) {
		ofstream f(LOGFILE,ios::app);
		f<<"  moved blocks.\n";
	}
}
void IncVPSC::splitBlocks() {
	moveBlocks();
	splitCnt=0;
	// Split each block if necessary on min LM
	for(Block *b : bs) {
		Constraint* v=b->findMinLM();
		if(v!=nullptr && v->lm < -0.0000001) {
			if (RECTANGLE_OVERLAP_LOGGING) {
				ofstream f(LOGFILE,ios::app);
				f<<"    found split point: "<<*v<<" lm="<<v->lm<<"\n";
			}
			splitCnt++;
			Block *b2 = v->left->block, *l=nullptr, *r=nullptr;
			assert(v->left->block == v->right->block);
			double pos = b2->posn;
			b2->split(l,r,v);
			l->posn=r->posn=pos;
			l->wposn = l->posn * l->weight;
			r->wposn = r->posn * r->weight;
			bs.insert(l);
			bs.insert(r);
			b2->deleted=true;
			inactive.push_back(v);
			if (RECTANGLE_OVERLAP_LOGGING) {
				ofstream f(LOGFILE,ios::app);
				f<<"  new blocks: "<<*l<<" and "<<*r<<"\n";
			}
		}
	}
	if (RECTANGLE_OVERLAP_LOGGING) {
		ofstream f(LOGFILE,ios::app);
		f<<"  finished splits.\n";
	}
	bs.cleanup();
}

/**
 * Scan constraint list for the most violated constraint, or the first equality
 * constraint
 */
double IncVPSC::mostViolated(ConstraintList &l, Constraint* &v) {
	double minSlack = DBL_MAX;
	if (RECTANGLE_OVERLAP_LOGGING) {
		ofstream f(LOGFILE,ios::app);
		f<<"Looking for most violated...\n";
	}
	ConstraintList::iterator end = l.end();
	ConstraintList::iterator deletePoint = end;
	for(ConstraintList::iterator i=l.begin();i!=end;i++) {
		Constraint *c=*i;
		double slack = c->slack();
		if(c->equality || slack < minSlack) {
			minSlack=slack;	
			v=c;
			deletePoint=i;
			if(c->equality) break;
		}
	}
	// Because the constraint list is not order dependent we just
	// move the last element over the deletePoint and resize
	// downwards.  There is always at least 1 element in the
	// vector because of search.
	if(deletePoint != end && minSlack<-0.0000001) {
		*deletePoint = l[l.size()-1];
		l.resize(l.size()-1);
	}
	if (RECTANGLE_OVERLAP_LOGGING) {
		ofstream f(LOGFILE,ios::app);
		f<<"  most violated is: "<<*v<<"\n";
	}
	return minSlack;
}

#include <map>
using std::map;
using std::vector;
struct node {
	set<node*> in;
	set<node*> out;
};
// useful in debugging - cycles would be BAD
bool VPSC::constraintGraphIsCyclic(const unsigned n, Variable *vs[]) {
	map<Variable*, node*> varmap;
	vector<std::unique_ptr<node>> graph;
	for(unsigned i=0;i<n;i++) {
		graph.emplace_back(new node);
		varmap[vs[i]]=graph.back().get();
	}
	for(unsigned i=0;i<n;i++) {
		for(Constraint *c : vs[i]->in) {
			Variable *l=c->left;
			varmap[vs[i]]->in.insert(varmap[l]);
		}

		for(Constraint *c : vs[i]->out) {
			Variable *r=c->right;
			varmap[vs[i]]->out.insert(varmap[r]);
		}
	}
	while(!graph.empty()) {
		node *u=nullptr;
		vector<std::unique_ptr<node>>::iterator i=graph.begin();
		for(;i!=graph.end();i++) {
			u=(*i).get();
			if(u->in.empty()) {
				break;
			}
		}
		if(i==graph.end()) {
			//cycle found!
			return true;
		} else {
			graph.erase(i);
			for(node *v : u->out) {
				v->in.erase(u);
			}
		}
	}
	return false;
}

// useful in debugging - cycles would be BAD
bool VPSC::blockGraphIsCyclic() {
	map<Block*, node*> bmap;
	vector<std::unique_ptr<node>> graph;
	for(Block *b : bs) {
		graph.emplace_back(new node);
		bmap[b]=graph.back().get();
	}
	for(Block *b : bs) {
		b->setUpInConstraints();
		Constraint *c=b->findMinInConstraint();
		while(c!=nullptr) {
			Block *l=c->left->block;
			bmap[b]->in.insert(bmap[l]);
			b->deleteMinInConstraint();
			c=b->findMinInConstraint();
		}

		b->setUpOutConstraints();
		c=b->findMinOutConstraint();
		while(c!=nullptr) {
			Block *r=c->right->block;
			bmap[b]->out.insert(bmap[r]);
			b->deleteMinOutConstraint();
			c=b->findMinOutConstraint();
		}
	}
	while(!graph.empty()) {
		node *u=nullptr;
		vector<std::unique_ptr<node>>::iterator i=graph.begin();
		for(;i!=graph.end();i++) {
			u=(*i).get();
			if(u->in.empty()) {
				break;
			}
		}
		if(i==graph.end()) {
			//cycle found!
			return true;
		}
		graph.erase(i);
		for(node *v : u->out) {
			v->in.erase(u);
		}
	}
	return false;
}
/**
 * @dir lib/vpsc
 * @brief library for solving the
 *        %Variable Placement with Separation Constraints problem
 *        for lib/neatogen
 *
 * This is a quadratic programming problem in which the squared differences
 * between a placement vector and some ideal placement are minimized subject
 * to a set of separation constraints. This is very useful in a number of layout problems.
 *
 * References:
 * - https://www.adaptagrams.org/documentation/libvpsc.html
 * - Tim Dwyer, Kim Marriott, and Peter J. Stuckey. Fast node overlap removal.
 *   In Proceedings 13th International Symposium on Graph Drawing (GD '05),
 *   volume 3843 of LNCS, pages 153-164. Springer, 2006.
 */
