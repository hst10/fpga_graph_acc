#include <string.h>
#define INPUT_LEN 4096

int intersect(volatile int input_a[INPUT_LEN], volatile int input_b[INPUT_LEN])
{
#pragma HLS INTERFACE m_axi depth=4096 port=input_a offset=slave bundle=INPUT_A
#pragma HLS INTERFACE m_axi depth=4096 port=input_b offset=slave bundle=INPUT_B
#pragma HLS INTERFACE s_axilite register    port=return

	unsigned int local_a[INPUT_LEN];
	unsigned int local_b[INPUT_LEN];

	memcpy(local_a, (const int*)input_a, INPUT_LEN*sizeof(int));
	memcpy(local_b, (const int*)input_b, INPUT_LEN*sizeof(int));

	unsigned int count = 0;
	unsigned int idx_a = 0;
	unsigned int idx_b = 0;

	while (idx_a < INPUT_LEN && idx_b < INPUT_LEN)
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
