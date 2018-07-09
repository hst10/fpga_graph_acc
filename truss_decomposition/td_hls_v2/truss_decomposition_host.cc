#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cassert>
/*#include <string>*/

#include <string.h>
#include <stdio.h>

#include "truss_decomposition.h"

void read_graph_td(const char *filename, 
                std::vector<int> &edge_list, 
                std::vector<int> &neighbor_list, 
                std::vector<int> &offset_list)
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
                neighbor_list.push_back(u);
                degree_count++;
            }
        }
    }
    ifs.close();
    offset_list.push_back(degree_count);
}


inline
int get_edge_idx(int node_a, int node_b, int* edge, int num_edges, int *hash_edge2idx)
{
//  std::cout << "TARGET: " << node_a << ", " << node_b << std::endl;
    unsigned int ht_idx = (unsigned int)(((node_a & 0x3ff) << 10) + (node_b & 0x3ff));

    bool edge_not_found = true;
    for (unsigned int entry_idx = 0; entry_idx < max_num_elems; entry_idx++)
    {
        unsigned int ht_value = hash_edge2idx[ht_idx*max_num_elems+entry_idx]; 
        if (ht_value != 0)
        {
            if ((node_a == edge[ht_value-1]) && (node_b == edge[ht_value])) // edge found
                return (ht_value+1)/2;
        }
        else
            break;
    }
    if (edge_not_found) // search in the whole edge list; not likely to be true
    {
        // std::cout << "SLOW SEARCH IN DRAM!!!" << std::endl;
        // std::cout << "=========================== SLOW: " << node_a << ", " << node_b << std::endl;
        for (int k = 0; k < num_edges; k+=2) // TODO: use binary search
        {
            if ((node_a == edge[k]) && (node_b == edge[k+1]))
                return k;
        }
        // std::cout << node_a << "->" << node_b << " DONE!!!" << std::endl;
    }
}

int init_triangle_counting(int neighbor[9],
                          int offset[7],
                          int edge[18],
                          int num_edges,
                          int edge_tc[9],
                          int *hash_edge2idx, 
                          unsigned int *local_a,
                          unsigned int *local_b,
                          std::vector<int>& triangle_list,
                          std::vector<int>& triangle_list_offset
                          )
{
    unsigned int i;
    unsigned int node_a, node_b;
    unsigned int offset_a, offset_b;
    unsigned int len_a, len_b;

    unsigned int local_count = 0; 
    unsigned int global_count = 0; 

    unsigned int idx_a = 0;
    unsigned int idx_b = 0;
    unsigned int prev_node_a = 0;
    unsigned int prev_node_b = 0;

    unsigned int edge_node_p = 0;
    unsigned int edge_node_q = 0;

    for (i = 0; i < num_edges; i+=2)
    {
#pragma HLS pipeline

        triangle_list_offset.push_back(global_count*2);

        node_a = edge[i];
        node_b = edge[i+1];

/*        if (node_a == 0 || node_b == 0)
            continue;*/

        if (node_a != prev_node_a)
        {
            offset_a = offset[node_a];
            len_a = offset[node_a + 1] - offset_a;
            memcpy(local_a, (const unsigned int*)&(neighbor[offset_a]), len_a*sizeof(unsigned int));
            prev_node_a = node_a; 
        }

        if (node_b != prev_node_b)
        {
            offset_b = offset[node_b];
            len_b = offset[node_b + 1] - offset_b;
            memcpy(local_b, (const unsigned int*)&(neighbor[offset_b]), len_b*sizeof(unsigned int));
            prev_node_b = node_b;
        }

        idx_a = 0;
        idx_b = 0;

        local_count = 0;

        while (idx_a < len_a && idx_b < len_b)
        {
#pragma HLS pipeline

            edge_node_p = local_a[idx_a]; 
            edge_node_q = local_b[idx_b]; 

            if (edge_node_p < edge_node_q)
                idx_a++;
            else if (edge_node_p > edge_node_q)
                idx_b++;
            else
            {
                int start_node, end_node; 
                start_node = (edge_node_p > node_a) ? edge_node_p : node_a;
                end_node   = (edge_node_p < node_a) ? edge_node_p : node_a;
                triangle_list.push_back(get_edge_idx(start_node, end_node, edge, num_edges, hash_edge2idx)); 
                start_node = (edge_node_p > node_b) ? edge_node_p : node_b;
                end_node   = (edge_node_p < node_b) ? edge_node_p : node_b;
                triangle_list.push_back(get_edge_idx(start_node, end_node, edge, num_edges, hash_edge2idx)); 
//                if ((edge_node_p != 0) && (edge_node_q != 0))
                    local_count++;
                idx_a++;
                idx_b++;
            }
        }
        global_count += local_count;
        edge_tc[i/2+1] = local_count;
    }
    triangle_list_offset.push_back(global_count*2);
}


int main()
{
    std::vector<int> edge_list, neighbor_list, offset_list, triangle_list, triangle_list_offset;
    read_graph_td("/home/shuang91/vivado_projects/graph_challenge/graph/soc-Epinions1_adj.tsv", edge_list, neighbor_list, offset_list);
    // read_graph_td("../graph/soc-Slashdot0811_adj.tsv", edge_list, neighbor_list, offset_list);
    std::cout << "neighbor_list size= " << neighbor_list.size() << std::endl;
    std::cout << "offset_list size= " << offset_list.size() << std::endl;
    std::cout << "edge_list size= " << edge_list.size() << std::endl;

    unsigned int local_a[BUF_SIZE];
    unsigned int local_b[BUF_SIZE];

    /* Construct hash table */
    std::cout << "hash_table_len = " << hash_table_len << std::endl;

    int *hash_edge2idx = (int *)malloc(max_num_elems*hash_table_len*sizeof(int));
    memset(hash_edge2idx, 0, max_num_elems*hash_table_len*sizeof(int));

    for (int i = 0; i < edge_list.size(); i+=2)
    {
        unsigned int u = edge_list[i]; 
        unsigned int v = edge_list[i+1]; 
        unsigned int ht_idx = (unsigned int)(((u & 0x3ff) << 10) + (v & 0x3ff));
        assert(ht_idx < hash_table_len); 
        int entry_idx = 0; 
        while((hash_edge2idx[ht_idx*max_num_elems+entry_idx] != 0) && (entry_idx < max_num_elems)) entry_idx++;
        if (entry_idx < max_num_elems)
            hash_edge2idx[ht_idx*max_num_elems+entry_idx] = i+1; 
        else
        {
            std::cout << "spilling edge: " << u << ", " << v << std::endl;
        }
    }

    std::vector<int> histogram; 
    histogram.resize(128); 
    for (int idx = 0; idx < histogram.size(); idx++)
        histogram[idx] = 0;

    for (int i = 0; i < hash_table_len; i++)
    {
        int counter = 0; 
        for (int j = 0; j < max_num_elems; j++)
            if (hash_edge2idx[i*max_num_elems+j] != 0)
                counter++;
        histogram[counter]++;
    }

    for (int i = 0; i < 128; i++)
    {
        if (histogram[i] != 0)
            std::cout << "size = " << i << ", num = " << histogram[i] << std::endl;
    }

    int *edges     = (int *)malloc(    edge_list.size()*sizeof(int));
    int *neighbors = (int *)malloc(neighbor_list.size()*sizeof(int));
    int *offsets   = (int *)malloc(  offset_list.size()*sizeof(int));
    int *edge_tc   = (int *)malloc((edge_list.size()/2+1)*sizeof(int));
    edge_tc[0] = 0;

    memcpy(edges    ,     edge_list.data(),     edge_list.size()*sizeof(int));
    memcpy(neighbors, neighbor_list.data(), neighbor_list.size()*sizeof(int));
    memcpy(offsets  ,   offset_list.data(),   offset_list.size()*sizeof(int));

    init_triangle_counting(neighbors, offsets, edges, edge_list.size(), edge_tc, hash_edge2idx, local_a, local_b, triangle_list, triangle_list_offset); 

    long long int count_check = 0;
    int max_num_triangles = 0;
    for (int i = 0; i <= edge_list.size()/2; i++)
    {
//        std::cout << edge_tc[i] << " ";
        count_check += edge_tc[i];
        if (edge_tc[i] > max_num_triangles)
            max_num_triangles = edge_tc[i];
    }

    std::cout << "num of triangles = " << count_check/3 << std::endl;
    std::cout << "max num of triangles of an edge  = " << max_num_triangles << std::endl;
    std::cout << "size of triangle_list = " << triangle_list.size() << std::endl;
    std::cout << "size of triangle_list / 6 = " << triangle_list.size() / 6 << std::endl;
    std::cout << "size of triangle_list_offset = " << triangle_list_offset.size() << std::endl;

    int *triangles        = (int *)malloc(triangle_list.size()*sizeof(int));
    int *triangle_offsets = (int *)malloc(triangle_list_offset.size()*sizeof(int));
    memcpy(triangles, triangle_list.data(), triangle_list.size()*sizeof(int));
    memcpy(triangle_offsets, triangle_list_offset.data(), triangle_list_offset.size()*sizeof(int));

    delete [] edges;
    delete [] neighbors;
    delete [] offsets;

    int num_edges = edge_list.size()/2;

    int progress[6]; 

//    return 0; 

    int result = truss_decomposition(edge_tc, triangles, triangle_offsets, progress, num_edges);
    printf("result = %d\n", result);
    return 0;
}
