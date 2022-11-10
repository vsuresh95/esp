// #include "cfg.h"
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <stdint.h>
#include "utils/fft2_utils.h"

#define COH_MODE 2
// #define ESP
#define ITERATIONS 100

#include "coh_func.h"
#include "sw_func.h"

uint64_t t_cpu_write;
uint64_t t_fft;
uint64_t t_fir;
uint64_t t_ifft;
uint64_t t_cpu_read;
uint64_t t_sw;

// //sw counters
// static uint64_t t_fft_sw_start = 0;
// static uint64_t t_ifft_sw_start = 0;
// static uint64_t t_fir_sw_start = 0;

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

/* User-defined code */
static int validate_buffer(token_t *out, float *gold)
{
	int j;
	unsigned errors = 0;
	// const unsigned num_samples = 1<<logn_samples;
	int local_len = len;

	/* Add the following 3 lines */
	/* For performance profiling */
	spandex_token_t out_data;
	void* dst;
	native_t val;

	// YJ: comment out the following line
	dst = (void*)(out+SYNC_VAR_SIZE);

	/* Change the conditions for the for-loop */
	/* For performance profiling */
	for (j = 0; j < 2 * local_len; j += 2, dst += 8) {
		/* Add the following 5 lines */
		/* For performance profiling */
		out_data.value_64 = read_mem(dst);
		val = fx2float(out_data.value_32_1, FX_IL);
		// if ((fabs(gold[j] - val) / fabs(gold[j])) > ERR_TH) {
		// 	// printf(" GOLD[%u] = %f vs %f = out[%u]\n", j, gold[j], val, j);
		// 	errors++;
		// }
		if (val == 12412.12412) j = 0;
		val = fx2float(out_data.value_32_2, FX_IL);
		// if ((fabs(gold[j + 1] - val) / fabs(gold[j + 1])) > ERR_TH) {
		// 	// printf(" GOLD[%u] = %f vs %f = out[%u]\n", j + 1, gold[j + 1], val, j + 1);
		// 	errors++;
		// }
		if (val == 22412.12412) j = 0;
	}

	return errors;
}


/* User-defined code */
static void init_buffer(token_t *in, float *gold, token_t *in_filter, int64_t *gold_filter)
{
	int j;
	int local_len = len;

	// srand((unsigned int) time(NULL));

	// convert input to fixed point
	// for (j = 0; j < 2 * len; j++) {
	// 	in[j+SYNC_VAR_SIZE] = float2fx((native_t) gold[j], FX_IL);
	// }

	// // convert input to fixed point
	// for (j = 0; j < 2 * (len+1); j++) {
	// 	in_filter[j] = float2fx((native_t) gold_filter[j], FX_IL);
	// }

	/* Add the following 3 lines */
	/* For performance profiling */
	spandex_token_t in_data;
	int64_t value_64;
	void* dst;

	// convert input to fixed point
	/* Add the following 1 line */
	/* For performance profiling */
	// YJ: comment out the following line
	dst = (void*)(in+SYNC_VAR_SIZE);
	/* Change the conditions for the for-loop */
	/* For performance profiling */
	for (j = 0; j < 2 * local_len; j += 2, dst += 8) {
	// for (j = 0; j < 2 * len; j += 2) {
		// in[j+SYNC_VAR_SIZE] = float2fx((native_t) gold[j], FX_IL);

		/* Add the following 3 lines */
		/* For performance profiling */
		in_data.value_32_1 = float2fx((native_t) gold[j], FX_IL);
		in_data.value_32_2 = float2fx((native_t) gold[j+1], FX_IL);
		write_mem(dst, in_data.value_64);
	}

	// convert input to fixed point
	/* Add the following 1 line */
	/* For performance profiling */
	// YJ: comment out the following line
	dst = (void*)(in_filter);
	/* Change the conditions for the for-loop */
	/* For performance profiling */
	for (j = 0; j < 2 * (local_len+1); ++j, dst += 8) {
	// for (j = 0; j < 2 * (len+1); ++j) {
		// in_filter[j] = float2fx((native_t) gold_filter[j], FX_IL);

		/* Add the following 2 lines */
		/* For performance profiling */
		value_64 = gold_filter[j];
		write_mem(dst, value_64);
	}
}

/* User-defined code */
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
	int errors;
    int j;

	t_cpu_write = 0;
	t_fft = 0;
	t_fir = 0;
	t_ifft = 0;
	t_cpu_read = 0;
	t_sw = 0;

	float *gold;
	token_t *buf;
	token_t *fxp_filters;

    // const float ERROR_COUNT_TH = 0.001;
	// const unsigned num_samples = (1 << logn_samples);

	init_params();

	buf = (token_t *) esp_alloc(mem_size);
	cfg_000[0].hw_buf = buf;
	cfg_001[0].hw_buf = buf;
	cfg_002[0].hw_buf = buf;
	gold = esp_alloc((8 * out_len) * sizeof(float));
	fxp_filters = esp_alloc((out_len + 2) * sizeof(token_t));

	// for (j = 0; j < acc_offset + (7 * out_len); j++) {
    //     buf[j] = 0;
    // }

	// for (j = 0; j < (8 * out_len); j++) {
    //     gold[j] = 0;
    // }

	printf("   buf = %p\n", buf);
	printf("   gold = %p\n", gold);
	printf("   fxp_filters = %p\n", fxp_filters);

    fft2_cfg_000[0].esp.coherence = coherence;
	fft2_cfg_000[0].spandex_conf = spandex_config.spandex_reg;
    
	fir_cfg_000[0].esp.coherence = coherence;
    fir_cfg_000[0].src_offset = acc_size;
    fir_cfg_000[0].dst_offset = acc_size;
	fir_cfg_000[0].spandex_conf = spandex_config.spandex_reg;

    fft2_cfg_001[0].esp.coherence = coherence;
	fft2_cfg_001[0].src_offset = 2 * acc_size;
    fft2_cfg_001[0].dst_offset = 2 * acc_size;
	fft2_cfg_001[0].spandex_conf = spandex_config.spandex_reg;

    // volatile token_t* sm_sync = (volatile token_t*) buf;

	printf("\n====== %s ======\n\n", cfg_000[0].devname);
	/* <<--print-params-->> */
	printf("  .logn_samples = %d\n", logn_samples);
	printf("   num_samples  = %d\n", (1 << logn_samples));
	printf("  .num_ffts = %d\n", num_ffts);
	printf("  .do_inverse = %d\n", do_inverse);
	printf("  .do_shift = %d\n", do_shift);
	printf("  .scale_factor = %d\n", scale_factor);
	printf("\n  ** START **\n");

	//////////////////////////////////////////////////////
	// Initialize golden buffers and compute golden output
	//////////////////////////////////////////////////////
	golden_data_init(gold, (gold + out_len) /* gold_ref*/, (gold + 2 * out_len) /* gold_filter */, (gold + 4 * out_len) /* gold_twiddle */);
	
	// for (j = 0; j < ITERATIONS; ++j) {
	// 	// Compute FFT output
	// 	start_counter();
	// 	fft2_comp((gold + out_len), num_ffts, num_samples, logn_samples, 0 /* do_inverse */, do_shift);
	// 	t_fft_sw_start += end_counter();

	// 	start_counter();
	// 	// Compute FIR output
	// 	fir_sw_impl((gold + out_len) /* gold_ref*/, (gold + 2 * out_len) /* gold_filter */, (gold + 4 * out_len) /* gold_twiddle */, (gold + 6 * out_len) /* gold_freqdata */);
	// 	t_fir_sw_start += end_counter();

	// 	start_counter();
	// 	// Compute IFFT output
	// 	fft2_comp((gold + out_len), num_ffts, num_samples, logn_samples, 1 /* do_inverse */, do_shift);
	// 	t_ifft_sw_start += end_counter();
	// }

	printf("  Mode: %s\n", print_coh);

	///////////////////////////////////////////////////////////////
	// Do repetitive things initially
	///////////////////////////////////////////////////////////////
	flt_twd_fxp_conv(fxp_filters /* gold_filter_fxp */, (gold + 2 * out_len) /* gold_filter */, (buf + 7 * acc_offset) /* in_twiddle */, (gold + 4 * out_len) /* gold_twiddle */);

	///////////////////////////////////////////////////////////////
	// Transfer to accelerator memory
	///////////////////////////////////////////////////////////////
	for (j = 0; j < ITERATIONS; ++j) {
		start_counter();
		init_buffer(buf, gold, (buf + 5 * acc_offset) /* in_filter */, (int64_t*) fxp_filters /* gold_filter */);
		t_cpu_write += end_counter();

		start_counter();
		esp_run(cfg_000, NACC);
		t_fft += end_counter();

		start_counter();
		esp_run(cfg_001, NACC);
		t_fir += end_counter();

		start_counter();
		esp_run(cfg_002, NACC);
		t_ifft += end_counter();

		start_counter();
		errors = validate_buffer(&buf[NUM_DEVICES*acc_offset], gold);
		t_cpu_read += end_counter();
	}

	esp_free(gold);
	esp_free(buf);
	esp_free(fxp_filters);

	// printf("  SW Time = %lu\n", t_sw / ITERATIONS);
	printf("  CPU write = %lu\n", t_cpu_write/ITERATIONS);
	printf("  FFT = %lu\n", t_fft/ITERATIONS);
	printf("  FIR = %lu\n", t_fir/ITERATIONS);
	printf("  IFFT = %lu\n", t_ifft/ITERATIONS);
	printf("  CPU read = %lu\n", t_cpu_read/ITERATIONS);

    // if ((float)(errors / (float)(2.0 * (float)num_ffts * (float)num_samples)) > ERROR_COUNT_TH)
	//     printf("  + TEST FAIL: exceeding error count threshold\n");
    // else
	//     printf("  + TEST PASS: not exceeding error count threshold\n");

	printf("\n====== %s ======\n\n", cfg_000[0].devname);

	return errors;
}
