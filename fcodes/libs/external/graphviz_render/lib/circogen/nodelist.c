/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include	<cgraph/alloc.h>
#include	<circogen/nodelist.h>
#include	<circogen/circular.h>
#include	<assert.h>
#include	<limits.h>
#include	<stddef.h>
#include	<string.h>

nodelist_t *mkNodelist(void)
{
    nodelist_t *list = gv_alloc(sizeof(nodelist_t));
    return list;
}

void freeNodelist(nodelist_t * list)
{
    if (!list)
	return;

    nodelist_free(list);
    free(list);
}

/* appendNodelist:
 * Add node after one.
 */
void appendNodelist(nodelist_t *list, size_t one, Agnode_t *n) {
  assert(one < nodelist_size(list));

  // expand the list by one element
  nodelist_append(list, NULL);

  // shuffle everything past where we will insert
  size_t to_move = sizeof(node_t*) * (nodelist_size(list) - one - 2);
  if (to_move > 0) {
    memmove(nodelist_at(list, one + 2), nodelist_at(list, one + 1), to_move);
  }

  // insert the new node
  nodelist_set(list, one + 1, n);
}

/* reverseNodelist;
 * Destructively reverse a list.
 */
void reverseNodelist(nodelist_t *list) {
  for (size_t i = 0; i < nodelist_size(list) / 2; ++i) {
    node_t *temp = nodelist_get(list, i);
    nodelist_set(list, i, nodelist_get(list, nodelist_size(list) - i - 1));
    nodelist_set(list, nodelist_size(list) - i - 1, temp);
  }
}

/* realignNodelist:
 * Make np new front of list, with current last hooked to
 * current first.
 */
void realignNodelist(nodelist_t *list, size_t np) {
  assert(np < nodelist_size(list));
  for (size_t i = np; i != 0; --i) {
    // rotate the list by 1
    nodelist_append(list, nodelist_get(list, 0));
    size_t to_move = sizeof(node_t*) * (nodelist_size(list) - 1);
    if (to_move > 0) {
      memmove(nodelist_at(list, 0), nodelist_at(list, 1), to_move);
    }
    nodelist_resize(list, nodelist_size(list) - 1, NULL);
  }
}

/* cloneNodelist:
 * Create a copy of list.
 */
nodelist_t *cloneNodelist(nodelist_t * list)
{
    nodelist_t *newlist = mkNodelist();
    for (size_t i = 0; i < nodelist_size(list); ++i) {
      nodelist_append(newlist, nodelist_get(list, i));
    }
    return newlist;
}

/* insertNodelist:
 * Remove cn. Then, insert cn before neighbor if pos == 0 and 
 * after neighbor otherwise.
 */
void
insertNodelist(nodelist_t * list, Agnode_t * cn, Agnode_t * neighbor,
	       int pos)
{
  nodelist_remove(list, cn);

  for (size_t i = 0; i < nodelist_size(list); ++i) {
    Agnode_t *here = nodelist_get(list, i);
    if (here == neighbor) {
      if (pos == 0) {
        nodelist_append(list, NULL);
        size_t to_move = sizeof(node_t*) * (nodelist_size(list) - i - 1);
        if (to_move > 0) {
          memmove(nodelist_at(list, i + 1), nodelist_at(list, i), to_move);
        }
        nodelist_set(list, i, cn);
      } else {
        appendNodelist(list, i, cn);
      }
      break;
    }
  }
}

/* concatNodelist:
 * attach l2 to l1.
 */
static void concatNodelist(nodelist_t * l1, nodelist_t * l2)
{
  for (size_t i = 0; i < nodelist_size(l2); ++i) {
    nodelist_append(l1, nodelist_get(l2, i));
  }
}

/* reverse_append;
 * Create l1 @ (rev l2)
 * Destroys and frees l2.
 */
void reverseAppend(nodelist_t * l1, nodelist_t * l2)
{
    reverseNodelist(l2);
    concatNodelist(l1, l2);
    freeNodelist(l2);
}

#ifdef DEBUG
void printNodelist(nodelist_t * list)
{
    for (size_t i = 0; i < nodelist_size(list); ++i) {
      fprintf(stderr, "%s ", agnameof(nodelist_get(list, i)));
    }
    fputs("\n", stderr);
}
#endif
