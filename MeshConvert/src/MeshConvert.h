#pragma once

#include <VtxData.h>
#include <assimp/scene.h>

extern bool bVerbose;

extern MeshData m_meshData;

extern uint32_t m_indexOffset;
extern uint32_t m_vertexOffset;

extern bool bExportTextureCoords;
extern bool bExportNormals;
extern bool bCalculateLODs;

extern float m_meshScale;

extern uint32_t m_numElementsToStore;

void ms_process_lods(std::vector<uint32_t>& indices, std::vector<float>& vertices, std::vector<std::vector<uint32_t>>& out_lods);

Mesh ms_convert_ai_mesh(const aiMesh* m);

bool ms_load_file(const char* fileName);

// void saveMeshToFile(FILE* f);