#include <string.h>
#include <stdio.h>

inline
int cal_intersect(volatile unsigned int neighbor[524288],
              volatile unsigned int offset_a,
              volatile unsigned int offset_b,
              volatile unsigned int len_a,
              volatile unsigned int len_b)
{
#pragma HLS INLINE
	volatile unsigned int *local_a = &(neighbor[offset_a]);
	volatile unsigned int *local_b = &(neighbor[offset_b]);

/*    memcpy(local_a, (const unsigned int*)&(neighbor[offset_a]), len_a*sizeof(unsigned int));
    memcpy(local_b, (const unsigned int*)&(neighbor[offset_b]), len_b*sizeof(unsigned int));*/

    unsigned int count = 0;
    unsigned int idx_a = 0;
    unsigned int idx_b = 0;

    while (idx_a < len_a && idx_b < len_b)
    {
#pragma HLS pipeline
        if (local_a[idx_a] < local_b[idx_b])
            idx_a++;
        else if (local_a[idx_a] > local_b[idx_b])
            idx_b++;
        else
        {
            count++;
            idx_a++;
            idx_b++;
        }
    }
    return count;
}

int triangle_counting(volatile const unsigned int neighbor_list[524288],
                      volatile const unsigned int offset_list[131072],
                      volatile const unsigned int edge_list[1048576],
                      unsigned int num_edges)
{
#pragma HLS INTERFACE ap_fifo port=neighbor_list depth=524288 offset=slave bundle=NEIGHBOR
#pragma HLS INTERFACE ap_fifo port=offset_list depth=131072 offset=slave bundle=OFFSET
#pragma HLS INTERFACE ap_fifo port=edge_list depth=1048576 offset=slave bundle=EDGE_LIST
/*
#pragma HLS INTERFACE m_axi port=neighbor_list depth=524288 offset=slave bundle=NEIGHBOR
#pragma HLS INTERFACE m_axi port=offset_list depth=131072 offset=slave bundle=OFFSET
#pragma HLS INTERFACE m_axi port=edge_list depth=1048576 offset=slave bundle=EDGE_LIST
*/
#pragma HLS INTERFACE ap_none register port=num_edges
#pragma HLS INTERFACE ap_ctrl_hs register port=return

	unsigned int neighbor[524288];
	unsigned int offset[131072];
//	unsigned int edge[1048576];
#pragma HLS ARRAY_PARTITION variable=neighbor dim=1 block factor=32
#pragma HLS ARRAY_PARTITION variable=offset   dim=1 block factor=32

    unsigned int count = 0;
    unsigned int i = 0;
    unsigned int node_a, node_b;
    unsigned int offset_a, offset_b;
    unsigned int len_a, len_b;

/*    memcpy(neighbor, (const unsigned int*)&(neighbor_list[0]), 524288*sizeof(unsigned int));
    memcpy(offset, (const unsigned int*)&(offset_list[0]), 131072*sizeof(unsigned int));*/
//    memcpy(edge, (const unsigned int*)&(edge_list[0]), 1048576*sizeof(unsigned int));

    for (i = 0; i < 524288; i++)
#pragma HLS pipeline
    	neighbor[i] = neighbor_list[i];

    for (i = 0; i < 131072; i++)
#pragma HLS pipeline
    	offset[i] = offset_list[i];


    for (i = 0; i < num_edges; i+=2)
    {
#pragma HLS unroll factor=32
        node_a = edge_list[i];
        node_b = edge_list[i+1];
        offset_a = offset[node_a];
        len_a = offset[node_a + 1] - offset_a;

        offset_b = offset[node_b];
        len_b = offset[node_b + 1] - offset_b;

        count += cal_intersect(neighbor, offset_a, offset_b, len_a, len_b);
    }
    return count;
}
