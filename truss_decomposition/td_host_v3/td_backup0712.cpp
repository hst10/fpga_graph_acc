#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>  // for high_resolution_clock

// #ifdef USE_CPU
// #include "triangle_counting.h"

extern "C"
{
#include "libxlnk_cma.h"
}

using namespace std;

const unsigned int BUF_SIZE = 4096;
const unsigned int hash_key_bitwidth = 20;
const unsigned int hash_table_len = 1 << hash_key_bitwidth;
const unsigned int max_num_elems = 8;

class accelerator
{
public:
    accelerator(int base_addr=0x43C00000, int range=0x00010000) : base_addr(base_addr), range(range)
    {
        // virt_base = base_addr & ~(getpagesize() - 1);
        virt_base = base_addr & ~(sysconf(_SC_PAGE_SIZE) - 1);
        virt_offset = base_addr - virt_base;
        mmap_file = open("/dev/mem", O_RDWR | O_SYNC);
        if (mmap_file == -1) 
            cout << "Unable to open /dev/mem" << endl;
        mmap_addr = (int*)mmap(NULL, range + virt_offset, PROT_READ | PROT_WRITE,
                        MAP_SHARED, mmap_file, virt_base);
        if (mmap_addr == MAP_FAILED)
            cout << "mmap fails. " << endl;

        mmap_space = mmap_addr + virt_offset; 
    }
    ~accelerator() { close(mmap_file); }

    int get(int offset) { return mmap_space[offset >> 2]; }

    void set(int offset, int value) { mmap_space[offset >> 2] = value; }

    void start() { mmap_space[0x00] |= 1; }

    bool done()  { return (mmap_space[0x00] & (1 << 1)); }

    bool idle() {  return (mmap_space[0x00] & (1 << 2)); }

    bool ready() { return (mmap_space[0x00] & (1 << 3)); }

    int get_return() {  return mmap_space[0x10 >> 2]; }

    int program(string bitfile_name)
    {
        char buf[4194304];
        const string BS_XDEVCFG = "/dev/xdevcfg";
        const string BS_IS_PARTIAL = "/sys/devices/soc0/amba/f8007000.devcfg/is_partial_bitstream";

        int partial_bs_dev = open(BS_IS_PARTIAL.c_str(), O_WRONLY | O_NONBLOCK);
        if (partial_bs_dev < 0)
        {
            printf("ERROR opening %s\n", BS_IS_PARTIAL.c_str());
            return -1;
        }
        int write_size = write(partial_bs_dev, "0", 1); 

        int fpga_dev = open(BS_XDEVCFG.c_str(), O_WRONLY | O_NONBLOCK);
        // int fpga_dev = open(BS_XDEVCFG.c_str(), O_WRONLY);
        if (fpga_dev < 0)
        {
            printf("ERROR opening %s\n", BS_XDEVCFG.c_str());
            return -1;
        }

        int bit_file = open(bitfile_name.c_str(), O_RDONLY);
        if (bit_file < 0)
        {
            printf("ERROR opening %s\n", bitfile_name.c_str());
            return -1;
        }

        int bit_file_size = read(bit_file, buf, 4194304); 
        write_size = write(fpga_dev, buf, bit_file_size); 

        close(partial_bs_dev);
        close(fpga_dev);
        close(bit_file);
        return 0;
    }

private:
    int base_addr; 
    int range; 
    int virt_base; 
    int virt_offset;
    int mmap_file;
    int *mmap_addr;
    int *mmap_space;
};

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


inline
int get_edge_idx(unsigned int node_a, unsigned int node_b, unsigned int* edge, unsigned int num_edges, unsigned int *hash_edge2idx)
{
//  std::cout << "TARGET: " << node_a << ", " << node_b << std::endl;
    unsigned int ht_idx = (unsigned int)(((node_a & 0x3ff) << 10) + (node_b & 0x3ff));

    bool edge_not_found = true;
    for (unsigned int entry_idx = 0; entry_idx < max_num_elems; entry_idx++)
    {
        unsigned int ht_value = hash_edge2idx[ht_idx*max_num_elems+entry_idx]; 
        if (ht_value != 0)
        {
            if ((node_a == edge[ht_value-1]) && (node_b == edge[ht_value])) // edge found
                return (ht_value+1)/2;
        }
        else
            break;
    }
    if (edge_not_found) // search in the whole edge list; not likely to be true
    {
        // std::cout << "SLOW SEARCH IN DRAM!!!" << std::endl;
        // std::cout << "=========================== SLOW: " << node_a << ", " << node_b << std::endl;
        for (int k = 0; k < num_edges; k+=2) // TODO: use binary search
        {
            if ((node_a == edge[k]) && (node_b == edge[k+1]))
                return k;
        }
        // std::cout << node_a << "->" << node_b << " DONE!!!" << std::endl;
    }
}

int init_triangle_counting(unsigned int neighbor[9],
                          unsigned int offset[7],
                          unsigned int edge[18],
                          unsigned int num_edges,
                          unsigned int edge_tc[9],
                          unsigned int *hash_edge2idx, 
                          unsigned int *local_a,
                          unsigned int *local_b,
                          std::vector<int>& triangle_list,
                          std::vector<int>& triangle_list_offset
                          )
{
    unsigned int i;
    unsigned int node_a, node_b;
    unsigned int offset_a, offset_b;
    unsigned int len_a, len_b;

    unsigned int local_count = 0; 
    unsigned int global_count = 0; 

    unsigned int idx_a = 0;
    unsigned int idx_b = 0;
    unsigned int prev_node_a = 0;
    unsigned int prev_node_b = 0;

    unsigned int edge_node_p = 0;
    unsigned int edge_node_q = 0;

    for (i = 0; i < num_edges; i+=2)
    {
#pragma HLS pipeline

        triangle_list_offset.push_back(global_count*2);

        node_a = edge[i];
        node_b = edge[i+1];

/*        if (node_a == 0 || node_b == 0)
            continue;*/

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

        local_count = 0;

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
                int start_node, end_node; 
                start_node = (edge_node_p > node_a) ? edge_node_p : node_a;
                end_node   = (edge_node_p < node_a) ? edge_node_p : node_a;
                triangle_list.push_back(get_edge_idx(start_node, end_node, edge, num_edges, hash_edge2idx)); 
                start_node = (edge_node_p > node_b) ? edge_node_p : node_b;
                end_node   = (edge_node_p < node_b) ? edge_node_p : node_b;
                triangle_list.push_back(get_edge_idx(start_node, end_node, edge, num_edges, hash_edge2idx)); 
//                if ((edge_node_p != 0) && (edge_node_q != 0))
                    local_count++;
                idx_a++;
                idx_b++;
            }
        }
        global_count += local_count;
        edge_tc[i/2+1] = local_count;
    }
    triangle_list_offset.push_back(global_count*2);
}


int main( int argc, char** argv )
{
    if (argc < 2)
    {
        std::cout << "usage: td <graph_adj>.tsv" << std::endl;
        exit(1); 
    }

    std::cout << "cma_pages_available = " << cma_pages_available() << std::endl;

	auto t_start = std::chrono::high_resolution_clock::now();

    std::vector<int> edge_list, neighbor_list, offset_list, triangle_list, triangle_list_offset;
    // read_graph_td("../graph/soc-Epinions1_adj.tsv", edge_list, neighbor_list, offset_list);
    // read_graph_td("../graph/soc-Slashdot0811_adj.tsv", edge_list, neighbor_list, offset_list);
    read_graph_td(argv[1], edge_list, neighbor_list, offset_list);
    std::cout << "neighbor_list size= " << neighbor_list.size() << std::endl;
    std::cout << "offset_list size= " << offset_list.size() << std::endl;
    std::cout << "edge_list size= " << edge_list.size() << std::endl;

    unsigned int local_a[BUF_SIZE];
    unsigned int local_b[BUF_SIZE];

    /* Construct hash table */
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
    for (auto item : histogram)
        item = 0;

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

    unsigned int *edges     = (unsigned int *)malloc(    edge_list.size()*sizeof(int));
    unsigned int *neighbors = (unsigned int *)malloc(neighbor_list.size()*sizeof(int));
    unsigned int *offsets   = (unsigned int *)malloc(  offset_list.size()*sizeof(int));
    unsigned int *edge_tc   = (unsigned int *)cma_alloc((edge_list.size()/2+1)*sizeof(int), false);
    unsigned int *k_progress= (unsigned int *)cma_alloc(                   6*sizeof(int), false);

    if (edge_tc == nullptr)
    {
        std::cout << "Fail to allocate edge_tc! " << std::endl;
        exit(1);
    }
    if (k_progress == nullptr)
    {
        std::cout << "Fail to allocate k_progress! " << std::endl;
        exit(1);
    }

    edge_tc[0] = 0;

    for (int i = 0; i < 6; i++)
        k_progress[i] = 0;

    std::memcpy(edges    ,     edge_list.data(),     edge_list.size()*sizeof(int));
    std::memcpy(neighbors, neighbor_list.data(), neighbor_list.size()*sizeof(int));
    std::memcpy(offsets  ,   offset_list.data(),   offset_list.size()*sizeof(int));

    init_triangle_counting(neighbors, offsets, edges, edge_list.size(), edge_tc, hash_edge2idx, local_a, local_b, triangle_list, triangle_list_offset); 

    long long int count_check = 0;
    int max_num_triangles = 0;
    for (int i = 0; i <= edge_list.size()/2; i++)
    {
//        std::cout << edge_tc[i] << " ";
        count_check += edge_tc[i];
        if (edge_tc[i] > max_num_triangles)
            max_num_triangles = edge_tc[i];
    }

    std::cout << "num of triangles = " << count_check/3 << std::endl;
    std::cout << "max num of triangles of an edge  = " << max_num_triangles << std::endl;
    std::cout << "size of triangle_list = " << triangle_list.size() << std::endl;
    std::cout << "size of triangle_list / 6 = " << triangle_list.size() / 6 << std::endl;
    std::cout << "size of triangle_list_offset = " << triangle_list_offset.size() << std::endl;

    int *triangles        = (int *)cma_alloc(triangle_list.size()*sizeof(int), false);
    int *triangle_offsets = (int *)cma_alloc(triangle_list_offset.size()*sizeof(int), false);
    if (triangles == nullptr)
    {
        std::cout << "Fail to allocate triangles! " << std::endl;
        exit(1);
    }
    if (triangle_offsets == nullptr)
    {
        std::cout << "Fail to allocate triangle_offsets! " << std::endl;
        exit(1);
    }

    memcpy(triangles, triangle_list.data(), triangle_list.size()*sizeof(int));
    memcpy(triangle_offsets, triangle_list_offset.data(), triangle_list_offset.size()*sizeof(int));

    delete [] edges;
    delete [] neighbors;
    delete [] offsets;

    int num_edges = edge_list.size()/2;

/*    int result = truss_decomposition(edge_tc, triangles, triangle_offsets, num_edges);
    printf("result = %d\n", result);*/

    cout << "start programming FPGA .." << endl;
    accelerator acc; 
    acc.program("/home/xilinx/code/graph_challenge/td_v2/td_v2.3.bit");
    cout << "FPGA programming done .." << endl;

    acc.set(0x18, cma_get_phy_addr(edge_tc)); 
    acc.set(0x20, cma_get_phy_addr(triangles)); 
    acc.set(0x28, cma_get_phy_addr(triangle_offsets)); 
    acc.set(0x30, cma_get_phy_addr(k_progress)); 
    acc.set(0x38, num_edges); 

    cout << "start execute.." << endl;

    auto t_acc_start = std::chrono::high_resolution_clock::now();

    acc.start(); 

    int tik = 1;
    while(!acc.done())
    {
        for (int i = 0; i < 6; i++)
            std::cout << k_progress[i] << " "; 
        std::cout << std::endl; 
/*        tik++;
        if ((tik % 10000) == 0) std::cout << "."; */
        //std::cout << tik << std::endl; 
/*       std::cout << progress[0] << " " << progress[1] << 
       " " << progress[2] << " " << progress[3] << " " << progress[4] << 
       " " << acc.get_return() << std::endl;*/
    }
//    int result = truss_decomposition(neighbors, offsets, edges, edge_tc, hash_edge2idx, edge_list.size(), offset_list.size(), neighbor_list.size());

    auto t_acc_finish = std::chrono::high_resolution_clock::now();

    cout << "\ndone execute.." << endl;

    std::cout << "result = " << acc.get_return() << std::endl;

    std::chrono::duration<double> total_exec_time = t_acc_finish - t_acc_start; 
    std::cout << "Kernel exec time: " << total_exec_time.count() << "s" << std::endl;

    cma_free(edge_tc);
    cma_free(k_progress);
    cma_free(triangles);
    cma_free(triangle_offsets);

    return 0;
}
