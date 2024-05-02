#include "stencil/mesh.h"
#include "logging.h"

#include <assert.h>
#include <stdlib.h>

#define BLOCK_SIZE_I 32   // Adapt L1
#define BLOCK_SIZE_J 256  // Adapt L2
#define BLOCK_SIZE_K 4    // Adapt L3

mesh_t mesh_new(usz dim_x, usz dim_y, usz dim_z, mesh_kind_t kind) {
    usz const ghost_size = 2 * STENCIL_ORDER;

    // Calculate the total number of cells
    usz total_cells = (dim_x + ghost_size) * (dim_y + ghost_size) * (dim_z + ghost_size);

    // Allocate memory for all cells at once
    cell_t* all_cells = malloc(total_cells * sizeof(cell_t));
    if (NULL == all_cells) {
        error("failed to allocate memory for all cells");
        return (mesh_t){0};  // Return an empty mesh on failure
    }

    // Allocate memory for the pointer structure
    cell_t*** cells = malloc((dim_x + ghost_size) * sizeof(cell_t**));
    if (NULL == cells) {
        free(all_cells);
        error("failed to allocate top-level pointers for mesh");
        return (mesh_t){0};
    }

    // Setup pointers in a blocked manner
    for (usz i = 0; i < dim_x + ghost_size; i += BLOCK_SIZE_I) {
        for (usz j = 0; j < dim_y + ghost_size; j += BLOCK_SIZE_J) {
            for (usz k = 0; k < dim_z + ghost_size; k += BLOCK_SIZE_K) {
                // Allocate memory for the block of pointers
                cell_t** block_ptr = malloc(BLOCK_SIZE_I * sizeof(cell_t*));
                if (NULL == block_ptr) {
                    // Cleanup previously allocated memory
                    for (usz bi = 0; bi < i; bi += BLOCK_SIZE_I) {
                        for (usz bj = 0; bj < j; bj += BLOCK_SIZE_J) {
                            free(cells[bi][bj]);
                        }
                    }
                    for (usz bi = 0; bi < i; bi += BLOCK_SIZE_I) {
                        free(cells[bi]);
                    }
                    free(cells);
                    free(all_cells);
                    error("failed to allocate memory for block pointers");
                    return (mesh_t){0};
                }

                // Setup pointers within the block
                for (usz bi = i; bi < i + BLOCK_SIZE_I && bi < dim_x + ghost_size; ++bi) {
                    for (usz bj = j; bj < j + BLOCK_SIZE_J && bj < dim_y + ghost_size; ++bj) {
                        // Calculate the index within the linear array
                        usz linear_idx = ((bi * (dim_y + ghost_size)) + bj) * (dim_z + ghost_size) + k;

                        // Set the pointer to the corresponding cell in the linear array
                        block_ptr[bi - i] = &all_cells[linear_idx];
                    }
                }

                // Assign the block pointer to the appropriate position in the cells array
                cells[i / BLOCK_SIZE_I][(j / BLOCK_SIZE_J) * ((dim_x + 2 * STENCIL_ORDER) / BLOCK_SIZE_I) + k / BLOCK_SIZE_K] = block_ptr;

            }
        }
    }

    return (mesh_t){
        .dim_x = dim_x + ghost_size,
        .dim_y = dim_y + ghost_size,
        .dim_z = dim_z + ghost_size,
        .cells = cells,
        .kind = kind,
    };
}

void mesh_drop(mesh_t* self) {
    if (NULL != self->cells) {
        // Free memory for all cells at once
        free(self->cells[0][0]);
        for (usz i = 0; i < (self->dim_x + 2 * STENCIL_ORDER) / BLOCK_SIZE_I; ++i) {
            for (usz j = 0; j < (self->dim_y + 2 * STENCIL_ORDER) / BLOCK_SIZE_J; ++j) {
                free(self->cells[i][j]);
            }
        }
        free(self->cells);
    }
}
