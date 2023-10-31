// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define ENABLE_SM
#define SPX

#ifdef SPX
	#define COH_MODE 2
#else
	#define IS_ESP 1
	#define COH_MODE 1
#endif

#define ITERATIONS 1000

#include <my_stringify.h>

#include "sw_func.h"
#include "sm.h"

static uint64_t t_start = 0;
static uint64_t t_end = 0;

uint64_t t_sw_sort;
uint64_t t_cpu_write;
uint64_t t_sort;
uint64_t t_cpu_read;

// static inline
void start_counter() {
    asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (t_start)
		:
		: "t0"
	);
}

// static inline
uint64_t end_counter() {
	asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (t_end)
		:
		: "t0"
	);

	return (t_end - t_start);
}

// static inline void write_mem (void* dst, int64_t value_64)
// {
// 	asm volatile (
// 		"mv t0, %0;"
// 		"mv t1, %1;"
// 		".word " QU(WRITE_CODE)
// 		:
// 		: "r" (dst), "r" (value_64)
// 		: "t0", "t1", "memory"
// 	);
// }

// static inline int64_t read_mem (void* dst)
// {
// 	int64_t value_64;

// 	asm volatile (
// 		"mv t0, %1;"
// 		".word " QU(READ_CODE) ";"
// 		"mv %0, t1"
// 		: "=r" (value_64)
// 		: "r" (dst)
// 		: "t0", "t1", "memory"
// 	);

// 	return value_64;
// }

static int check_gold (float *gold, float *array, int len, bool verbose)
{
	int i;
	int rtn = 0;
	spandex_token_t out_data;
	void* dst;

	// for (i = 0; i < len; i++) {
	// 	if (array[i] != gold[i]) {
	// 		if (verbose)
	// 			printf("A[%d]: array=%.15g; gold=%.15g\n", i, array[i], gold[i]);
	// 		rtn++;
	// 	}
	// 	printf("A[%d]: array=%.15g; gold=%.15g\n", i, array[i], gold[i]);
	// }

	dst = (void*) array;
	for (i = 0; i < len; i += 2, dst += 8) {
		out_data.value_64 = read_mem_reqodata(dst);

		if (out_data.value_32_1 != gold[i]) {
			rtn += 1;
		}
		if (out_data.value_32_2 != gold[i + 1]) {
			rtn += 1;
		}
	}
	return rtn;
}

static void init_buf (float *buf, float* gold, unsigned sort_size, unsigned sort_batch)
{
	int i, j;
	// printf("Generate random input...\n");
	// srand(time(NULL));

	spandex_token_t in_data;
	void* dst;

	dst = (void*) buf;

	for (j = 0; j < sort_batch; j++)
		for (i = 0; i < sort_size; i += 2, dst += 8) {
			/* TAV rand between 0 and 1 */
			// buf[sort_size * j + i] = gold[i];
			in_data.value_32_1 = gold[i];
			in_data.value_32_2 = gold[i + 1];

			write_mem_wtfwd(dst, in_data.value_64);
		}
}

int main(int argc, char *argv[])
{
	int errors, i, j;

	errors = 0;

	t_sw_sort = 0;
	t_cpu_write = 0;
	t_sort = 0;
	t_cpu_read = 0;

	float* gold;
	float* buf;

	buf = (float*) esp_alloc(2 * LEN * sizeof(float) + 2 * SYNC_VAR_SIZE * sizeof(unsigned));
	gold = (float*) esp_alloc(LEN * sizeof(float));
	cfg_000[0].hw_buf = buf;

	printf("	buf = %p\n", buf);
	printf("	gold = %p\n", gold);
	printf("	sizeof(int) = %lu\n", sizeof(int));
	printf("	sizeof(unsigned) = %lu\n", sizeof(unsigned));
	printf("	sizeof(float) = %lu\n", sizeof(float));

	sort_cfg_000[0].esp.coherence = coherence;
	sort_cfg_000[0].spandex_conf = spandex_config.spandex_reg;
	sort_cfg_000[0].input_offset = SYNC_VAR_SIZE;
	sort_cfg_000[0].output_offset = SYNC_VAR_SIZE + LEN + SYNC_VAR_SIZE;
	sort_cfg_000[0].prod_valid_offset = VALID_FLAG_OFFSET;
	sort_cfg_000[0].prod_ready_offset = READY_FLAG_OFFSET;
	sort_cfg_000[0].cons_valid_offset = SYNC_VAR_SIZE + LEN + VALID_FLAG_OFFSET;
	sort_cfg_000[0].cons_ready_offset = SYNC_VAR_SIZE + LEN + READY_FLAG_OFFSET;

	printf("\n====== %s ======\n\n", cfg_000[0].devname);
	printf("	.size = %u\n", sort_cfg_000[0].size);
	printf("	.batch = %u\n", sort_cfg_000[0].batch);
	printf("	Coherence = %s\n", CohPrintHeader);
	printf("	ITERATIONS = %u\n", ITERATIONS);

	for (i = 0; i < ITERATIONS / 10; ++i) {
		// srand(time(NULL));
		for (j = 0; j < LEN; ++j) {
			// gold[j] = ((float) rand () / (float) RAND_MAX);
			gold[j] = (1.0 / (float) j + 1);
		}

		start_counter();
		quicksort(gold, LEN);
		t_sw_sort += end_counter();
	}

#ifdef ENABLE_SM
	sort_cfg_000[0].esp.start_stop = 1;
	esp_run(cfg_000, NACC);

	// Reset all sync variables to default values.
	UpdateSync((void*) &buf[VALID_FLAG_OFFSET], 0);
	UpdateSync((void*) &buf[READY_FLAG_OFFSET], 1);
	UpdateSync((void*) &buf[END_FLAG_OFFSET], 0);
	UpdateSync((void*) &buf[SYNC_VAR_SIZE + LEN + VALID_FLAG_OFFSET], 0);
	UpdateSync((void*) &buf[SYNC_VAR_SIZE + LEN + READY_FLAG_OFFSET], 1);
	UpdateSync((void*) &buf[SYNC_VAR_SIZE + LEN + END_FLAG_OFFSET], 0);

	for (i = 0; i < ITERATIONS; ++i) {
		// printf("SM Enabled\n");
		start_counter();
		// Wait for the accelerator to be ready
		SpinSync((void*) &buf[READY_FLAG_OFFSET], 1);
		// Reset flag for the next iteration
		UpdateSync((void*) &buf[READY_FLAG_OFFSET], 0);
		// When the accelerator is ready, we write the input data to it
		init_buf((float *) &buf[SYNC_VAR_SIZE], gold, LEN, 1);
		

		if (i == ITERATIONS - 1) {
			UpdateSync((void*) &buf[END_FLAG_OFFSET], 1);
			UpdateSync((void*) &buf[SYNC_VAR_SIZE + LEN + END_FLAG_OFFSET], 1);
		}
		// Inform the accelerator to start.
		UpdateSync((void*) &buf[VALID_FLAG_OFFSET], 1);
		t_cpu_write += end_counter();
		// printf("Buf Initialized\n");

		start_counter();
		// Wait for the accelerator to send output.
		SpinSync((void*) &buf[SYNC_VAR_SIZE + LEN + VALID_FLAG_OFFSET], 1);
		// Reset flag for next iteration.
		UpdateSync((void*) &buf[SYNC_VAR_SIZE + LEN + VALID_FLAG_OFFSET], 0);
		t_sort += end_counter();
		// printf("Acc done\n");
		
		start_counter();
		errors += check_gold(gold, &buf[SYNC_VAR_SIZE + LEN + SYNC_VAR_SIZE], LEN, true);
		// Inform the accelerator - ready for next iteration.
		UpdateSync((void*) &buf[SYNC_VAR_SIZE + LEN + READY_FLAG_OFFSET], 1);
		t_cpu_read += end_counter();
		// printf("Output read\n");
	}
#else
	for (i = 0; i < ITERATIONS; ++i) {
		start_counter();
		init_buf(&buf[SYNC_VAR_SIZE], gold, LEN, 1);
		t_cpu_write += end_counter();

		// for (j = 0; j < LEN; ++j){
		// 	// printf("A[%d]: array=%.15g; gold=%.15g\n", j, buf[SYNC_VAR_SIZE + j], gold[j]);
		// 	printf("A[%d] = %.15g;\n", j, gold[j]);
		// }

		start_counter();
		esp_run(cfg_000, NACC);
		t_sort += end_counter();
		
		start_counter();
		quicksort(gold, LEN);
		t_sw_sort += end_counter();

		start_counter();
		errors += check_gold(gold, &buf[SYNC_VAR_SIZE + LEN + SYNC_VAR_SIZE], LEN, true);
		t_cpu_read += end_counter();

		// for (j = 0; j < LEN; ++j){
		// 	printf("A[%d]: array=%.15g; gold=%.15g\n", j, buf[SYNC_VAR_SIZE + LEN + SYNC_VAR_SIZE + j], gold[j]);
		// }
	}
#endif

	esp_free(gold);
	esp_free(buf);

	printf("	Error = %d\n", errors);

	printf("	SW Time = %lu\n", t_sw_sort);
	printf("	CPU Write Time = %lu\n", t_cpu_write);
	printf("	Sort Time = %lu\n", t_sort);
	printf("	CPU Read Time = %lu\n", t_cpu_read);

	return errors;

}


