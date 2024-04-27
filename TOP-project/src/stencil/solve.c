#include "stencil/solve.h"

#include <assert.h>
#include <math.h>

#define BLOCK_SIZE_I 32   //adapt L1
#define BLOCK_SIZE_J 256  //adapt L2
#define BLOCK_SIZE_K 4    //adapt L3

void solve_jacobi(mesh_t* A, mesh_t const* B, mesh_t* C) {
    assert(A->dim_x == B->dim_x && B->dim_x == C->dim_x);
    assert(A->dim_y == B->dim_y && B->dim_y == C->dim_y);
    assert(A->dim_z == B->dim_z && B->dim_z == C->dim_z);

    usz const dim_x = A->dim_x;
    usz const dim_y = A->dim_y;
    usz const dim_z = A->dim_z;
    usz i, j, k, o, bi, bj, bk;

    for (k = STENCIL_ORDER; k < dim_z - STENCIL_ORDER; k += BLOCK_SIZE_K) {
        for (j = STENCIL_ORDER; j < dim_y - STENCIL_ORDER; j += BLOCK_SIZE_J) {
            for (i = STENCIL_ORDER; i < dim_x - STENCIL_ORDER; i += BLOCK_SIZE_I) {
                // loop unrolling and fission
                for (bk = k; bk < k + BLOCK_SIZE_K && bk < dim_z - STENCIL_ORDER; ++bk) {
                    for (bj = j; bj < j + BLOCK_SIZE_J && bj < dim_y - STENCIL_ORDER; ++bj) {
                        for (bi = i; bi < i + BLOCK_SIZE_I && bi < dim_x - STENCIL_ORDER; ++bi) {
                            f64 sum = A->cells[bi][bj][bk].value * B->cells[bi][bj][bk].value;
                            for (o = 1; o <= STENCIL_ORDER; ++o) {
                                sum += A->cells[bi + o][bj][bk].value * B->cells[bi + o][bj][bk].value / pow(17.0, (f64)o);
                                sum += A->cells[bi - o][bj][bk].value * B->cells[bi - o][bj][bk].value / pow(17.0, (f64)o);
                                sum += A->cells[bi][bj + o][bk].value * B->cells[bi][bj + o][bk].value / pow(17.0, (f64)o);
                                sum += A->cells[bi][bj - o][bk].value * B->cells[bi][bj - o][bk].value / pow(17.0, (f64)o);
                                sum += A->cells[bi][bj][bk + o].value * B->cells[bi][bj][bk + o].value / pow(17.0, (f64)o);
                                sum += A->cells[bi][bj][bk - o].value * B->cells[bi][bj][bk - o].value / pow(17.0, (f64)o);
                            }
                            C->cells[bi][bj][bk].value = sum;
                        }
                    }
                }
            }
        }
    }

    mesh_copy_core(A, C);
}

