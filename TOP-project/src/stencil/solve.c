#include "stencil/solve.h"

#include <assert.h>
#include <math.h>
#include <omp.h> // Inclusion de la bibliothèque OpenMP

#define BLOCK_SIZE_I 32   // Taille de bloc pour s'adapter à la mémoire cache L1 
#define BLOCK_SIZE_J 256  // Taille de bloc pour s'adapter à la mémoire cache L2 
#define BLOCK_SIZE_K 4    // Taille de bloc pour s'adapter à la mémoire cache L3

void solve_jacobi(mesh_t* A, mesh_t const* B, mesh_t* C) {
    assert(A->dim_x == B->dim_x && B->dim_x == C->dim_x);
    assert(A->dim_y == B->dim_y && B->dim_y == C->dim_y);
    assert(A->dim_z == B->dim_z && B->dim_z == C->dim_z);

    usz const dim_x = A->dim_x;
    usz const dim_y = A->dim_y;
    usz const dim_z = A->dim_z;
    usz i, j, k, o, bi, bj, bk;

    // Precompute powers of 17
    double precomputed_powers[STENCIL_ORDER + 1];
    for (int i = 1; i <= STENCIL_ORDER; i++) {
        precomputed_powers[i] = pow(17.0, (double)i);
    }

    // Fix the number of threads to 24 using OpenMP
    
    //omp_set_num_threads(1);
    //omp_set_num_threads(2);
    //omp_set_num_threads(4);
    //omp_set_num_threads(8);
    //omp_set_num_threads(16);
    omp_set_num_threads(24);

    #pragma omp parallel for private(i, j, k, bi, bj, bk, o) collapse(3) schedule(dynamic)
    for (k = STENCIL_ORDER; k < dim_z - STENCIL_ORDER; k += BLOCK_SIZE_K) {
        for (j = STENCIL_ORDER; j < dim_y - STENCIL_ORDER; j += BLOCK_SIZE_J) {
            for (i = STENCIL_ORDER; i < dim_x - STENCIL_ORDER; i += BLOCK_SIZE_I) {
                for (bk = k; bk < k + BLOCK_SIZE_K && bk < dim_z - STENCIL_ORDER; ++bk) {
                    for (bj = j; bj < j + BLOCK_SIZE_J && bj < dim_y - STENCIL_ORDER; ++bj) {
                        for (bi = i; bi < i + BLOCK_SIZE_I && bi < dim_x - STENCIL_ORDER; ++bi) {
                            f64 sum = A->cells[bi][bj][bk].value * B->cells[bi][bj][bk].value;
                            for (o = 1; o <= STENCIL_ORDER; ++o) {
                                sum += ((A->cells[bi + o][bj][bk].value * B->cells[bi + o][bj][bk].value) 
                                     + (A->cells[bi - o][bj][bk].value * B->cells[bi - o][bj][bk].value) 
                                     + (A->cells[bi][bj + o][bk].value * B->cells[bi][bj + o][bk].value) 
                                     + (A->cells[bi][bj - o][bk].value * B->cells[bi][bj - o][bk].value) 
                                     + (A->cells[bi][bj][bk + o].value * B->cells[bi][bj][bk + o].value) 
                                     + (A->cells[bi][bj][bk - o].value * B->cells[bi][bj][bk - o].value)) 
                                     / precomputed_powers[o]; 
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
