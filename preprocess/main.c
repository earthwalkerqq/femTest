#include <gmshc.h>
#include <stdlib.h>
#include <stdio.h>

#define GL_SILENCE_DEPRECATION

#include <GLFW/glfw3.h>

#define DIMENSION 2

size_t findNodeIndex(size_t *nodeTags, size_t nodeTags_n, size_t tag) {
    size_t error = -1;
    for (size_t i = 0; i < nodeTags_n; i++) {
        if(nodeTags[i] == tag) return i;
    }
    return error;
}

int main(int argc, char **argv) {
    int error = 0;
    if(argc < 2) {
        printf("Usage: %s model.step\n", argv[0]);
        return EXIT_FAILURE;
    }

    const int clmin = 10;
    const int clmax = 20;

    const char *modelFile = "mesh.msh";
    const char *coordsFile = "../build/geo.txt";

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

    // fprintf(file, "%zu\n", nodeTags_n);
    // for (int i = 0; i < nodeTags_n; i++) {
    //     fprintf(file, "%.2f %.2f %.2f\n", coords[i * 3], coords[i * 3 + 1], coords[i * 3 + 2]);
    // }

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

    // int etype = *elementTypes; // только линейный треугольный КЭ

    const int ndof = 3; // число узлов КЭ

    fprintf(file, "%zu\n", *elementNodeTags_n);

    for(size_t e = 0; e < *elementTags_n; e++) {
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

            fprintf(file, "%.3f %.3f %.3f\n", x, y, z);
        }
    }

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