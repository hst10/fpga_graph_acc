#include <string.h>
#include <stdio.h>
#define BUF_SIZE 4096
#define MAX_NEIGHBORSIZE    1000000
#define MAX_OFFSETSIZE      100000
#define MAX_EDGESIZE        1000000
#define NUM_PE 8

int neighbor_buf[MAX_NEIGHBORSIZE];
int offset_buf[MAX_OFFSETSIZE];
int edge_buf[NUM_PE][MAX_EDGESIZE];
/*int edge1_buf[MAX_EDGESIZE];
int edge2_buf[MAX_EDGESIZE];
int edge3_buf[MAX_EDGESIZE];
int edge4_buf[MAX_EDGESIZE];
int edge5_buf[MAX_EDGESIZE];
int edge6_buf[MAX_EDGESIZE];
int edge7_buf[MAX_EDGESIZE];*/
int num_edges_buf[NUM_PE];
int triangle_counting(int neighbor[9],
                      int offset[7],
                      int edge[18],
                      int num_edges)
{
// #pragma HLS INTERFACE m_axi port=neighbor depth = 32 offset=slave bundle=NEIGHBOR
//#pragma HLS INTERFACE m_axi port=offset depth = 32 offset=slave bundle=OFFSET
//#pragma HLS INTERFACE m_axi port=edge depth = 32 offset=slave bundle=EDGE

    unsigned int count = 0;
    unsigned int i = 0;
    unsigned int node_a, node_b;
    unsigned int offset_a, offset_b;
    unsigned int len_a, len_b;

    unsigned int idx_a = 0;
    unsigned int idx_b = 0;

    unsigned int prev_node_a = 0;
    unsigned int prev_node_b = 0;

    unsigned int local_a[BUF_SIZE];
    unsigned int local_b[BUF_SIZE];

    for (i = 0; i < num_edges; i+=2)
    {
#pragma HLS pipeline
        node_a = edge[i];
        node_b = edge[i+1];
        
        if (node_a != prev_node_a)
        {
            offset_a = offset[node_a];
            len_a = offset[node_a + 1] - offset_a;
            //memcpy(local_a, (const unsigned int*)&(neighbor[offset_a]), len_a*sizeof(unsigned int));
            for (int index=0; index<len_a; index++){
                local_a[index]=neighbor[offset_a+index];
            }
            prev_node_a = node_a; 
        }

        if (node_b != prev_node_b)
        {
            offset_b = offset[node_b];
            len_b = offset[node_b + 1] - offset_b;
            //memcpy(local_b, (const unsigned int*)&(neighbor[offset_b]), len_b*sizeof(unsigned int));
            for (int index=0; index<len_b; index++){
                local_b[index]=neighbor[offset_b+index];
            }
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
    return count;
}




void triangle_counting_onePort(int* data_in)
{
#pragma HLS INTERFACE m_axi port=data_in

    int addr = 1;
    int out[NUM_PE];
    int s=0;
    memcpy(num_edges_buf, (const unsigned int*)&(data_in[addr]), NUM_PE*sizeof(unsigned int));
    //int num_edges = data_in[addr];
    addr+=NUM_PE;

    int neighbor_size = data_in[addr];
    addr++;
    int offset_size = data_in[addr];
    addr++;
   
    memcpy(neighbor_buf, (const unsigned int*)&(data_in[addr]), neighbor_size*sizeof(unsigned int));
    addr += neighbor_size;

    memcpy(offset_buf, (const unsigned int*)&(data_in[addr]), offset_size*sizeof(unsigned int));
    addr += offset_size;

    for(int i=0; i< NUM_PE; i++){
        memcpy(edge_buf[i], (const unsigned int*)&(data_in[addr]), num_edges_buf[i]*sizeof(unsigned int));
        addr += num_edges_buf[i];
    }
    for(int i =0 ; i < NUM_PE; i++  ){
        out[i]=triangle_counting(neighbor_buf,
                            offset_buf,
                            edge_buf[i],
                            num_edges_buf[i]);
    }
    for(int i =0 ; i < NUM_PE; i++  ){
        s+=out[i];
        printf("%d ",out[i]);
    }

    data_in[0]=s;



}


