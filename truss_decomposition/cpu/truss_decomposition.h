#ifndef _TRUSS_DECOMPOSITION_H_
#define _TRUSS_DECOMPOSITION_H_

/*#define HLS*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cassert>
/*#include <string>*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const unsigned int BUF_SIZE = 4096;
const unsigned int hash_key_bitwidth = 20;
const unsigned int hash_table_len = 1 << hash_key_bitwidth;
const unsigned int max_num_elems = 8;


void read_graph_td(const char *filename, 
                std::vector<int> &edge_list, 
                std::vector<int> &neighbor_list, 
                std::vector<int> &offset_list); 

#ifndef HLS
inline
int triangle_counting(unsigned int neighbor_list[1024],
                      unsigned int offset_list[1024],
                      unsigned int edge_list[1024],
                      unsigned int edge_tc[1024],
                      int num_edges,
                      unsigned int *local_a,
                      unsigned int *local_b);
#else
inline
int triangle_counting(volatile unsigned int neighbor_list[1024],
                      volatile unsigned int offset_list[1024],
                      volatile unsigned int edge_list[1024],
                      volatile unsigned int edge_tc[1024],
                      int num_edges,
                      volatile unsigned int *local_a,
                      volatile unsigned int *local_b);
#endif

#ifndef HLS
inline
void decrease_tc(int node_a, 
                 int node_b, 
                 unsigned int *edge_list, 
                 unsigned int *edge_tc, 
                 unsigned int *hash_edge2idx, 
                 int num_edges);
#else
inline
void decrease_tc(int node_a, 
                 int node_b, 
                 volatile unsigned int *edge_list, 
                 volatile unsigned int *edge_tc, 
                 volatile unsigned int *hash_edge2idx, 
                 int num_edges);
#endif

#ifndef HLS
int truss_decomposition(unsigned int *neighbor_list,
                        unsigned int *offset_list,
                        unsigned int *edge_list,
                        unsigned int *edge_tc,
                        unsigned int *hash_edge2idx,
                        unsigned int num_edges,
                        unsigned int num_offsets,
                        unsigned int num_neighbors);
#else
int truss_decomposition(volatile unsigned int *neighbor_list,
                        volatile unsigned int *offset_list,
                        volatile unsigned int *edge_list,
                        volatile unsigned int *edge_tc,
                        volatile unsigned int *hash_edge2idx,
                        unsigned int num_edges,
                        unsigned int num_offsets,
                        unsigned int num_neighbors);
#endif

#endif /* _TRUSS_DECOMPOSITION_H_ */