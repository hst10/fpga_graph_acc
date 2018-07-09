#include "truss_decomposition.h"

int truss_decomposition(unsigned int neighbor_list[811480],
                        unsigned int offset_list[75883],
                        unsigned int edge_list[811480],
                        unsigned int edge_tc[405740],
                        unsigned int hash_edge2idx[hash_table_len],
                        unsigned int num_edges);

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

int main()
{
    std::vector<int> edge_list, neighbor_list, offset_list;
    // read_graph_td("../graph/soc-Epinions1_adj.tsv", edge_list, neighbor_list, offset_list);
    read_graph_td("/home/shuang91/vivado_projects/graph_challenge/graph/soc-Slashdot0811_adj.tsv", edge_list, neighbor_list, offset_list);
    std::cout << "neighbor_list size= " << neighbor_list.size() << std::endl;
    std::cout << "offset_list size= " << offset_list.size() << std::endl;
    std::cout << "edge_list size= " << edge_list.size() << std::endl;

    unsigned int *edges     = (unsigned int *)malloc(    edge_list.size()*sizeof(int));
    unsigned int *neighbors = (unsigned int *)malloc(neighbor_list.size()*sizeof(int));
    unsigned int *offsets   = (unsigned int *)malloc(  offset_list.size()*sizeof(int));
    unsigned int *edge_tc   = (unsigned int *)malloc(  edge_list.size()/2*sizeof(int));

    memcpy(edges    ,     edge_list.data(),     edge_list.size()*sizeof(int));
    memcpy(neighbors, neighbor_list.data(), neighbor_list.size()*sizeof(int));
    memcpy(offsets  ,   offset_list.data(),   offset_list.size()*sizeof(int));

    long long int count_check = 0;
    for (int i = 0; i < edge_list.size()/2; i++)
    {
//        std::cout << edge_tc[i] << " ";
        count_check += edge_tc[i];
    }

    std::cout << "num of triangles = " << count_check/3 << std::endl;

    std::cout << "hash_table_len = " << hash_table_len << std::endl;

    unsigned int *hash_edge2idx = (unsigned int *)malloc(max_num_elems*hash_table_len*sizeof(int));
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
    for (int i = 0; i < histogram.size(); i++)
        histogram[i] = 0;

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

    
    int result = truss_decomposition(neighbors, offsets, edges, edge_tc, hash_edge2idx, edge_list.size());
    printf("result = %d\n", result);
    return 0;
}
