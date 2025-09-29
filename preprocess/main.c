#include <gmshc.h>
#include <stdlib.h>
#include <stdio.h>

#define GL_SILENCE_DEPRECATION

#include <GLFW/glfw3.h>

#define DIMENSION 2

size_t findNodeIndex(size_t *nodeTags, size_t nodeTags_n, size_t tag) {
    for (int i = 0; i < nodeTags_n; i++) {
        if(nodeTags[i] == tag) return i;
    }
    return (size_t)-1;
}


int main(int argc, char **argv) {

    glfwInit();

    GLFWwindow* window = glfwCreateWindow(600, 500, "Mesh", NULL, NULL);

    glfwMakeContextCurrent(window);

    int error = 0;
    if(argc < 2) {
        printf("Usage: %s model.step\n", argv[0]);
        return EXIT_FAILURE;
    }

    const int clmin = 30;
    const int clmax = 30;

    const char *modelFile = "mesh.msh";
    const char *coordsFile = "geo.txt";

    gmshInitialize(argc, argv, 1, 0, &error);
    gmshMerge(argv[1], &error);
    gmshModelOccSynchronize(&error);

    // --- настраиваем генерацию сетки ---
    gmshOptionSetNumber("Mesh.RecombineAll", 0, &error);   // запретить квады
    gmshOptionSetNumber("Mesh.Algorithm", 6, &error);      // только треугольники
    gmshOptionSetNumber("Mesh.CharacteristicLengthMin", clmin, &error);
    gmshOptionSetNumber("Mesh.CharacteristicLengthMax", clmax, &error);

    // --- генерация 2D сетки ---
    gmshModelMeshGenerate(DIMENSION, &error);

    gmshWrite(modelFile, &error);

    FILE *file = fopen(coordsFile, "w");

    // получение координат узлов
    int rectTag = 8; // конкретная поверхность

    size_t *nodeTags; // массив с глобальными номерами узлов
    size_t nodeTags_n; // количество узлов
    double *coords; // массив координат узлов (x, y, z)
    size_t coords_n; // размер массива coords (nodeTags_n * 3)

    gmshModelMeshGetNodes(&nodeTags, &nodeTags_n,
                        &coords, &coords_n,
                        NULL, 0,
                        DIMENSION, rectTag,
                        1,  /* includeBoundary = 1 (включить узлы на границе) */
                        0,  /* returnParametricCoord = 0 (параметрические координаты на элементе) */
                        &error);

    fprintf(file, "%zu\n", nodeTags_n);
    for (int i = 0; i < nodeTags_n; i++) {
        fprintf(file, "%.2f %.2f %.2f\n", coords[i * 3], coords[i * 3 + 1], coords[i * 3 + 2]);
    }

    // достаем элементы образующие треугольные КЭ

    int *elementTypes; // только треугольники
    size_t elementTypes_n = 1;
    size_t **elementTags;
    size_t *elementTags_n;
    size_t elementTags_nn;
    size_t **elementNodeTags;
    size_t *elementNodeTags_n;
    size_t elementNodetags_nn;

    gmshModelMeshGetElements(&elementTypes, &elementTypes_n,
                            &elementTags, &elementTags_n, &elementTags_nn,
                            &elementNodeTags, &elementNodeTags_n, &elementNodetags_nn,
                            DIMENSION,
                            rectTag,
                            &error);

    int etype = *elementTypes; // только линейный треугольный КЭ

    int ndof = 3; // число узлов КЭ

    fprintf(file, "%zu\n", *elementNodeTags_n / ndof);

    while (!glfwWindowShouldClose(window)) {
        
        glClearColor(1.f, 1.f, 1.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        glPushMatrix();

        glPointSize(5.0f);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(-0.5, 0., 0.);

        for(size_t e = 0; e < *elementTags_n; e++) {
            printf("Элемент %zu:\n", e+1);
            glBegin(GL_TRIANGLES);
            for(int n = 0; n < ndof; n++) {
                size_t nodeTag = (*elementNodeTags)[e*ndof + n];
                size_t idx = findNodeIndex(nodeTags, nodeTags_n, nodeTag);

                if(idx == (size_t)-1) {
                    fprintf(stderr, "Ошибка: узел %zu не найден\n", nodeTag);
                    continue;
                }

                double x = coords[idx*3 + 0];
                double y = coords[idx*3 + 1];
                double z = coords[idx*3 + 2];

                printf("  узел %zu: (%.3f, %.3f, %.3f)\n", nodeTag, x, y, z);

                glColor3f(0.f, 0.f, 0.f);
                glVertex3f(x / 100., y / 100., z / 100.);
            }
            
            glEnd();
        }

        for(size_t e = 0; e < *elementTags_n; e++) {
            for(int n = 0; n < ndof; n++) {
                glBegin(GL_LINES);
                size_t nodeTag1 = (*elementNodeTags)[e*ndof + n];
                size_t idx1 = findNodeIndex(nodeTags, nodeTags_n, nodeTag1);

                if(idx1 == (size_t)-1) {
                    fprintf(stderr, "Ошибка: узел %zu не найден\n", nodeTag1);
                    continue;
                }

                size_t nodeTag2 = (*elementNodeTags)[e*ndof + (n + 1) % ndof];
                size_t idx2 = findNodeIndex(nodeTags, nodeTags_n, nodeTag2);

                if(idx2 == (size_t)-1) {
                    fprintf(stderr, "Ошибка: узел %zu не найден\n", nodeTag2);
                    continue;
                }

                double x1 = coords[idx1*3 + 0];
                double y1 = coords[idx1*3 + 1];
                double z1 = coords[idx1*3 + 2];

                double x2 = coords[idx2*3 + 0];
                double y2 = coords[idx2*3 + 1];
                double z2 = coords[idx2*3 + 2];

                glColor3f(1.f, 1.f, 1.f);
                glVertex3f(x1 / 100., y1 / 100., z1 / 100.);
                glVertex3f(x2 / 100., y2 / 100., z2 / 100.);

                glEnd();
            }
            
        }
        
        glPopMatrix();
        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    fclose(file);

    /*--------------------------*/   
    int *dimTags;
    size_t dimTags_n;
    gmshModelGetEntities(&dimTags, &dimTags_n, 2, &error);

    size_t numSurfaces = dimTags_n / 2;
    printf("Найдено поверхностей: %zu\n", numSurfaces);

    for(size_t i = 0; i < numSurfaces; i++) {
        int dim = dimTags[2 * i];     // всегда будет 2
        int tag = dimTags[2 * i + 1]; // уникальный номер поверхности
        printf("Поверхность %zu: dim=%d, tag=%d\n", i+1, dim, tag);
    }

    gmshFinalize(&error);
    return 0;
}