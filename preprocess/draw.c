#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define GL_SILENCE_DEPRECATION

#include <GLFW/glfw3.h>

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 500

int main(int argc, char **argv) {
    char error = EXIT_SUCCESS;
    
    if (argc < 2) {
        error = EXIT_FAILURE;
        return error;
    }

    const int ndof = 3;

    int nys;
    int nelem;

    double **coords = NULL;
    int **jt03 = NULL;

    double *dataCoords = NULL;
    int *dataJt03 = NULL;

    FILE *file = fopen(argv[1], "r");

    if (file == NULL) {
        error = EXIT_FAILURE;
        return error;
    }

    error += fscanf(file, "%d", &nys) != 1;

    if (!error) {
        dataCoords = (double*)malloc(nys * ndof * sizeof(double));
        error += dataCoords == NULL;
        coords = (double**)malloc(nys * sizeof(double*));
        for (int i = 0; i < nys && !error; i++) {
            coords[i] = dataCoords + i * ndof;
        }
    } else return error;

    assert(!error);

    for (int i = 0; i < nys && !error; i++) {
        error += fscanf(file, "%lf%lf%lf", coords[i], coords[i] + 1, coords[i] + 2) != 3;
    }

    error += fscanf(file, "%d", &nelem) != 1;

    if (!error) {
        dataJt03 = (int*)malloc(nelem * ndof * sizeof(int));
        error += dataJt03 == NULL;
        jt03 = (int**)malloc(nelem * sizeof(int*));
        for (int i = 0; i < nelem && !error; i++) {
            jt03[i] = dataJt03 + i * ndof;
        }
    } else return error;

    assert(!error);

    for (int i = 0; i < nelem && !error; i++) {
        error += fscanf(file, "%d%d%d", jt03[i], jt03[i] + 1, jt03[i] + 2) != 3;
    }

    assert(!error);

    if (!glfwInit()) {
        error = EXIT_FAILURE;
        return error;
    }

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Model", NULL, NULL);

    if (!window) {
        error = EXIT_FAILURE;
        return error;
    }

    glfwMakeContextCurrent(window);

    while (!glfwWindowShouldClose(window) && !error) {
        glClearColor(1.f, 1.f, 1.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        glPushMatrix();

        glPointSize(5.0f);

        for (int i = 0; i < nelem; i++) {
            glBegin(GL_TRIANGLES);
                float r_color = (i % 10) / 10.;
                printf("%f\n", r_color);
                glColor3f(r_color, 0.f, 0.f);
                for (int j = 0; j < ndof; j++) {
                    int elem = jt03[i][j];
                    glVertex2f(coords[elem - 1][0] / 100, coords[elem - 1][2] / 100);
                }
            glEnd();

            for (int j = 0; j < 3; j++) {
                glBegin(GL_LINES);
                    glColor3f(1.f, 1.f, 1.f);
                    int point1 = jt03[i][j] - 1;
                    glVertex2f(coords[point1][0], coords[point1][2]);
                    int point2 = (j == 2) ? jt03[i][0] - 1: jt03[i][j + 1] - 1;
                    glVertex2f(coords[point2][0], coords[point2][2]);
                glEnd();
            }
        }

        for (int i = 0; i < nelem; i++ ){
            for (int j = 0; j < 3; j++) {
                glBegin(GL_LINES);
                    glColor3f(1.f, 1.f, 1.f);
                    int point1 = jt03[i][j] - 1;
                    glVertex2f(coords[point1][0], coords[point1][2]);
                    int point2 = jt03[i][(j + 1) % 3] - 1;
                    glVertex2f(coords[point2][0], coords[point2][2]);
                glEnd();
            }
        }
        
        glPopMatrix();
        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    if (dataCoords != NULL) free(dataCoords);
    if (coords != NULL) free(coords);

    if (dataJt03 != NULL) free(dataJt03);
    if (jt03 != NULL) free(jt03);

    fclose(file);

    return EXIT_SUCCESS;
}