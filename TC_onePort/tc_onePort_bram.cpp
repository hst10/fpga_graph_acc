
#include <string.h>
#include <stdio.h>
#define BUF_SIZE 4096
#define MAX_NEIGHBORSIZE    1000000
#define MAX_OFFSETSIZE      100000
#define MAX_EDGESIZE        1000000
int neighbor[MAX_NEIGHBORSIZE];
int offset[MAX_OFFSETSIZE];
int edge[MAX_EDGESIZE];


void triangle_counting_onePort(int* data_in)
{
#pragma HLS INTERFACE m_axi port=data_in

	int addr = 1;
    
	int num_edges = data_in[addr];
	addr++;
    int neighbor_size = data_in[addr];
    addr++;
    int offset_size = data_in[addr];
    addr++;
    int edge_size = data_in[addr];
    addr++;
    //int* neighbor = data_in + addr;
    for(int i =0; i<neighbor_size;i++){
        neighbor[i]=data_in[addr+i];
    }
    addr += neighbor_size;
    //int* offset = data_in + addr;
    for(int i =0; i<offset_size;i++){
        offset[i]=data_in[addr+i];
    }
    addr += offset_size;
    //int* edge = data_in + addr;
    for(int i =0; i<edge_size;i++){
        edge[i]=data_in[addr+i];
    }
    int count = 0;
    int i = 0;
    int node_a, node_b;
    int offset_a, offset_b;
    int len_a, len_b;

    int idx_a = 0;
    int idx_b = 0;

    int prev_node_a = 0;
    int prev_node_b = 0;

    int local_a[BUF_SIZE];
    int local_b[BUF_SIZE];

    for (i = 0; i < num_edges; i+=2)
    {
#pragma HLS pipeline
        node_a = edge[i];
        node_b = edge[i+1];

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
    }
    data_in[0] = count;
}
