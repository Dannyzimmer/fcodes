/**
 * \brief Bridge for C programs to access solve_VPSC (which is in C++)
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


#include <iostream>
#include <vpsc/variable.h>
#include <vpsc/constraint.h>
#include <vpsc/generate-constraints.h>
#include <vpsc/solve_VPSC.h>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <vpsc/csolve_VPSC.h>
Variable* newVariable(int id, double desiredPos, double weight) {
	return new Variable(id,desiredPos,weight);
}
Constraint* newConstraint(Variable* left, Variable* right, double gap) {
	return new Constraint(left,right,gap);
}
VPSC* newIncVPSC(int n, Variable* vs[], int m, Constraint* cs[]) {
	return new IncVPSC(n,vs,m,cs);
}

int genXConstraints(int n, boxf *bb, Variable **vs, Constraint ***cs,
                    bool transitiveClosure) {
	std::vector<Rectangle> rs;
	for(int i=0;i<n;i++) {
		rs.emplace_back(bb[i].LL.x,bb[i].UR.x,bb[i].LL.y,bb[i].UR.y);
	}
	const int m = generateXConstraints(rs, vs, *cs, transitiveClosure);
	return m;
}
int genYConstraints(int n, boxf* bb, Variable** vs, Constraint*** cs) {
	std::vector<Rectangle> rs;
	for(int i=0;i<n;i++) {
		rs.emplace_back(bb[i].LL.x,bb[i].UR.x,bb[i].LL.y,bb[i].UR.y);
	}
	const int m = generateYConstraints(rs, vs, *cs);
	return m;
}

Constraint** newConstraints(int m) {
	return new Constraint*[m];
}
void deleteConstraints(int m, Constraint **cs) {
	for(int i=0;i<m;i++) {
		delete cs[i];
	}
	delete [] cs;
}
void deleteConstraint(Constraint* c) {
	delete c;
}
void deleteVariable(Variable* v) {
	delete v;
}
void satisfyVPSC(VPSC* vpsc) {
	try {
		vpsc->satisfy();
	} catch(const char *e) {
		std::cerr << e << "\n";
		std::exit(1);
	}
}
void deleteVPSC(VPSC *vpsc) {
	assert(vpsc!=nullptr);
	delete vpsc;
}
void solveVPSC(VPSC* vpsc) {
	vpsc->solve();
}
void setVariableDesiredPos(Variable *v, double desiredPos) {
	v->desiredPosition = desiredPos;
}
double getVariablePos(const Variable *v) {
	return v->position();
}
void remapInConstraints(Variable *u, Variable *v, double dgap) {
	for (Constraint *c : u->in) {
		c->right=v;
		c->gap+=dgap;
		v->in.push_back(c);
	}
	u->in.clear();
}
void remapOutConstraints(Variable *u, Variable *v, double dgap) {
	for (Constraint *c : u->out) {
		c->left=v;
		c->gap+=dgap;
		v->out.push_back(c);
	}
	u->out.clear();
}
