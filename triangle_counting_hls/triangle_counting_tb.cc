#include <stdio.h>
#include <iostream>
#include <fstream>

#include "triangle_counting.h"

int triangle_counting(volatile unsigned int neighbor[9],
                      volatile unsigned int offset[7],
                      volatile unsigned int edge[18],
                      unsigned int num_edges,
                      unsigned int progress[5]);

int main()
{
    std::ifstream fin("/home/shuang91/vivado_projects/graph_challenge/triangle_counting_hls/soc-Epinions1_adj_parsed.tsv", std::ifstream::in);
    unsigned int len_neighbor, len_offset, len_edge;
    fin >> len_neighbor;
    fin >> len_offset;
    fin >> len_edge;

    std::cout << len_neighbor << " " << len_offset << " " << len_edge;

    unsigned int *neighbor = new unsigned int[len_neighbor];
    unsigned int *offset = new unsigned int[len_offset];
    unsigned int *edge = new unsigned int[len_edge];

    for (int i = 0; i < len_neighbor; i++)
        fin >> neighbor[i];
    for (int i = 0; i < len_offset; i++)
            fin >> offset[i];
    for (int i = 0; i < len_edge; i++)
            fin >> edge[i];

    fin.close();


/*  unsigned int neighbor[] = {2, 4, 5, 3, 4, 5, 4, 5, 5};
    unsigned int offset[] = {0, 0, 3, 6, 8, 9, 9};
    unsigned int edge[] = {5, 4, 5, 3, 5, 2, 5, 1, 4, 3, 4, 2, 4, 1, 3, 2, 2, 1};
    */
    unsigned int progress[5];
    int result = triangle_counting(neighbor, offset, edge, len_edge, progress);
    printf("result = %d\n", result);
    return 0;
}
