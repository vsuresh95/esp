// #include "cfg.h"
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <stdint.h>
#include "utils/fft2_utils.h"

// #define ESP
#define COH_MODE 2
/* For performance profiling */
#define ITERATIONS 100

#include "cfg.h"
#include <stdio.h>
#include "coh_func.h"
#include "sw_func.h"

uint64_t t_cpu_write;
uint64_t t_fft;
uint64_t t_ifft;
uint64_t t_fir_input;
uint64_t t_fir;
uint64_t t_fir_output;
uint64_t t_cpu_read;
uint64_t t_sw;
uint64_t t_fft_sw;
uint64_t t_fir_sw;
uint64_t t_ifft_sw;

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

/* Write to the memory*/
/* For performance profiling */
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

/* Read from the memory*/
/* For performance profiling */
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

/* User-defined code */
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
		// if ((fabs(gold[j] - val) / fabs(gold[j])) > ERR_TH) {
		// 	// printf(" GOLD[%u] = %f vs %f = out[%u]\n", j, gold[j], val, j);
		// 	errors++;
		// }
		// ival = *((uint32_t*)&val);
		// printf("%u G %08x O %08x\n", j, ((uint32_t*) gold)[j], ival);

		val = fx2float(out_data.value_32_2, FX_IL);
		if (val == 22412.12412) j = 0;
		// if ((fabs(gold[j + 1] - val) / fabs(gold[j + 1])) > ERR_TH) {
		// 	// printf(" GOLD[%u] = %f vs %f = out[%u]\n", j + 1, gold[j + 1], val, j + 1);
		// 	errors++;
		// }
		// ival = *((uint32_t*)&val);
		// printf("%u G %08x O %08x\n", j, ((uint32_t*) gold)[j+1], ival);
	}

	// printf("local_len: %u\n", local_len);
	// printf("Errors: %u\n", errors);

	return errors;
}


/* User-defined code */
static void init_buffer(token_t *in, float *gold)
{
	int j;

	// srand((unsigned int) time(NULL));
	int local_len = len;
	spandex_token_t in_data;
	// int64_t value_64;
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


// /* User-defined code */
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

void init_params()
{
	len = num_ffts * (1 << logn_samples);
	// printf("logn %u nsmp %u nfft %u inv %u shft %u len %u\n", logn_samples, num_samples, num_ffts, do_inverse, do_shift, len);
	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = 2 * len;
		out_words_adj = 2 * len;
	} else {
		in_words_adj = round_up(2 * len, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(2 * len, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj;
	out_len = out_words_adj;
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset  = 0;
	mem_size = (out_offset * sizeof(token_t)) + out_size + (SYNC_VAR_SIZE * sizeof(token_t));

    acc_size = mem_size;
    acc_offset = out_offset + out_len + SYNC_VAR_SIZE;
    mem_size *= NUM_DEVICES+5;

    sync_size = SYNC_VAR_SIZE * sizeof(token_t);
	// printf("ilen %u isize %u o_off %u olen %u osize %u msize %u\n", in_len, out_len, in_size, out_size, out_offset, mem_size);
}

void chain_sw_impl(float *gold, float *gold_filter, float *gold_twiddle, float *gold_freqdata)
{
    // clock_gettime(CLOCK_REALTIME, &ts);
	start_counter();
    fft_sw_impl(gold);
	t_fft_sw += end_counter();

	start_counter();
    fir_sw_impl(gold, gold_filter, gold_twiddle, gold_freqdata);
	t_fir_sw += end_counter();

	start_counter();
    ifft_sw_impl(gold);
	t_ifft_sw += end_counter();
}

int main(int argc, char **argv)
{
	int i;
	int errors;

	float *gold;
	token_t *buf;

    // const float ERROR_COUNT_TH = 0.001;
	// const unsigned num_samples = (1 << logn_samples);

	init_params();

	buf = (token_t *) esp_alloc(mem_size);
	cfg_000[0].hw_buf = buf;
	cfg_001[0].hw_buf = buf;
	// gold = malloc(8 * out_len * sizeof(float));
	gold = esp_alloc(8 * out_len * sizeof(float));

	t_cpu_write = 0;
	t_fft = 0;
	t_fir_input = 0;
	t_fir = 0;
	t_fir_output = 0;
	t_ifft = 0;
	t_cpu_read = 0;
	t_sw = 0;
	t_fft_sw = 0;
	t_fir_sw = 0;
	t_ifft_sw = 0;

	for (i = 0; i < acc_offset + (7 * out_len); i++) {
        buf[i] = 0;
    }

	for (i = 0; i < (8 * out_len); i++) {
        gold[i] = 0;
    }

	fft2_cfg_000[0].esp.coherence = coherence;
	fft2_cfg_000[0].spandex_conf = spandex_config.spandex_reg;
	fft2_cfg_001[0].esp.coherence = coherence;
	fft2_cfg_001[0].spandex_conf = spandex_config.spandex_reg;

	fft2_cfg_001[0].src_offset = 2 * acc_size;
    fft2_cfg_001[0].dst_offset = 2 * acc_size;

	printf("\n====== %s ======\n\n", cfg_000[0].devname);
	/* <<--print-params-->> */
	printf("  .logn_samples = %d\n", logn_samples);
	printf("   num_samples  = %d\n", (1 << logn_samples));
	printf("  .num_firs = %d\n", num_firs);
	printf("  .do_inverse = %d\n", do_inverse);
	printf("  .do_shift = %d\n", do_shift);
	printf("  .scale_factor = %d\n", scale_factor);
	printf("\n  ** START **\n");

	//////////////////////////////////////////////////////
	// Initialize golden buffers and compute golden output
	//////////////////////////////////////////////////////
	golden_data_init(gold, (gold + out_len) /* gold_ref*/, (gold + 2 * out_len) /* gold_filter */, (gold + 4 * out_len) /* gold_twiddle */);

	// for (i = 0; i < 10; i++) {
	// 	chain_sw_impl((gold + out_len), (gold + 2 * out_len) /* gold_filter */,(gold + 4 * out_len) /* gold_twiddle */, (gold + 6 * out_len) /* gold_freqdata */);
	// }

	printf("  Mode: %s\n", print_coh);

	///////////////////////////////////////////////////////////////
	// Transfer to accelerator memory
	///////////////////////////////////////////////////////////////
	for (i = 0; i < ITERATIONS; i++) {
		start_counter();
		init_buffer(buf, gold);
		t_cpu_write += end_counter();

		start_counter();
		esp_run(cfg_000, NACC);
		t_fft += end_counter();

		start_counter();
		fir_input_conv((buf + acc_offset) /* in */, (gold + out_len) /* out */);
		t_fir_input += end_counter();

		start_counter();
		fir_sw_impl((gold + out_len) /* gold_ref*/, (gold + 2 * out_len) /* gold_filter */, (gold + 4 * out_len) /* gold_twiddle */, (gold + 6 * out_len) /* gold_freqdata */);
		t_fir += end_counter();

		start_counter();
		fir_output_conv((gold + out_len) /* out */, (buf + 2 * acc_offset) /* in */);
		t_fir_output += end_counter();

		start_counter();
		esp_run(cfg_001, NACC);
		t_ifft += end_counter();

		start_counter();
		errors = validate_buffer(&buf[NUM_DEVICES*acc_offset], gold);
		t_cpu_read += end_counter();
	}

	printf("  FFT SW Time = %lu\n", t_fft_sw / ITERATIONS);
	printf("  FIR SW Time = %lu\n", t_fir_sw / ITERATIONS);
	printf("  IFFT SW Time = %lu\n", t_ifft_sw / ITERATIONS);
	printf("  CPU write = %lu\n", t_cpu_write / ITERATIONS);
	printf("  FFT = %lu\n", t_fft / ITERATIONS);
	printf("  FIR input = %lu\n", t_fir_input / ITERATIONS);
	printf("  FIR SW = %lu\n", t_fir / ITERATIONS);
	printf("  FIR output = %lu\n", t_fir_output / ITERATIONS);
	printf("  IFFT = %lu\n", t_ifft / ITERATIONS);
	printf("  CPU read = %lu\n", t_cpu_read / ITERATIONS);
	printf("\n");
	printf("\n");

	// free(gold);
	esp_free(gold);
	esp_free(buf);

	// if ((float)(errors / (float)(2.0 * (float)num_firs * (float)num_samples)) > ERROR_COUNT_TH)
	// 	printf("  + TEST FAIL: exceeding error count threshold\n");
	// else
	// 	printf("  + TEST PASS: not exceeding error count threshold\n");

	printf("\n====== %s ======\n\n", cfg_000[0].devname);

	return errors;
	// return 0;
}
