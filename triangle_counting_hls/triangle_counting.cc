#include <string.h>
#include <stdio.h>

#define BUF_SIZE 4096

int cal_intersect(volatile unsigned int neighbor[9],
			  volatile unsigned int offset_a,
			  volatile unsigned int offset_b,
              volatile unsigned int len_a,
              volatile unsigned int len_b)
{
    unsigned int local_a[BUF_SIZE];
    unsigned int local_b[BUF_SIZE];

    memcpy(local_a, (const unsigned int*)&(neighbor[offset_a]), len_a*sizeof(unsigned int));
    memcpy(local_b, (const unsigned int*)&(neighbor[offset_b]), len_b*sizeof(unsigned int));

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


int triangle_counting(volatile unsigned int neighbor[9],
                      volatile unsigned int offset[7],
                      volatile unsigned int edge[18],
					  unsigned int num_edges,
					  unsigned int progress[5])
{
#pragma HLS INTERFACE m_axi port=neighbor depth = 32 offset=slave bundle=NEIGHBOR
#pragma HLS INTERFACE m_axi port=offset depth = 32 offset=slave bundle=OFFSET
#pragma HLS INTERFACE m_axi port=edge depth = 32 offset=slave bundle=EDGE
#pragma HLS INTERFACE m_axi port=progress depth = 32 offset=slave bundle=PROGRESS
#pragma HLS INTERFACE s_axilite register port=num_edges
#pragma HLS INTERFACE s_axilite register port=return

	unsigned int count = 0;
	unsigned int i = 0;
	unsigned int node_a, node_b;
	unsigned int offset_a, offset_b;
	unsigned int len_a, len_b;

	for (i = 0; i < num_edges; i+=2)
	{
#pragma HLS pipeline
		progress[0] = i;
		node_a = edge[i];
		node_b = edge[i+1];
		offset_a = offset[node_a];
		len_a = offset[node_a + 1] - offset_a;

		offset_b = offset[node_b];
		len_b = offset[node_b + 1] - offset_b;
//		count += neighbor[offset_a] + neighbor[offset_b];
//		count += offset_a + offset_b + len_a + len_b;
		printf("calling intersect: offset_a = %d, offset_b = %d, len_a = %d, len_b = %d\n", offset_a, offset_b, len_a, len_b);
		progress[1] = offset_a;
		progress[2] = offset_b;
		progress[3] = len_a;
		progress[4] = len_b;

		count += cal_intersect(neighbor, offset_a, offset_b, len_a, len_b);
	}
	return count;
}
