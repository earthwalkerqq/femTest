#ifndef GENERATE_MESH_H
#define GENERATE_MESH_H

#define MESH_ORDER 1
#define DIMENSION 2

typedef enum {
    FALSE = 0,
    TRUE
} bool_t;

bool_t hyperMesh(char *cadFile, double clmin, double clmax);

#endif