/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <cgraph/alloc.h>
#include <sparse/LinkedList.h>

SingleLinkedList SingleLinkedList_new(void *data){
  SingleLinkedList head = gv_alloc(sizeof(struct SingleLinkedList_struct));
  head->data = data;
  return head;
}

SingleLinkedList SingleLinkedList_new_int(int i){
  int *data = gv_alloc(sizeof(int));
  data[0] = i;
  return SingleLinkedList_new(data);
}
  

void SingleLinkedList_delete(SingleLinkedList head,  void (*linklist_deallocator)(void*)){
  SingleLinkedList next;

  if (!head) return;
  do {
    next = head->next;
    if (head->data) linklist_deallocator(head->data);
    free(head);
    head = next;
  } while (head);

}


SingleLinkedList SingleLinkedList_prepend(SingleLinkedList l, void *data){
  SingleLinkedList head = SingleLinkedList_new(data);
  head->next = l;
  return head;
}

SingleLinkedList SingleLinkedList_prepend_int(SingleLinkedList l, int i){
  int *data = gv_alloc(sizeof(int));
  data[0] = i;
  return SingleLinkedList_prepend(l, data);
}

void* SingleLinkedList_get_data(SingleLinkedList l){
  return l->data;
}

SingleLinkedList SingleLinkedList_get_next(SingleLinkedList l){
  return l->next;
}
void SingleLinkedList_print(SingleLinkedList head, void (*linkedlist_print)(void*)){

  if (!head) return;
  do {
    if (head->data) linkedlist_print(head->data);
    head = head->next;
  } while (head);
 
}
