#include "VtxData.h"
#include "Utils.h"

#include <stdio.h>
#include <assert.h>

MeshFileHeader loadMeshData(const char *meshFile, MeshData& out)
{
    MeshFileHeader header;

    FILE* f = fopen(meshFile, "rb");
    assert(f);

    if(!f)
    {
        printf("Cannot open %s. Did you forget to run CH5 MeshConvert?\n", meshFile);
        exit(EXIT_FAILURE);
    }

    if(fread(&header, 1, sizeof(header), f) != sizeof(header))
    {
        printf("Unable to read mesh file header from %s\n", meshFile);
        exit(EXIT_FAILURE);
    }

    out.meshes.resize(header.meshCount);
    if (fread(out.meshes.data(), sizeof(Mesh), header.meshCount, f) != header.meshCount)
    {
        printf("Could not read mesh descriptor from %s\n", meshFile);
        exit(EXIT_FAILURE);
    }

    out.boxes.resize(header.meshCount);
    if (fread(out.boxes.data(), sizeof(BoundingBox), header.meshCount, f) != header.meshCount)
    {
        printf("Could not read bounding box data from %s\n", meshFile);
        exit(EXIT_FAILURE);
    }

    out.indexData.resize(header.indexDataSize / sizeof(uint32_t));
    out.vertexData.resize(header.vertexDataSize / sizeof(float));

    if(fread(out.indexData.data(), 1, header.indexDataSize, f) != header.indexDataSize)
    {
        printf("Unable to read index data from %s\n", meshFile);
        exit(255);
    }

    if(fread(out.vertexData.data(), 1, header.vertexDataSize, f) != header.vertexDataSize)
    {
        printf("Unable to read vertex data from %s\n", meshFile);
    }

    fclose(f);

    return header;
}

void saveMeshData(const char *fileName, MeshData &m)
{
    FILE* f = fopen(fileName, "wb");

    const MeshFileHeader header =
    {
        .magicValue = 0x12345678,
        .meshCount = (uint32_t)m.meshes.size(),
        .dataBlockStartOffset = (uint32_t)(sizeof(MeshFileHeader) + (m.meshes.size() * sizeof(Mesh)) ),
        .indexDataSize = (uint32_t)(m.indexData.size() * sizeof(uint32_t)),
        .vertexDataSize = (uint32_t)(m.vertexData.size() * sizeof(float))
    };

    fwrite(&header, 1, sizeof(header), f);
    fwrite(m.meshes.data(), sizeof(Mesh), header.meshCount, f);
    fwrite(m.boxes.data(), sizeof(BoundingBox), header.meshCount, f);
    fwrite(m.indexData.data(), 1, header.indexDataSize, f);
    fwrite(m.vertexData.data(), 1, header.vertexDataSize, f);

    fclose(f);
}

void recalculateBoundingBoxes(MeshData &m)
{
    m.boxes.clear();

    for(const Mesh& mesh : m.meshes)
    {
        const uint64_t numIndices = mesh.getLODIndicesCount(0);

        glm::vec3 vmin(std::numeric_limits<float>::max());
        glm::vec3 vmax(std::numeric_limits<float>::lowest());

        for (uint64_t i = 0; i != numIndices; i++)
        {
            uint32_t vtxOffset = m.indexData[mesh.indexOffset + i] + mesh.vertexOffset;
            const float* vf = &m.vertexData[vtxOffset * kMaxStreams];
            vmin = glm::min(vmin, vec3(vf[0], vf[1], vf[2]));
            vmax = glm::max(vmax, vec3(vf[0], vf[1], vf[2]));
        }

        m.boxes.emplace_back(vmin, vmax);
    }
}

MeshFileHeader mergeMeshData(MeshData &m, const std::vector<MeshData *> md)
{
    uint32_t totalVertedDataSize = 0;
    uint32_t totalIndexDataSize = 0;

    uint32_t offs = 0;
    for(const MeshData* i : md)
    {
        mergeVectors(m.indexData, i->indexData);
        mergeVectors(m.vertexData, i->vertexData);
        mergeVectors(m.meshes, i->meshes);
        mergeVectors(m.boxes, i->boxes);

        uint32_t vtxOffset = totalVertedDataSize / 8;

        for(size_t j = 0; j < (uint32_t)i->meshes.size(); j++)
        {
            // m.vertexCount, m.lodCount, and m.streamCount do not change
            // m.vertexOffset also does not change, because vertex offsets are local (i.e., baked into the indices)
            m.meshes[offs + j].indexOffset += totalIndexDataSize;
        }

        // Shift inidividual indices
        for(size_t j = 0; j < i->indexData.size(); j++)
        {
            m.indexData[totalIndexDataSize + j] += vtxOffset;
        }

        offs += (uint32_t)i->meshes.size();

        totalIndexDataSize += (uint32_t)i->indexData.size();
        totalVertedDataSize += (uint32_t)i->vertexData.size();
    }

    return MeshFileHeader
    {
        .magicValue = 0x12345678,
        .meshCount = (uint32_t)offs,
        .dataBlockStartOffset = (uint32_t)(sizeof(MeshFileHeader) + offs * sizeof(Mesh)),
        .indexDataSize = static_cast<uint32_t>(totalIndexDataSize * sizeof(uint32_t)),
        .vertexDataSize = static_cast<uint32_t>(totalVertedDataSize * sizeof(float))
    };
}
