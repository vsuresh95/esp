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

#define ITERATIONS 100

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

// static const char usage_str[] = "usage: sort coherence cmd [n_elems] [n_batches] [-v]\n"
// 	"  coherence: none|llc-coh-dma|coh-dma|coh\n"
// 	"  cmd: config|test|run|hw|flush\n"
// 	"\n"
// 	"Optional arguments: n_elems and batch apply to 'config', 'hw' and 'test':\n"
// 	"  n_elems: number of elements per batch to be sorted\n"
// 	"  n_batches: number of batches\n"
// 	"\n"
// 	"The remaining option is only optional for 'test':\n"
// 	"  -v: enable verbose output for output-to-gold comparison\n";

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

// static void init_buf (float *buf, unsigned sort_size, unsigned sort_batch)
// {
// 	int i, j;
// 	printf("Generate random input...\n");
// 	srand(time(NULL));
// 	for (j = 0; j < sort_batch; j++)
// 		for (i = 0; i < sort_size; i++) {
// 			/* TAV rand between 0 and 1 */
// 			buf[sort_size * j + i] = ((float) rand () / (float) RAND_MAX);
// 			/* /\* More general testbench *\/ */
// 			/* float M = 100000.0; */
// 			/* buf[sort_size * j + i] =  M * ((float) rand() / (float) RAND_MAX) - M/2; */
// 			/* /\* Easyto debug...! *\/ */
// 			/* buf[sort_size * j + i] = (float) (sort_size - i);; */
// 		}
// }

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

// static inline size_t sort_size(struct sort_test *t)
// {
// 	return t->n_elems * t->n_batches * sizeof(float);
// }

// static void sort_alloc_buf(struct test_info *info)
// {
// 	struct sort_test *t = to_sort(info);

// 	t->hbuf = malloc0_check(sort_size(t));
// 	if (!strcmp(info->cmd, "test"))
// 		t->sbuf = malloc0_check(sort_size(t));
// }

// static void sort_alloc_contig(struct test_info *info)
// {
// 	struct sort_test *t = to_sort(info);

// 	printf("HW buf size: %zu\n", sort_size(t));
// 	if (contig_alloc(sort_size(t), &info->contig) == NULL)
// 		die_errno(__func__);
// }

// static void sort_init_bufs(struct test_info *info)
// {
// 	struct sort_test *t = to_sort(info);

// 	init_buf(t->hbuf, t->n_elems, t->n_batches);
// 	contig_copy_to(info->contig, 0, t->hbuf, sort_size(t));
// 	if (!strcmp(info->cmd, "test"))
// 		memcpy(t->sbuf, t->hbuf, sort_size(t));
// }

// static void sort_set_access(struct test_info *info)
// {
// 	struct sort_test *t = to_sort(info);

// 	t->desc.size = t->n_elems;
// 	t->desc.batch = t->n_batches;
// }

// static bool sort_diff_ok(struct test_info *info)
// {
// 	struct sort_test *t = to_sort(info);
// 	int total_err = 0;
// 	int i;

// 	contig_copy_from(t->hbuf, info->contig, 0, sort_size(t));
// 	for (i = 0; i < t->n_batches; i++) {
// 		int err;

// 		err = check_gold(t->sbuf, t->hbuf, t->n_elems, t->verbose);
// 		if (err)
// 			printf("Batch %d: %d mismatches\n", i, err);
// 		total_err += err;
// 	}
// 	if (t->verbose) {
// 		for (i = 0; i < t->n_batches; i++) {
// 			int j;

// 			printf("BATCH %d\n", i);
// 			for (j = 0; j < t->n_elems; j++) {
// 				printf("      \t%d : %.15g\n",
// 					i, t->hbuf[t->n_elems * i + j]);
// 			}
// 			printf("\n");
// 		}
// 	}
// 	if (total_err)
// 		printf("%d mismatches in total\n", total_err);
// 	return !total_err;
// }

// static struct sort_test sort_test = {
// 	.info = {
// 		.name		= NAME,
// 		.devname	= DEVNAME,
// 		.alloc_buf	= sort_alloc_buf,
// 		.alloc_contig	= sort_alloc_contig,
// 		.init_bufs	= sort_init_bufs,
// 		.set_access	= sort_set_access,
// 		.comp		= sort_comp,
// 		.diff_ok	= sort_diff_ok,
// 		.esp		= &sort_test.desc.esp,
// 		.cm		= SORT_STRATUS_IOC_ACCESS,
// 	},
// };

// static void NORETURN usage(void)
// {
// 	fprintf(stderr, "%s", usage_str);
// 	exit(1);
// }

int main(int argc, char *argv[])
{
	// int n_argc;

	// if (argc < 3)
	// 	usage();

	// if (!strcmp(argv[2], "run") || !strcmp(argv[2], "flush"))
	// 	n_argc = 3;
	// else
	// 	n_argc = 5;

	// if (argc < n_argc)
	// 	usage();

	// if (n_argc > 3) {
	// 	sort_test.n_elems = strtoul(argv[3], NULL, 0);
	// 	sort_test.n_batches = strtoul(argv[4], NULL, 0);
	// 	if (argc == 6) {
	// 		if (strcmp(argv[5], "-v"))
	// 			usage();
	// 		sort_test.verbose = true;
	// 	}
	// }
	// return test_main(&sort_test.info, argv[1], argv[2]);

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

		srand(time(NULL));
		for (j = 0; j < LEN; ++j) {
			gold[j] = ((float) rand () / (float) RAND_MAX);
			// gold[j] = (1.0 / (float) j + 1);
		}
		// printf("Gold Initialized\n");

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
		quicksort(gold, LEN);
		t_sw_sort += end_counter();
		// printf("SW Computed\n");

		start_counter();
		errors += check_gold(gold, &buf[SYNC_VAR_SIZE + LEN + SYNC_VAR_SIZE], LEN, true);
		// Inform the accelerator - ready for next iteration.
		UpdateSync((void*) &buf[SYNC_VAR_SIZE + LEN + READY_FLAG_OFFSET], 1);
		t_cpu_read += end_counter();
		// printf("Output read\n");
	}
#else
	for (i = 0; i < ITERATIONS; ++i) {
		srand(time(NULL));
		for (j = 0; j < LEN; ++j) {
			gold[j] = ((float) rand () / (float) RAND_MAX);
			// gold[j] = (1.0 / (float) j + 1);
		}

		start_counter();
		init_buf(&buf[SYNC_VAR_SIZE], gold, LEN, 1);
		t_cpu_write += end_counter();

		// for (j = 0; j < LEN; ++j){
		// 	// printf("A[%d]: array=%.15g; gold=%.15g\n", j, buf[SYNC_VAR_SIZE + j], gold[j]);
		// 	printf("A[%d] = %.15g;\n", j, gold[j]);
		// }
		
		start_counter();
		quicksort(gold, LEN);
		t_sw_sort += end_counter();
		
		start_counter();
		esp_run(cfg_000, NACC);
		t_sort += end_counter();

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


