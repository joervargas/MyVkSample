#include "MeshConvert.h"

#include <stdio.h>

int main(int argc, char* argv[])
{
    if(argc == 1)
    {
        printf("Please provide a file as an argument");
        exit(EXIT_FAILURE);
    }

    if(argc > 2)
    {
        printf("More than 1 specified commands have been entered");
        exit(EXIT_FAILURE);
    }

    loadFile(argv[1]);

    MeshData mesh_data;
    std::vector<DrawData> grid;
    // for(size_t i = 0; i < )

    return 0;
}