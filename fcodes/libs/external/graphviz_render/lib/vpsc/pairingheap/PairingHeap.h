/**
 * \brief Pairing heap datastructure implementation
 *
 * Based on example code in "Data structures and Algorithm Analysis in C++"
 * by Mark Allen Weiss, used and released under the LGPL by permission
 * of the author.
 *
 * No promises about correctness.  Use at your own risk!
 *
 * Authors:
 *   Mark Allen Weiss
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
#ifndef PAIRING_HEAP_H_
#define PAIRING_HEAP_H_
#include <stdlib.h>
#include <fstream>
// Pairing heap class
//
// CONSTRUCTION: with no parameters
//
// ******************PUBLIC OPERATIONS*********************
// PairNode & insert( x ) --> Insert x
// deleteMin( minItem )   --> Remove (and optionally return) smallest item
// T findMin( )  --> Return smallest item
// bool isEmpty( )        --> Return true if empty; else false
// void makeEmpty( )      --> Remove all items
// ******************ERRORS********************************
// Throws Underflow as warranted


// Node and forward declaration because g++ does
// not understand nested classes.
template <class T> 
class PairingHeap;

template <class T>
std::ostream& operator<< (std::ostream &os,const PairingHeap<T> &b);

template <class T>
class PairNode
{
	friend std::ostream& operator<< <T>(std::ostream &os,const PairingHeap<T> &b);
	T   element;
	PairNode    *leftChild;
	PairNode    *nextSibling;
	PairNode    *prev;

	PairNode( const T & theElement ) :
	       	element( theElement ),
		leftChild(nullptr), nextSibling(nullptr), prev(nullptr)
       	{ }
	friend class PairingHeap<T>;
};

template <class T>
class PairingHeap
{
	friend std::ostream& operator<< <T>(std::ostream &os,const PairingHeap<T> &b);
public:
	PairingHeap( bool (*lessThan)(T const &lhs, T const &rhs) );
	~PairingHeap( );

	bool isEmpty( ) const;

	PairNode<T> *insert( const T & x );
	const T & findMin( ) const;
	void deleteMin( );
	void makeEmpty( );
	void merge( PairingHeap<T> *rhs )
	{	
		PairNode<T> *broot=rhs->getRoot();
		if (root == nullptr) {
			if(broot != nullptr) {
				root = broot;
			}
		} else {
			compareAndLink(root, broot);
		}
	}

	PairingHeap(const PairingHeap &rhs) = delete;
	PairingHeap(PairingHeap &&rhs) = delete;
	PairingHeap &operator=(const PairingHeap &rhs) = delete;
	PairingHeap &operator=(PairingHeap &&rhs) = delete;
protected:
	PairNode<T> * getRoot() {
		PairNode<T> *r=root;
		root=nullptr;
		return r;
	}
private:
	PairNode<T> *root;
	bool (*lessThan)(T const &lhs, T const &rhs);
	void reclaimMemory( PairNode<T> *t ) const;
	void compareAndLink( PairNode<T> * & first, PairNode<T> *second ) const;
	PairNode<T> * combineSiblings( PairNode<T> *firstSibling ) const;
};

#include "PairingHeap.cpp"
#endif
