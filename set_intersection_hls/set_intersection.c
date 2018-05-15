#include <string.h>
#define INPUT_LEN 4096

int intersect(volatile unsigned int input_a[INPUT_LEN], volatile unsigned int input_b[INPUT_LEN], volatile unsigned int len_a, volatile unsigned int len_b)
{
#pragma HLS INTERFACE m_axi depth=4096 port=input_a offset=slave bundle=INPUT_A
#pragma HLS INTERFACE m_axi depth=4096 port=input_b offset=slave bundle=INPUT_B
#pragma HLS INTERFACE s_axilite register    port=len_a
#pragma HLS INTERFACE s_axilite register    port=len_b
#pragma HLS INTERFACE s_axilite register    port=return

	unsigned int local_a[INPUT_LEN];
	unsigned int local_b[INPUT_LEN];

	memcpy(local_a, (const unsigned int*)input_a, len_a*sizeof(unsigned int));
	memcpy(local_b, (const unsigned int*)input_b, len_b*sizeof(unsigned int));

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
