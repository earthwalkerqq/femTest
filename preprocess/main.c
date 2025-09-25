#include <stdlib.h>
#include <stdio.h>

#include "generateMesh.h"

int main(int argc, char **argv) {
    const double clmin = 40;
    const double clmax = 50;

    bool_t error = FALSE;

    if (argc < 2) {
        fprintf(stderr, "LOSS PARAMETRS COMMAND LINE");
        return EXIT_FAILURE;
    }

    error = hyperMesh(argv[1], clmin, clmax);
    return error;
}