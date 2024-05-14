/**
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

#include <vector>
#include <iostream>
class Block;
struct Constraint;
#include <vpsc/block.h>

typedef std::vector<Constraint*> Constraints;
struct Variable
{
	friend std::ostream& operator <<(std::ostream &os, const Variable &v);
public:
	const int id; // useful in log files
	double desiredPosition;
	const double weight;
	double offset;
	Block *block = nullptr;
	bool visited;
	Constraints in;
	Constraints out;
	char *toString();
	Variable(const int id, const double desiredPos, const double weight)
		: id(id)
		, desiredPosition(desiredPos)
		, weight(weight)
		, offset(0)
		, visited(false)
	{
	}
	double position() const {
		return block->posn+offset;
	}
};
