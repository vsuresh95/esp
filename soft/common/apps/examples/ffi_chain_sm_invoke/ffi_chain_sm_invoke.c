/* Copyright (c) 2011-2019 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __riscv
#include <stdlib.h>
#endif

#include <stdio.h>

#include <esp_accelerator.h>
#include <stdint.h>

#include "ffi_chain_sm_invoke_cfg.h"
#include "utils/fft2_utils.h"
#include "utils/ffi_chain_utils.h"

uint64_t t_wait_cons;
uint64_t t_cpu_write;
uint64_t t_wait_prod;
uint64_t t_cpu_read;

static uint64_t t_start = 0;
static uint64_t t_end = 0;

static inline void start_counter() {
	asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (t_start)
		:
		: "t0"
	);
}

static inline uint64_t end_counter() {
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

static inline void write_mem (void* dst, int64_t value_64)
{
	asm volatile (
		"mv t0, %0\n\t"
		"mv t1, %1\n\t"
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
		"mv t0, %1\n\t"
		".word " QU(READ_CODE) "\n\t"
		"mv %0, t1"
		: "=r" (value_64)
		: "r" (dst)
		: "t0", "t1", "memory"
	);

	return value_64;
}

static int validate_buffer(token_t *out, float *gold)
{
	int j;
	unsigned errors = 0;
	int local_len = len;
	spandex_token_t out_data;
	void* dst;
	native_t val;
	// uint32_t ival;

	dst = (void*)(out+SYNC_VAR_SIZE);

	for (j = 0; j < 2 * local_len; j+=2, dst+=8) {
		out_data.value_64 = read_mem(dst);

		val = fx2float(out_data.value_32_1, FX_IL);
		if (val == 12412.12412) j = 0;

		val = fx2float(out_data.value_32_2, FX_IL);
		if (val == 22412.12412) j = 0;
	}

	return errors;
}

static void init_buffer_data(token_t *in, float *gold)
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
		// printf("IN %u %llx\n", j, (in_data.value_64));
	}
}

static void init_buffer_filters(token_t *in_filter, int64_t *gold_filter)
{
	int j;
	int local_len = len;
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

int main(int argc, char **argv)
{
	int i;

	t_wait_cons = 0;
	t_cpu_write = 0;
	t_wait_prod = 0;
	t_cpu_read = 0;

	float *gold;
	token_t *buf;
	token_t *fxp_filters;

	///////////////////////////////////////////////////////////////
	// Init data size parameters
	///////////////////////////////////////////////////////////////
	init_params();

	///////////////////////////////////////////////////////////////
	// Allocate memory pointers
	///////////////////////////////////////////////////////////////
	buf = (token_t *) esp_alloc(mem_size);
	gold = esp_alloc(8 * out_len * sizeof(float));
	fxp_filters = (token_t *) esp_alloc((out_len + 2) * sizeof(token_t));

	///////////////////////////////////////////////////////////////
	// Update accelerator configs
	///////////////////////////////////////////////////////////////
	cfg_000[0].hw_buf = buf;
	cfg_001[0].hw_buf = buf;
	cfg_002[0].hw_buf = buf;

	fft_cfg_000[0].esp.coherence = coherence;
	fir_cfg_000[0].esp.coherence = coherence;
	fft_cfg_001[0].esp.coherence = coherence;

	fft_cfg_000[0].spandex_conf = spandex_config.spandex_reg;
	fir_cfg_000[0].spandex_conf = spandex_config.spandex_reg;
	fft_cfg_001[0].spandex_conf = spandex_config.spandex_reg;

	fft_cfg_000[0].prod_valid_offset = VALID_FLAG_OFFSET;
	fft_cfg_000[0].prod_ready_offset = READY_FLAG_OFFSET;
	fft_cfg_000[0].cons_valid_offset = acc_len + VALID_FLAG_OFFSET;
	fft_cfg_000[0].cons_ready_offset = acc_len + READY_FLAG_OFFSET;
	fft_cfg_000[0].input_offset = SYNC_VAR_SIZE;
	fft_cfg_000[0].output_offset = acc_len + SYNC_VAR_SIZE;

	fir_cfg_000[0].prod_valid_offset = acc_len + VALID_FLAG_OFFSET;
	fir_cfg_000[0].prod_ready_offset = acc_len + READY_FLAG_OFFSET;
	fir_cfg_000[0].flt_prod_valid_offset = acc_len + FLT_VALID_FLAG_OFFSET;
	fir_cfg_000[0].flt_prod_ready_offset = acc_len + FLT_READY_FLAG_OFFSET;
	fir_cfg_000[0].cons_valid_offset = (2 * acc_len) + VALID_FLAG_OFFSET;
	fir_cfg_000[0].cons_ready_offset = (2 * acc_len) + READY_FLAG_OFFSET;
	fir_cfg_000[0].input_offset = acc_len + SYNC_VAR_SIZE;
	fir_cfg_000[0].flt_input_offset = 5 * acc_len;
	fir_cfg_000[0].twd_input_offset = 7 * acc_len;
	fir_cfg_000[0].output_offset = (2 * acc_len) + SYNC_VAR_SIZE;

	fft_cfg_001[0].prod_valid_offset = (2 * acc_len) + VALID_FLAG_OFFSET;
	fft_cfg_001[0].prod_ready_offset = (2 * acc_len) + READY_FLAG_OFFSET;
	fft_cfg_001[0].cons_valid_offset = (3 * acc_len) + VALID_FLAG_OFFSET;
	fft_cfg_001[0].cons_ready_offset = (3 * acc_len) + READY_FLAG_OFFSET;
	fft_cfg_001[0].input_offset = (2 * acc_len) + SYNC_VAR_SIZE;
	fft_cfg_001[0].output_offset = (3 * acc_len) + SYNC_VAR_SIZE;

    volatile token_t* sm_sync = (volatile token_t*) buf;

	//////////////////////////////////////////////////////
	// Initialize golden buffers and compute golden output
	//////////////////////////////////////////////////////
	golden_data_init(gold, (gold + out_len) /* gold_ref*/, (gold + 2 * out_len) /* gold_filter */, (gold + 4 * out_len) /* gold_twiddle */);
	chain_sw_impl((gold + out_len), (gold + 2 * out_len) /* gold_filter */,(gold + 4 * out_len) /* gold_twiddle */, (gold + 6 * out_len) /* gold_freqdata */);

	///////////////////////////////////////////////////////////////
	// Do repetitive things initially
	///////////////////////////////////////////////////////////////
	flt_twd_fxp_conv(fxp_filters /* gold_filter_fxp */, (gold + 2 * out_len) /* gold_filter */, (buf + 7 * acc_offset) /* in_twiddle */, (gold + 4 * out_len) /* gold_twiddle */);

	printf("  Mode: %s\n", print_coh);

	///////////////////////////////////////////////////////////////
	// Start all accelerators
	///////////////////////////////////////////////////////////////
	sm_sync[0*acc_offset + VALID_FLAG_OFFSET] = 0;
	sm_sync[1*acc_offset + VALID_FLAG_OFFSET] = 0;
	sm_sync[2*acc_offset + VALID_FLAG_OFFSET] = 0;
	sm_sync[3*acc_offset + VALID_FLAG_OFFSET] = 0;

	sm_sync[0*acc_offset + READY_FLAG_OFFSET] = 1;
	sm_sync[1*acc_offset + READY_FLAG_OFFSET] = 1;
	sm_sync[2*acc_offset + READY_FLAG_OFFSET] = 1;
	sm_sync[3*acc_offset + READY_FLAG_OFFSET] = 1;

	sm_sync[1*acc_offset + FLT_VALID_FLAG_OFFSET] = 0;
	sm_sync[1*acc_offset + FLT_READY_FLAG_OFFSET] = 1;

	sm_sync[0*acc_offset + END_FLAG_OFFSET] = 0;
	sm_sync[1*acc_offset + END_FLAG_OFFSET] = 0;
	sm_sync[2*acc_offset + END_FLAG_OFFSET] = 0;

	unsigned local_cons_rdy_flag_offset = 0*acc_offset + READY_FLAG_OFFSET;
	unsigned local_cons_vld_flag_offset = 0*acc_offset + VALID_FLAG_OFFSET;
	unsigned local_flt_rdy_flag_offset = 1*acc_offset + FLT_READY_FLAG_OFFSET;
	unsigned local_flt_vld_flag_offset = 1*acc_offset + FLT_VALID_FLAG_OFFSET;
	unsigned local_prod_rdy_flag_offset = 3*acc_offset + READY_FLAG_OFFSET;
	unsigned local_prod_vld_flag_offset = 3*acc_offset + VALID_FLAG_OFFSET;

	// Invoke accelerators but do not check for end
	fft_cfg_000[0].esp.start_stop = 1;
	fir_cfg_000[0].esp.start_stop = 1;
	fft_cfg_001[0].esp.start_stop = 1;

	esp_run(cfg_000, NACC);
	esp_run(cfg_002, NACC);
	esp_run(cfg_001, NACC);

#if (IS_PIPELINE == 0)
	///////////////////////////////////////////////////////////////
	// NON PIPELINED VERSION
	///////////////////////////////////////////////////////////////
	for (i = 0; i < ITERATIONS; i++)
	{
		///////////////////////////////////////////////////////////////
		// Write inputs for accelerators
		///////////////////////////////////////////////////////////////
		start_counter();
		// Wait for accelerator 1 (consumer) to be ready
		while(sm_sync[local_cons_rdy_flag_offset] != 1);
		// Reset the same flag (consumer)
		sm_sync[local_cons_rdy_flag_offset] = 0;
		t_wait_cons += end_counter();

		start_counter();
		// Write input data for accelerator 1
		init_buffer_data(buf, gold);
		// Start accelerator 1 (consumer)
		sm_sync[local_cons_vld_flag_offset] = 1;
		t_cpu_write += end_counter();

		start_counter();
		// Wait for accelerator 2 (consumer) to be ready
		while(sm_sync[local_flt_rdy_flag_offset] != 1);
		// Reset the same flag (consumer)
		sm_sync[local_flt_rdy_flag_offset] = 0;
		t_wait_cons += end_counter();

		start_counter();
		// Write input filters for accelerator 2
		init_buffer_filters((buf + 5 * acc_offset) /* in_filter */, (int64_t*) fxp_filters /* gold_filter */);
		// Update accelerator 2 (consumer)
		sm_sync[local_flt_vld_flag_offset] = 1;
		t_cpu_write += end_counter();

		///////////////////////////////////////////////////////////////
		// Wait for accelerator operation
		///////////////////////////////////////////////////////////////
		start_counter();
		// Wait for last accelerator (producer) to send output
		while(sm_sync[local_prod_vld_flag_offset] != 1);
		// Reset the same flag (producer)
		sm_sync[local_prod_vld_flag_offset] = 0;
		t_wait_prod += end_counter();

		///////////////////////////////////////////////////////////////
		// Read back output
		///////////////////////////////////////////////////////////////
		start_counter();
		validate_buffer(&buf[NUM_DEVICES*acc_offset], (gold + out_len));
		sm_sync[local_prod_rdy_flag_offset] = 1;
		t_cpu_read += end_counter();
	}

	// Write end-of-accelerator only in the last iteration
	sm_sync[0*acc_offset + END_FLAG_OFFSET] = 1;
	sm_sync[1*acc_offset + END_FLAG_OFFSET] = 1;
	sm_sync[2*acc_offset + END_FLAG_OFFSET] = 1;

	sm_sync[local_cons_vld_flag_offset] = 1;
	sm_sync[local_flt_vld_flag_offset] = 1;
	while(sm_sync[local_prod_vld_flag_offset] != 1);
#else
	///////////////////////////////////////////////////////////////
	// PIPELINED VERSION
	// Write first N_DEVICES inputs
	///////////////////////////////////////////////////////////////
	for (i = 0; i < NUM_DEVICES; i++)
	{
		start_counter();
		// Wait for accelerator 1 (consumer) to be ready
		while(sm_sync[local_cons_rdy_flag_offset] != 1);
		// Reset the same flag (consumer)
		sm_sync[local_cons_rdy_flag_offset] = 0;
		t_wait_cons += end_counter();

		start_counter();
		// Write input data for accelerator 1
		init_buffer_data(buf, gold);
		// Start accelerator 1 (consumer)
		sm_sync[local_cons_vld_flag_offset] = 1;
		t_cpu_write += end_counter();

		start_counter();
		// Wait for accelerator 2 (consumer) to be ready
		while(sm_sync[local_flt_rdy_flag_offset] != 1);
		// Reset the same flag (consumer)
		sm_sync[local_flt_rdy_flag_offset] = 0;
		t_wait_cons += end_counter();

		start_counter();
		// Write input filters for accelerator 2
		init_buffer_filters((buf + 5 * acc_offset) /* in_filter */, (int64_t*) fxp_filters /* gold_filter */);
		// Update accelerator 2 (consumer)
		sm_sync[local_flt_vld_flag_offset] = 1;
		t_cpu_write += end_counter();
	}

	///////////////////////////////////////////////////////////////
	// Start the loop
	///////////////////////////////////////////////////////////////
	for (i = NUM_DEVICES; i < ITERATIONS; i++)
	{
		start_counter();
		// Wait for last accelerator (producer) to send output
		while(sm_sync[local_prod_vld_flag_offset] != 1);
		// Reset the same flag (producer)
		sm_sync[local_prod_vld_flag_offset] = 0;
		t_wait_prod += end_counter();

		start_counter();
		// Read back output
		validate_buffer(&buf[NUM_DEVICES*acc_offset], (gold + out_len));
		sm_sync[local_prod_rdy_flag_offset] = 1;
		t_cpu_read += end_counter();

		start_counter();
		// Wait for accelerator 1 (consumer) to be ready
		while(sm_sync[local_cons_rdy_flag_offset] != 1);
		// Reset the same flag (consumer)
		sm_sync[local_cons_rdy_flag_offset] = 0;
		t_wait_cons += end_counter();

		// Write input data for accelerator 1
		start_counter();
		init_buffer_data(buf, gold);
		// Start accelerator 1 (consumer)
		sm_sync[local_cons_vld_flag_offset] = 1;
		t_cpu_write += end_counter();

		start_counter();
		// Wait for accelerator 1 (consumer) to be ready
		while(sm_sync[local_flt_rdy_flag_offset] != 1);
		// Reset the same flag (consumer)
		sm_sync[local_flt_rdy_flag_offset] = 0;
		t_wait_cons += end_counter();

		// Write input filters for accelerator 2
		init_buffer_filters((buf + 5 * acc_offset) /* in_filter */, (int64_t*) fxp_filters /* gold_filter */);
		// Update accelerator 2 (consumer)
		sm_sync[local_flt_vld_flag_offset] = 1;
		t_cpu_write += end_counter();
	}

	///////////////////////////////////////////////////////////////
	// Read last N_DEVICES outputs
	///////////////////////////////////////////////////////////////
	for (i = 0; i < NUM_DEVICES; i++)
	{
		start_counter();
		// Wait for last accelerator (producer) to send output
		while(sm_sync[local_prod_vld_flag_offset] != 1);
		// Reset the same flag (producer)
		sm_sync[local_prod_vld_flag_offset] = 0;
		t_wait_prod += end_counter();

		start_counter();
		// Read back output
		validate_buffer(&buf[NUM_DEVICES*acc_offset], (gold + out_len));
		sm_sync[local_prod_rdy_flag_offset] = 1;
		t_cpu_read += end_counter();
	}

	// Write end-of-accelerator only in the last iteration
	sm_sync[0*acc_offset + END_FLAG_OFFSET] = 1;
	sm_sync[1*acc_offset + END_FLAG_OFFSET] = 1;
	sm_sync[2*acc_offset + END_FLAG_OFFSET] = 1;

	sm_sync[local_cons_vld_flag_offset] = 1;
	sm_sync[local_flt_vld_flag_offset] = 1;
	while(sm_sync[local_prod_vld_flag_offset] != 1);
#endif

	printf("  Wait CONS = %lu\n", t_wait_cons/ITERATIONS);
	printf("  CPU write = %lu\n", t_cpu_write/ITERATIONS);
	printf("  Wait PROD = %lu\n", t_wait_prod/ITERATIONS);
	printf("  CPU read = %lu\n", t_cpu_read/ITERATIONS);

	esp_free(gold);
	esp_free(fxp_filters);
	esp_free(buf);

	return 0;
}
