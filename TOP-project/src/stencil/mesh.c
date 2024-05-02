#include "stencil/mesh.h"
#include "logging.h"

#include <assert.h>
#include <stdlib.h>
#include <omp.h>

mesh_t mesh_new(usz dim_x, usz dim_y, usz dim_z, mesh_kind_t kind) {
    usz const ghost_size = 2 * STENCIL_ORDER;
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

    // Setup pointers
    for (usz i = 0; i < dim_x + ghost_size; ++i) {
        cells[i] = malloc((dim_y + ghost_size) * sizeof(cell_t*));
        if (NULL == cells[i]) {
            // Cleanup previously allocated memory
            for (usz k = 0; k < i; k++) {
                free(cells[k]);
            }
            free(cells);
            free(all_cells);
            error("failed to allocate second-level pointers for mesh");
            return (mesh_t){0};
        }

        for (usz j = 0; j < dim_y + ghost_size; ++j) {
            cells[i][j] = &all_cells[(i * (dim_y + ghost_size) + j) * (dim_z + ghost_size)];
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
        free(self->cells[0][0]); // Free all cells at once
        for (usz i = 0; i < self->dim_x; ++i) {
            free(self->cells[i]);
        }
        free(self->cells);
    }
}