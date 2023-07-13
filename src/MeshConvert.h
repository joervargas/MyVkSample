#pragma once

#include "VtxData.h"
#include <assimp/scene.h>


Mesh convertAIMesh(const aiMesh* m);

bool loadFile(const char* fileName);

void saveMeshToFile(FILE* f);