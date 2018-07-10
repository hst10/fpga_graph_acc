
#ifndef HLS
#include "truss_decomposition.h"
#endif 

#define LOCAL_BUF_SIZE 2048

#ifndef HLS
int truss_decomposition(int *edge_tc, 
                        int *triangles, 
                        int *triangle_offsets, 
                        int num_edges)
#else
int truss_decomposition(volatile int *edge_tc, 
                        volatile int *triangles, 
                        volatile int *triangle_offsets, 
                        int num_edges)
#endif
{
#ifdef HLS
#pragma HLS INTERFACE m_axi port=edge_tc depth=32 offset=slave bundle=EDGE_TC
#pragma HLS INTERFACE m_axi port=triangles depth=32 offset=slave bundle=TRIANGLES
#pragma HLS INTERFACE m_axi port=triangle_offsets depth=32 offset=slave bundle=TRI_OFFSETS
#pragma HLS INTERFACE s_axilite register port=num_edges
#pragma HLS INTERFACE s_axilite register port=return
#endif
    int num_edges_left = 0;
    bool has_new_deletes = true; 
    int num_new_deletes = 0;

    int local_buf_1[LOCAL_BUF_SIZE];
    int local_buf_2[LOCAL_BUF_SIZE];
    int local_buf_3[LOCAL_BUF_SIZE];

    int prev_buf_1 = 0;
    int prev_buf_2 = 0;
    int prev_buf_3 = 0;

    unsigned int k = 0; 
    while (1)
    {
        if (!has_new_deletes)
        {
            k++;
            std::cout << "k = " << k << std::endl;
        }

        num_edges_left = 0;
        has_new_deletes = false; 
        num_new_deletes = 0;

        for (int i = 1; i <= num_edges; i++) // edge idx starts from 1
        {
            int num_triangles = edge_tc[i]; 
            if ((num_triangles < k) && (num_triangles != 0))
            {
                edge_tc[i] = 0; 
                has_new_deletes = true; 
                num_new_deletes++;
                int triangle_list_start = triangle_offsets[i-1];
                int triangle_list_len   = triangle_offsets[i] - triangle_list_start;

                if (i != prev_buf_1)
                {
                    memcpy(local_buf_1, (const int*)&(triangles[triangle_list_start]), triangle_list_len*sizeof(int));
                    prev_buf_1 = i;
                }

                for (int triangle_idx = 0; triangle_idx < triangle_list_len; triangle_idx+=2)
                {
                    int affected_edge_a = local_buf_1[triangle_idx];
                    int affected_edge_b = local_buf_1[triangle_idx+1];
                    if (affected_edge_a != 0) 
                    {
                        int affected_edge_a_start = triangle_offsets[affected_edge_a-1];
                        int affected_edge_b_start = triangle_offsets[affected_edge_b-1];
                        int affected_edge_a_len = triangle_offsets[affected_edge_a] - affected_edge_a_start;
                        int affected_edge_b_len = triangle_offsets[affected_edge_b] - affected_edge_b_start;

                        if (affected_edge_a != prev_buf_2)
                        {
                            memcpy(local_buf_2, (const int*)&(triangles[affected_edge_a_start]), affected_edge_a_len*sizeof(int));
                            prev_buf_2 = affected_edge_a;
                        }
                        if (affected_edge_b != prev_buf_3)
                        {
                            memcpy(local_buf_3, (const int*)&(triangles[affected_edge_b_start]), affected_edge_b_len*sizeof(int));
                            prev_buf_3 = affected_edge_b;
                        }

                        for (int affected_idx = 0; affected_idx < affected_edge_a_len; affected_idx+=2)
                        {
                            int tmp_candidate_a = local_buf_2[affected_idx];
                            int tmp_candidate_b = local_buf_2[affected_idx+1];
                            if ((tmp_candidate_a == i) || (tmp_candidate_b == i))
                            {
                                triangles[affected_edge_a_start+affected_idx] = 0; 
                                triangles[affected_edge_a_start+affected_idx+1] = 0;
                                edge_tc[affected_edge_a]--;
                                break; 
                            }
                        }

                        for (int affected_idx = 0; affected_idx < affected_edge_b_len; affected_idx+=2)
                        {
                            int tmp_candidate_a = local_buf_3[affected_idx];
                            int tmp_candidate_b = local_buf_3[affected_idx+1];
                            if ((tmp_candidate_a == i) || (tmp_candidate_b == i))
                            {
                                triangles[affected_edge_b_start+affected_idx] = 0; 
                                triangles[affected_edge_b_start+affected_idx+1] = 0;
                                edge_tc[affected_edge_b]--;
                                break; 
                            }
                        }
                    }
                }
            }
            else
            {
                if (num_triangles != 0) 
                    num_edges_left++;
            }
        }

        if (num_edges_left == 0)
            break; 
    }

    return k+1;
}
