/**
 * \brief Functions to automatically generate constraints for the rectangular
 * node overlap removal problem.
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

#include <algorithm>
#include <set>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <vector>
#include <vpsc/generate-constraints.h>
#include <vpsc/constraint.h>

using std::set;
using std::vector;

std::ostream& operator <<(std::ostream &os, const Rectangle &r) {
	os << "{"<<r.minX<<","<<r.maxX<<","<<r.minY<<","<<r.maxY<<"},";
	return os;
}
Rectangle::Rectangle(double x, double X, double y, double Y) 
: minX(x),maxX(X),minY(y),maxY(Y) {
		assert(x<=X);
		assert(y<=Y);
}

namespace {
struct Node;
struct CmpNodePos { bool operator()(const Node* u, const Node* v) const; };

typedef set<Node*,CmpNodePos> NodeSet;

struct Node {
	Variable *v;
	const Rectangle &r;
	double pos;
	Node *firstAbove, *firstBelow;
	NodeSet leftNeighbours, rightNeighbours;
	Node(Variable *v, const Rectangle &r, double p) : v(v),r(r),pos(p) {
		firstAbove=firstBelow=nullptr;
		assert(r.width()<1e40);
	}
	void addLeftNeighbour(Node *u) {
		leftNeighbours.insert(u);
	}
	void addRightNeighbour(Node *u) {
		rightNeighbours.insert(u);
	}
	void setNeighbours(const NodeSet &left, const NodeSet &right) {
		leftNeighbours=left;
		rightNeighbours=right;
		for(Node *n : left) {
			n->addRightNeighbour(this);
		}
		for(Node *n : right) {
			n->addLeftNeighbour(this);
		}
	}
};
bool CmpNodePos::operator() (const Node* u, const Node* v) const {
	if (u->pos < v->pos) {
		return true;
	}
	if (v->pos < u->pos) {
		return false;
	}
	return u < v;
}
}

static NodeSet getLeftNeighbours(NodeSet &scanline,Node *v) {
	NodeSet leftv;
	NodeSet::iterator i=scanline.find(v);
	while(i!=scanline.begin()) {
		Node *u=*(--i);
		if(u->r.overlapX(v->r)<=0) {
			leftv.insert(u);
			return leftv;
		}
		if(u->r.overlapX(v->r)<=u->r.overlapY(v->r)) {
			leftv.insert(u);
		}
	}
	return leftv;
}
static NodeSet getRightNeighbours(NodeSet &scanline,Node *v) {
	NodeSet rightv;
	NodeSet::iterator i=scanline.find(v);
	for(i++;i!=scanline.end(); i++) {
		Node *u=*(i);
		if(u->r.overlapX(v->r)<=0) {
			rightv.insert(u);
			return rightv;
		}
		if(u->r.overlapX(v->r)<=u->r.overlapY(v->r)) {
			rightv.insert(u);
		}
	}
	return rightv;
}

typedef enum {Open, Close} EventType;
struct Event {
	EventType type;
	std::shared_ptr<Node> v;
	double pos;
	Event(EventType t, const std::shared_ptr<Node> &v, double p)
	  : type(t),v(v),pos(p) {};
};

static bool compare_events(const Event &ea, const Event &eb) {
	if(&ea.v->r==&eb.v->r) {
		// when comparing opening and closing from the same rect
		// open must come first
		if(ea.type == Open && eb.type != Open) return true;
		return false;
	} else if(ea.pos > eb.pos) {
		return false;
	} else if(ea.pos < eb.pos) {
		return true;
	}
	return false;
}

/**
 * Prepares constraints in order to apply VPSC horizontally.  Assumes variables have already been created.
 * useNeighbourLists determines whether or not a heuristic is used to deciding whether to resolve
 * all overlap in the x pass, or leave some overlaps for the y pass.
 */
int generateXConstraints(const vector<Rectangle> &rs, Variable** vars,
	  Constraint** &cs, const bool useNeighbourLists) {

	vector<Event> events;
	events.reserve(2 * rs.size());
	for(size_t i=0;i<rs.size();i++) {
		vars[i]->desiredPosition=rs[i].getCentreX();
		auto v = std::make_shared<Node>(vars[i],rs[i],rs[i].getCentreX());
		events.emplace_back(Open,v,rs[i].getMinY());
		events.emplace_back(Close,v,rs[i].getMaxY());
	}
	std::sort(events.begin(), events.end(), compare_events);

	NodeSet scanline;
	vector<Constraint*> constraints;
	for(Event &e : events) {
		Node *v = e.v.get();
		if(e.type==Open) {
			scanline.insert(v);
			if(useNeighbourLists) {
				v->setNeighbours(
					getLeftNeighbours(scanline,v),
					getRightNeighbours(scanline,v)
				);
			} else {
				NodeSet::iterator it=scanline.find(v);
				if(it!=scanline.begin()) {
					Node *u=*--it;
					v->firstAbove=u;
					u->firstBelow=v;
				}
				it=scanline.find(v);
				if(++it!=scanline.end()) {
					Node *u=*it;
					v->firstBelow=u;
					u->firstAbove=v;
				}
			}
		} else {
			// Close event
			if(useNeighbourLists) {
				for(Node *u : v->leftNeighbours) {
					double sep = (v->r.width()+u->r.width())/2.0;
					constraints.push_back(new Constraint(u->v,v->v,sep));
					u->rightNeighbours.erase(v);
				}
				
				for(Node *u : v->rightNeighbours) {
					double sep = (v->r.width()+u->r.width())/2.0;
					constraints.push_back(new Constraint(v->v,u->v,sep));
					u->leftNeighbours.erase(v);
				}
			} else {
				Node *l=v->firstAbove, *r=v->firstBelow;
				if(l!=nullptr) {
					double sep = (v->r.width()+l->r.width())/2.0;
					constraints.push_back(new Constraint(l->v,v->v,sep));
					l->firstBelow=v->firstBelow;
				}
				if(r!=nullptr) {
					double sep = (v->r.width()+r->r.width())/2.0;
					constraints.push_back(new Constraint(v->v,r->v,sep));
					r->firstAbove=v->firstAbove;
				}
			}
			scanline.erase(v);
		}
	}
	int m =constraints.size();
	cs=new Constraint*[m];
	for(int i=0;i<m;i++) cs[i]=constraints[i];
	return m;
}

/**
 * Prepares constraints in order to apply VPSC vertically to remove ALL overlap.
 */
int generateYConstraints(const vector<Rectangle> &rs, Variable** vars,
	  Constraint** &cs) {

	vector<Event> events;
	events.reserve(2 * rs.size());
	for(size_t i=0;i<rs.size();i++) {
		vars[i]->desiredPosition=rs[i].getCentreY();
		auto v = std::make_shared<Node>(vars[i],rs[i],rs[i].getCentreY());
		events.emplace_back(Open,v,rs[i].getMinX());
		events.emplace_back(Close,v,rs[i].getMaxX());
	}
	std::sort(events.begin(), events.end(), compare_events);
	NodeSet scanline;
	vector<Constraint*> constraints;
	for(Event &e : events) {
		Node *v = e.v.get();
		if(e.type==Open) {
			scanline.insert(v);
			NodeSet::iterator i=scanline.find(v);
			if(i!=scanline.begin()) {
				Node *u=*--i;
				v->firstAbove=u;
				u->firstBelow=v;
			}
			i=scanline.find(v);
			if(++i!=scanline.end())	 {
				Node *u=*i;
				v->firstBelow=u;
				u->firstAbove=v;
			}
		} else {
			// Close event
			Node *l=v->firstAbove, *r=v->firstBelow;
			if(l!=nullptr) {
				double sep = (v->r.height()+l->r.height())/2.0;
				constraints.push_back(new Constraint(l->v,v->v,sep));
				l->firstBelow=v->firstBelow;
			}
			if(r!=nullptr) {
				double sep = (v->r.height()+r->r.height())/2.0;
				constraints.push_back(new Constraint(v->v,r->v,sep));
				r->firstAbove=v->firstAbove;
			}
			scanline.erase(v);
		}
	}
	int m =constraints.size();
	cs=new Constraint*[m];
	for(int i=0;i<m;i++) cs[i]=constraints[i];
	return m;
}
