/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <assert.h>
#include <cgraph/alloc.h>
#include <cgraph/bitarray.h>
#include <cgraph/prisize_t.h>
#include <limits.h>
#include <sparse/BinaryHeap.h>
#include <stdlib.h>

BinaryHeap BinaryHeap_new(int (*cmp)(void*item1, void*item2)){
  size_t max_len = 1<<8;

  BinaryHeap h = gv_alloc(sizeof(struct BinaryHeap_struct));
  h->max_len = max_len;
  h->len = 0;
  h->heap = gv_calloc(max_len, sizeof(h->heap[0]));
  h->id_to_pos = gv_calloc(max_len, sizeof(h->id_to_pos[0]));
  for (size_t i = 0; i < max_len; i++) h->id_to_pos[i] = SIZE_MAX;

  h->pos_to_id = gv_calloc(max_len, sizeof(h->pos_to_id[0]));
  h->id_stack = (int_stack_t){0};
  h->cmp = cmp;
  return h;
}


void BinaryHeap_delete(BinaryHeap h, void (*del)(void* item)){
  if (!h) return;
  free(h->id_to_pos);
  free(h->pos_to_id);
  int_stack_free(&h->id_stack);
  if (del) for (size_t i = 0; i < h->len; i++) del((h->heap)[i]);
  free(h->heap);
  free(h);
}

static void BinaryHeap_realloc(BinaryHeap h) {
  size_t max_len0 = h->max_len, max_len = h->max_len, i;
  max_len = max_len + MAX(max_len / 5, 10);
  h->max_len = max_len;

  h->heap = gv_recalloc(h->heap, max_len0, max_len, sizeof(h->heap[0]));
  h->id_to_pos = gv_recalloc(h->id_to_pos, max_len0, max_len, sizeof(h->id_to_pos[0]));
  h->pos_to_id = gv_recalloc(h->pos_to_id, max_len0, max_len, sizeof(h->pos_to_id[0]));

  for (i = max_len0; i < max_len; i++) h->id_to_pos[i] = SIZE_MAX;
}

#define ParentPos(pos) (pos - 1)/2
#define ChildrenPos1(pos) (2*(pos)+1)
#define ChildrenPos2(pos) (2*(pos)+2)

static void swap(BinaryHeap h, size_t parentPos, size_t nodePos){
  int parentID, nodeID;
  void *tmp;
  void **heap = h->heap;
  size_t *id_to_pos = h->id_to_pos;
  int *pos_to_id = h->pos_to_id;

  assert(parentPos < h->len);
  assert(nodePos < h->len);

  parentID = pos_to_id[parentPos];
  nodeID = pos_to_id[nodePos];

  tmp = heap[parentPos];
  heap[parentPos] = heap[nodePos];
  heap[nodePos] = tmp;

  pos_to_id[parentPos] = nodeID;
  id_to_pos[nodeID] = parentPos;

  pos_to_id[nodePos] = parentID;
  id_to_pos[parentID] = nodePos;

}

static size_t siftUp(BinaryHeap h, size_t nodePos){
  void **heap = h->heap;


  if (nodePos != 0) {
    size_t parentPos = ParentPos(nodePos);

    if ((h->cmp)(heap[parentPos], heap[nodePos]) == 1) {/* if smaller than parent, swap */
      swap(h, parentPos, nodePos);
      nodePos = siftUp(h, parentPos);
    }
  }
  return nodePos;
}

static size_t siftDown(BinaryHeap h, size_t nodePos){
  size_t childPos;

  void **heap = h->heap;


  size_t childPos1 = ChildrenPos1(nodePos);
  size_t childPos2 = ChildrenPos2(nodePos);
  assert(h->len > 0);
  if (childPos1 > h->len - 1) return nodePos;/* no child */
  if (childPos1 == h->len - 1) {
    childPos = childPos1;/* one child */
  } else {/* two child */
    if ((h->cmp)(heap[childPos1], heap[childPos2]) == 1){ /* pick the smaller of the two child */
      childPos = childPos2;
    } else {
      childPos = childPos1;
    }
  }

  if ((h->cmp)(heap[nodePos], heap[childPos]) == 1) {
    /* if larger than child, swap */
    swap(h, nodePos, childPos);
    nodePos = siftDown(h, childPos);
  } 

  return nodePos;
}

int BinaryHeap_insert(BinaryHeap h, void *item){
  size_t len = h->len;
  assert(len <= (size_t)INT_MAX);
  int id = (int)len;

  /* insert an item, and return its ID. This ID can be used later to extract the item */
  if (len > h->max_len - 1) {
    BinaryHeap_realloc(h);
  }
  
  // check if we have IDs in the stack to reuse
  if (!int_stack_is_empty(&h->id_stack)) {
    id = int_stack_pop(&h->id_stack);
  }

  h->heap[len] = item;
  h->id_to_pos[id] = len;
  h->pos_to_id[len] = id;

  (h->len)++;

  size_t pos = siftUp(h, len);
  assert(h->id_to_pos[id] == pos);
  assert(h->pos_to_id[pos] == id);
  (void)pos;

  return id;
}


void* BinaryHeap_get_min(BinaryHeap h){
  /* return the min item */
  return h->heap[0];
}

void* BinaryHeap_extract_min(BinaryHeap h){
  /* return and remove the min item */
  if (h->len == 0) return NULL;
  return BinaryHeap_extract_item(h, (h->pos_to_id)[0]);
}

void* BinaryHeap_extract_item(BinaryHeap h, int id){
  /* extract an item with ID out and delete it */
  void *item;
  size_t *id_to_pos = h->id_to_pos;

  if (id >= 0 && (size_t)id >= h->max_len) return NULL;

  size_t pos = id_to_pos[id];

  if (pos == SIZE_MAX) return NULL;

  assert(pos < h->len);

  item = (h->heap)[pos];

  int_stack_push(&h->id_stack, id);

  if (pos < h->len - 1){/* move the last item to occupy the position of extracted item */
    swap(h, pos, h->len - 1);
    (h->len)--;
    pos = siftUp(h, pos);
    pos = siftDown(h, pos);
  } else {
    (h->len)--;
  }
 
  (h->id_to_pos)[id] = SIZE_MAX;

  return item;

}

size_t BinaryHeap_reset(BinaryHeap h, int id, void *item){
  /* reset value of an item with specified id */

  if (id >= 0 && (size_t)id >= h->max_len) return SIZE_MAX;
  size_t pos = h->id_to_pos[id];
  if (pos == SIZE_MAX) return SIZE_MAX;

  h->heap[pos] = item;

  pos = siftUp(h, pos);

  pos = siftDown(h, pos);

  return pos;

}
void* BinaryHeap_get_item(BinaryHeap h, int id){
  /* get an item with ID out, without deleting */

  if (id >= 0 && (size_t)id >= h->max_len) return NULL;
  
  size_t pos = h->id_to_pos[id];
  
  if (pos == SIZE_MAX) return NULL;
  return (h->heap)[pos];
}

void BinaryHeap_print(BinaryHeap h, void (*pnt)(void*)){
  size_t k = 2;

  for (size_t i = 0; i < h->len; i++){
    pnt(h->heap[i]);
    fprintf(stderr, "(%d) ",(h->pos_to_id)[i]);
    if (i == k-2) {
      fprintf(stderr, "\n");
      k *= 2;
    }
  }
  fprintf(stderr, "\nSpare keys =");
  for (size_t i = 0; i < int_stack_size(&h->id_stack); i++) {
    fprintf(stderr, "%d(%" PRISIZE_T ") ", int_stack_get(&h->id_stack, i),
            h->id_to_pos[int_stack_get(&h->id_stack, i)]);
  }
  fprintf(stderr, "\n");
}
