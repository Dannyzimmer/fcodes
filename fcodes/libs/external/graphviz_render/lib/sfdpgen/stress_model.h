#pragma once

void stress_model(int dim, SparseMatrix D, double **x, int maxit, double tol,
                  int *flag);
