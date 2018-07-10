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
int truss_decomposition(int *edge_tc, 
                        int *triangles, 
                        int *triangle_offsets, 
                        int num_edges);
#else
int truss_decomposition(volatile int *edge_tc, 
                        volatile int *triangles, 
                        volatile int *triangle_offsets, 
                        int num_edges)
#endif

#endif /* _TRUSS_DECOMPOSITION_H_ */