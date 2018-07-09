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

int triangle_counting(int neighbor[9],
                      int offset[7],
                      int edge[18],
                      int num_edges);
void triangle_counting_onePort(int* data_in);

void read_graph(const char *filename, 
                std::vector<int> *edge_list, 
                unsigned int num_pe, 
                std::vector<int> &neighbor_list, 
                std::vector<int> &offset_list)
{
    std::ifstream ifs(filename);

    int degree_count = 0;
    int prev_node = 0;
    int pe_idx = 0;
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
                    edge_list[pe_idx % num_pe].push_back(v);
                    edge_list[pe_idx % num_pe].push_back(u);
                    pe_idx++;
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
//    num_edge = edge_list.size() / 2;
}

int main()
{
    int num_edge = 0;
    std::vector<int> edge_list[8], neighbor_list, offset_list;
    read_graph("soc-Epinions1_adj.tsv", edge_list,8, neighbor_list, offset_list);
    std::cout << "neighbor_list size= " << neighbor_list.size() << std::endl;
    std::cout << "offset_list size= " << offset_list.size() << std::endl;
    std::cout << "edge_list0 size= " << edge_list[0].size() << std::endl;
    std::cout << "edge_list1 size= " << edge_list[1].size() << std::endl;
    std::cout << "edge_list2 size= " << edge_list[2].size() << std::endl;
    std::cout << "edge_list3 size= " << edge_list[3].size() << std::endl;
    std::cout << "edge_list4 size= " << edge_list[4].size() << std::endl;
    std::cout << "edge_list5 size= " << edge_list[5].size() << std::endl;
    std::cout << "edge_list6 size= " << edge_list[6].size() << std::endl;
    std::cout << "edge_list7 size= " << edge_list[7].size() << std::endl;
    

    int neighbor_size = neighbor_list.size();
    int offset_size = offset_list.size();

    int num_edge0 = edge_list[0].size();
    int num_edge1 = edge_list[1].size();
    int num_edge2 = edge_list[2].size();
    int num_edge3 = edge_list[3].size();
    int num_edge4 = edge_list[4].size();
    int num_edge5 = edge_list[5].size();
    int num_edge6 = edge_list[6].size();
    int num_edge7 = edge_list[7].size();
    



    int *all_data = (int *)malloc( (11+ neighbor_size +offset_size + 
        num_edge0+num_edge1+num_edge2+num_edge3+num_edge4+num_edge5+num_edge6+num_edge7) * sizeof(int) );

    //// all_data format
    // [0]: result --- count
    // [1-8]: num_edge
    // [9]: neighbor_size
    // [10]: offset_size
    // [..]: neighbors
    // [..]: offsets
    // [..]: edges

    int *addr = all_data + 1;

    std::memcpy(addr, &num_edge0, 1 * sizeof(int));
    addr++;
    std::memcpy(addr, &num_edge1, 1 * sizeof(int));
    addr++;
    std::memcpy(addr, &num_edge2, 1 * sizeof(int));
    addr++;
    std::memcpy(addr, &num_edge3, 1 * sizeof(int));
    addr++;
    std::memcpy(addr, &num_edge4, 1 * sizeof(int));
    addr++;
    std::memcpy(addr, &num_edge5, 1 * sizeof(int));
    addr++;
    std::memcpy(addr, &num_edge6, 1 * sizeof(int));
    addr++;
    std::memcpy(addr, &num_edge7, 1 * sizeof(int));
    addr++;

    std::memcpy(addr, &neighbor_size, 1 * sizeof(int));
    addr++;

    std::memcpy(addr, &offset_size,	1 * sizeof(int));
    addr++;

    std::memcpy(addr, neighbor_list.data(), neighbor_size * sizeof(int));
    addr += neighbor_size;

    std::memcpy(addr, offset_list.data(), offset_size * sizeof(int));
    addr += offset_size;

    std::memcpy(addr, edge_list[0].data(), num_edge0 * sizeof(int));
    addr += num_edge0;
    std::memcpy(addr, edge_list[1].data(), num_edge1 * sizeof(int));
    addr += num_edge1;
    std::memcpy(addr, edge_list[2].data(), num_edge2 * sizeof(int));
    addr += num_edge2;
    std::memcpy(addr, edge_list[3].data(), num_edge3 * sizeof(int));
    addr += num_edge3;
    std::memcpy(addr, edge_list[4].data(), num_edge4 * sizeof(int));
    addr += num_edge4;
    std::memcpy(addr, edge_list[5].data(), num_edge5 * sizeof(int));
    addr += num_edge5;
    std::memcpy(addr, edge_list[6].data(), num_edge6 * sizeof(int));
    addr += num_edge6;
    std::memcpy(addr, edge_list[7].data(), num_edge7 * sizeof(int));



    //int result = triangle_counting(neighbors, offsets, edges, num_edge);
    triangle_counting_onePort(all_data);

    printf("result = %d\n", all_data[0]);
    return 0;
}
