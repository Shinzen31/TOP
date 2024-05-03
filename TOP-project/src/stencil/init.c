#include "stencil/init.h"
#include "stencil/comm_handler.h"
#include "stencil/mesh.h"

#include <assert.h>
#include <math.h>
#include <omp.h>

// Cache blocking sizes
#define BLOCK_SIZE_I 32
#define BLOCK_SIZE_J 256
#define BLOCK_SIZE_K 4

static f64 compute_core_pressure(usz i, usz j, usz k) {
    return sin((f64)k * cos((f64)i + 0.311) * cos((f64)j + 0.817) + 0.613);
}

// Optimized setup_mesh_cell_values with cache blocking
static void setup_mesh_cell_values(mesh_t* mesh, comm_handler_t const* comm_handler) {
    #pragma omp parallel for collapse(3)
    for (usz k = 0; k < mesh->dim_z; k += BLOCK_SIZE_K) {
        for (usz j = 0; j < mesh->dim_y; j += BLOCK_SIZE_J) {
            for (usz i = 0; i < mesh->dim_x; i += BLOCK_SIZE_I) {
                for (usz bk = k; bk < k + BLOCK_SIZE_K && bk < mesh->dim_z; ++bk) {
                    for (usz bj = j; bj < j + BLOCK_SIZE_J && bj < mesh->dim_y; ++bj) {
                        for (usz bi = i; bi < i + BLOCK_SIZE_I && bi < mesh->dim_x; ++bi) {
                            switch (mesh->kind) {
                                case MESH_KIND_CONSTANT:
                                    mesh->cells[bi][bj][bk].value = compute_core_pressure(
                                        comm_handler->coord_x + bi,
                                        comm_handler->coord_y + bj,
                                        comm_handler->coord_z + bk
                                    );
                                    break;
                                case MESH_KIND_INPUT:
                                    if ((bi >= STENCIL_ORDER && (bi < mesh->dim_x - STENCIL_ORDER)) &&
                                        (bj >= STENCIL_ORDER && (bj < mesh->dim_y - STENCIL_ORDER)) &&
                                        (bk >= STENCIL_ORDER && (bk < mesh->dim_z - STENCIL_ORDER))) {
                                        mesh->cells[bi][bj][bk].value = 1.0;
                                    } else {
                                        mesh->cells[bi][bj][bk].value = 0.0;
                                    }
                                    break;
                                case MESH_KIND_OUTPUT:
                                    mesh->cells[bi][bj][bk].value = 0.0;
                                    break;
                                default:
                                    __builtin_unreachable();
                            }
                        }
                    }
                }
            }
        }
    }
}

static void setup_mesh_cell_kinds(mesh_t* mesh) {
    #pragma omp parallel for collapse(3)
    for (usz i = 0; i < mesh->dim_x; ++i) {
        for (usz j = 0; j < mesh->dim_y; ++j) {
            for (usz k = 0; k < mesh->dim_z; ++k) {
                mesh->cells[i][j][k].kind = mesh_set_cell_kind(mesh, i, j, k);
            }
        }
    }
}

void init_meshes(mesh_t* A, mesh_t* B, mesh_t* C, comm_handler_t const* comm_handler) {
    assert(
        A->dim_x == B->dim_x && B->dim_x == C->dim_x &&
        C->dim_x == comm_handler->loc_dim_x + STENCIL_ORDER * 2
    );
    assert(
        A->dim_y == B->dim_y && B->dim_y == C->dim_y &&
        C->dim_y == comm_handler->loc_dim_y + STENCIL_ORDER * 2
    );
    assert(
        A->dim_z == B->dim_z && B->dim_z == C->dim_z &&
        C->dim_z == comm_handler->loc_dim_z + STENCIL_ORDER * 2
    );

    // Setup mesh cell values and kinds for each mesh
    #pragma omp parallel for collapse(3)
    for (usz i = 0; i < A->dim_x; ++i) {
        for (usz j = 0; j < A->dim_y; ++j) {
            for (usz k = 0; k < A->dim_z; ++k) {
                // Compute cell value based on mesh kind
                switch (A->kind) {
                    case MESH_KIND_CONSTANT:
                        A->cells[i][j][k].value = compute_core_pressure(
                            comm_handler->coord_x + i - STENCIL_ORDER,
                            comm_handler->coord_y + j - STENCIL_ORDER,
                            comm_handler->coord_z + k - STENCIL_ORDER
                        );
                        break;
                    case MESH_KIND_INPUT:
                        A->cells[i][j][k].value = (i >= STENCIL_ORDER && i < A->dim_x - STENCIL_ORDER &&
                                                   j >= STENCIL_ORDER && j < A->dim_y - STENCIL_ORDER &&
                                                   k >= STENCIL_ORDER && k < A->dim_z - STENCIL_ORDER) ? 1.0 : 0.0;
                        break;
                    case MESH_KIND_OUTPUT:
                        A->cells[i][j][k].value = 0.0;
                        break;
                    default:
                        assert(0 && "Invalid mesh kind");
                }
                // Set cell kind based on position in mesh
                A->cells[i][j][k].kind = mesh_set_cell_kind(A, i, j, k);
            }
        }
    }

    // Copy initialized mesh A to meshes B and C
    mesh_copy_core(B, A);
    mesh_copy_core(C, A);
}
