#include "MeshConvert.h"

#include <assimp/postprocess.h>
#include <assimp/cimport.h>


bool verbose = true;

std::vector<Mesh> m_meshes;
std::vector<uint32_t> m_indexData;
std::vector<float> m_vertexData;
// MeshData m_meshData;

uint32_t m_indexOffset = 0;
uint32_t m_vertexOffset = 0;
bool bExportTextures = false;
bool bExportNormals = false;

constexpr uint32_t m_numElementsToStore = 3;

// float m_meshScale = 0.01f;
// bool m_bCalculateLODs = false;

Mesh convertAIMesh(const aiMesh *m)
{
    // Check whether the original mesh has texture coordinates
    const bool hasTexCoords = m->HasTextureCoords(0);

    const uint32_t numIndices = m->mNumFaces * 3;
    const uint32_t numElements = m_numElementsToStore;

    const uint32_t streamElementSize = static_cast<uint32_t>(numElements * sizeof(float));
    const uint32_t meshSize = static_cast<uint32_t>(m->mNumVertices * streamElementSize + numIndices * sizeof(uint32_t));

    const Mesh result = 
    { 
        .lodCount = 1, 
        .streamCount = 1, 
        .materialID = 0, 
        .meshSize = meshSize, 
        .vertexCount = m->mNumVertices,
        .lodOffset = {
            (uint32_t)(m_indexOffset * sizeof(uint32_t)),
            (uint32_t)((m_indexOffset + numIndices) * sizeof(uint32_t))
        },
        .streamOffset = {
            m_vertexOffset * streamElementSize
        },
        .streamElementSize = { streamElementSize }
    };

    for(size_t i = 0; i != m->mNumVertices; i++)
    {
        const aiVector3D& v = m->mVertices[i];
        const aiVector3D& n = m->mNormals[i];
        const aiVector3D& t = hasTexCoords ? m->mTextureCoords[0][i] : aiVector3D();
        
        m_vertexData.push_back(v.x);
        m_vertexData.push_back(v.y);
        m_vertexData.push_back(v.z);
        
        if(bExportTextures)
        {
            m_vertexData.push_back(t.x);
            m_vertexData.push_back(t.y);
        }

        if(bExportNormals)
        {
            m_vertexData.push_back(n.x);
            m_vertexData.push_back(n.y);
            m_vertexData.push_back(n.z);
        }
    }

    for(size_t i = 0; i != m->mNumFaces; i++)
    {
        const aiFace& F = m->mFaces[i];
        m_indexData.push_back(F.mIndices[0] + m_vertexOffset);
        m_indexData.push_back(F.mIndices[1] + m_vertexOffset);
        m_indexData.push_back(F.mIndices[2] + m_vertexOffset);
    }

    m_indexOffset += numIndices;
    m_vertexOffset += m->mNumVertices;

    return result;
}


bool loadFile(const char *fileName)
{
    if(verbose) printf("Loading '%s'...\n", fileName);
    const unsigned int flags =
        aiProcess_JoinIdenticalVertices |
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_PreTransformVertices |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_FindDegenerates |
        aiProcess_FindInvalidData |
        aiProcess_FindInstances |
        aiProcess_OptimizeMeshes;
    
    const aiScene* scene = aiImportFile(fileName, flags);
    if(!scene || !scene->HasMeshes())
    {
        printf("Unable to load '%s'\n", fileName);
        return false;
    }

    m_meshes.reserve(scene->mNumMeshes);
    for(size_t i = 0; i != scene->mNumMeshes; i++)
    {
        m_meshes.push_back(convertAIMesh(scene->mMeshes[i]));
    }
    return true;
}

void saveMeshToFile(FILE *f)
{
    const MeshFileHeader header =
    {
        .magicValue = 0x12345678,
        .meshCount = (uint32_t)m_meshes.size(),
        .dataBlockStartOffset = (uint32_t)(sizeof(MeshFileHeader) + m_meshes.size() * sizeof(Mesh)),
        .indexDataSize = (uint32_t)(m_indexData.size() * sizeof(uint32_t)),
        .vertexDataSize = (uint32_t)(m_vertexData.size() * sizeof(float))
    };

    fwrite(&header, 1, sizeof(header), f);
    fwrite(m_meshes.data(), header.meshCount, sizeof(Mesh), f);
}
