#include "stencil/solve.h"

#include <assert.h>
#include <math.h>
#include <omp.h>
#include <immintrin.h> // Include AVX2 intrinsics

#define BLOCK_SIZE_I 32
#define BLOCK_SIZE_J 256
#define BLOCK_SIZE_K 4
#define VECTOR_SIZE 4  // Assuming AVX2, where each vector register can hold 4 doubles

void solve_jacobi(mesh_t* A, mesh_t const* B, mesh_t* C) {
    assert(A->dim_x == B->dim_x && B->dim_x == C->dim_x);
    assert(A->dim_y == B->dim_y && B->dim_y == C->dim_y);
    assert(A->dim_z == B->dim_z && B->dim_z == C->dim_z);

    usz const dim_x = A->dim_x;
    usz const dim_y = A->dim_y;
    usz const dim_z = A->dim_z;
    usz i, j, k, o, bi, bj, bk;

    // Adjust block sizes to fit vectorization
    const usz block_size_i_v = BLOCK_SIZE_I / VECTOR_SIZE;
    const usz block_size_j_v = BLOCK_SIZE_J / VECTOR_SIZE;

    #pragma omp parallel for private(i, j, k, bi, bj, bk, o) collapse(3)
    for (k = STENCIL_ORDER; k < dim_z - STENCIL_ORDER; k += BLOCK_SIZE_K) {
        for (j = STENCIL_ORDER; j < dim_y - STENCIL_ORDER; j += BLOCK_SIZE_J) {
            for (i = STENCIL_ORDER; i < dim_x - STENCIL_ORDER; i += BLOCK_SIZE_I) {
                for (bk = k; bk < k + BLOCK_SIZE_K && bk < dim_z - STENCIL_ORDER; ++bk) {
                    for (bj = j; bj < j + BLOCK_SIZE_J && bj < dim_y - STENCIL_ORDER; ++bj) {
                        for (bi = i; bi < i + BLOCK_SIZE_I && bi < dim_x - STENCIL_ORDER; bi += VECTOR_SIZE) {
                            // Load vectors of values from A and B
                            __m256d sum_vec = _mm256_setzero_pd();
                            for (o = 1; o <= STENCIL_ORDER; ++o) {
                                __m256d a_value = _mm256_loadu_pd(&A->cells[bi + o][bj][bk].value);
                                __m256d b_value = _mm256_loadu_pd(&B->cells[bi + o][bj][bk].value);
                                sum_vec = _mm256_fmadd_pd(a_value, b_value, sum_vec);

                                a_value = _mm256_loadu_pd(&A->cells[bi - o][bj][bk].value);
                                b_value = _mm256_loadu_pd(&B->cells[bi - o][bj][bk].value);
                                sum_vec = _mm256_fmadd_pd(a_value, b_value, sum_vec);

                                a_value = _mm256_loadu_pd(&A->cells[bi][bj + o][bk].value);
                                b_value = _mm256_loadu_pd(&B->cells[bi][bj + o][bk].value);
                                sum_vec = _mm256_fmadd_pd(a_value, b_value, sum_vec);

                                a_value = _mm256_loadu_pd(&A->cells[bi][bj - o][bk].value);
                                b_value = _mm256_loadu_pd(&B->cells[bi][bj - o][bk].value);
                                sum_vec = _mm256_fmadd_pd(a_value, b_value, sum_vec);

                                a_value = _mm256_loadu_pd(&A->cells[bi][bj][bk + o].value);
                                b_value = _mm256_loadu_pd(&B->cells[bi][bj][bk + o].value);
                                sum_vec = _mm256_fmadd_pd(a_value, b_value, sum_vec);

                                a_value = _mm256_loadu_pd(&A->cells[bi][bj][bk - o].value);
                                b_value = _mm256_loadu_pd(&B->cells[bi][bj][bk - o].value);
                                sum_vec = _mm256_fmadd_pd(a_value, b_value, sum_vec);
                            }
                            // Store the result back to C
                            _mm256_storeu_pd(&C->cells[bi][bj][bk].value, sum_vec);
                        }
                    }
                }
            }
        }
    }
    mesh_copy_core(A, C);
}
