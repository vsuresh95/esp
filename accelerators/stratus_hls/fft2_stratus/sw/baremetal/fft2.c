/* Copyright (c) 2011-2019 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include "utils/fft2_utils.h"

#define COH_MODE 0
#define ESP
#define ITERATIONS 1000
#define LOG_LEN 10

#include "cfg.h"
#include "sw_func.h"
#include "coh_func.h"
#include "acc_func.h"

static uint64_t t_start = 0;
static uint64_t t_end = 0;

uint64_t t_cpu_write;
uint64_t t_acc;
uint64_t t_cpu_read;

static inline void start_counter() {
	asm volatile (
		"li t0, 0;"
		"csrr t0, mcycle;"
		"mv %0, t0"
		: "=r" (t_start)
		:
		: "t0"
	);
}

static inline uint64_t end_counter() {
	asm volatile (
		"li t0, 0;"
		"csrr t0, mcycle;"
		"mv %0, t0"
		: "=r" (t_end)
		:
		: "t0"
	);

	return (t_end - t_start);
}

static inline void write_mem (void* dst, int64_t value_64)
{
	asm volatile (
		"mv t0, %0;"
		"mv t1, %1;"
		".word " QU(WRITE_CODE)
		:
		: "r" (dst), "r" (value_64)
		: "t0", "t1", "memory"
	);
}

static inline int64_t read_mem (void* dst)
{
	int64_t value_64;

	asm volatile (
		"mv t0, %1;"
		".word " QU(READ_CODE) ";"
		"mv %0, t1"
		: "=r" (value_64)
		: "r" (dst)
		: "t0", "t1", "memory"
	);

	return value_64;
}

static void validate_buf(token_t *out, float *gold)
{
	int j;
	unsigned errors = 0;
	int local_len = len;
	spandex_token_t out_data;
	void* dst;
	native_t val;
	uint32_t ival;

	dst = (void*)(out+SYNC_VAR_SIZE);

	for (j = 0; j < 2 * local_len; j+=2, dst+=8) {
		out_data.value_64 = read_mem(dst);

		val = fx2float(out_data.value_32_1, FX_IL);
		if (val == 12412.12412) j = 0;
		// ival = *((uint32_t*)&val);
		// printf("%u G %08x O %08x\n", j, ((uint32_t*)gold)[j], ival);

		val = fx2float(out_data.value_32_2, FX_IL);
		if (val == 22412.12412) j = 0;
		// ival = *((uint32_t*)&val);
		// printf("%u G %08x O %08x\n", j, ((uint32_t*)gold)[j+1], ival);
	}
}

static void init_buf(token_t *in, float *gold, token_t *in_filter, float *gold_filter, token_t *in_twiddle, float *gold_twiddle)
{
	int j;
	int local_len = len;
	spandex_token_t in_data;
	void* dst;

	dst = (void*)(in+SYNC_VAR_SIZE);

	// convert input to fixed point -- TODO here all the inputs gold values are refetched
	for (j = 0; j < 2 * local_len; j+=2, dst+=8)
	{
		in_data.value_32_1 = float2fx((native_t) gold[j], FX_IL);
		in_data.value_32_2 = float2fx((native_t) gold[j+1], FX_IL);

		write_mem(dst, in_data.value_64);
		// printf("IN %u %llx\n", j, ((uint32_t*) in_data.value_64));
	}

	dst = (void*)(in_filter);

	// convert filter to fixed point
	for (j = 0; j < 2 * (local_len+1); j+=2, dst+=8)
	{
		in_data.value_32_1 = float2fx((native_t) gold_filter[j], FX_IL);
		in_data.value_32_2 = float2fx((native_t) gold_filter[j+1], FX_IL);

		write_mem(dst, in_data.value_64);
		// printf("FLT %u %llx\n", j, ((uint32_t*) in_data.value_64));
	}

	dst = (void*)(in_twiddle);

	// convert twiddle to fixed point
	for (j = 0; j < local_len; j+=2, dst+=8)
	{
		in_data.value_32_1 = float2fx((native_t) gold_twiddle[j], FX_IL);
		in_data.value_32_2 = float2fx((native_t) gold_twiddle[j+1], FX_IL);

		write_mem(dst, in_data.value_64);
		// printf("TWD %u %llx\n", j, ((uint32_t*) in_data.value_64));
	}
}

int main(int argc, char * argv[])
{
	int i;
	t_cpu_write = 0;
	t_acc = 0;
	t_cpu_read = 0;

	///////////////////////////////////////////////////////////////
	// Init data size parameters
	///////////////////////////////////////////////////////////////
	init_params();

	///////////////////////////////////////////////////////////////
	// Allocate memory pointers
	///////////////////////////////////////////////////////////////
	gold = aligned_malloc((7 * out_len) * sizeof(float));
	mem = aligned_malloc(mem_size);
	printf("  memory buffer base-address = %p\n", mem);

    volatile token_t* sm_sync = (volatile token_t*) mem;

	// Allocate and populate page table
	ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (i = 0; i < NCHUNK(mem_size); i++)
		ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];

	printf("  ptable = %p\n", ptable);
	printf("  nchunk = %lu\n", NCHUNK(mem_size));

	//////////////////////////////////////////////////////
	// Initialize golden buffers and compute golden output
	//////////////////////////////////////////////////////
	golden_data_init(gold, (gold + out_len) /* gold_ref*/, (gold + 2 * out_len) /* gold_filter */, (gold + 4 * out_len) /* gold_twiddle */);

	chain_sw_impl((gold + out_len) /* gold_ref*/, (gold + 2 * out_len) /* gold_filter */, (gold + 4 * out_len) /* gold_twiddle */, (gold + 6 * out_len) /* gold_freqdata */);

	///////////////////////////////////////////////////////////////
	// Start all accelerators
	///////////////////////////////////////////////////////////////
	sm_sync[0] = 0;
	sm_sync[acc_offset] = 0;
	sm_sync[2*acc_offset] = 0;
	sm_sync[3*acc_offset] = 0;

	printf("  Mode: %s\n", print_coh);

	sm_sync[2] = 0;
	sm_sync[acc_offset+2] = 0;
	sm_sync[2*acc_offset+2] = 0;

	start_acc();

	///////////////////////////////////////////////////////////////
	// Transfer to accelerator memory
	///////////////////////////////////////////////////////////////
	for (i = 0; i < ITERATIONS; i++)
	{
		start_counter();
		init_buf(mem, gold, (mem + 5 * acc_offset) /* in_filter */, (gold + 2 * out_len) /* gold_filter */,
				(mem + 7 * acc_offset) /* in_twiddle */, (gold + 4 * out_len) /* gold_twiddle */);
		t_cpu_write += end_counter();

		start_counter();

		if (i == ITERATIONS - 1) {
			sm_sync[2] = 1;
			sm_sync[acc_offset+2] = 1;
			sm_sync[2*acc_offset+2] = 1;
		}

		sm_sync[0] = 1;
		while(sm_sync[NUM_DEVICES*acc_offset] != 1);
		sm_sync[NUM_DEVICES*acc_offset] = 0;
		t_acc += end_counter();

		///////////////////////////////////////////////////////////////
		// Read back output
		///////////////////////////////////////////////////////////////
		start_counter();
		validate_buf(&mem[NUM_DEVICES*acc_offset], (gold + out_len));
		t_cpu_read += end_counter();
	}

	///////////////////////////////////////////////////////////////
	// Terminate all accelerators
	///////////////////////////////////////////////////////////////
	terminate_acc();

	aligned_free(ptable);
	aligned_free(mem);
	aligned_free(gold);

	printf("  CPU write = %lu\n", t_cpu_write/ITERATIONS);
	printf("  ACC = %lu\n", t_acc/ITERATIONS);
	printf("  CPU read = %lu\n", t_cpu_read/ITERATIONS);
	printf("\n");

    // while(1);

	return 0;
}
