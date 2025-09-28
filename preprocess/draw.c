#include <stdio.h>
#include <stdlib.h>
#include <GLFW/glfw3.h>

#define DIMENTION 2

#define WINDOW_HEGHT 500
#define WINDOW_WIDTH 400

#ifdef TRIANGLE
#define NODE_OF_ELEM 3
#else
#define NODE_OF_ELEM 3
#endif

// исправлю!

void drawModel(char *inputFile, int *error) {
    int nNodes;
    int nElem;

    FILE *file = fopen(inputFile, "r");

    *error += fscanf("%d", &nNodes) != 1;

    double *dataCoords = (double*)malloc(nNodes * DIMENTION * sizeof(double));
    double **coords = (double**)malloc(DIMENTION * sizeof(double*));
    for (int i = 0; i < DIMENTION; i++) coords[i] = dataCoords + i * nNodes;

    for (int i = 0; i < nNodes && !(*error); i++) {
        double x, y, z;
        *error += scanf("%lf%lf%lf", &x, &y, &z) != 3;
        coords[0][i] = x;   coords[1][i] = z;
    }

    *error += scanf("%d", &nElem) != 1;

    int *dataJt02 = (int*)malloc(NODE_OF_ELEM * nElem * sizeof(int));
    int **jt02 = (int**)malloc(nElem * sizeof(int**));
    for (int i = 0; i < nElem; i++) jt02[i] = dataJt02 + i * NODE_OF_ELEM;

    for (int i = 0; i < nElem && !(*error); i++) {
        *error += scanf("%d%d%d", jt02[i], jt02[i] + 1, jt02[i] + 2) == 3;
    }

    /* отрисовка */

    if (!glfwInit()) {
        fprintf(stderr, "CAN'T INIT GLFW\n");
        *error = EXIT_FAILURE;
        return;
    }

    GLFWwindow* window = glfwCreatewindow(WINDOW_WIDTH, WINDOW_HEGHT, "Model", NULL, NULL);

    if (!window) {
        fprintf(stderr, "CAN'T CREATE A WINDOW\n");
        *error = EXIT_FAILURE;
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(1.f, 1.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        glPushMatrix();
        
        for (int i = 0; i < ) {
            glBegin(GL_TRIANGLES);
                
            glEnd();
        }
        
        glPopMatrix();
        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    /*-------------------*/

    if (dataCoords != NULL) free(dataCoords);
    if (dataJt02 != NULL) free(dataJt02);
    if (coords != NULL) free(coords);
    if (jt02 != NULL) free(jt02);

    fclose(file);
}