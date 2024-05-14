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

#include <vector>
#include <list>
#include <vpsc/pairingheap/dsexceptions.h>
#include <vpsc/pairingheap/PairingHeap.h>

#ifndef PAIRING_HEAP_CPP
#define PAIRING_HEAP_CPP
using namespace std;
/**
* Construct the pairing heap.
*/
template <class T>
PairingHeap<T>::PairingHeap( bool (*lessThan)(T const &lhs, T const &rhs) )
{
	root = nullptr;
	this->lessThan=lessThan;
}


/**
* Destroy the leftist heap.
*/
template <class T>
PairingHeap<T>::~PairingHeap( )
{
	makeEmpty( );
}

/**
* Insert item x into the priority queue, maintaining heap order.
* Return a pointer to the node containing the new item.
*/
template <class T>
PairNode<T> *
PairingHeap<T>::insert( const T & x )
{
	PairNode<T> *newNode = new PairNode<T>( x );

	if( root == nullptr )
		root = newNode;
	else
		compareAndLink( root, newNode );
	return newNode;
}
/**
* Find the smallest item in the priority queue.
* Return the smallest item, or throw Underflow if empty.
*/
template <class T>
const T & PairingHeap<T>::findMin( ) const
{
	if( isEmpty( ) )
		throw Underflow( );
	return root->element;
}
/**
 * Remove the smallest item from the priority queue.
 * Throws Underflow if empty.
 */
template <class T>
void PairingHeap<T>::deleteMin( )
{
    if( isEmpty( ) )
        throw Underflow( );

    PairNode<T> *oldRoot = root;

    if( root->leftChild == nullptr )
        root = nullptr;
    else
        root = combineSiblings( root->leftChild );
    delete oldRoot;
}

/**
* Test if the priority queue is logically empty.
* Returns true if empty, false otherwise.
*/
template <class T>
bool PairingHeap<T>::isEmpty( ) const
{
	return root == nullptr;
}

/**
* Make the priority queue logically empty.
*/
template <class T>
void PairingHeap<T>::makeEmpty( )
{
	reclaimMemory( root );
	root = nullptr;
}

/**
* Internal method to make the tree empty.
* WARNING: This is prone to running out of stack space.
*/
template <class T>
void PairingHeap<T>::reclaimMemory( PairNode<T> * t ) const
{
	if( t != nullptr )
	{
		reclaimMemory( t->leftChild );
		reclaimMemory( t->nextSibling );
		delete t;
	}
}

/**
* Internal method that is the basic operation to maintain order.
* Links first and second together to satisfy heap order.
* first is root of tree 1, which may not be nullptr.
*    first->nextSibling MUST be nullptr on entry.
* second is root of tree 2, which may be nullptr.
* first becomes the result of the tree merge.
*/
template <class T>
void PairingHeap<T>::
compareAndLink( PairNode<T> * & first,
			   PairNode<T> *second ) const
{
	if( second == nullptr )
		return;
	if( lessThan(second->element,first->element) )
	{
		// Attach first as leftmost child of second
		second->prev = first->prev;
		first->prev = second;
		first->nextSibling = second->leftChild;
		if( first->nextSibling != nullptr )
			first->nextSibling->prev = first;
		second->leftChild = first;
		first = second;
	}
	else
	{
		// Attach second as leftmost child of first
		second->prev = first;
		first->nextSibling = second->nextSibling;
		if( first->nextSibling != nullptr )
			first->nextSibling->prev = first;
		second->nextSibling = first->leftChild;
		if( second->nextSibling != nullptr )
			second->nextSibling->prev = second;
		first->leftChild = second;
	}
}

/**
* Internal method that implements two-pass merging.
* firstSibling the root of the conglomerate;
*     assumed not nullptr.
*/
template <class T>
PairNode<T> *
PairingHeap<T>::combineSiblings( PairNode<T> *firstSibling ) const
{
	if( firstSibling->nextSibling == nullptr )
		return firstSibling;

	vector<PairNode<T>*> treeArray;

	// Store the subtrees in an array
	size_t numSiblings = 0;
	for( ; firstSibling != nullptr; numSiblings++ )
	{
		treeArray.push_back(firstSibling);
		firstSibling->prev->nextSibling = nullptr;  // break links
		firstSibling = firstSibling->nextSibling;
	}
	treeArray.push_back(nullptr);

	// Combine subtrees two at a time, going left to right
	size_t i = 0;
	for( ; i + 1 < numSiblings; i += 2 )
		compareAndLink( treeArray[ i ], treeArray[ i + 1 ] );

	size_t j = i - 2;

	// j has the result of last compareAndLink.
	// If an odd number of trees, get the last one.
	if (j + 3 == numSiblings)
		compareAndLink( treeArray[ j ], treeArray[ j + 2 ] );

	// Now go right to left, merging last tree with
	// next to last. The result becomes the new last.
	for( ; j >= 2; j -= 2 )
		compareAndLink( treeArray[ j - 2 ], treeArray[ j ] );
	return treeArray[ 0 ];
}

template <class T>
ostream& operator <<(ostream &os, const PairingHeap<T> &b)
{
	os<<"Heap:";
	if (b.root != nullptr) {
		PairNode<T> *r = b.root;
		list<PairNode<T>*> q;
		q.push_back(r);
		while (!q.empty()) {
			r = q.front();
			q.pop_front();
			if (r->leftChild != nullptr) {
				os << *r->element << ">";
				PairNode<T> *c = r->leftChild;
				while (c != nullptr) {
					q.push_back(c);
					os << "," << *c->element;
					c = c->nextSibling;
				}
				os << "|";
			}
		}
	}
    return os;
}
#endif
