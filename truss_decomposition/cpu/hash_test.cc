#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <algorithm>
#include <cassert>

/*#include <string>*/

#include <string.h>
#include <stdio.h>

#define BUF_SIZE 4096


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


/*int truss_decomposition(volatile unsigned int neighbor[9],
                      volatile unsigned int offset[7],
                      volatile unsigned int edge[18],
                      unsigned int num_edges)*/
int truss_decomposition(  int neighbor[9],
                          int offset[7],
                          int edge[18],
                          int num_edges,
                          int num_offsets,
                          int num_neighbors)
{
#pragma HLS INTERFACE m_axi port=neighbor depth = 32 offset=slave bundle=NEIGHBOR
#pragma HLS INTERFACE m_axi port=offset depth = 32 offset=slave bundle=OFFSET
#pragma HLS INTERFACE m_axi port=edge depth = 32 offset=slave bundle=EDGE
#pragma HLS INTERFACE s_axilite register port=num_edges
#pragma HLS INTERFACE s_axilite register port=return

    unsigned int i = 0;
    unsigned int node_a, node_b;
    unsigned int offset_a, offset_b;
    unsigned int len_a, len_b;

    unsigned int idx_a = 0;
    unsigned int idx_b = 0;

    bool a_pos_avail = false;
    bool b_pos_avail = false;
    unsigned int pos_idx_a = 0; // the pos of b in a's neighbor list
    unsigned int pos_idx_b = 0; // the pos of a in b's neighbor list

    unsigned int edge_node_p = 0;
    unsigned int edge_node_q = 0;

    unsigned int prev_node_a = 0;
    unsigned int prev_node_b = 0;

    unsigned int local_a[BUF_SIZE];
    unsigned int local_b[BUF_SIZE];
    unsigned int intersect[BUF_SIZE];
    unsigned int count = 0;

    unsigned int k = 0; 

    bool has_new_deletes = true; 
    int num_new_deletes = 0;

    int num_edges_left = 0;

    while (1)
    {
        num_edges_left = 0;
        if (!has_new_deletes)
        {
            k++;
            std::cout << "k = " << k << std::endl;
        }

        num_edges_left = 0;
        has_new_deletes = false; 
        num_new_deletes = 0;

        for (i = 0; i < num_edges; i+=2)
        {
#pragma HLS pipeline

            node_a = edge[i];
            node_b = edge[i+1];

            if (node_a == 0 || node_b == 0)
                continue;
            else
                num_edges_left++;

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

            a_pos_avail = false;
            b_pos_avail = false;

            count = 0;

            while (idx_a < len_a && idx_b < len_b)
            {
#pragma HLS pipeline

                edge_node_p = local_a[idx_a]; 
                edge_node_q = local_b[idx_b]; 

                if (edge_node_p == node_b)
                {
                    a_pos_avail = true;
                    pos_idx_a = idx_a; 
                }
                if (edge_node_q == node_a)
                {
                    b_pos_avail = true;
                    pos_idx_b = idx_b; 
                }


                if (edge_node_p < edge_node_q)
                    idx_a++;
                else if (edge_node_p > edge_node_q)
                    idx_b++;
                else
                {
                    if ((edge_node_p != 0) && (edge_node_q != 0))
                        intersect[count++] = edge_node_p;
                    idx_a++;
                    idx_b++;
                }
            }

            if (count < k)
            {
                has_new_deletes = true;
                num_new_deletes++;
                // std::cout << "Deleting edge: " << edge[i] << "->" << edge[i+1] << std::endl; 
                edge[i] = 0;
                edge[i+1] = 0;
                num_edges_left--;
                if (!a_pos_avail)
                {
                    while ((idx_a < len_a) && (local_a[idx_a++] != node_b)); 
                    pos_idx_a = idx_a - 1;
                }
                if (!b_pos_avail)
                {
                    while ((idx_b < len_b) && (local_b[idx_b++] != node_a)); 
                    pos_idx_b = idx_b - 1;
                }

                assert(neighbor[offset_a+pos_idx_a] == node_b);
                assert(neighbor[offset_b+pos_idx_b] == node_a);

                neighbor[offset_a+pos_idx_a] = 0;
                neighbor[offset_b+pos_idx_b] = 0;
            }
        }

/*        std::cout << "edges left = " << num_edges_left << std::endl; */
        std::cout << "num_new_deletes = " << num_new_deletes << std::endl; 

        if (num_edges_left == 0)
            break; 
    }
    return k+1;
}

int main()
{
    std::vector<int> edge_list, neighbor_list, offset_list;
    read_graph_td("../graph/soc-Epinions1_adj.tsv", edge_list, neighbor_list, offset_list);
    std::cout << "neighbor_list size= " << neighbor_list.size() << std::endl;
    std::cout << "offset_list size= " << offset_list.size() << std::endl;
    std::cout << "edge_list size= " << edge_list.size() << std::endl;

    const unsigned int bitwidth = 20;
    const unsigned int hash_table_len = 1 << bitwidth;

    std::cout << hash_table_len << std::endl; 

    std::array<unsigned int, hash_table_len> counters;

    for (int i = 0; i < hash_table_len; i++)
        counters[i] = 0;

    for (int i = 0; i < edge_list.size(); i+=2)
    {
        unsigned int u = edge_list[i]; 
        unsigned int v = edge_list[i+1]; 
        unsigned int idx = (unsigned int)(((u & 0x3ff) << 10) + (v & 0x3ff));
/*        std::cout << "u = " << u << "; v = " << v << "; idx = " << idx << std::endl;
        std::cout << "(u & 0xff) << 8 = " << ((u & 0xff) << 8) << "; (v & 0xff) = " << (v & 0xff) << "; idx = " << idx << std::endl;*/
        assert(idx < hash_table_len); 
        counters[idx]++;
    }

    std::sort(counters.begin(), counters.end());

    std::vector<int> histogram; 
    histogram.resize(128); 
    for (auto item : histogram)
        item = 0;

    for (int i = 0; i < hash_table_len; i++)
        histogram[counters[i]]++;

    for (int i = 0; i < 128; i++)
    {
        if (histogram[i] != 0)
            std::cout << "size = " << i << ", num = " << histogram[i] << std::endl;
    }

//        std::cout << counters[i] << " "; 

    std::cout << std::endl; 





/*    int *edges     = (int *)malloc(    edge_list.size()*sizeof(int));
    int *neighbors = (int *)malloc(neighbor_list.size()*sizeof(int));
    int *offsets   = (int *)malloc(  offset_list.size()*sizeof(int));

    memcpy(edges    ,     edge_list.data(),     edge_list.size()*sizeof(int));
    memcpy(neighbors, neighbor_list.data(), neighbor_list.size()*sizeof(int));
    memcpy(offsets  ,   offset_list.data(),   offset_list.size()*sizeof(int));*/

/*    int result = truss_decomposition(neighbors, offsets, edges, edge_list.size(), offset_list.size(), neighbor_list.size());
    printf("result = %d\n", result);
*/
    return 0;
}
