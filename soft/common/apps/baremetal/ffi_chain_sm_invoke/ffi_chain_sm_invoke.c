/* Copyright (c) 2011-2019 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>

#include "utils/fft2_utils.h"
#include "utils/ffi_chain_utils.h"

static uint64_t t_start = 0;
static uint64_t t_stop = 0;

uint64_t t_cpu_write;
uint64_t t_acc;
uint64_t t_cpu_read;
uint64_t t_begin;
uint64_t t_end;

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
		: "=r" (t_stop)
		:
		: "t0"
	);

	return (t_stop - t_start);
}

static inline uint64_t get_counter() {
	uint64_t count;

	asm volatile (
		"li t0, 0;"
		"csrr t0, mcycle;"
		"mv %0, t0"
		: "=r" (count)
		:
		: "t0"
	);

	return count;
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

void UpdateSync(unsigned FlagOFfset, int64_t UpdateValue) {
	volatile token_t* sync = mem + FlagOFfset;

	asm volatile ("fence w, w");

	// Need to cast to void* for extended ASM code.
	write_mem((void *) sync, UpdateValue);

	asm volatile ("fence w, w");
}

void SpinSync(unsigned FlagOFfset, int64_t SpinValue) {
	volatile token_t* sync = mem + FlagOFfset;
	int64_t ExpectedValue = SpinValue;
	int64_t ActualValue = 0xcafedead;

	while (ActualValue != ExpectedValue) {
		// Need to cast to void* for extended ASM code.
		ActualValue = read_mem((void *) sync);
	}
}

bool TestSync(unsigned FlagOFfset, int64_t TestValue) {
	volatile token_t* sync = mem + FlagOFfset;
	int64_t ExpectedValue = TestValue;
	int64_t ActualValue = 0xcafedead;

	// Need to cast to void* for extended ASM code.
	ActualValue = read_mem((void *) sync);

	if (ActualValue != ExpectedValue) return false;
	else return true;
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
		// printf("%u G %08x O %08x\n", j, ((uint32_t*) gold)[j], ival);

		val = fx2float(out_data.value_32_2, FX_IL);
		if (val == 22412.12412) j = 0;
		// ival = *((uint32_t*)&val);
		// printf("%u G %08x O %08x\n", j, ((uint32_t*) gold)[j+1], ival);
	}
}

static void init_buf_data(token_t *in, float *gold)
{
	int j;
	int local_len = len;
	spandex_token_t in_data;
	int64_t value_64;
	void* dst;

	dst = (void*)(in+SYNC_VAR_SIZE);

	// convert input to fixed point -- TODO here all the inputs gold values are refetched
	for (j = 0; j < 2 * local_len; j+=2, dst+=8)
	{
		in_data.value_32_1 = float2fx((native_t) gold[j], FX_IL);
		in_data.value_32_2 = float2fx((native_t) gold[j+1], FX_IL);

		write_mem(dst, in_data.value_64);
		// printf("IN %u %llx\n", j, (in_data.value_64));
	}
}

static void init_buf_filters(token_t *in_filter, int64_t *gold_filter)
{
	int j;
	int local_len = len;
	spandex_token_t in_data;
	int64_t value_64;
	void* dst;

	dst = (void*)(in_filter);

	// convert filter to fixed point
	for (j = 0; j < (local_len+1); j++, dst+=8)
	{
		value_64 = gold_filter[j];

		write_mem(dst, value_64);
		// printf("FLT %u %llx\n", j, value_64);
	}
}

static void flt_twd_fxp_conv(token_t *gold_filter_fxp, float *gold_filter, token_t *in_twiddle, float *gold_twiddle)
{
	int j;
	int local_len = len;
	spandex_token_t in_data;
	void* dst;

	// convert filter to fixed point
	for (j = 0; j < 2 * (local_len+1); j++)
	{
		gold_filter_fxp[j] = float2fx((native_t) gold_filter[j], FX_IL);
	}

	dst = (void*)(in_twiddle);

	// convert twiddle to fixed point
	for (j = 0; j < local_len; j+=2, dst+=8)
	{
		in_data.value_32_1 = float2fx((native_t) gold_twiddle[j], FX_IL);
		in_data.value_32_2 = float2fx((native_t) gold_twiddle[j+1], FX_IL);

		write_mem(dst, in_data.value_64);
		// printf("TWD %u %llx\n", j, in_data.value_64);
	}
}

int main(int argc, char * argv[])
{
	int i;
	t_cpu_write = 0;
	t_acc = 0;
	t_cpu_read = 0;
	t_begin = 0;
	t_end = 0;

	///////////////////////////////////////////////////////////////
	// Init data size parameters
	///////////////////////////////////////////////////////////////
	init_params();

	///////////////////////////////////////////////////////////////
	// Allocate memory pointers
	///////////////////////////////////////////////////////////////
	gold = aligned_malloc((8 * out_len) * sizeof(float));
	fxp_filters = aligned_malloc((out_len + 2) * sizeof(float));
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

	// chain_sw_impl((gold + out_len) /* gold_ref*/, (gold + 2 * out_len) /* gold_filter */, (gold + 4 * out_len) /* gold_twiddle */, (gold + 6 * out_len) /* gold_freqdata */);

	///////////////////////////////////////////////////////////////
	// Do repetitive things initially
	///////////////////////////////////////////////////////////////
	flt_twd_fxp_conv(fxp_filters /* gold_filter_fxp */, (gold + 2 * out_len) /* gold_filter */, (mem + 7 * acc_offset) /* in_twiddle */, (gold + 4 * out_len) /* gold_twiddle */);

	printf("  Mode: %s\n", print_coh);

	///////////////////////////////////////////////////////////////
	// Start all accelerators
	///////////////////////////////////////////////////////////////
	// Reset all sync variables to default values.
	for (unsigned ChainID = 0; ChainID < 4; ChainID++) {
		UpdateSync(ChainID*acc_len + VALID_FLAG_OFFSET, 0);
		UpdateSync(ChainID*acc_len + READY_FLAG_OFFSET, 1);
		UpdateSync(ChainID*acc_len + END_FLAG_OFFSET, 0);
	}

	UpdateSync(acc_len + FLT_VALID_FLAG_OFFSET, 0);
	UpdateSync(acc_len + FLT_READY_FLAG_OFFSET, 1);

	probe_fir();
	probe_fft();
	start_acc();

	// Helper flags for sync flags that the CPU needs to access.
	// We add a pair of sync flags for FIR filter weights, so that
	// this write can be overlapped with FFT operations.
	const unsigned ConsRdyFlag = 0*acc_len + READY_FLAG_OFFSET;
	const unsigned ConsVldFlag = 0*acc_len + VALID_FLAG_OFFSET;
	const unsigned FltRdyFlag = 1*acc_len + FLT_READY_FLAG_OFFSET;
	const unsigned FltVldFlag = 1*acc_len + FLT_VALID_FLAG_OFFSET;
	const unsigned ProdRdyFlag = 3*acc_len + READY_FLAG_OFFSET;
	const unsigned ProdVldFlag = 3*acc_len + VALID_FLAG_OFFSET;

	// Time marker to measure total execution time
	t_start = get_counter();

#if (IS_PIPELINE == 0)
	///////////////////////////////////////////////////////////////
	// NON PIPELINED VERSION
	///////////////////////////////////////////////////////////////
	unsigned IterationsLeft = ITERATIONS;

	while (IterationsLeft != 0) {
		start_counter();
		// Wait for FFT (consumer) to be ready.
		SpinSync(ConsRdyFlag, 1);
		// Reset flag for next iteration.
		UpdateSync(ConsRdyFlag, 0);
		// Write input data for FFT.
		init_buf_data(mem, gold);

		// Wait for FIR (consumer) to be ready.
		SpinSync(FltRdyFlag, 1);
		// Reset flag for next iteration.
		UpdateSync(FltRdyFlag, 0);
		// // Write input data for FIR filters.
		// init_buf_filters((mem + 5 * acc_offset) /* in_filter */, (int64_t*) fxp_filters /* gold_filter */);
		// Inform FIR (consumer) of filters ready.
		UpdateSync(FltVldFlag, 1);
		// Inform FFT (consumer) to start.
		UpdateSync(ConsVldFlag, 1);
		t_cpu_write += end_counter();

		start_counter();
		// Wait for IFFT (producer) to send output.
		SpinSync(ProdVldFlag, 1);
		// Reset flag for next iteration.
		UpdateSync(ProdVldFlag, 0);
		t_acc += end_counter();

		start_counter();
		// Read back output from IFFT
		validate_buf(&mem[NUM_DEVICES*acc_offset], (gold + out_len));
		// Inform IFFT (producer) - ready for next iteration.
		UpdateSync(ProdRdyFlag, 1);
		t_cpu_read += end_counter();

		IterationsLeft--;
	}
#else
	///////////////////////////////////////////////////////////////
	// PIPELINED VERSION
	///////////////////////////////////////////////////////////////
	unsigned InputIterationsLeft = ITERATIONS;
	unsigned FilterIterationsLeft = ITERATIONS;
	unsigned OutputIterationsLeft = ITERATIONS;

	// Check if any of the tasks are pending. If yes,
	// check if any of them a ready, based on a simple priority.
	// Once a task is complete, we reduce the number of tasks left
	// for that respective task.
	while (InputIterationsLeft != 0 || FilterIterationsLeft != 0 || OutputIterationsLeft != 0) {
		if (InputIterationsLeft) {
			// Wait for FFT (consumer) to be ready
			if (TestSync(ConsRdyFlag, 1)) {
				UpdateSync(ConsRdyFlag, 0);

				// Write input data for FFT
        		start_counter();
				init_buf_data(mem, gold);
				t_cpu_write += end_counter();

				// Inform FFT (consumer)
				UpdateSync(ConsVldFlag, 1);
				InputIterationsLeft--;
			}
		}

		if (FilterIterationsLeft) {
			// Wait for FIR (consumer) to be ready
			if (TestSync(FltRdyFlag, 1)) {
				UpdateSync(FltRdyFlag, 0);

				// Write input data for filters
        		start_counter();
				// init_buf_filters((mem + 5 * acc_offset) /* in_filter */, (int64_t*) fxp_filters /* gold_filter */);
				t_cpu_write += end_counter();

				// Inform FIR (consumer)
				UpdateSync(FltVldFlag, 1);
				FilterIterationsLeft--;
			}
		}

		if (OutputIterationsLeft) {
			// Wait for IFFT (producer) to send output
			if (TestSync(ProdVldFlag, 1)) {
				UpdateSync(ProdVldFlag, 0);
				
				// Read back output
        		start_counter();
				validate_buf(&mem[NUM_DEVICES*acc_offset], (gold + out_len));
				t_cpu_read += end_counter();

				// Inform IFFT (producer)
				UpdateSync(ProdRdyFlag, 1);
				OutputIterationsLeft--;
			}
		}
	}
#endif

	// Time marker to measure total execution time
	t_end = get_counter();

	aligned_free(ptable);
	aligned_free(mem);
	aligned_free(gold);

	printf("  CPU write = %lu\n", t_cpu_write/ITERATIONS);
	printf("  ACC op = %lu\n", t_acc/ITERATIONS);
	printf("  CPU read = %lu\n", t_cpu_read/ITERATIONS);
	printf("  Total time = %lu\n", (t_end - t_begin)/ITERATIONS);
	printf("\n");

    // while(1);

	return 0;
}