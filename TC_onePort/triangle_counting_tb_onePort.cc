#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

int triangle_counting( int neighbor[9],
                       int offset[7],
                       int edge[18],
                       int num_edges);

void triangle_counting_onePort(int* data_in);

void read_graph(const char *filename,
                std::vector<int> &edge_list,
                std::vector<int> &neighbor_list,
                std::vector<int> &offset_list,
                int &num_edge)
{
    std::ifstream ifs(filename);

    int degree_count = 0;
    int prev_node = 0;
    offset_list.push_back(0);

    if (ifs.is_open() && ifs.good())
    {
        std::string str;
        while (std::getline(ifs, str))
        {
            if (!str.empty() && str[0] != '#')
            {
                std::istringstream ss(str);
                int u, v;
                ss >> u >> v;
                if (prev_node != v)
                {
                    offset_list.push_back(degree_count);
                }

                prev_node = v;
                if (u < v)
                {
                    edge_list.push_back(v);
                    edge_list.push_back(u);
                }
                else
                {
                    neighbor_list.push_back(u);
                    degree_count++;
                }
            }
        }
    }
    ifs.close();
    offset_list.push_back(degree_count);
    num_edge = edge_list.size();
}


int main()
{
    int num_edge = 0;
    std::vector<int> edge_list, neighbor_list, offset_list;
    read_graph("soc-Epinions1_adj.tsv", edge_list, neighbor_list, offset_list, num_edge);
    std::cout << "neighbor_list size= " << neighbor_list.size() << std::endl;
    std::cout << "offset_list size= " << offset_list.size() << std::endl;
    std::cout << "edge_list size= " << edge_list.size() << std::endl;
    std::cout << "initialized num_edge = " << num_edge << std::endl;

    int *edges     = (int *)malloc(	edge_list.size()*sizeof(int) );
    int *neighbors = (int *)malloc(	neighbor_list.size()*sizeof(int) );
    int *offsets   = (int *)malloc(	offset_list.size()*sizeof(int) );

    std::memcpy(edges    ,     edge_list.data(),     edge_list.size()*sizeof(int));
    std::memcpy(neighbors, neighbor_list.data(), neighbor_list.size()*sizeof(int));
    std::memcpy(offsets  ,   offset_list.data(),   offset_list.size()*sizeof(int));

    int neighbor_size = neighbor_list.size();
    int offset_size = offset_list.size();
    int edge_size = edge_list.size();


    int *all_data = (int *)malloc( (neighbor_size + edge_size + offset_size + 4) * sizeof(int) );

    //// all_data format
    // [0]: result --- count
    // [1]: num_edge
    // [2]: neighbor_size
    // [3]: offset_size
    // [4]: edge_size
    // [..]: neighbors
    // [..]: offsets
    // [..]: edges

    int *addr = all_data + 1;
    std::memcpy(addr, &num_edge, 1 * sizeof(int));
    addr++;
    std::memcpy(addr, &neighbor_size, 1 * sizeof(int));
    addr++;
    std::memcpy(addr, &offset_size,	1 * sizeof(int));
    addr++;
    std::memcpy(addr, &edge_size, 1 * sizeof(int));
    addr++;
    std::memcpy(addr, neighbors, neighbor_size * sizeof(int));
    addr += neighbor_size;
    std::memcpy(addr, offsets, offset_size * sizeof(int));
    addr += offset_size;
    std::memcpy(addr, edges, edge_size * sizeof(int));


    //int result = triangle_counting(neighbors, offsets, edges, num_edge);
    triangle_counting_onePort(all_data);

    printf("result = %d\n", all_data[0]);
    return 0;
}
