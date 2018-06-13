#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <cstring>
#include <cstdint>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>  // for high_resolution_clock

extern "C"
{
#include "libxlnk_cma.h"
}

using namespace std;

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

void read_graph(const char *filename, 
                std::vector<int> &edge_list, 
                std::vector<int> &neighbor_list, 
                std::vector<int> &offset_list, 
                int &num_edge)
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
                else
                {
                    neighbor_list.push_back(u);
                    degree_count++;
                }
            }
        }
    }
    ifs.close();
    offset_list.push_back(degree_count);
    num_edge = edge_list.size() / 2;
}

int main( int argc, char** argv )
{

    auto t_start = std::chrono::high_resolution_clock::now();

    int num_edge = 0; 
    std::vector<int> edge_list, neighbor_list, offset_list;
    read_graph("../graph/soc-Epinions1_adj.tsv", edge_list, neighbor_list, offset_list, num_edge);
    std::cout << "neighbor_list size= " << neighbor_list.size() << std::endl;
    std::cout << "offset_list size= " << offset_list.size() << std::endl;
    std::cout << "edge_list size= " << edge_list.size() << std::endl;
    std::cout << "initialized num_edge = " << num_edge << std::endl;

    int *edges     = (int *)cma_alloc(    edge_list.size()*sizeof(int), false);
    int *neighbors = (int *)cma_alloc(neighbor_list.size()*sizeof(int), false);
    int *offsets   = (int *)cma_alloc(  offset_list.size()*sizeof(int), false);
    int *progress  = (int *)cma_alloc(                   5*sizeof(int), false);

    std::memcpy(edges    ,     edge_list.data(),     edge_list.size()*sizeof(int));
    std::memcpy(neighbors, neighbor_list.data(), neighbor_list.size()*sizeof(int));
    std::memcpy(offsets  ,   offset_list.data(),   offset_list.size()*sizeof(int));

    accelerator acc; 
    acc.program("/home/xilinx/code/tc/triangle_counting.bit");

    auto t_program_done = std::chrono::high_resolution_clock::now();

    acc.set(0x18, cma_get_phy_addr(neighbors)); 
    acc.set(0x20, cma_get_phy_addr(offsets)); 
    acc.set(0x28, cma_get_phy_addr(edges)); 
    acc.set(0x30, edge_list.size()); 
    acc.set(0x38, cma_get_phy_addr(progress)); 

    cout << "start execute.." << endl;

    auto t_acc_start = std::chrono::high_resolution_clock::now();

    acc.start(); 

    int tik = 1;
    while(!acc.done())
    {
        tik++;
        if ((tik % 10000) == 0) std::cout << "."; 
        //std::cout << tik << std::endl; 
/*       std::cout << progress[0] << " " << progress[1] << 
       " " << progress[2] << " " << progress[3] << " " << progress[4] << 
       " " << acc.get_return() << std::endl;*/
    }

    auto t_acc_finish = std::chrono::high_resolution_clock::now();

    cout << "\ndone execute.." << endl;

    std::cout << "result = " << acc.get_return() << std::endl;

    std::chrono::duration<double> total_exec_time = t_acc_finish - t_acc_start; 
    std::cout << "Kernel exec time: " << total_exec_time.count() << "s" << std::endl;

    cma_free(edges);
    cma_free(neighbors);
    cma_free(offsets);
    cma_free(progress);

    return 0; 
}
