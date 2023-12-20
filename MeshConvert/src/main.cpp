#include "MeshConvert.h"

#include <stdio.h>


int main(int argc, char* argv[])
{
    // if(argc == 1)
    // {
    //     printf("Please provide a file as an argument\n");
    //     exit(EXIT_FAILURE);
    // }

    // if(argc == 1 || argc < 3)
    // {
    //     printf("Usage: meshconvert <input> <output> [options]\n\n");
    //     printf("Options: \n");
    //     printf("\t--export-texcoords\t|\t-t:\texport texture coordinates\n");
    //     printf("\t--export-normals\t|\t-n:\texport normals\n");
    //     exit(EXIT_FAILURE);
    // }

    // bool b_export_texture_coords = false;
    // bool b_export_normals = false;

    // for(int i = 3; i < argc; i++)
    // {
    //     b_export_texture_coords |= !strcmp(argv[i], "--export-texcoords") || !strcmp(argv[i], "-t");
    //     b_export_normals |= !strcmp(argv[i], "--export-normals") || !strcmp(argv[i], "-n");
    //     const bool export_all = !strcmp(argv[i], "-tn") || !strcmp(argv[i], "-nt");

    //     b_export_texture_coords |= export_all;
    //     b_export_normals |= export_all;
    // }
    // bExportTextureCoords = b_export_texture_coords;
    // bExportNormals = b_export_normals;
    bExportTextureCoords = true;
    bExportNormals = true;

    if(bExportTextureCoords) m_numElementsToStore += 2;
    if(bExportNormals) m_numElementsToStore += 3;

    // Load file after all parameters are set
    // if(!ms_load_file(argv[1])) exit(EXIT_FAILURE);
    if(!ms_load_file("./assets/meshes/Exterior/exterior.obj")) exit(EXIT_FAILURE);
    // printf("File: '%s' loaded successfully\n", argv[1]);
    printf("Setting dst_file");
    // char* dst_file = argv[2];
    char dst_file[] = "./assets/meshes/Exterior/exterior.vk_sample";
    // dst_file = argv[2];
    printf("dst_file set...");

    std::vector<DrawData> grid;
    m_vertexOffset = 0;
    for(size_t i = 0; i < m_meshData.meshes.size(); i++)
    {
        grid.push_back(
            DrawData
            {
                .meshIndex = (uint32_t)i,
                .materialIndex = 0,
                .LOD = 0,
                .indexOffset = m_meshData.meshes[i].indexOffset,
                .vertexOffset = m_vertexOffset,
                .transformIndex = 0
            }
        );
        m_vertexOffset += m_meshData.meshes[i].vertexCount;
    }
    printf("Saving mesh data to %s ...\n", dst_file);
    saveMeshData(dst_file, m_meshData);

    char drawdata_file_extension[] = ".drawdata";
    // size_t buffer_size = strlen(drawdata_file_extension) + strlen(dst_file) + 1;
    const size_t buffer_size = 3072;
    // char grid_file[buffer_size] = "";
    char grid_file[buffer_size];
    memset(grid_file, '\0', buffer_size);
    strcpy(grid_file, dst_file);
    strcat(grid_file, drawdata_file_extension);

    printf("Saving draw data to %s ...\n", grid_file);
    FILE* f = fopen(grid_file, "wb");
    fwrite(grid.data(), grid.size(), sizeof(DrawData), f);
    fclose(f);

    return 0;
}