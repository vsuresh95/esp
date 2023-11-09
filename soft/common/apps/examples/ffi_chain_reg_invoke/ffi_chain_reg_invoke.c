/* Copyright (c) 2011-2019 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __riscv
#include <stdlib.h>
#endif

#include <stdio.h>

#include <esp_accelerator.h>
#include <stdint.h>

#include "ffi_chain_reg_invoke_cfg.h"
#include "utils/fft2_utils.h"
#include "utils/ffi_chain_utils.h"

uint64_t t_cpu_write;
uint64_t t_fft;
uint64_t t_fir;
uint64_t t_ifft;
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

#if (SW_FIR == 0)
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
#endif

#if (SW_FIR == 1)
static void fir_input_conv(token_t *in, float *out)
{
	int j;
	int local_len = len;
	spandex_token_t in_data;
	void* dst;

	dst = (void*)(in+SYNC_VAR_SIZE);

	for (j = 0; j < 2 * local_len; j+=2, dst+=8) {
		in_data.value_64 = read_mem(dst);

		out[j] = (float) fx2float(in_data.value_32_1, FX_IL);
		out[j+1] = (float) fx2float(in_data.value_32_2, FX_IL);
	}
}

static void fir_output_conv(float *in, token_t *out)
{
	int j;
	int local_len = len;
	spandex_token_t in_data;
	void* dst;

	dst = (void*)(out+SYNC_VAR_SIZE);

	for (j = 0; j < 2 * local_len; j+=2, dst+=8) {
		in_data.value_32_1 = float2fx((native_t) in[j], FX_IL);
		in_data.value_32_2 = float2fx((native_t) in[j+1], FX_IL);

		write_mem(dst, in_data.value_64);
	}
}
#else
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
#endif

int main(int argc, char **argv)
{
	int i;
	int errors;

	t_cpu_write = 0;
	t_fft = 0;
	t_fir = 0;
	t_ifft = 0;
	t_cpu_read = 0;

	///////////////////////////////////////////////////////////////
	// Init data size parameters
	///////////////////////////////////////////////////////////////
	init_params();

	///////////////////////////////////////////////////////////////
	// Allocate memory pointers
	///////////////////////////////////////////////////////////////
	float *gold;
	token_t *buf;
	buf = (token_t *) esp_alloc(mem_size);
	gold = esp_alloc(8 * out_len * sizeof(float));

#if (SW_FIR == 0)
	token_t *fxp_filters;
	fxp_filters = (token_t *) esp_alloc((out_len + 2) * sizeof(token_t));
#endif

	///////////////////////////////////////////////////////////////
	// Update accelerator configs
	///////////////////////////////////////////////////////////////
	cfg_000[0].hw_buf = buf;
	cfg_001[0].hw_buf = buf;
	cfg_002[0].hw_buf = buf;

	fft_cfg_000[0].esp.coherence = coherence;

	fir_cfg_000[0].src_offset = acc_size;
	fir_cfg_000[0].dst_offset = acc_size;
	fir_cfg_000[0].esp.coherence = coherence;

	fft_cfg_001[0].src_offset = 2 * acc_size;
	fft_cfg_001[0].dst_offset = 2 * acc_size;
	fft_cfg_001[0].esp.coherence = coherence;

	//////////////////////////////////////////////////////
	// Initialize golden buffers and compute golden output
	//////////////////////////////////////////////////////
	golden_data_init(gold, (gold + out_len) /* gold_ref*/, (gold + 2 * out_len) /* gold_filter */, (gold + 4 * out_len) /* gold_twiddle */);
	chain_sw_impl((gold + out_len), (gold + 2 * out_len) /* gold_filter */,(gold + 4 * out_len) /* gold_twiddle */, (gold + 6 * out_len) /* gold_freqdata */);

	printf("  Mode: %s\n", print_coh);

#if (SW_FIR == 0)
	///////////////////////////////////////////////////////////////
	// Do repetitive things initially
	///////////////////////////////////////////////////////////////
	flt_twd_fxp_conv(fxp_filters /* gold_filter_fxp */, (gold + 2 * out_len) /* gold_filter */, (buf + 7 * acc_offset) /* in_twiddle */, (gold + 4 * out_len) /* gold_twiddle */);
#endif

	for (i = 0; i < ITERATIONS; i++) {
		///////////////////////////////////////////////////////////////
		// Write inputs for accelerators
		///////////////////////////////////////////////////////////////
		start_counter();
		init_buffer_data(buf, gold);
#if (SW_FIR == 0)
		init_buffer_filters((buf + 5 * acc_offset) /* in_filter */, (int64_t*) fxp_filters /* gold_filter */);
#endif
		t_cpu_write += end_counter();

		///////////////////////////////////////////////////////////////
		// HW FFT
		///////////////////////////////////////////////////////////////
		start_counter();
		esp_run(cfg_000, NACC);
		t_fft += end_counter();

		///////////////////////////////////////////////////////////////
		// HW/SW FIR
		///////////////////////////////////////////////////////////////
		start_counter();
#if (SW_FIR == 0)
		esp_run(cfg_002, NACC);
#else
		fir_input_conv((buf + acc_offset) /* in */, (gold + out_len) /* out */);

		fir_sw_impl((gold + out_len) /* gold_ref*/,
					(gold + 2 * out_len) /* gold_filter */,
					(gold + 4 * out_len) /* gold_twiddle */,
					(gold + 6 * out_len) /* gold_freqdata */);

		fir_output_conv((gold + out_len) /* out */, (buf + 2 * acc_offset) /* in */);
#endif

		t_fir += end_counter();

		///////////////////////////////////////////////////////////////
		// HW IFFT
		///////////////////////////////////////////////////////////////
		start_counter();
		esp_run(cfg_001, NACC);
		t_ifft += end_counter();

		///////////////////////////////////////////////////////////////
		// Read back output
		///////////////////////////////////////////////////////////////
		start_counter();
		errors = validate_buffer(&buf[NUM_DEVICES*acc_offset], (gold + out_len));
		t_cpu_read += end_counter();
	}

	printf("  CPU write = %lu\n", t_cpu_write / ITERATIONS);
	printf("  FFT = %lu\n", t_fft / ITERATIONS);
	printf("  FIR = %lu\n", t_fir / ITERATIONS);
	printf("  IFFT = %lu\n", t_ifft / ITERATIONS);
	printf("  CPU read = %lu\n", t_cpu_read / ITERATIONS);

	esp_free(gold);
	esp_free(buf);

	return errors;
}
