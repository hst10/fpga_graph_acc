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
                std::vector<int> *edge_list, 
                unsigned int num_pe, 
                std::vector<int> &neighbor_list, 
                std::vector<int> &offset_list)
{
    std::ifstream ifs(filename);

    int degree_count = 0;
    int prev_node = 0;
    int pe_idx = 0;
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
                    edge_list[pe_idx % num_pe].push_back(v);
                    edge_list[pe_idx % num_pe].push_back(u);
                    pe_idx++;
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
//    num_edge = edge_list.size() / 2;
}

int main( int argc, char** argv )
{

    auto t_start = std::chrono::high_resolution_clock::now();

//    int num_edge = 0; 
    std::vector<int> edge_list[7], neighbor_list, offset_list;
    read_graph("../graph/soc-Epinions1_adj.tsv", edge_list, 7, neighbor_list, offset_list);
    std::cout << "neighbor_list size= " << neighbor_list.size() << std::endl;
    std::cout << "offset_list size= " << offset_list.size() << std::endl;

    auto t_file_done = std::chrono::high_resolution_clock::now();

    int *edges0    = (int *)cma_alloc( edge_list[0].size()*sizeof(int), false);
    int *edges1    = (int *)cma_alloc( edge_list[1].size()*sizeof(int), false);
    int *edges2    = (int *)cma_alloc( edge_list[2].size()*sizeof(int), false);
    int *edges3    = (int *)cma_alloc( edge_list[3].size()*sizeof(int), false);
    int *edges4    = (int *)cma_alloc( edge_list[4].size()*sizeof(int), false);
    int *edges5    = (int *)cma_alloc( edge_list[5].size()*sizeof(int), false);
    int *edges6    = (int *)cma_alloc( edge_list[6].size()*sizeof(int), false);
    int *neighbors = (int *)cma_alloc(neighbor_list.size()*sizeof(int), false);
    int *offsets   = (int *)cma_alloc(  offset_list.size()*sizeof(int), false);
    int *progress  = (int *)cma_alloc(                   5*sizeof(int), false);

    auto t_malloc_done = std::chrono::high_resolution_clock::now();

    std::memcpy(edges0   ,  edge_list[0].data(),  edge_list[0].size()*sizeof(int));
    std::memcpy(edges1   ,  edge_list[1].data(),  edge_list[1].size()*sizeof(int));
    std::memcpy(edges2   ,  edge_list[2].data(),  edge_list[2].size()*sizeof(int));
    std::memcpy(edges3   ,  edge_list[3].data(),  edge_list[3].size()*sizeof(int));
    std::memcpy(edges4   ,  edge_list[4].data(),  edge_list[4].size()*sizeof(int));
    std::memcpy(edges5   ,  edge_list[5].data(),  edge_list[5].size()*sizeof(int));
    std::memcpy(edges6   ,  edge_list[6].data(),  edge_list[6].size()*sizeof(int));
    std::memcpy(neighbors, neighbor_list.data(), neighbor_list.size()*sizeof(int));
    std::memcpy(offsets  ,   offset_list.data(),   offset_list.size()*sizeof(int));

    auto t_memcpy_done = std::chrono::high_resolution_clock::now();

    accelerator acc0(0x43C00000, 0x00010000); 
    accelerator acc1(0x43C10000, 0x00010000); 
    accelerator acc2(0x43C20000, 0x00010000); 
    accelerator acc3(0x43C30000, 0x00010000); 
    accelerator acc4(0x43C40000, 0x00010000); 
    accelerator acc5(0x43C50000, 0x00010000); 
    accelerator acc6(0x43C60000, 0x00010000); 
    acc0.program("/home/xilinx/code/tc/tc_opt.bit");

    auto t_program_done = std::chrono::high_resolution_clock::now();

    acc0.set(0x18, cma_get_phy_addr(neighbors)); 
    acc0.set(0x20, cma_get_phy_addr(offsets)); 
    acc0.set(0x28, cma_get_phy_addr(edges0)); 
    acc0.set(0x30, edge_list[0].size()); 
    acc0.set(0x38, cma_get_phy_addr(progress)); 

    acc1.set(0x18, cma_get_phy_addr(neighbors)); 
    acc1.set(0x20, cma_get_phy_addr(offsets)); 
    acc1.set(0x28, cma_get_phy_addr(edges1)); 
    acc1.set(0x30, edge_list[1].size()); 
    acc1.set(0x38, cma_get_phy_addr(progress)); 

    acc2.set(0x18, cma_get_phy_addr(neighbors)); 
    acc2.set(0x20, cma_get_phy_addr(offsets)); 
    acc2.set(0x28, cma_get_phy_addr(edges2)); 
    acc2.set(0x30, edge_list[2].size()); 
    acc2.set(0x38, cma_get_phy_addr(progress)); 
    
    acc3.set(0x18, cma_get_phy_addr(neighbors)); 
    acc3.set(0x20, cma_get_phy_addr(offsets)); 
    acc3.set(0x28, cma_get_phy_addr(edges3)); 
    acc3.set(0x30, edge_list[3].size()); 
    acc3.set(0x38, cma_get_phy_addr(progress)); 
    
    acc4.set(0x18, cma_get_phy_addr(neighbors)); 
    acc4.set(0x20, cma_get_phy_addr(offsets)); 
    acc4.set(0x28, cma_get_phy_addr(edges4)); 
    acc4.set(0x30, edge_list[4].size()); 
    acc4.set(0x38, cma_get_phy_addr(progress)); 
    
    acc5.set(0x18, cma_get_phy_addr(neighbors)); 
    acc5.set(0x20, cma_get_phy_addr(offsets)); 
    acc5.set(0x28, cma_get_phy_addr(edges5)); 
    acc5.set(0x30, edge_list[5].size()); 
    acc5.set(0x38, cma_get_phy_addr(progress)); 
    
    acc6.set(0x18, cma_get_phy_addr(neighbors)); 
    acc6.set(0x20, cma_get_phy_addr(offsets)); 
    acc6.set(0x28, cma_get_phy_addr(edges6)); 
    acc6.set(0x30, edge_list[6].size()); 
    acc6.set(0x38, cma_get_phy_addr(progress)); 
    
    cout << "start execute.." << endl;

    auto t_acc_start = std::chrono::high_resolution_clock::now();
    acc0.start(); 
    acc1.start(); 
    acc2.start(); 
    acc3.start(); 
    acc4.start(); 
    acc5.start(); 
    acc6.start(); 

    int tik = 1;
    while(!acc0.done())
    {
        tik++;
        if ((tik % 10000) == 0) std::cout << "."; 
    }

    std::cout << "acc0 done! " << std::endl;

    while(!acc1.done())
    {
        tik++;
        if ((tik % 10000) == 0) std::cout << "."; 
    }
    std::cout << "acc1 done! " << std::endl;

    while(!acc2.done())
    {
        tik++;
        if ((tik % 10000) == 0) std::cout << "."; 
    }
    std::cout << "acc2 done! " << std::endl;
    
    while(!acc3.done())
    {
        tik++;
        if ((tik % 10000) == 0) std::cout << "."; 
    }
    std::cout << "acc3 done! " << std::endl;

    while(!acc4.done())
    {
        tik++;
        if ((tik % 10000) == 0) std::cout << "."; 
    }
    std::cout << "acc4 done! " << std::endl;
    
    while(!acc5.done())
    {
        tik++;
        if ((tik % 10000) == 0) std::cout << "."; 
    }
    std::cout << "acc5 done! " << std::endl;
    
    while(!acc6.done())
    {
        tik++;
        if ((tik % 10000) == 0) std::cout << "."; 
    }
    std::cout << "acc6 done! " << std::endl;

    auto t_acc_finish = std::chrono::high_resolution_clock::now();

    cout << "\ndone execute.." << endl;

    std::cout << "result = " << acc0.get_return() + acc1.get_return() + acc2.get_return() + acc3.get_return() + 
                                acc4.get_return() + acc5.get_return() + acc6.get_return() << std::endl;

    std::cout << "acc0 result = " << acc0.get_return() << std::endl;
    std::cout << "acc1 result = " << acc1.get_return() << std::endl;
    std::cout << "acc2 result = " << acc2.get_return() << std::endl;
    std::cout << "acc3 result = " << acc3.get_return() << std::endl;
    std::cout << "acc4 result = " << acc4.get_return() << std::endl;
    std::cout << "acc5 result = " << acc5.get_return() << std::endl;
    std::cout << "acc6 result = " << acc6.get_return() << std::endl;

    std::chrono::duration<double> total_io_time = t_file_done - t_start; 
    std::chrono::duration<double> total_malloc_time = t_malloc_done - t_file_done; 
    std::chrono::duration<double> total_memcpy_time = t_memcpy_done - t_malloc_done; 
    std::chrono::duration<double> total_program_time = t_program_done - t_memcpy_done; 
    std::chrono::duration<double> total_exec_time = t_acc_finish - t_acc_start; 
    std::cout << "File IO time: " << total_io_time.count() << "s" << std::endl;
    std::cout << "CMA alloc time: " << total_malloc_time.count() << "s" << std::endl;
    std::cout << "Memcpy time: " << total_memcpy_time.count() << "s" << std::endl;
    std::cout << "FPGA program time: " << total_program_time.count() << "s" << std::endl;
    std::cout << "Kernel exec time: " << total_exec_time.count() << "s" << std::endl;

    cma_free(edges0);
    cma_free(edges1);
    cma_free(edges2);
    cma_free(edges3);
    cma_free(edges4);
    cma_free(edges5);
    cma_free(edges6);
    cma_free(neighbors);
    cma_free(offsets);
    cma_free(progress);

    return 0; 
}
