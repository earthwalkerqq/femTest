#include <gmshc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "generateMesh.h"

#define MESH_ORDER 1
#define DIMENSION 2

void initHyperMesh(char *inputFile, char *outputMeshFile, double clmin, double clmax, int *error) {
    if (!gmshIsInitialized(error)) {
        gmshInitialize(1, NULL, 1, 0, error);
    }

    if (!*error) {
        gmshMerge(inputFile, error);
        if (*error) return;

        gmshOptionSetNumber("Mesh.CharacteristicLengthMax", clmax, error);
        gmshOptionSetNumber("Mesh.CharacteristicLengthMin", clmin, error);

        gmshModelMeshSetOrder(MESH_ORDER, error);

        gmshModelMeshGenerate(DIMENSION, error);

        gmshWrite(outputMeshFile, error);
    }
}

bool_t hyperMesh(char *cadFile, double clmin, double clmax) {
    bool_t error = EXIT_SUCCESS;

    const char *outputNodeFile = "mesh_output.txt";
    const char *outputMeshFile = "HyperMesh.msh";

    if (!cadFile) {
        perror("CAD file not provided.");
        return EXIT_FAILURE;
    }

    initHyperMesh(cadFile, (char *)outputMeshFile, clmin, clmax, (int *)&error);
    if (error) {
        fprintf(stderr, "Mesh generation failed.\n");
        return EXIT_FAILURE;
    }

    // Получение узлов
    size_t nodeTags_n, coord_n, param_n;
    double *nodeCoords = NULL, *nodeParams = NULL;
    size_t *nodeTags = NULL;

    gmshModelMeshGetNodes(&nodeTags, &nodeTags_n, &nodeCoords, &coord_n, &nodeParams, &param_n, -1, 0, 0, 0, (int *)&error);
    if (error || nodeTags == NULL || nodeCoords == NULL) {
        fprintf(stderr, "Failed to get mesh nodes.\n");
        return EXIT_FAILURE;
    }

    // Сопоставление тегов узлов с индексами
    size_t maxTag = 0;
    for (size_t i = 0; i < nodeTags_n; i++) {
        if (nodeTags[i] > maxTag) maxTag = nodeTags[i];
    }

    size_t *tagIndex = (size_t *)calloc(maxTag + 1, sizeof(size_t));
    if (!tagIndex) {
        fprintf(stderr, "Memory allocation failed.\n");
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < nodeTags_n; i++) {
        tagIndex[nodeTags[i]] = i + 1;  // индексы от 1
    }

    // Получение элементов (только треугольники)
    int *elementTypes = NULL;
    size_t elementTypes_n = 0;
    size_t **elementTags = NULL, **elementNodes = NULL;
    size_t *elementTags_n = NULL, *elementNodes_n = NULL;
    size_t elementTags_nn = 0, elementNodes_nn = 0;

    gmshModelMeshGetElements(&elementTypes, &elementTypes_n,
                             &elementTags, &elementTags_n, &elementTags_nn,
                             &elementNodes, &elementNodes_n, &elementNodes_nn,
                             -1, -1, NULL);

    FILE *file = fopen(outputNodeFile, "w");
    if (!file) {
        perror("Failed to open output file.");
        free(tagIndex);
        return EXIT_FAILURE;
    }

    // Пишем количество узлов
    fprintf(file, "%zu\n", nodeTags_n);

    // Пишем координаты узлов
    for (size_t i = 0; i < nodeTags_n; i++) {
        fprintf(file, "%.6lf %.6lf %.6lf\n",
                nodeCoords[i * 3],
                nodeCoords[i * 3 + 1],
                nodeCoords[i * 3 + 2]);
    }

    // Подсчитываем и записываем только треугольники
    size_t triCount = 0;

    for (size_t i = 0; i < elementTypes_n; i++) {
        if (elementTypes[i] == 2) {  // тип 2 — треугольник
            triCount += elementTags_n[i];
        }
    }

    fprintf(file, "%zu\n", triCount);  // количество треугольников

    for (size_t i = 0; i < elementTypes_n; i++) {
        if (elementTypes[i] != 2) continue;

        size_t nElem = elementTags_n[i];
        size_t nodesPerElem = elementNodes_n[i] / nElem;

        for (size_t j = 0; j < nElem; j++) {
            for (size_t k = 0; k < nodesPerElem; k++) {
                size_t nodeTag = elementNodes[i][j * nodesPerElem + k];
                size_t index = tagIndex[nodeTag];
                fprintf(file, "%zu%c", index, (k < nodesPerElem - 1) ? ' ' : '\n');
            }
        }
    }

    fclose(file);
    free(tagIndex);
    gmshFinalize((int *)&error);

    return error;
}
