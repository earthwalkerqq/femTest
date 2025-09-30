#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define GL_SILENCE_DEPRECATION

#include <GLFW/glfw3.h>

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 500

#define KOEF 100.

static int zoom = 0;

static float translate_x = -0.5;
static float translate_y = 0.;
static float translate_z = 0.;

void key_callback(GLFWwindow *window, int key, int __attribute__((unused)) scanmode, int action, int __attribute__((unused)) mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GL_TRUE);
                break;
            case '=': // +
                zoom += 2;
                break;
            case '-':
                zoom -= 2;
                break;
            case GLFW_KEY_UP:
                translate_y += 0.1;
                break;
            case GLFW_KEY_DOWN:
                translate_y -= 0.1;
                break;
            case GLFW_KEY_LEFT:
                translate_x -= 0.1;
                break;
            case GLFW_KEY_RIGHT:
                translate_x += 0.1;
                break;
        }
    }
}

int main(int argc, char **argv) {
    char error = EXIT_SUCCESS;
    
    if (argc < 2) {
        fprintf(stderr, "YOU NEED TO ADD *.txt FILE IN COMMAND LINE\n");
        error = EXIT_FAILURE;
        return error;
    }

    const int ndof = 3;

    int nys;

    double **coords = NULL;
    double *dataCoords = NULL;

    FILE *file = fopen(argv[1], "r");

    if (file == NULL) {
        fprintf(stderr, "CAN'T OPEN FILE\n");
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

    assert(!error);

    if (!glfwInit()) {
        fprintf(stderr, "CAN'T INIT GLFW\n");
        error = EXIT_FAILURE;
        return error;
    }

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Model", NULL, NULL);

    if (!window) {
        fprintf(stderr, "CAN'T CREATE WINDOW\n");
        error = EXIT_FAILURE;
        return error;
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);

    while (!glfwWindowShouldClose(window) && !error) {
        glClearColor(1.f, 1.f, 1.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        glPushMatrix();

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(translate_x, translate_y, translate_z);

        for (int i = 0; i < nys; i += ndof) {
            glBegin(GL_TRIANGLES);
                glColor3f(0.f, 0.f, 0.f);
                for (int j = i; j < i + ndof; j++) {
                    glVertex3f(coords[j][0] / (KOEF - zoom), coords[j][1] / (KOEF - zoom), coords[j][2] / (KOEF - zoom));
                }
            glEnd();

            for (int j = 0; j < ndof; j++) {
                glBegin(GL_LINES);
                    glColor3f(1.f, 1.f, 1.f);
                    glVertex3f(coords[i + j][0] / (KOEF - zoom), coords[i + j][1] / (KOEF - zoom), coords[i + j][2] / (KOEF - zoom));
                    glVertex3f(coords[i + (j + 1) % 3][0] / (KOEF - zoom), coords[i + (j + 1) % 3][1] / (KOEF - zoom), coords[i + (j + 1) % 3][2] / (KOEF - zoom));
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

    fclose(file);

    return EXIT_SUCCESS;
}