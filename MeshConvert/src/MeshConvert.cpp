#include "MeshConvert.h"

#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <meshoptimizer.h>


bool bVerbose = true;

MeshData m_meshData;

uint32_t m_indexOffset = 0;
uint32_t m_vertexOffset = 0;

bool bExportTextureCoords = false;
bool bExportNormals = false;
bool bCalculateLODs = false;

float m_meshScale = 0.01f;

uint32_t m_numElementsToStore = 3;

// float m_meshScale = 0.01f;
// bool m_bCalculateLODs = false;

void ms_process_lods(std::vector<uint32_t> &indices, std::vector<float> &vertices, std::vector<std::vector<uint32_t>> &out_lods)
{
    size_t verticesCountIn = vertices.size() / 2;
    size_t targetIndicesCount = indices.size();

    uint8_t LOD = 1;

    if(bVerbose) printf("\n\tLOD0: %i indices", int(indices.size()));

    out_lods.push_back(indices);
    
    while(targetIndicesCount > 1024 && LOD < 8)
    {
        targetIndicesCount = indices.size() / 2;
        bool bSloppy = false;

        size_t numOptIndices = meshopt_simplify(
            indices.data(),
            indices.data(), (uint32_t)indices.size(),
            vertices.data(), verticesCountIn,
            sizeof(float) * 3,
            targetIndicesCount, 0.02f
        );

        // cannot simplify further
        if(static_cast<size_t>(numOptIndices * 1.1f) > indices.size())
        {
            if(LOD > 1)
            {
                // try without u32
                numOptIndices = meshopt_simplifySloppy(
                    indices.data(),
                    indices.data(), indices.size(),
                    vertices.data(), verticesCountIn,
                    sizeof(float) * 3,
                    targetIndicesCount, 0.02f
                );
            } else {
                break;
            }
        }

        indices.resize(numOptIndices);

        meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), verticesCountIn);

        if(bVerbose) printf("\n\tLOD%i: %i indices", int(LOD), int(indices.size()));

        LOD++;

        out_lods.push_back(indices);
    }
}

Mesh ms_convert_ai_mesh(const aiMesh *m)
{
    // Check whether the original mesh has texture coordinates
    const bool hasTexCoords = m->HasTextureCoords(0);
    const uint32_t streamElementSize = static_cast<uint32_t>(m_numElementsToStore * sizeof(float));

    std::vector<float> srcVertices;
    std::vector<uint32_t> srcIndices;

    std::vector<std::vector<uint32_t>> out_lods;

    std::vector<float>& vertices = m_meshData.vertexData;

    for(size_t i = 0; i != m->mNumVertices; i++)
    {
        const aiVector3D v = m->mVertices[i];
        const aiVector3D n = m->mNormals[i];
        const aiVector3D t = hasTexCoords ? m->mTextureCoords[0][i] : aiVector3D();

        if(bCalculateLODs)
        {
            srcVertices.push_back(v.x);
            srcVertices.push_back(v.y);
            srcVertices.push_back(v.z);
        }

        vertices.push_back(v.x * m_meshScale);
        vertices.push_back(v.y * m_meshScale);
        vertices.push_back(v.z * m_meshScale);

        if(bExportTextureCoords)
        {
            vertices.push_back(t.x);
            vertices.push_back(1.0f - t.y);
        }
        
        if(bExportNormals)
        {
            vertices.push_back(n.x);
            vertices.push_back(n.y);
            vertices.push_back(n.z);
        }
    }

    Mesh result =
    {
        .streamCount = 1,
        .indexOffset = m_indexOffset,
        .vertexOffset = m_vertexOffset,
        .vertexCount = m->mNumVertices,
        .streamOffset = { m_vertexOffset * streamElementSize },
        .streamElementSize = { streamElementSize }
    };

    for(size_t i = 0; i != m->mNumFaces; i++)
    {
        if(m->mFaces[i].mNumIndices != 3) continue;

        for(unsigned int j = 0; j != m->mFaces[i].mNumIndices; j++)
        {
            srcIndices.push_back(m->mFaces[i].mIndices[j]);
        }
    }

    if(!bCalculateLODs)
    {
        out_lods.push_back(srcIndices);
    } else {
        ms_process_lods(srcIndices, srcVertices, out_lods);
    }

    printf("\nCalculated LOD count: %u\n", (unsigned int)out_lods.size());

    uint32_t numIndices = 0;
    for(size_t l = 0; l < out_lods.size(); l++)
    {
        for(size_t i = 0; i < out_lods[i].size(); i++)
        {
            m_meshData.indexData.push_back(out_lods[l][i]);
        }
        result.lodOffset[l] = numIndices;
        numIndices += (int)out_lods[l].size();
    }

    result.lodOffset[out_lods.size()] = numIndices;
    result.lodCount = (uint32_t)out_lods.size();

    m_indexOffset += numIndices;
    m_vertexOffset += m->mNumVertices;

    return result;
}


bool ms_load_file(const char *fileName)
{
    if(bVerbose) printf("Loading '%s'...\n", fileName);

    const unsigned int flags =
        aiProcess_JoinIdenticalVertices |
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_PreTransformVertices |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_FindDegenerates |
        aiProcess_FindInvalidData |
        aiProcess_FindInstances |
        aiProcess_OptimizeMeshes |
        aiProcess_GenUVCoords;
    
    const aiScene* scene = aiImportFile(fileName, flags);
    if(!scene || !scene->HasMeshes())
    {
        printf("Unable to load '%s'\n", fileName);
        return false;
    }

    m_meshData.meshes.reserve(scene->mNumMeshes);
    m_meshData.boxes.reserve(scene->mNumMeshes);

    for(size_t i = 0; i != scene->mNumMeshes; i++)
    {
        printf("\nConverting meshes %lu/%u...", i + 1, scene->mNumMeshes);
		fflush(stdout);
        m_meshData.meshes.push_back(ms_convert_ai_mesh(scene->mMeshes[i]));
    }

    // printf("Calculating Bounding Boxes..."); fflush(stdout);
    // recalculateBoundingBoxes(m_meshData);
    
    return true;
}

