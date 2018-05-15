#include <string.h>
#define INPUT_LEN ((131072))

int mem_test(volatile unsigned int input[INPUT_LEN], int mem_done)
{
#pragma HLS INTERFACE m_axi depth=4096 port=input offset=slave bundle=INPUT
#pragma HLS INTERFACE s_axilite register    port=mem_done
#pragma HLS INTERFACE s_axilite register    port=return

	unsigned int local_buf[INPUT_LEN];
	unsigned long long int result = 0;

	memcpy(local_buf, (const unsigned int*)input, INPUT_LEN*sizeof(unsigned int));
	mem_done = 1;

	for (int i = 0; i < INPUT_LEN; i++)
#pragma HLS pipeline
		result += local_buf[i];
	return result;

}
