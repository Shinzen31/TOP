#include "stencil/mesh.h"
#include "logging.h"

#include <assert.h>
#include <stdlib.h>

#define BLOCK_SIZE_I 32   // Adapt L1
#define BLOCK_SIZE_J 256  // Adapt L2
#define BLOCK_SIZE_K 4    // Adapt L3

mesh_t mesh_new(usz dim_x, usz dim_y, usz dim_z, mesh_kind_t kind) {
    usz const ghost_size = 2 * STENCIL_ORDER;

    // Calculate the total number of cells in the mesh
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
            // Calculate the index for the current block in the cells array
            usz block_index = (i / BLOCK_SIZE_I) * ((dim_y + ghost_size) / BLOCK_SIZE_J) + (j / BLOCK_SIZE_J);
            // Assign the block pointer to the corresponding position in the cells array
            cells[block_index] = &all_cells[(i * (dim_y + ghost_size) + j) * (dim_z + ghost_size)];
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
        for (usz i = 0; i < self->dim_x; ++i) {
            for (usz j = 0; j < self->dim_y; ++j) {
                free(self->cells[i][j]);
            }
            free(self->cells[i]);
        }
        free(self->cells);
    }
}

static char const* mesh_kind_as_str(mesh_t const* self) {
    static char const* MESH_KINDS_STR[] = {
        "CONSTANT",
        "INPUT",
        "OUTPUT",
    };
    return MESH_KINDS_STR[(usz)self->kind];
}

void mesh_print(mesh_t const* self, char const* name) {
    fprintf(
        stderr,
        "****************************************\n"
        "MESH `%s`\n\tKIND: %s\n\tDIMS: %zux%zux%zu\n\tVALUES:\n",
        name,
        mesh_kind_as_str(self),
        self->dim_x,
        self->dim_y,
        self->dim_z
    );

    for (usz i = 0; i < self->dim_x; ++i) {
        for (usz j = 0; j < self->dim_y; ++j) {
            for (usz k = 0; k < self->dim_z; ++k) {
                printf(
                    "%s%6.3lf%s ",
                    CELL_KIND_CORE == self->cells[i][j][k].kind ? "\x1b[1m" : "",
                    self->cells[i][j][k].value,
                    "\x1b[0m"
                );
            }
            puts("");
        }
        puts("");
    }
}

cell_kind_t mesh_set_cell_kind(mesh_t const* self, usz i, usz j, usz k) {
    if ((i >= STENCIL_ORDER && i < self->dim_x - STENCIL_ORDER) &&
        (j >= STENCIL_ORDER && j < self->dim_y - STENCIL_ORDER) &&
        (k >= STENCIL_ORDER && k < self->dim_z - STENCIL_ORDER))
    {
        return CELL_KIND_CORE;
    } else {
        return CELL_KIND_PHANTOM;
    }
}

void mesh_copy_core(mesh_t* dst, mesh_t const* src) {
    assert(dst->dim_x == src->dim_x);
    assert(dst->dim_y == src->dim_y);
    assert(dst->dim_z == src->dim_z);

    for (usz k = STENCIL_ORDER; k < dst->dim_z - STENCIL_ORDER; ++k) {
        for (usz j = STENCIL_ORDER; j < dst->dim_y - STENCIL_ORDER; ++j) {
            for (usz i = STENCIL_ORDER; i < dst->dim_x - STENCIL_ORDER; ++i) {
                assert(dst->cells[i][j][k].kind == CELL_KIND_CORE);
                assert(src->cells[i][j][k].kind == CELL_KIND_CORE);
                dst->cells[i][j][k].value = src->cells[i][j][k].value;
            }
        }
    }
}
