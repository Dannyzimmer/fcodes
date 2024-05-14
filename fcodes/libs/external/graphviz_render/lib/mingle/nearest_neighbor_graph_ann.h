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

#include <vector>

void nearest_neighbor_graph_ann(int nPts, int k, double *x, int &nz0,
                                std::vector<int> &irn, std::vector<int> &jcn,
                                std::vector<double> &val);
