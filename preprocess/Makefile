CC := gcc
CFLAGS := -Wall -Wextra -Werror

GMSH_LIB = ../dependencies/gmsh/lib
GMSH_INCLUDE = ../dependencies/gmsh/include

OBJ_RES = meshGen.o

all: build

build:
	$(CC) main.c generateMesh.c -L$(GMSH_LIB) -I$(GMSH_INCLUDE) -lgmsh -o $(OBJ_RES)

clean:
	rm -rf ./*.o
	rm -rf ./*.out

rebuild: clean build