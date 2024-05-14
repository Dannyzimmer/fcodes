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
#include <cgraph/prisize_t.h>
#include <common/render.h>
#include <math.h>
#include <patchwork/tree_map.h>

static void squarify(size_t n, double *area, rectangle *recs, size_t nadded, double maxarea, double minarea, double totalarea,
		     double asp, rectangle fillrec){
  /* add a list of area in fillrec using squarified treemap alg.
     n: number of items to add
     area: area of these items, Sum to 1 (?).
     nadded: number of items already added
     maxarea: maxarea of already added items
     minarea: min areas of already added items
     asp: current worst aspect ratio of the already added items so far
     fillrec: the rectangle to be filled in.
   */
  double w = fmin(fillrec.size[0], fillrec.size[1]);

  if (n == 0) return;

  if (Verbose) {
    fprintf(stderr, "trying to add to rect {%f +/- %f, %f +/- %f}\n",fillrec.x[0], fillrec.size[0], fillrec.x[1], fillrec.size[1]);
    fprintf(stderr, "total added so far = %" PRISIZE_T "\n", nadded);
  }

  if (nadded == 0){
    nadded = 1;
    maxarea = minarea = area[0];
    asp = fmax(area[0] / (w * w), w * w / area[0]);
    totalarea = area[0];
    squarify(n, area, recs, nadded, maxarea, minarea, totalarea, asp, fillrec);
  } else {
    double newmaxarea, newminarea, s, h, maxw, minw, newasp, hh, ww, xx, yy;
    if (nadded < n){
      newmaxarea = fmax(maxarea, area[nadded]);
      newminarea = fmin(minarea, area[nadded]);
      s = totalarea + area[nadded];
      h = s/w;
      maxw = newmaxarea/h;
      minw = newminarea/h;
      newasp = fmax(h / minw, maxw / h);/* same as MAX{s^2/(w^2*newminarea), (w^2*newmaxarea)/(s^2)}*/
    }
    if (nadded < n && newasp <= asp){/* aspectio improved, keep adding */
      squarify(n, area, recs, ++nadded, newmaxarea, newminarea, s, newasp, fillrec);
    } else {
      /* aspectio worsen if add another area, fixed the already added recs */
      if (Verbose) fprintf(stderr, "adding %" PRISIZE_T
                           " items, total area = %f, w = %f, area/w=%f\n",
                           nadded, totalarea, w, totalarea/w);
      if (fillrec.size[0] <= fillrec.size[1]) {
        // tall rec. fix the items along x direction, left to right, at top
	hh = totalarea/w;
	xx = fillrec.x[0] - fillrec.size[0]/2;
	for (size_t i = 0; i < nadded; i++){
	  recs[i].size[1] = hh;
	  ww = area[i]/hh;
	  recs[i].size[0] = ww;
	  recs[i].x[1] = fillrec.x[1] + 0.5*(fillrec.size[1]) - hh/2;
	  recs[i].x[0] = xx + ww/2;
	  xx += ww;
	}
	fillrec.x[1] -= hh/2;/* the new empty space is below the filled space */
	fillrec.size[1] -= hh;
      } else {/* short rec. fix along y top to bot, at left*/
	ww = totalarea/w;
	yy = fillrec.x[1] + fillrec.size[1]/2;
	for (size_t i = 0; i < nadded; i++){
	  recs[i].size[0] = ww;
	  hh = area[i]/ww;
	  recs[i].size[1] = hh;
	  recs[i].x[0] = fillrec.x[0] - 0.5*(fillrec.size[0]) + ww/2;
	  recs[i].x[1] = yy - hh/2;
	  yy -= hh;
	}
	fillrec.x[0] += ww/2;/* the new empty space is right of the filled space */
	fillrec.size[0] -= ww;
      }
      squarify(n - nadded, area + nadded, recs + nadded, 0, 0., 0., 0., 1., fillrec);
    }

  }
}

/* tree_map:
 * Perform a squarified treemap layout on a single level.
 *  n - number of rectangles
 *  area - area of rectangles
 *  fillred - rectangle to be filled
 *  return array of rectangles 
 */
rectangle* tree_map(size_t n, double *area, rectangle fillrec){
  /* fill a rectangle rec with n items, each item i has area[i] area. */
  double total = 0, minarea = 1., maxarea = 0., asp = 1, totalarea = 0;

  for (size_t i = 0; i < n; i++) total += area[i];
    /* make sure there is enough area */
  if (total > fillrec.size[0] * fillrec.size[1] + 0.001)
    return NULL;
  
  rectangle *recs = gv_calloc(n, sizeof(rectangle));
  squarify(n, area, recs, 0, maxarea, minarea, totalarea, asp, fillrec);
  return recs;
}
