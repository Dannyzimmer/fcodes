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
#pragma once
#include <iostream>
#include <vector>

class Rectangle {	
	friend std::ostream& operator <<(std::ostream &os, const Rectangle &r);
public:
	Rectangle(double x, double X, double y, double Y);
	double getMaxX() const { return maxX; }
	double getMaxY() const { return maxY; }
	double getMinX() const { return minX; }
	double getMinY() const { return minY; }
	double getMinD(unsigned const d) const {
		return ( d == 0 ? getMinX() : getMinY() );
	}
	double getMaxD(unsigned const d) const {
		return ( d == 0 ? getMaxX() : getMaxY() );
	}
	double getCentreX() const { return minX+width()/2.0; }
	double getCentreY() const { return minY+height()/2.0; }
	double width() const { return getMaxX()-minX; }
	double height() const { return getMaxY()-minY; }
	void moveCentreX(double x) {
		moveMinX(x-width()/2.0);
	}
	void moveCentreY(double y) {
		moveMinY(y-height()/2.0);
	}
	void moveMinX(double x) {
		maxX=x+width();
		minX=x;
	}
	void moveMinY(double y) {
		maxY=y+height();
		minY=y;
	}
	double overlapX(const Rectangle &r) const {
		if (getCentreX() <= r.getCentreX() && r.minX < getMaxX())
			return getMaxX() - r.minX;
		if (r.getCentreX() <= getCentreX() && minX < r.getMaxX())
			return r.getMaxX() - minX;
		return 0;
	}
	double overlapY(const Rectangle &r) const {
		if (getCentreY() <= r.getCentreY() && r.minY < getMaxY())
			return getMaxY() - r.minY;
		if (r.getCentreY() <= getCentreY() && minY < r.getMaxY())
			return r.getMaxY() - minY;
		return 0;
	}
private:
	double minX,maxX,minY,maxY;
};


struct Variable;
struct Constraint;

// returns number of constraints generated
int generateXConstraints(const std::vector<Rectangle> &rs, Variable** vars,
	Constraint** &cs, const bool useNeighbourLists);
int generateYConstraints(const std::vector<Rectangle> &rs, Variable** vars,
	Constraint** &cs);
