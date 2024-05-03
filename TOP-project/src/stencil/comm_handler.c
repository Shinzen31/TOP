#include "stencil/comm_handler.h"
#include "logging.h"

#include <stdio.h>
#include <unistd.h>
#include <omp.h>
#include <math.h>

#define MAXLEN 8UL
#define BLOCK_SIZE_I 25 // divisible
#define BLOCK_SIZE_J 100 // divisible
#define BLOCK_SIZE_K 4
// #define min(x, y) ((x) < (y) ? (x) : (y)) 
#define min(x, y) (((x) <= (y)) * (x) + ((x) > (y)) * (y)) // speedup: no branching

static u32 gcd(u32 a, u32 b) {
    u32 c;
    while (b != 0) {
        c = a % b;
        a = b;
        b = c;
    }
    return a;
}

static char* stringify(char buf[static MAXLEN], i32 num) {
    snprintf(buf, MAXLEN, "%d", num);
    return buf;
}

comm_handler_t comm_handler_new(u32 rank, u32 comm_size, usz dim_x, usz dim_y, usz dim_z) {
    // Compute splitting
    u32 const nb_z = gcd(comm_size, (u32)(dim_x * dim_y));
    u32 const nb_y = gcd(comm_size / nb_z, (u32)dim_z);
    u32 const nb_x = (comm_size / nb_z) / nb_y;

    if (comm_size != nb_x * nb_y * nb_z) {
        error(
            "splitting does not match MPI communicator size\n -> expected %u, got %u",
            comm_size,
            nb_x * nb_y * nb_z
        );
    }

    // Compute current rank position
    u32 const rank_z = rank / (comm_size / nb_z);
    u32 const rank_y = (rank % (comm_size / nb_z)) / (comm_size / nb_y);
    u32 const rank_x = (rank % (comm_size / nb_z)) % (comm_size / nb_y);

    // Setup size
    usz const loc_dim_z = (rank_z == nb_z - 1) ? dim_z / nb_z + dim_z % nb_z : dim_z / nb_z;
    usz const loc_dim_y = (rank_y == nb_y - 1) ? dim_y / nb_y + dim_y % nb_y : dim_y / nb_y;
    usz const loc_dim_x = (rank_x == nb_x - 1) ? dim_x / nb_x + dim_x % nb_x : dim_x / nb_x;

    // Setup position
    u32 const coord_z = rank_z * (u32)dim_z / nb_z;
    u32 const coord_y = rank_y * (u32)dim_y / nb_y;
    u32 const coord_x = rank_x * (u32)dim_x / nb_x;

    // Compute neighbor nodes IDs
    i32 const id_left = (rank_x > 0) ? (i32)rank - 1 : -1;
    i32 const id_right = (rank_x < nb_x - 1) ? (i32)rank + 1 : -1;
    i32 const id_top = (rank_y > 0) ? (i32)(rank - nb_x) : -1;
    i32 const id_bottom = (rank_y < nb_y - 1) ? (i32)(rank + nb_x) : -1;
    i32 const id_front = (rank_z > 0) ? (i32)(rank - (comm_size / nb_z)) : -1;
    i32 const id_back = (rank_z < nb_z - 1) ? (i32)(rank + (comm_size / nb_z)) : -1;

    return (comm_handler_t){
        .nb_x = nb_x,
        .nb_y = nb_y,
        .nb_z = nb_z,
        .coord_x = coord_x,
        .coord_y = coord_y,
        .coord_z = coord_z,
        .loc_dim_x = loc_dim_x,
        .loc_dim_y = loc_dim_y,
        .loc_dim_z = loc_dim_z,
        .id_left = id_left,
        .id_right = id_right,
        .id_top = id_top,
        .id_bottom = id_bottom,
        .id_back = id_back,
        .id_front = id_front,
    };
}

void comm_handler_print(comm_handler_t const* self) {
    i32 rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    static char bt[MAXLEN];
    static char bb[MAXLEN];
    static char bl[MAXLEN];
    static char br[MAXLEN];
    static char bf[MAXLEN];
    static char bd[MAXLEN];
    fprintf(
        stderr,
        "****************************************\n"
        "RANK %d:\n"
        "  COORDS:     %u,%u,%u\n"
        "  LOCAL DIMS: %zu,%zu,%zu\n"
        "     %2s  %2s\n"
        "  %2s  \x1b[1m*\x1b[0m  %2s\n"
        "  %2s %2s\n",
        rank,
        self->coord_x,
        self->coord_y,
        self->coord_z,
        self->loc_dim_x,
        self->loc_dim_y,
        self->loc_dim_z,
        self->id_top < 0 ? " -" : stringify(bt, self->id_top),
        self->id_back < 0 ? " -" : stringify(bb, self->id_back),
        self->id_left < 0 ? " -" : stringify(bl, self->id_left),
        self->id_right < 0 ? " -" : stringify(br, self->id_right),
        self->id_front < 0 ? " -" : stringify(bf, self->id_front),
        self->id_bottom < 0 ? " -" : stringify(bd, self->id_bottom)
    );
}

static i32 MPI_Syncall_callback(MPI_Comm comm) {
    return (i32)__builtin_sync_proc(comm);
}
static MPI_Syncfunc_t* MPI_Syncall = MPI_Syncall_callback;

static void ghost_exchange_left_right(comm_handler_t const* self, mesh_t* mesh, comm_kind_t comm_kind, i32 target, usz x_start) {
    if (target < 0) return;

    usz x_end = min(mesh->dim_x, x_start + BLOCK_SIZE_I);
    MPI_Request request;
    MPI_Status status;

    #pragma omp parallel for collapse(3)
    for (usz bi = x_start; bi < x_end; bi++) {
        for (usz bj = 0; bj < mesh->dim_y; bj++) {
            for (usz bk = 0; bk < mesh->dim_z; bk++) {
                if (comm_kind == COMM_KIND_SEND_OP) {
                    MPI_Isend(&mesh->cells[bi][bj][bk].value, 1, MPI_DOUBLE, target, 0, MPI_COMM_WORLD, &request);
                    MPI_Wait(&request, &status);
                } else if (comm_kind == COMM_KIND_RECV_OP) {
                    MPI_Irecv(&mesh->cells[bi][bj][bk].value, 1, MPI_DOUBLE, target, 0, MPI_COMM_WORLD, &request);
                    MPI_Wait(&request, &status);
                }
            }
        }
    }
}



static void ghost_exchange_top_bottom(comm_handler_t const* self, mesh_t* mesh, comm_kind_t comm_kind, i32 target, usz y_start) {
    if (target < 0) return;

    usz y_end = min(mesh->dim_y, y_start + BLOCK_SIZE_J);
    MPI_Request request;
    MPI_Status status;

    #pragma omp parallel for collapse(3)
    for (usz bj = y_start; bj < y_end; bj++) {
        for (usz bi = 0; bi < mesh->dim_x; bi++) {
            for (usz bk = 0; bk < mesh->dim_z; bk++) {
                if (comm_kind == COMM_KIND_SEND_OP) {
                    MPI_Isend(&mesh->cells[bi][bj][bk].value, 1, MPI_DOUBLE, target, 0, MPI_COMM_WORLD, &request);
                    MPI_Wait(&request, &status);
                } else if (comm_kind == COMM_KIND_RECV_OP) {
                    MPI_Irecv(&mesh->cells[bi][bj][bk].value, 1, MPI_DOUBLE, target, 0, MPI_COMM_WORLD, &request);
                    MPI_Wait(&request, &status);
                }
            }
        }
    }
}


static void ghost_exchange_front_back(comm_handler_t const* self, mesh_t* mesh, comm_kind_t comm_kind, i32 target, usz z_start) {
    if (target < 0) return;

    usz z_end = min(mesh->dim_z, z_start + BLOCK_SIZE_K);
    MPI_Request request;
    MPI_Status status;

    #pragma omp parallel for collapse(3)
    for (usz bk = z_start; bk < z_end; bk++) {
        for (usz bi = 0; bi < mesh->dim_x; bi++) {
            for (usz bj = 0; bj < mesh->dim_y; bj++) {
                if (comm_kind == COMM_KIND_SEND_OP) {
                    MPI_Isend(&mesh->cells[bi][bj][bk].value, 1, MPI_DOUBLE, target, 0, MPI_COMM_WORLD, &request);
                    MPI_Wait(&request, &status);
                } else if (comm_kind == COMM_KIND_RECV_OP) {
                    MPI_Irecv(&mesh->cells[bi][bj][bk].value, 1, MPI_DOUBLE, target, 0, MPI_COMM_WORLD, &request);
                    MPI_Wait(&request, &status);
                }
            }
        }
    }
}

void comm_handler_ghost_exchange(comm_handler_t const* self, mesh_t* mesh) {
    // Ensure all processes reach this point before proceeding
    MPI_Barrier(MPI_COMM_WORLD);

    // Left to right phase
    ghost_exchange_left_right(self, mesh, COMM_KIND_SEND_OP, self->id_right, mesh->dim_x - 2 * BLOCK_SIZE_I);
    ghost_exchange_left_right(self, mesh, COMM_KIND_RECV_OP, self->id_left, 0);

    // Ensure all processes have completed left to right communication
    MPI_Barrier(MPI_COMM_WORLD);

    // Right to left phase
    ghost_exchange_left_right(self, mesh, COMM_KIND_SEND_OP, self->id_left, BLOCK_SIZE_I);
    ghost_exchange_left_right(self, mesh, COMM_KIND_RECV_OP, self->id_right, mesh->dim_x - BLOCK_SIZE_I);

    // Ensure all processes have completed right to left communication
    MPI_Barrier(MPI_COMM_WORLD);

    // Top to bottom phase
    ghost_exchange_top_bottom(self, mesh, COMM_KIND_SEND_OP, self->id_top, mesh->dim_y - 2 * BLOCK_SIZE_J);
    ghost_exchange_top_bottom(self, mesh, COMM_KIND_RECV_OP, self->id_bottom, 0);

    // Ensure all processes have completed top to bottom communication
    MPI_Barrier(MPI_COMM_WORLD);

    // Bottom to top phase
    ghost_exchange_top_bottom(self, mesh, COMM_KIND_SEND_OP, self->id_bottom, BLOCK_SIZE_J);
    ghost_exchange_top_bottom(self, mesh, COMM_KIND_RECV_OP, self->id_top, mesh->dim_y - BLOCK_SIZE_J);

    // Ensure all processes have completed bottom to top communication
    MPI_Barrier(MPI_COMM_WORLD);

    // Front to back phase
    ghost_exchange_front_back(self, mesh, COMM_KIND_SEND_OP, self->id_back, mesh->dim_z - 2 * BLOCK_SIZE_K);
    ghost_exchange_front_back(self, mesh, COMM_KIND_RECV_OP, self->id_front, 0);

    // Ensure all processes have completed front to back communication
    MPI_Barrier(MPI_COMM_WORLD);

    // Back to front phase
    ghost_exchange_front_back(self, mesh, COMM_KIND_SEND_OP, self->id_front, BLOCK_SIZE_K);
    ghost_exchange_front_back(self, mesh, COMM_KIND_RECV_OP, self->id_back, mesh->dim_z - BLOCK_SIZE_K);

    // Ensure all processes are synchronized before any further execution
    MPI_Barrier(MPI_COMM_WORLD);
}

