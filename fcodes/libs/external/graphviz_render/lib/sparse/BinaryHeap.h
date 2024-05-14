/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#pragma once

#include <cgraph/list.h>
#include <sparse/general.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_LIST(int_stack, int)

/* binary heap code. 
   Caution: items inserted should be kept untouched, e.g., the value of the item should be kepted unchanged while the heap is still in use! 
   To change the valud of am item, use BinaryHeap_reset
*/

struct BinaryHeap_struct {
  size_t max_len;/* storage allocated for the heap */
  size_t len;/*number of elements in the heap so far. <= maxlen */
  void **heap;
  size_t *id_to_pos;/* this array store the position of an item with ID. For ID that was not in use,
		    i.e., no in pos_to_id[1:len],
		    id_to_pos[id] = SIZE_MAX
		  */
  int *pos_to_id;/* this array stores the ID of item at position pos. 
		    pos_to_id[id_to_pos[i]] = i, for i not in the id_stack & i < length(id_stack)+len
		    id_to_pos[pos_to_id[i]] = i, 0 <= i < len
		 */
  int_stack_t id_stack;/* a stack that stores IDs that are no longer used, to
		       be assigned to newly inserted elements.
		       For sanity check, the union of items in
		       the id_stack, and that is pos_to_id[1:len],
		       should form a set of contiguous numbers 
		       {1, 2, ..., len, ...}
		    */
  int (*cmp)(void*item1, void*item2);/* comparison function. Return 1,0,-1
				      if item1 >, =, < item2 */
};

typedef struct BinaryHeap_struct* BinaryHeap;

BinaryHeap BinaryHeap_new(int (*cmp)(void*item1, void*item2));


void BinaryHeap_delete(BinaryHeap h, void (*del)(void* item));/* delete not just the heap data structure, but also the data items
								       through a user supplied del function. */

int BinaryHeap_insert(BinaryHeap h, void *item);/* insert an item, and return its ID. 
						   This ID can be used later to extract the item. ID
						   are always nonnegative. If the return value is 
						   negative, it is an error message */

void* BinaryHeap_get_min(BinaryHeap h);/* return the min item */

void* BinaryHeap_extract_min(BinaryHeap h);/* return and remove the min item */

void* BinaryHeap_extract_item(BinaryHeap h, int id);/* extract an item with ID out and delete it */

void* BinaryHeap_get_item(BinaryHeap h, int id);/* get an item with ID out, without deleting */

size_t BinaryHeap_reset(BinaryHeap h, int id, void *item);/* reset value of an item with specified id. Return new pos. If ID is invalue, return SIZE_MAX */

void BinaryHeap_print(BinaryHeap h, void (*pnt)(void*));

#ifdef __cplusplus
}
#endif
