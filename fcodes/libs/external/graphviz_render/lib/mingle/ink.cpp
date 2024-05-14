/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <common/types.h>
#include <common/globals.h>
#include <sparse/general.h>
#include <mingle/ink.h>
#include <vector>

double ink_count;

static point_t addPoint (point_t a, point_t b)
{
  a.x += b.x;
  a.y += b.y;
  return a;
}

static point_t subPoint (point_t a, point_t b)
{
  a.x -= b.x;
  a.y -= b.y;
  return a;
}

static point_t scalePoint (point_t a, double d)
{
  a.x *= d;
  a.y *= d;
  return a;
}

static double dotPoint(point_t a, point_t b){
  return a.x*b.x + a.y*b.y;
}

static const point_t Origin = {0, 0};

/* sumLengths:
 */
static double sumLengths_avoid_bad_angle(const std::vector<point_t> &points,
                                         point_t end, point_t meeting,
                                         double angle_param) {
  /* avoid sharp turns, we want cos_theta to be as close to -1 as possible */
  double len0, len, sum = 0;
  double diff_x, diff_y, diff_x0, diff_y0;
  double cos_theta, cos_max = -10;

  diff_x0 = end.x-meeting.x;
  diff_y0 = end.y-meeting.y;
  len0 = sum = hypot(diff_x0, diff_y0);

  // distance form each of 'points' till 'meeting'
  for (const point_t &p : points) {
    diff_x = p.x - meeting.x;
    diff_y = p.y - meeting.y;
    len = hypot(diff_x, diff_y);
    sum += len;
    cos_theta = (diff_x0 * diff_x + diff_y0 * diff_y)
                / std::max(len * len0, 0.00001);
    cos_max = std::max(cos_max, cos_theta);
  }

  // distance of single line from 'meeting' to 'end'
  return sum*(cos_max + angle_param);/* straight line gives angle_param - 1, turning angle of 180 degree gives angle_param + 1 */
}
static double sumLengths(const std::vector<point_t> &points, point_t end,
                         point_t meeting) {
  double sum = 0;
  double diff_x, diff_y;

    // distance form each of 'points' till 'meeting'
  for (const point_t &p : points) {
        diff_x = p.x - meeting.x;
        diff_y = p.y - meeting.y;
        sum += hypot(diff_x, diff_y);
  }
    // distance of single line from 'meeting' to 'end'
  diff_x = end.x-meeting.x;
  diff_y = end.y-meeting.y;
  sum += hypot(diff_x, diff_y);
  return sum;
}

/* bestInk:
 */
static double bestInk(const std::vector<point_t> &points, point_t begin,
                      point_t end, double prec, point_t *meet,
                      double angle_param) {
  point_t first, second, third, fourth, diff, meeting;
  double value1, value2, value3, value4;

  first = begin;
  fourth = end;

  do {
    diff = subPoint(fourth,first);
    second = addPoint(first,scalePoint(diff,1.0/3.0));
    third = addPoint(first,scalePoint(diff,2.0/3.0));

    if (angle_param < 1){
      value1 = sumLengths(points, end, first);
      value2 = sumLengths(points, end, second);
      value3 = sumLengths(points, end, third);
      value4 = sumLengths(points, end, fourth);
    } else {
      value1 = sumLengths_avoid_bad_angle(points, end, first, angle_param);
      value2 = sumLengths_avoid_bad_angle(points, end, second, angle_param);
      value3 = sumLengths_avoid_bad_angle(points, end, third, angle_param);
      value4 = sumLengths_avoid_bad_angle(points, end, fourth, angle_param);
    }

    if (value1<value2) {
      if (value1<value3) {
        if (value1<value4) {
          // first is smallest
          fourth = second;
        }
        else {
          // fourth is smallest
          first = third;
        }
      }
      else {
        if (value3<value4) {
          // third is smallest
          first = second;
        }
        else {
          // fourth is smallest
          first = third;
        }
      }
    }
    else {
      if (value2<value3) {
        if (value2<value4) {
          // second is smallest
          fourth = third;
        }
        else {
          // fourth is smallest
          first = third;
        }
      }
      else {
        if (value3<value4) {
          // third is smallest
          first = second;
        }
        else {
          // fourth is smallest
          first = third;
        }
      }
    }
  } while (fabs(value1 - value4) / (std::min(value1, value4) + 1e-10) > prec
           && dotPoint(diff, diff) > 1.e-20);

  meeting = scalePoint(addPoint(first,fourth),0.5);
  *meet = meeting;

  return sumLengths(points, end, meeting);

}

static double project_to_line(point_t pt, point_t left, point_t right, double angle){
  /* pt
     ^  ^          
     .   \ \ 
     .    \   \        
   d .    a\    \
     .      \     \
     .       \       \
     .   c    \  alpha \             b
     .<------left:0 ----------------------------> right:1. Find the projection of pt on the left--right line. If the turning angle is small, 
     |                  |
     |<-------f---------
     we should get a negative number. Let a := left->pt, b := left->right, then we are calculating:
     c = |a| cos(a,b)/|b| b
     d = a - c
     f = -ctan(alpha)*|d|/|b| b
     and the project of alpha degree on the left->right line is
     c-f = |a| cos(a,b)/|b| b - -ctan(alpha)*|d|/|b| b
     = (|a| a.b/(|a||b|) + ctan(alpha)|a-c|)/|b| b
     = (a.b/|b| + ctan(alpha)|a-c|)/|b| b
     the dimentionless projection is:
     a.b/|b|^2 + ctan(alpha)|a-c|/|b|
     = a.b/|b|^2 + ctan(alpha)|d|/|b|
  */


  point_t b, a;
  double bnorm, dnorm;
  double alpha, ccord;

  if (angle <=0 || angle >= M_PI) return 2;/* return outside of the interval which should be handled as a sign of infeasible turning angle */
  alpha = angle;

  assert(alpha > 0 && alpha < M_PI);
  b = subPoint(right, left);
  a = subPoint(pt, left);
  bnorm = std::max(1.e-10, dotPoint(b, b));
  ccord = dotPoint(b, a)/bnorm;
  dnorm = dotPoint(a,a)/bnorm - ccord*ccord;
  if (alpha == M_PI/2){
    return ccord;
  } else {
    return ccord + sqrt(std::max(0.0, dnorm)) / tan(alpha);
  }
}

/* ink:
 * Compute minimal ink used the input edges are bundled.
 * Assumes tails all occur on one side and heads on the other.
 */
double ink(pedge* edges, int numEdges, int *pick, double *ink0, point_t *meet1, point_t *meet2, double angle_param, double angle)
{
  int i;
  point_t begin, end, mid, diff;
  pedge e;
  double *x;
  double inkUsed;
  double eps = 1.0e-2;
  double cend = 0, cbegin = 0;
  double wgt = 0;

  ink_count += numEdges;

  *ink0 = 0;

  /* canonicalize so that edges 1,2,3 and 3,2,1 gives the same optimal ink */
  if (pick) vector_sort_int(numEdges, pick);

  begin = end = Origin;
  for (i = 0; i < numEdges; i++) {
    if (pick) {
      e = edges[pick[i]];
    } else {
      e = edges[i];
    }
    x = e->x;
    point_t source = {x[0], x[1]};
    point_t target = {x[e->dim*e->npoints - e->dim],
                      x[e->dim*e->npoints - e->dim + 1]};
    (*ink0) += hypot(source.x - target.x, source.y - target.y);
    begin = addPoint (begin, scalePoint(source, e->wgt));
    end = addPoint (end, scalePoint(target, e->wgt));
    wgt += e->wgt;
  }

  begin = scalePoint (begin, 1.0/wgt);
  end = scalePoint (end, 1.0/wgt);


  if (numEdges == 1){
    *meet1 = begin;
    *meet2 = end;
      return *ink0;
  }

  /* shift the begin and end point to avoid sharp turns */
  std::vector<point_t> sources;
  std::vector<point_t> targets;
  for (i = 0; i < numEdges; i++) {
    if (pick) {
      e = edges[pick[i]];
    } else {
      e = edges[i];
    }
    x = e->x;
    sources.push_back(point_t{x[0], x[1]});
    targets.push_back(point_t{x[e->dim*e->npoints - e->dim],
                              x[e->dim*e->npoints - e->dim + 1]});
    /* begin(1) ----------- mid(0) */
    if (i == 0){
      cbegin = project_to_line(sources[i], begin, end, angle);
      cend =   project_to_line(targets[i], end, begin, angle);
    } else {
      cbegin = std::max(cbegin, project_to_line(sources[i], begin, end, angle));
      cend = std::max(cend, project_to_line(targets[i], end, begin, angle));
    }
  }

  if (angle > 0 && angle < M_PI){
    if (cbegin + cend > 1 || cbegin > 1 || cend > 1){
      /* no point can be found that satisfies the angular constraints, so we give up and set ink to a large value */
      inkUsed = 1000*(*ink0);
      return inkUsed;
    }
    /* make sure the turning angle is no more than alpha degree */
    cbegin = std::max(0.0, cbegin);/* make sure the new adjusted point is with in [begin,end] internal */
    diff = subPoint(end, begin);
    begin = addPoint(begin, scalePoint(diff, cbegin));
    
    cend = std::max(0.0, cend);/* make sure the new adjusted point is with in [end,begin] internal */
    end = subPoint(end, scalePoint(diff, cend));
  }
  mid = scalePoint (addPoint(begin,end),0.5);

  inkUsed = bestInk(sources, begin, mid, eps, meet1, angle_param)
	     + bestInk(targets, end, mid, eps, meet2, angle_param);

  return inkUsed;
}

double ink1(pedge e){
  double *x, xx, yy;

  double ink0 = 0;

  x = e->x;
  xx = x[0] - x[e->dim*e->npoints - e->dim];
  yy = x[1] - x[e->dim*e->npoints - e->dim + 1];
  ink0 += hypot(xx, yy);
  return ink0;
}
