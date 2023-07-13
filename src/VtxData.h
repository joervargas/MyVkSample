#pragma once

#include <stdint.h>

#include <glm/glm.hpp>

#include "UtilsMath.h"

// Max number of LODs
constexpr const uint32_t kMaxLODs       = 8;
// Max number of vertex streams; vertex streams is a term for vertex attributes in an homogenous array
constexpr const uint32_t kMaxStreams    = 8;

struct Mesh final
{
    /* Number of LODs on this mesh*/
    uint32_t lodCount = 1;

    /* Number of vertex data streams (vertex attribute arrays) */
    uint32_t streamCount = 0;

    // /* The offset for the index data is equal to the total count of all previous vertices in this mesh */
    // uint32_t indexOffset = 0;

    // uint32_t vertexOffset = 0;

    /* An abstract identifier to reference material data stored elsewhere (subject to change)*/
    uint32_t materialID = 0;

    /* equal to the total sum of all LOD index array sizes and the sum of all stream sizes */
    uint32_t meshSize;

    /* The number of vertices in this mesh */
    uint32_t vertexCount;

    /* Contains all the offsets to the LOD index data. 
     * Extra space at the end is a marker to calculate the size of last LOD. 
     */
    uint32_t lodOffset[kMaxLODs];

    /**
     * we use a function to calculate the size;
     */
    inline uint64_t getLODSize(uint32_t lod) const
    {
        return lodOffset[lod + 1] - lodOffset[lod];
    }

    /**
     *  stores offsets to all the individual vertex data streams (vertex attribute arrays)
     */
    uint64_t streamOffset[kMaxStreams];
    
    /**
     * contains the element size for each attribute in vertex streams
     */
    uint32_t streamElementSize[kMaxStreams];
};

struct MeshFileHeader
{
    /* Hexadecimal value at top of file */
    uint32_t magicValue;

    /* The number of different meshes in this file */
    uint32_t meshCount;

    /* The offset to the beginning of the mesh data */
    uint32_t dataBlockStartOffset;

    /* index size in bytes */
    uint32_t indexDataSize;

    /* vertex size in bytes */
    uint32_t vertexDataSize;
};

struct DrawData
{
    uint32_t meshIndex;
    uint32_t materialIndex;
    uint32_t LOD;
    uint32_t indexOffset;
    uint32_t transformIndex;
};

struct MeshData
{
    std::vector<uint32_t> indexData;
    std::vector<float> vertexData;
    std::vector<Mesh> meshes;
    std::vector<BoundingBox> boxes;
};

struct InstanceData
{
    float transform[16];
    uint32_t meshIndex = 0;
    uint32_t materialIndex = 0;
    uint32_t LOD = 0;
    uint32_t m_indexOffset = 0;
};