#define HLS 

#include "truss_decomposition.h"

#ifndef HLS
inline
void triangle_counting(unsigned int *neighbor_list,
                      unsigned int *offset_list,
                      unsigned int *edge_list,
                      unsigned int *edge_tc,
                      int num_edges,
                      unsigned int *local_a,
                      unsigned int *local_b)
#else
inline
void triangle_counting(volatile unsigned int neighbor_list[811480],
                       volatile unsigned int offset_list[75883],
                       volatile unsigned int edge_list[811480],
                       volatile unsigned int edge_tc[405740],
                      int num_edges,
                      unsigned int local_a[BUF_SIZE],
                      unsigned int local_b[BUF_SIZE])
#endif
{
#pragma HLS inline
    unsigned int i;
    unsigned int node_a, node_b;
    unsigned int offset_a, offset_b;
    unsigned int len_a, len_b;

    unsigned int count = 0; 

    unsigned int idx_a = 0;
    unsigned int idx_b = 0;
    unsigned int prev_node_a = 0;
    unsigned int prev_node_b = 0;

    unsigned int edge_node_p = 0;
    unsigned int edge_node_q = 0;

    for (i = 0; i < num_edges; i+=2)
    {
#pragma HLS pipeline

        node_a = edge_list[i];
        node_b = edge_list[i+1];

        if (node_a == 0 || node_b == 0)
            continue;

        if (node_a != prev_node_a)
        {
            offset_a = offset_list[node_a];
            len_a = offset_list[node_a + 1] - offset_a;
            memcpy(local_a, (const unsigned int*)&(neighbor_list[offset_a]), len_a*sizeof(unsigned int));
            prev_node_a = node_a; 
        }

        if (node_b != prev_node_b)
        {
            offset_b = offset_list[node_b];
            len_b = offset_list[node_b + 1] - offset_b;
            memcpy(local_b, (const unsigned int*)&(neighbor_list[offset_b]), len_b*sizeof(unsigned int));
            prev_node_b = node_b;
        }

        idx_a = 0;
        idx_b = 0;

        count = 0;

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
                if ((edge_node_p != 0) && (edge_node_q != 0))
                    count++;
                idx_a++;
                idx_b++;
            }
        }
        edge_tc[i/2] = count;
    }
}

#ifndef HLS
inline
void decrease_tc(int node_a, 
                 int node_b, 
                 unsigned int *edge_list, 
                 unsigned int *edge_tc, 
                 unsigned int *hash_edge2idx, 
                 int num_edges)
#else
inline
void decrease_tc(int node_a, 
                 int node_b, 
                 volatile unsigned int edge_list[811480], 
                 volatile unsigned int edge_tc[405740], 
                 volatile unsigned int hash_edge2idx[hash_table_len], 
                 int num_edges,
                 volatile unsigned int k_progress[6])
#endif
{
#pragma HLS inline
//	std::cout << "TARGET: " << node_a << ", " << node_b << std::endl;
    unsigned int ht_idx = (unsigned int)(((node_a & 0x3ff) << 10) + (node_b & 0x3ff));

    bool edge_not_found = true;
    for (unsigned int entry_idx = 0; entry_idx < max_num_elems; entry_idx++)
    {
        unsigned int ht_value = hash_edge2idx[ht_idx*max_num_elems+entry_idx]; 
        if (ht_value != 0)
        {
            if ((node_a == edge_list[ht_value-1]) && (node_b == edge_list[ht_value])) // edge_list found
            {
                // std::cout << "MATCH: " << edge_list[ht_value-1] << ", " << edge_list[ht_value] << std::endl;
                edge_tc[(ht_value-1)/2]--;
                edge_not_found = false;
                break;
            }
        }
        else
            break;
    }
    if (edge_not_found) // search in the whole edge_list list; not likely to be true
    {
        k_progress[4]++;
        k_progress[5] = 1; 
        // std::cout << "SLOW SEARCH IN DRAM!!!" << std::endl;
        // std::cout << "=========================== SLOW: " << node_a << ", " << node_b << std::endl;
        for (int k = 0; k < num_edges; k+=2) // TODO: use binary search
        {
            if ((node_a == edge_list[k]) && (node_b == edge_list[k+1]))
            {
                edge_tc[k/2]--;
                // std::cout << node_a << "->" << node_b << " FOUND!!!" << std::endl;
                break;
            }
            //else if ((node_a == edge_list[k]))
            	// std::cout << "node_b's: " << edge_list[k+1] << " FOUND!!!" << std::endl;

        }
        // std::cout << node_a << "->" << node_b << " DONE!!!" << std::endl;
        k_progress[5] = 0; 
    }
}

#ifndef HLS
int truss_decomposition(unsigned int *neighbor_list,
                        unsigned int *offset_list,
                        unsigned int *edge_list,
                        unsigned int *edge_tc,
                        unsigned int *hash_edge2idx,
                        unsigned int num_edges)
#else
int truss_decomposition(volatile unsigned int neighbor_list[811480],
                        volatile unsigned int offset_list[75883],
                        volatile unsigned int edge_list[811480],
                        volatile unsigned int edge_tc[405740],
                        volatile unsigned int hash_edge2idx[hash_table_len],
                        unsigned int num_edges,
                        volatile unsigned int k_progress[6])
#endif
{
#ifdef HLS
#pragma HLS INTERFACE m_axi port=neighbor_list depth=811480  offset=slave bundle=NEIGHBOR_LIST
#pragma HLS INTERFACE m_axi port=offset_list   depth=75883   offset=slave bundle=OFFSET_LIST
#pragma HLS INTERFACE m_axi port=edge_list     depth=811480  offset=slave bundle=EDGE_LIST
#pragma HLS INTERFACE m_axi port=edge_tc       depth=405740  offset=slave bundle=EDGE_TC_HASH
#pragma HLS INTERFACE m_axi port=hash_edge2idx depth=8388608 offset=slave bundle=EDGE_TC_HASH
#pragma HLS INTERFACE m_axi port=k_progress    depth=6       offset=slave bundle=EDGE_TC_HASH
#pragma HLS INTERFACE s_axilite register port=num_edges
#pragma HLS INTERFACE s_axilite register port=return
#endif

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

    unsigned int count = 0;

    unsigned int k = 0; 

    bool has_new_deletes = true; 
    int num_new_deletes = 0;

    int num_edges_left = 0;

    unsigned int local_a[BUF_SIZE];
    unsigned int local_b[BUF_SIZE];

    for (int i = 0; i < 6; i++)
    	k_progress[i] = 0;

    triangle_counting(neighbor_list, offset_list, edge_list, edge_tc, num_edges, local_a, local_b); 
    k_progress[1] = 1; 

    while (1)
    {
        num_edges_left = 0;
        if (!has_new_deletes)
        {
            k++;
            k_progress[0] = k;
#ifndef HLS
            std::cout << "k = " << k << std::endl;
#endif
        }

        num_edges_left = 0;
        has_new_deletes = false; 
        num_new_deletes = 0;

        for (i = 0; i < num_edges; i+=2)
        {
            node_a = edge_list[i];
            node_b = edge_list[i+1];
            if ((node_a != 0) && (node_b != 0))
                num_edges_left++;
            else
                continue;

            if (edge_tc[i/2] < k)
            {
                if (node_a != prev_node_a)
                {
                    offset_a = offset_list[node_a];
                    len_a = offset_list[node_a + 1] - offset_a;
                    memcpy(local_a, (const unsigned int*)&(neighbor_list[offset_a]), len_a*sizeof(unsigned int));
                    prev_node_a = node_a; 
                }

                if (node_b != prev_node_b)
                {
                    offset_b = offset_list[node_b];
                    len_b = offset_list[node_b + 1] - offset_b;
                    memcpy(local_b, (const unsigned int*)&(neighbor_list[offset_b]), len_b*sizeof(unsigned int));
                    prev_node_b = node_b;
                }

                idx_a = 0;
                idx_b = 0;

                a_pos_avail = false;
                b_pos_avail = false;

                count = 0;

                // find affected edges and decrease tc by 1
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
                        {
                            int target_node_a, target_node_b; 
                            if (node_a > edge_node_p)
                            {
                                target_node_a = node_a;
                                target_node_b = edge_node_p;
                            }
                            else
                            {
                                target_node_a = edge_node_p;
                                target_node_b = node_a;
                            }
                            decrease_tc(target_node_a, target_node_b, edge_list, edge_tc, hash_edge2idx, num_edges, k_progress); 
                            if (node_b > edge_node_p)
                            {
                                target_node_a = node_b;
                                target_node_b = edge_node_p;
                            }
                            else
                            {
                                target_node_a = edge_node_p;
                                target_node_b = node_b;
                            }
                            decrease_tc(target_node_a, target_node_b, edge_list, edge_tc, hash_edge2idx, num_edges, k_progress); 
                        }
                        idx_a++;
                        idx_b++;
                    }
                }

                has_new_deletes = true;
                num_new_deletes++;
                // std::cout << "Deleting edge_list: " << edge_list[i] << "->" << edge_list[i+1] << std::endl; 
                edge_list[i] = 0;
                edge_list[i+1] = 0;
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

                assert(neighbor_list[offset_a+pos_idx_a] == node_b);
                assert(neighbor_list[offset_b+pos_idx_b] == node_a);

                neighbor_list[offset_a+pos_idx_a] = 0;
                neighbor_list[offset_b+pos_idx_b] = 0;

                // make sure local copy is coherent
                local_a[pos_idx_a] = 0;
                local_b[pos_idx_b] = 0;
            }
        }

        k_progress[2] = num_new_deletes;
        k_progress[3] = num_edges_left;

        // std::cout << "num_new_deletes = " << num_new_deletes << std::endl; 

        if (num_edges_left == 0)
            break; 
    }
    return k+1;
}
