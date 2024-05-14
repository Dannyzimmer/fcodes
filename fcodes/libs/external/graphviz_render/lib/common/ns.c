/**
 * @file
 * @brief Network Simplex algorithm for ranking nodes of a DAG, @ref rank, @ref rank2
 */

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
#include <cgraph/prisize_t.h>
#include <common/render.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

static void dfs_cutval(node_t * v, edge_t * par);
static int dfs_range_init(node_t * v, edge_t * par, int low);
static int dfs_range(node_t * v, edge_t * par, int low);
static int x_val(edge_t * e, node_t * v, int dir);
#ifdef DEBUG
static void check_cycles(graph_t * g);
#endif

#define LENGTH(e)		(ND_rank(aghead(e)) - ND_rank(agtail(e)))
#define SLACK(e)		(LENGTH(e) - ED_minlen(e))
#define SEQ(a,b,c)		((a) <= (b) && (b) <= (c))
#define TREE_EDGE(e)	(ED_tree_index(e) >= 0)

static graph_t *G;
static size_t N_nodes, N_edges;
static int Maxrank;
static size_t S_i;			/* search index for enter_edge */
static int Search_size;
#define SEARCHSIZE 30
static nlist_t Tree_node;
static elist Tree_edge;

static int add_tree_edge(edge_t * e)
{
    node_t *n;
    //fprintf(stderr,"add tree edge %p %s ", (void*)e, agnameof(agtail(e))) ; fprintf(stderr,"%s\n", agnameof(aghead(e))) ;
    if (TREE_EDGE(e)) {
	agerr(AGERR, "add_tree_edge: missing tree edge\n");
	return -1;
    }
    assert(Tree_edge.size <= INT_MAX);
    ED_tree_index(e) = (int)Tree_edge.size;
    Tree_edge.list[Tree_edge.size++] = e;
    if (!ND_mark(agtail(e)))
	Tree_node.list[Tree_node.size++] = agtail(e);
    if (!ND_mark(aghead(e)))
	Tree_node.list[Tree_node.size++] = aghead(e);
    n = agtail(e);
    ND_mark(n) = TRUE;
    ND_tree_out(n).list[ND_tree_out(n).size++] = e;
    ND_tree_out(n).list[ND_tree_out(n).size] = NULL;
    if (ND_out(n).list[ND_tree_out(n).size - 1] == 0) {
	agerr(AGERR, "add_tree_edge: empty outedge list\n");
	return -1;
    }
    n = aghead(e);
    ND_mark(n) = TRUE;
    ND_tree_in(n).list[ND_tree_in(n).size++] = e;
    ND_tree_in(n).list[ND_tree_in(n).size] = NULL;
    if (ND_in(n).list[ND_tree_in(n).size - 1] == 0) {
	agerr(AGERR, "add_tree_edge: empty inedge list\n");
	return -1;
    }
    return 0;
}

/**
 * Invalidate DFS attributes by walking up the tree from to_node till lca
 * (inclusively). Called when updating tree to improve pruning in dfs_range().
 * Assigns ND_low(n) = -1 for the affected nodes.
 */
static void invalidate_path(node_t *lca, node_t *to_node) {
    while (true) {
        if (ND_low(to_node) == -1)
          break;

        ND_low(to_node) = -1;

        edge_t *e = ND_par(to_node);
        if (e == NULL)
          break;

        if (ND_lim(to_node) >= ND_lim(lca)) {
          if (to_node != lca)
            agerr(AGERR, "invalidate_path: skipped over LCA\n");
          break;
        }

        if (ND_lim(agtail(e)) > ND_lim(aghead(e)))
          to_node = agtail(e);
        else
          to_node = aghead(e);
    }
}

static void exchange_tree_edges(edge_t * e, edge_t * f)
{
    node_t *n;

    ED_tree_index(f) = ED_tree_index(e);
    Tree_edge.list[ED_tree_index(e)] = f;
    ED_tree_index(e) = -1;

    n = agtail(e);
    size_t i = --ND_tree_out(n).size;
    size_t j;
    for (j = 0; j <= i; j++)
	if (ND_tree_out(n).list[j] == e)
	    break;
    ND_tree_out(n).list[j] = ND_tree_out(n).list[i];
    ND_tree_out(n).list[i] = NULL;
    n = aghead(e);
    i = --ND_tree_in(n).size;
    for (j = 0; j <= i; j++)
	if (ND_tree_in(n).list[j] == e)
	    break;
    ND_tree_in(n).list[j] = ND_tree_in(n).list[i];
    ND_tree_in(n).list[i] = NULL;

    n = agtail(f);
    ND_tree_out(n).list[ND_tree_out(n).size++] = f;
    ND_tree_out(n).list[ND_tree_out(n).size] = NULL;
    n = aghead(f);
    ND_tree_in(n).list[ND_tree_in(n).size++] = f;
    ND_tree_in(n).list[ND_tree_in(n).size] = NULL;
}

static
void init_rank(void)
{
    int i;
    nodequeue *Q;
    node_t *v;
    edge_t *e;

    Q = new_queue(N_nodes);
    size_t ctr = 0;

    for (v = GD_nlist(G); v; v = ND_next(v)) {
	if (ND_priority(v) == 0)
	    enqueue(Q, v);
    }

    while ((v = dequeue(Q))) {
	ND_rank(v) = 0;
	ctr++;
	for (i = 0; (e = ND_in(v).list[i]); i++)
	    ND_rank(v) = MAX(ND_rank(v), ND_rank(agtail(e)) + ED_minlen(e));
	for (i = 0; (e = ND_out(v).list[i]); i++) {
	    if (--ND_priority(aghead(e)) <= 0)
		enqueue(Q, aghead(e));
	}
    }
    if (ctr != N_nodes) {
	agerr(AGERR, "trouble in init_rank\n");
	for (v = GD_nlist(G); v; v = ND_next(v))
	    if (ND_priority(v))
		agerr(AGPREV, "\t%s %d\n", agnameof(v), ND_priority(v));
    }
    free_queue(Q);
}

static edge_t *leave_edge(void)
{
    edge_t *f, *rv = NULL;
    int cnt = 0;

    size_t j = S_i;
    while (S_i < Tree_edge.size) {
	if (ED_cutvalue(f = Tree_edge.list[S_i]) < 0) {
	    if (rv) {
		if (ED_cutvalue(rv) > ED_cutvalue(f))
		    rv = f;
	    } else
		rv = Tree_edge.list[S_i];
	    if (++cnt >= Search_size)
		return rv;
	}
	S_i++;
    }
    if (j > 0) {
	S_i = 0;
	while (S_i < j) {
	    if (ED_cutvalue(f = Tree_edge.list[S_i]) < 0) {
		if (rv) {
		    if (ED_cutvalue(rv) > ED_cutvalue(f))
			rv = f;
		} else
		    rv = Tree_edge.list[S_i];
		if (++cnt >= Search_size)
		    return rv;
	    }
	    S_i++;
	}
    }
    return rv;
}

static edge_t *Enter;
static int Low, Lim, Slack;

static void dfs_enter_outedge(node_t * v)
{
    int i, slack;
    edge_t *e;

    for (i = 0; (e = ND_out(v).list[i]); i++) {
	if (!TREE_EDGE(e)) {
	    if (!SEQ(Low, ND_lim(aghead(e)), Lim)) {
		slack = SLACK(e);
		if (slack < Slack || Enter == NULL) {
		    Enter = e;
		    Slack = slack;
		}
	    }
	} else if (ND_lim(aghead(e)) < ND_lim(v))
	    dfs_enter_outedge(aghead(e));
    }
    for (i = 0; (e = ND_tree_in(v).list[i]) && (Slack > 0); i++)
	if (ND_lim(agtail(e)) < ND_lim(v))
	    dfs_enter_outedge(agtail(e));
}

static void dfs_enter_inedge(node_t * v)
{
    int i, slack;
    edge_t *e;

    for (i = 0; (e = ND_in(v).list[i]); i++) {
	if (!TREE_EDGE(e)) {
	    if (!SEQ(Low, ND_lim(agtail(e)), Lim)) {
		slack = SLACK(e);
		if (slack < Slack || Enter == NULL) {
		    Enter = e;
		    Slack = slack;
		}
	    }
	} else if (ND_lim(agtail(e)) < ND_lim(v))
	    dfs_enter_inedge(agtail(e));
    }
    for (i = 0; (e = ND_tree_out(v).list[i]) && Slack > 0; i++)
	if (ND_lim(aghead(e)) < ND_lim(v))
	    dfs_enter_inedge(aghead(e));
}

static edge_t *enter_edge(edge_t * e)
{
    node_t *v;
    int outsearch;

    /* v is the down node */
    if (ND_lim(agtail(e)) < ND_lim(aghead(e))) {
	v = agtail(e);
	outsearch = FALSE;
    } else {
	v = aghead(e);
	outsearch = TRUE;
    }
    Enter = NULL;
    Slack = INT_MAX;
    Low = ND_low(v);
    Lim = ND_lim(v);
    if (outsearch)
	dfs_enter_outedge(v);
    else
	dfs_enter_inedge(v);
    return Enter;
}

static void init_cutvalues(void)
{
    dfs_range_init(GD_nlist(G), NULL, 1);
    dfs_cutval(GD_nlist(G), NULL);
}

/* functions for initial tight tree construction */
// borrow field from network simplex - overwritten in init_cutvalues() forgive me
#define ND_subtree(n) (subtree_t*)ND_par(n)
#define ND_subtree_set(n,value) (ND_par(n) = (edge_t*)value)

typedef struct subtree_s {
        node_t *rep;            /* some node in the tree */
        int    size;            /* total tight tree size */
        int    heap_index;      /* required to find non-min elts when merged */
        struct subtree_s *par;  /* union find */
} subtree_t;

/* find initial tight subtrees */
static int tight_subtree_search(Agnode_t *v, subtree_t *st)
{
    Agedge_t *e;
    int     i;
    int     rv;

    rv = 1;
    ND_subtree_set(v,st);
    for (i = 0; (e = ND_in(v).list[i]); i++) {
        if (TREE_EDGE(e)) continue;
        if (ND_subtree(agtail(e)) == 0 && SLACK(e) == 0) {
               if (add_tree_edge(e) != 0) {
                   return -1;
               }
               rv += tight_subtree_search(agtail(e),st);
        }
    }
    for (i = 0; (e = ND_out(v).list[i]); i++) {
        if (TREE_EDGE(e)) continue;
        if (ND_subtree(aghead(e)) == 0 && SLACK(e) == 0) {
               if (add_tree_edge(e) != 0) {
                   return -1;
               }
               rv += tight_subtree_search(aghead(e),st);
        }
    }
    return rv;
}

static subtree_t *find_tight_subtree(Agnode_t *v)
{
    subtree_t       *rv;
    rv = gv_alloc(sizeof(subtree_t));
    rv->rep = v;
    rv->size = tight_subtree_search(v,rv);
    if (rv->size < 0) {
        free(rv);
        return NULL;
    }
    rv->par = rv;
    return rv;
}

typedef struct STheap_s {
        subtree_t       **elt;
        int             size;
} STheap_t;

static subtree_t *STsetFind(Agnode_t *n0)
{
  subtree_t *s0 = ND_subtree(n0);
  while  (s0->par && s0->par != s0) {
    if (s0->par->par) {s0->par = s0->par->par;}  /* path compression for the code weary */
    s0 = s0->par;
  }
  return s0;
}
 
static subtree_t *STsetUnion(subtree_t *s0, subtree_t *s1)
{
  subtree_t *r0, *r1, *r;

  for (r0 = s0; r0->par && r0->par != r0; r0 = r0->par);
  for (r1 = s1; r1->par && r1->par != r1; r1 = r1->par);
  if (r0 == r1) return r0;  /* safety code but shouldn't happen */
  assert(r0->heap_index > -1 || r1->heap_index > -1);
  if (r1->heap_index == -1) r = r0;
  else if (r0->heap_index == -1) r = r1;
  else if (r1->size < r0->size) r = r0;
  else r = r1;

  r0->par = r1->par = r;
  r->size = r0->size + r1->size;
  assert(r->heap_index >= 0);
  return r;
}

/* find tightest edge to another tree incident on the given tree */
static Agedge_t *inter_tree_edge_search(Agnode_t *v, Agnode_t *from, Agedge_t *best)
{
    int i;
    Agedge_t *e;
    subtree_t *ts = STsetFind(v);
    if (best && SLACK(best) == 0) return best;
    for (i = 0; (e = ND_out(v).list[i]); i++) {
      if (TREE_EDGE(e)) {
          if (aghead(e) == from) continue;  // do not search back in tree
          best = inter_tree_edge_search(aghead(e),v,best); // search forward in tree
      }
      else {
        if (STsetFind(aghead(e)) != ts) {   // encountered candidate edge
          if (best == 0 || SLACK(e) < SLACK(best)) best = e;
        }
        /* else ignore non-tree edge between nodes in the same tree */
      }
    }
    /* the following code must mirror the above, but for in-edges */
    for (i = 0; (e = ND_in(v).list[i]); i++) {
      if (TREE_EDGE(e)) {
          if (agtail(e) == from) continue;
          best = inter_tree_edge_search(agtail(e),v,best);
      }
      else {
        if (STsetFind(agtail(e)) != ts) {
          if (best == 0 || SLACK(e) < SLACK(best)) best = e;
        }
      }
    }
    return best;
}

static Agedge_t *inter_tree_edge(subtree_t *tree)
{
    Agedge_t *rv;
    rv = inter_tree_edge_search(tree->rep, NULL, NULL);
    return rv;
}

static
int STheapsize(STheap_t *heap) { return heap->size; }

static 
void STheapify(STheap_t *heap, int i)
{
    int left, right, smallest;
    subtree_t **elt = heap->elt;
    do {
        left = 2*(i+1)-1;
        right = 2*(i+1);
        if (left < heap->size && elt[left]->size < elt[i]->size) smallest = left;
        else smallest = i;
        if (right < heap->size && elt[right]->size < elt[smallest]->size) smallest = right;
        else smallest = i;
        if (smallest != i) {
            subtree_t *temp;
            temp = elt[i];
            elt[i] = elt[smallest];
            elt[smallest] = temp;
            elt[i]->heap_index = i;
            elt[smallest]->heap_index = smallest;
            i = smallest;
        }
        else break;
    } while (i < heap->size);
}

static
STheap_t *STbuildheap(subtree_t **elt, int size)
{
    int     i;
    STheap_t *heap = gv_alloc(sizeof(STheap_t));
    heap->elt = elt;
    heap->size = size;
    for (i = 0; i < heap->size; i++) heap->elt[i]->heap_index = i;
    for (i = heap->size/2; i >= 0; i--)
        STheapify(heap,i);
    return heap;
}

static
subtree_t *STextractmin(STheap_t *heap)
{
    subtree_t *rv;
    rv = heap->elt[0];
    rv->heap_index = -1;
    heap->elt[0] = heap->elt[heap->size - 1];
    heap->elt[0]->heap_index = 0;
    heap->elt[heap->size -1] = rv;    /* needed to free storage later */
    heap->size--;
    STheapify(heap,0);
    return rv;
}

static
void tree_adjust(Agnode_t *v, Agnode_t *from, int delta)
{
    int i;
    Agedge_t *e;
    Agnode_t *w;
    ND_rank(v) = ND_rank(v) + delta;
    for (i = 0; (e = ND_tree_in(v).list[i]); i++) {
      w = agtail(e);
      if (w != from)
        tree_adjust(w, v, delta);
    }
    for (i = 0; (e = ND_tree_out(v).list[i]); i++) {
      w = aghead(e);
      if (w != from)
        tree_adjust(w, v, delta);
    }
}

static
subtree_t *merge_trees(Agedge_t *e)   /* entering tree edge */
{
  int       delta;
  subtree_t *t0, *t1, *rv;

  assert(!TREE_EDGE(e));

  t0 = STsetFind(agtail(e));
  t1 = STsetFind(aghead(e));

  //fprintf(stderr,"merge trees of %d %d of %d, delta %d\n",t0->size,t1->size,N_nodes,delta);

  if (t0->heap_index == -1) {   // move t0
    delta = SLACK(e);
    if (delta != 0)
      tree_adjust(t0->rep,NULL,delta);
  }
  else {  // move t1
    delta = -SLACK(e);
    if (delta != 0)
      tree_adjust(t1->rep,NULL,delta);
  }
  if (add_tree_edge(e) != 0) {
    return NULL;
  }
  rv = STsetUnion(t0,t1);
  
  return rv;
}

/* Construct initial tight tree. Graph must be connected, feasible.
 * Adjust ND_rank(v) as needed.  add_tree_edge() on tight tree edges.
 * trees are basically lists of nodes stored in nodequeues.
 * Return 1 if input graph is not connected; 0 on success.
 */
static
int feasible_tree(void)
{
  Agnode_t *n;
  Agedge_t *ee;
  subtree_t **tree, *tree0, *tree1;
  int i, subtree_count = 0;
  STheap_t *heap = NULL;
  int error = 0;

  /* initialization */
  for (n = GD_nlist(G); n; n = ND_next(n)) {
      ND_subtree_set(n,0);
  }

  tree = N_NEW(N_nodes,subtree_t*);
  /* given init_rank, find all tight subtrees */
  for (n = GD_nlist(G); n; n = ND_next(n)) {
        if (ND_subtree(n) == 0) {
                tree[subtree_count] = find_tight_subtree(n);
                if (tree[subtree_count] == NULL) {
                    error = 2;
                    goto end;
                }
                subtree_count++;
        }
  }

  /* incrementally merge subtrees */
  heap = STbuildheap(tree,subtree_count);
  while (STheapsize(heap) > 1) {
    tree0 = STextractmin(heap);
    if (!(ee = inter_tree_edge(tree0))) {
      error = 1;
      break;
    }
    tree1 = merge_trees(ee);
    if (tree1 == NULL) {
      error = 2;
      break;
    }
    STheapify(heap,tree1->heap_index);
  }

end:
  free(heap);
  for (i = 0; i < subtree_count; i++) free(tree[i]);
  free(tree);
  if (error) return error;
  assert(Tree_edge.size == N_nodes - 1);
  init_cutvalues();
  return 0;
}

/* walk up from v to LCA(v,w), setting new cutvalues. */
static Agnode_t *treeupdate(Agnode_t * v, Agnode_t * w, int cutvalue, int dir)
{
    edge_t *e;
    int d;

    while (!SEQ(ND_low(v), ND_lim(w), ND_lim(v))) {
	e = ND_par(v);
	if (v == agtail(e))
	    d = dir;
	else
	    d = !dir;
	if (d)
	    ED_cutvalue(e) += cutvalue;
	else
	    ED_cutvalue(e) -= cutvalue;
	if (ND_lim(agtail(e)) > ND_lim(aghead(e)))
	    v = agtail(e);
	else
	    v = aghead(e);
    }
    return v;
}

static void rerank(Agnode_t * v, int delta)
{
    int i;
    edge_t *e;

    ND_rank(v) -= delta;
    for (i = 0; (e = ND_tree_out(v).list[i]); i++)
	if (e != ND_par(v))
	    rerank(aghead(e), delta);
    for (i = 0; (e = ND_tree_in(v).list[i]); i++)
	if (e != ND_par(v))
	    rerank(agtail(e), delta);
}

/* e is the tree edge that is leaving and f is the nontree edge that
 * is entering.  compute new cut values, ranks, and exchange e and f.
 */
static int
update(edge_t * e, edge_t * f)
{
    int cutvalue, delta;
    Agnode_t *lca;

    delta = SLACK(f);
    /* "for (v = in nodes in tail side of e) do ND_rank(v) -= delta;" */
    if (delta > 0) {
	size_t s = ND_tree_in(agtail(e)).size + ND_tree_out(agtail(e)).size;
	if (s == 1)
	    rerank(agtail(e), delta);
	else {
	    s = ND_tree_in(aghead(e)).size + ND_tree_out(aghead(e)).size;
	    if (s == 1)
		rerank(aghead(e), -delta);
	    else {
		if (ND_lim(agtail(e)) < ND_lim(aghead(e)))
		    rerank(agtail(e), delta);
		else
		    rerank(aghead(e), -delta);
	    }
	}
    }

    cutvalue = ED_cutvalue(e);
    lca = treeupdate(agtail(f), aghead(f), cutvalue, 1);
    if (treeupdate(aghead(f), agtail(f), cutvalue, 0) != lca) {
	agerr(AGERR, "update: mismatched lca in treeupdates\n");
	return 2;
    }

    // invalidate paths from LCA till affected nodes:
    int lca_low = ND_low(lca);
    invalidate_path(lca, aghead(f));
    invalidate_path(lca, agtail(f));

    ED_cutvalue(f) = -cutvalue;
    ED_cutvalue(e) = 0;
    exchange_tree_edges(e, f);
    dfs_range(lca, ND_par(lca), lca_low);
    return 0;
}

static void scan_and_normalize(void)
{
    node_t *n;

    int Minrank = INT_MAX;
    Maxrank = -INT_MAX;
    for (n = GD_nlist(G); n; n = ND_next(n)) {
	if (ND_node_type(n) == NORMAL) {
	    Minrank = MIN(Minrank, ND_rank(n));
	    Maxrank = MAX(Maxrank, ND_rank(n));
	}
    }
    for (n = GD_nlist(G); n; n = ND_next(n))
	ND_rank(n) -= Minrank;
    Maxrank -= Minrank;
}

static void
freeTreeList (graph_t* g)
{
    node_t *n;
    for (n = GD_nlist(g); n; n = ND_next(n)) {
	free_list(ND_tree_in(n));
	free_list(ND_tree_out(n));
	ND_mark(n) = FALSE;
    }
}

static void LR_balance(void)
{
    int delta;
    edge_t *e, *f;

    for (size_t i = 0; i < Tree_edge.size; i++) {
	e = Tree_edge.list[i];
	if (ED_cutvalue(e) == 0) {
	    f = enter_edge(e);
	    if (f == NULL)
		continue;
	    delta = SLACK(f);
	    if (delta <= 1)
		continue;
	    if (ND_lim(agtail(e)) < ND_lim(aghead(e)))
		rerank(agtail(e), delta / 2);
	    else
		rerank(aghead(e), -delta / 2);
	}
    }
    freeTreeList (G);
}

static int decreasingrankcmpf(node_t **n0, node_t **n1) {
  if (ND_rank(*n1) < ND_rank(*n0)) {
    return -1;
  }
  if (ND_rank(*n1) > ND_rank(*n0)) {
    return 1;
  }
  return 0;
}

static int increasingrankcmpf(node_t **n0, node_t **n1) {
  if (ND_rank(*n0) < ND_rank(*n1)) {
    return -1;
  }
  if (ND_rank(*n0) > ND_rank(*n1)) {
    return 1;
  }
  return 0;
}

static void TB_balance(void)
{
    node_t *n;
    edge_t *e;
    int low, high, choice, *nrank;
    int inweight, outweight;
    int adj = 0;
    char *s;

    scan_and_normalize();

    /* find nodes that are not tight and move to less populated ranks */
    nrank = N_NEW(Maxrank + 1, int);
    for (int i = 0; i <= Maxrank; i++)
	nrank[i] = 0;
    if ( (s = agget(G,"TBbalance")) ) {
         if (streq(s,"min")) adj = 1;
         else if (streq(s,"max")) adj = 2;
         if (adj) for (n = GD_nlist(G); n; n = ND_next(n))
              if (ND_node_type(n) == NORMAL) {
                if (ND_in(n).size == 0 && adj == 1) {
                   ND_rank(n) = 0;
                }
                if (ND_out(n).size == 0 && adj == 2) {
                   ND_rank(n) = Maxrank;
                }
              }
    }
    size_t ii;
    for (ii = 0, n = GD_nlist(G); n; ii++, n = ND_next(n)) {
      Tree_node.list[ii] = n;
    }
    Tree_node.size = ii;
    qsort(Tree_node.list, Tree_node.size, sizeof(Tree_node.list[0]),
        adj > 1? (int(*)(const void*,const void*))decreasingrankcmpf
               : (int(*)(const void*,const void*))increasingrankcmpf);
    for (size_t i = 0; i < Tree_node.size; i++) {
        n = Tree_node.list[i];
        if (ND_node_type(n) == NORMAL)
          nrank[ND_rank(n)]++;
    }
    for (ii = 0; ii < Tree_node.size; ii++) {
      n = Tree_node.list[ii];
      if (ND_node_type(n) != NORMAL)
        continue;
      inweight = outweight = 0;
      low = 0;
      high = Maxrank;
      for (size_t i = 0; (e = ND_in(n).list[i]); i++) {
        inweight += ED_weight(e);
        low = MAX(low, ND_rank(agtail(e)) + ED_minlen(e));
      }
      for (size_t i = 0; (e = ND_out(n).list[i]); i++) {
        outweight += ED_weight(e);
        high = MIN(high, ND_rank(aghead(e)) - ED_minlen(e));
      }
      if (low < 0)
        low = 0;		/* vnodes can have ranks < 0 */
      if (adj) {
        if (inweight == outweight)
            ND_rank(n) = (adj == 1? low : high);
      }
      else {
                if (inweight == outweight) {
                    choice = low;
                    for (int i = low + 1; i <= high; i++)
                        if (nrank[i] < nrank[choice])
                            choice = i;
                    nrank[ND_rank(n)]--;
                    nrank[choice]++;
                    ND_rank(n) = choice;
                }
      }
      free_list(ND_tree_in(n));
      free_list(ND_tree_out(n));
      ND_mark(n) = FALSE;
    }
    free(nrank);
}

static bool init_graph(graph_t *g) {
    node_t *n;
    edge_t *e;

    G = g;
    N_nodes = N_edges = S_i = 0;
    for (n = GD_nlist(g); n; n = ND_next(n)) {
	ND_mark(n) = FALSE;
	N_nodes++;
	for (size_t i = 0; (e = ND_out(n).list[i]); i++)
	    N_edges++;
    }

    Tree_node.list = ALLOC(N_nodes, Tree_node.list, node_t *);
    Tree_node.size = 0;
    Tree_edge.list = ALLOC(N_nodes, Tree_edge.list, edge_t *);
    Tree_edge.size = 0;

    bool feasible = true;
    for (n = GD_nlist(g); n; n = ND_next(n)) {
	ND_priority(n) = 0;
	size_t i;
	for (i = 0; (e = ND_in(n).list[i]); i++) {
	    ND_priority(n)++;
	    ED_cutvalue(e) = 0;
	    ED_tree_index(e) = -1;
	    if (ND_rank(aghead(e)) - ND_rank(agtail(e)) < ED_minlen(e))
		feasible = false;
	}
	ND_tree_in(n).list = N_NEW(i + 1, edge_t *);
	ND_tree_in(n).size = 0;
	for (i = 0; (e = ND_out(n).list[i]); i++);
	ND_tree_out(n).list = N_NEW(i + 1, edge_t *);
	ND_tree_out(n).size = 0;
    }
    return feasible;
}

/* graphSize:
 * Compute no. of nodes and edges in the graph
 */
static void
graphSize (graph_t * g, int* nn, int* ne)
{
    int i, nnodes, nedges;
    node_t *n;
    edge_t *e;
   
    nnodes = nedges = 0;
    for (n = GD_nlist(g); n; n = ND_next(n)) {
	nnodes++;
	for (i = 0; (e = ND_out(n).list[i]); i++) {
	    nedges++;
	}
    }
    *nn = nnodes;
    *ne = nedges;
}

/* rank:
 * Apply network simplex to rank the nodes in a graph.
 * Uses ED_minlen as the internode constraint: if a->b with minlen=ml,
 * rank b - rank a >= ml.
 * Assumes the graph has the following additional structure:
 *   A list of all nodes, starting at GD_nlist, and linked using ND_next.
 *   Out and in edges lists stored in ND_out and ND_in, even if the node
 *  doesn't have any out or in edges.
 * The node rank values are stored in ND_rank.
 * Returns 0 if successful; returns 1 if the graph was not connected;
 * returns 2 if something seriously wrong;
 */
int rank2(graph_t * g, int balance, int maxiter, int search_size)
{
    int iter = 0;
    char *ns = "network simplex: ";
    edge_t *e, *f;

#ifdef DEBUG
    check_cycles(g);
#endif
    if (Verbose) {
	int nn, ne;
	graphSize (g, &nn, &ne);
	fprintf(stderr, "%s %d nodes %d edges maxiter=%d balance=%d\n", ns,
	    nn, ne, maxiter, balance);
	start_timer();
    }
    bool feasible = init_graph(g);
    if (!feasible)
	init_rank();

    if (search_size >= 0)
	Search_size = search_size;
    else
	Search_size = SEARCHSIZE;

    {
	int err = feasible_tree();
	if (err != 0) {
	    freeTreeList (g);
	    return err;
	}
    }
    if (maxiter <= 0) {
	freeTreeList (g);
	return 0;
    }

    while ((e = leave_edge())) {
	int err;
	f = enter_edge(e);
	err = update(e, f);
	if (err != 0) {
	    freeTreeList (g);
	    return err;
	}
	iter++;
	if (Verbose && iter % 100 == 0) {
	    if (iter % 1000 == 100)
		fputs(ns, stderr);
	    fprintf(stderr, "%d ", iter);
	    if (iter % 1000 == 0)
		fputc('\n', stderr);
	}
	if (iter >= maxiter)
	    break;
    }
    switch (balance) {
    case 1:
	TB_balance();
	break;
    case 2:
	LR_balance();
	break;
    default:
	scan_and_normalize();
	freeTreeList (G);
	break;
    }
    if (Verbose) {
	if (iter >= 100)
	    fputc('\n', stderr);
	fprintf(stderr, "%s%" PRISIZE_T " nodes %" PRISIZE_T " edges %d iter %.2f sec\n",
		ns, N_nodes, N_edges, iter, elapsed_sec());
    }
    return 0;
}

int rank(graph_t * g, int balance, int maxiter)
{
    char *s;
    int search_size;

    if ((s = agget(g, "searchsize")))
	search_size = atoi(s);
    else
	search_size = SEARCHSIZE;

    return rank2 (g, balance, maxiter, search_size);
}

/* set cut value of f, assuming values of edges on one side were already set */
static void x_cutval(edge_t * f)
{
    node_t *v;
    edge_t *e;
    int i, sum, dir;

    /* set v to the node on the side of the edge already searched */
    if (ND_par(agtail(f)) == f) {
	v = agtail(f);
	dir = 1;
    } else {
	v = aghead(f);
	dir = -1;
    }

    sum = 0;
    for (i = 0; (e = ND_out(v).list[i]); i++)
	sum += x_val(e, v, dir);
    for (i = 0; (e = ND_in(v).list[i]); i++)
	sum += x_val(e, v, dir);
    ED_cutvalue(f) = sum;
}

static int x_val(edge_t * e, node_t * v, int dir)
{
    node_t *other;
    int d, rv, f;

    if (agtail(e) == v)
	other = aghead(e);
    else
	other = agtail(e);
    if (!(SEQ(ND_low(v), ND_lim(other), ND_lim(v)))) {
	f = 1;
	rv = ED_weight(e);
    } else {
	f = 0;
	if (TREE_EDGE(e))
	    rv = ED_cutvalue(e);
	else
	    rv = 0;
	rv -= ED_weight(e);
    }
    if (dir > 0) {
	if (aghead(e) == v)
	    d = 1;
	else
	    d = -1;
    } else {
	if (agtail(e) == v)
	    d = 1;
	else
	    d = -1;
    }
    if (f)
	d = -d;
    if (d < 0)
	rv = -rv;
    return rv;
}

static void dfs_cutval(node_t * v, edge_t * par)
{
    int i;
    edge_t *e;

    for (i = 0; (e = ND_tree_out(v).list[i]); i++)
	if (e != par)
	    dfs_cutval(aghead(e), e);
    for (i = 0; (e = ND_tree_in(v).list[i]); i++)
	if (e != par)
	    dfs_cutval(agtail(e), e);
    if (par)
	x_cutval(par);
}

/*
* Initializes DFS range attributes (par, low, lim) over tree nodes such that:
* ND_par(n) - parent tree edge
* ND_low(n) - min DFS index for nodes in sub-tree (>= 1)
* ND_lim(n) - max DFS index for nodes in sub-tree
*/
static int dfs_range_init(node_t *v, edge_t *par, int low) {
    int i, lim;

    lim = low;
    ND_par(v) = par;
    ND_low(v) = low;

    for (i = 0; ND_tree_out(v).list[i]; i++) {
        edge_t *e = ND_tree_out(v).list[i];
        if (e != par) {
            lim = dfs_range_init(aghead(e), e, lim);
        }
    }

    for (i = 0; ND_tree_in(v).list[i]; i++) {
        edge_t *e = ND_tree_in(v).list[i];
        if (e != par) {
            lim = dfs_range_init(agtail(e), e, lim);
        }
    }

    ND_lim(v) = lim;

    return lim + 1;
}

/*
 * Incrementally updates DFS range attributes
 */
static int dfs_range(node_t * v, edge_t * par, int low)
{
    edge_t *e;
    int i, lim;

    if (ND_par(v) == par && ND_low(v) == low) {
	return ND_lim(v) + 1;
    }

    lim = low;
    ND_par(v) = par;
    ND_low(v) = low;
    for (i = 0; (e = ND_tree_out(v).list[i]); i++)
	if (e != par)
	    lim = dfs_range(aghead(e), e, lim);
    for (i = 0; (e = ND_tree_in(v).list[i]); i++)
	if (e != par)
	    lim = dfs_range(agtail(e), e, lim);
    ND_lim(v) = lim;
    return lim + 1;
}

#ifdef DEBUG
void tchk(void)
{
    int i, n_cnt, e_cnt;
    node_t *n;
    edge_t *e;

    n_cnt = 0;
    e_cnt = 0;
    for (n = agfstnode(G); n; n = agnxtnode(G, n)) {
	n_cnt++;
	for (i = 0; (e = ND_tree_out(n).list[i]); i++) {
	    e_cnt++;
	    if (SLACK(e) > 0)
		fprintf(stderr, "not a tight tree %p", e);
	}
    }
    if (n_cnt != Tree_node.size || e_cnt != Tree_edge.size)
	fprintf(stderr, "something missing\n");
}

void check_fast_node(node_t * n)
{
    node_t *nptr;
    nptr = GD_nlist(agraphof(n));
    while (nptr && nptr != n)
	nptr = ND_next(nptr);
    assert(nptr != NULL);
}

static char* dump_node (node_t* n)
{
    static char buf[50];

    if (ND_node_type(n)) {
	snprintf(buf, sizeof(buf), "%p", n);
	return buf;
    }
    else
	return agnameof(n);
}

static void dump_graph (graph_t* g)
{
    int i;
    edge_t *e;
    node_t *n,*w;
    FILE* fp = fopen ("ns.gv", "w");
    fprintf (fp, "digraph \"%s\" {\n", agnameof(g));
    for (n = GD_nlist(g); n; n = ND_next(n)) {
	fprintf (fp, "  \"%s\"\n", dump_node(n));
    }
    for (n = GD_nlist(g); n; n = ND_next(n)) {
	for (i = 0; (e = ND_out(n).list[i]); i++) {
	    fprintf (fp, "  \"%s\"", dump_node(n));
	    w = aghead(e);
	    fprintf (fp, " -> \"%s\"\n", dump_node(w));
	}
    }

    fprintf (fp, "}\n");
    fclose (fp);
}

static node_t *checkdfs(graph_t* g, node_t * n)
{
    edge_t *e;
    node_t *w,*x;
    int i;

    if (ND_mark(n))
	return 0;
    ND_mark(n) = TRUE;
    ND_onstack(n) = true;
    for (i = 0; (e = ND_out(n).list[i]); i++) {
	w = aghead(e);
	if (ND_onstack(w)) {
	    dump_graph (g);
	    fprintf(stderr, "cycle: last edge %lx %s(%lx) %s(%lx)\n",
		(uint64_t)e,
	       	agnameof(n), (uint64_t)n,
		agnameof(w), (uint64_t)w);
	    return w;
	}
	else {
	    if (!ND_mark(w)) {
		x = checkdfs(g, w);
		if (x) {
		    fprintf(stderr,"unwind %lx %s(%lx)\n",
			(uint64_t)e,
			agnameof(n), (uint64_t)n);
		    if (x != n) return x;
		    fprintf(stderr,"unwound to root\n");
		    fflush(stderr);
		    abort();
		    return 0;
		}
	    }
	}
    }
    ND_onstack(n) = false;
    return 0;
}

void check_cycles(graph_t * g)
{
    node_t *n;
    for (n = GD_nlist(g); n; n = ND_next(n)) {
	ND_mark(n) = FALSE;
	ND_onstack(n) = false;
    }
    for (n = GD_nlist(g); n; n = ND_next(n))
	checkdfs(g, n);
}
#endif				/* DEBUG */
