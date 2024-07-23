// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include "libesp.h"
#include "cfg.h"
#include "utils/fft2_utils.h"

const float ERR_TH = 0.05;

#include "coh_func.h"
#include "sm.h"

static uint64_t t_start = 0;
static uint64_t t_end = 0;

uint64_t t_sw_input;
uint64_t t_sw;
uint64_t t_sw_output;
uint64_t t_acc_input;
uint64_t t_acc;
uint64_t t_acc_output;

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned size;

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

void sw_run(float *gold)
{
	int j;
	unsigned len = 2 * (1 << logn_samples);

    spandex_token_t gold_data;
    void* src = (void*) gold;

	start_counter();
	for (j = 0; j < len; j+=2, src+=8) {
		gold_data.value_32_1 = j % 100;
		gold_data.value_32_2 = j % 100;
        write_mem(src, gold_data.value_64);
	}
	t_sw_input += end_counter();

	// Compute golden output
	start_counter();
	fft2_comp(gold, 1, 1 << logn_samples, logn_samples, do_inverse, do_shift);
	t_sw += end_counter();

	start_counter();
	for (j = 0; j < len; j+=2, src+=8) {
        gold_data.value_64 = read_mem(src);
	}
	t_sw_output += end_counter();
}

/* User-defined code */
int validate_buffer(token_t *mem, native_t *gold)
{
	int j;
	unsigned errors = 0;
	unsigned len = 2 * (1 << logn_samples);

    spandex_token_t out_data;
    void* src = (void*) mem;

	for (j = 0; j < len; j+=2, src+=8) {
        out_data.value_64 = read_mem_reqodata(src);
		if (fx2float(out_data.value_32_1, FX_IL) == 0x11223344) errors++;
		if (fx2float(out_data.value_32_2, FX_IL) == 0x11223344) errors++;
	}

	return errors;
}

/* User-defined code */
void init_buffer(token_t *mem, native_t *gold)
{
	int j;
	unsigned len = 2 * (1 << logn_samples);

    spandex_token_t in_data;
    void* src = (void*) mem;

	for (j = 0; j < len; j+=2, src+=8) {
		in_data.value_32_1 = float2fx((native_t) (j % 100), FX_IL);
		in_data.value_32_2 = float2fx((native_t) (j % 100), FX_IL);
        write_mem_wtfwd(src, in_data.value_64);
	}
}

/* User-defined code */
static void init_parameters()
{
	const unsigned num_samples = (1 << logn_samples);

	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = (2 * num_samples) + SYNC_VAR_SIZE;
		out_words_adj = (2 * num_samples) + SYNC_VAR_SIZE;
	} else {
		in_words_adj = round_up((2 * num_samples) + SYNC_VAR_SIZE, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up((2 * num_samples) + SYNC_VAR_SIZE, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj;
	out_len =  out_words_adj;
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset = in_len;
	size = (out_offset * sizeof(token_t)) + out_size;

	// printf("ilen %u isize %u o_off %u olen %u osize %u msize %u\n", in_len, out_len, in_size, out_size, out_offset, size);
}

int main(int argc, char **argv)
{
	int i;
	int errors = 0;

    native_t *gold;
    token_t *mem;

	t_sw_input = 0;
	t_sw = 0;
	t_sw_output = 0;
	t_acc_input = 0;
	t_acc = 0;
	t_acc_output = 0;

    audio_fft_cfg_000[0].esp.coherence = coherence;
	audio_fft_cfg_000[0].spandex_conf = spandex_config.spandex_reg;

    // printf("\n====== %s ======\n\n", cfg_000[0].devname);
	// printf("	Coherence = %s\n", CohPrintHeader);
	// printf("	ITERATIONS = %u\n", ITERATIONS);

	init_parameters();

	// Program sync flags
	unsigned local_cons_rdy_flag_offset = 0*in_len + READY_FLAG_OFFSET;
	unsigned local_cons_vld_flag_offset = 0*in_len + VALID_FLAG_OFFSET;
	unsigned local_prod_rdy_flag_offset = 1*in_len + READY_FLAG_OFFSET;
	unsigned local_prod_vld_flag_offset = 1*in_len + VALID_FLAG_OFFSET;

	audio_fft_cfg_000[0].prod_valid_offset = local_cons_vld_flag_offset;
	audio_fft_cfg_000[0].prod_ready_offset = local_cons_rdy_flag_offset;
	audio_fft_cfg_000[0].cons_valid_offset = local_prod_vld_flag_offset;
	audio_fft_cfg_000[0].cons_ready_offset = local_prod_rdy_flag_offset;
	audio_fft_cfg_000[0].input_offset = SYNC_VAR_SIZE;
	audio_fft_cfg_000[0].output_offset = in_len + SYNC_VAR_SIZE;

    // allocations
    // printf("  Allocations\n");
	const unsigned num_samples = (1 << logn_samples);
    mem = (token_t *) esp_alloc(size);
    gold = (native_t*) esp_alloc(2 * num_samples);
    cfg_000[0].hw_buf = mem;

	const unsigned sw_iterations = ((ENABLE_SM == 1) ? 1 : (ITERATIONS / ((logn_samples > 10) ? 10 : 1)));

	for (i = 0; i < ITERATIONS; i++)
	{
		sw_run(gold);
	}

#if (ENABLE_SM == 1)
	// Reset all sync variables to default values.
	UpdateSync((void*) &mem[local_cons_vld_flag_offset], 0);
	UpdateSync((void*) &mem[local_cons_rdy_flag_offset], 1);
	UpdateSync((void*) &mem[END_FLAG_OFFSET], 0);
	UpdateSync((void*) &mem[local_prod_vld_flag_offset], 0);
	UpdateSync((void*) &mem[local_prod_rdy_flag_offset], 1);

    audio_fft_cfg_000[0].esp.start_stop = 1;
	esp_run(cfg_000, NACC);

	for (i = 0; i < ITERATIONS; ++i) {
		// printf("SM Enabled\n");
		start_counter();
		// Wait for the accelerator to be ready
		SpinSync((void*) &mem[local_cons_rdy_flag_offset], 1);
		// Reset flag for the next iteration
		UpdateSync((void*) &mem[local_cons_rdy_flag_offset], 0);
		// When the accelerator is ready, we write the input data to it
		init_buffer(&mem[SYNC_VAR_SIZE], gold);

		if (i == ITERATIONS - 1) {
			UpdateSync((void*) &mem[END_FLAG_OFFSET], 1);
		}

		// Inform the accelerator to start.
		UpdateSync((void*) &mem[local_cons_vld_flag_offset], 1);
		t_acc_input += end_counter();

		start_counter();
		// Wait for the accelerator to send output.
		SpinSync((void*) &mem[local_prod_vld_flag_offset], 1);
		// Reset flag for next iteration.
		UpdateSync((void*) &mem[local_prod_vld_flag_offset], 0);
		t_acc += end_counter();

		start_counter();
		errors += validate_buffer(&mem[in_len + SYNC_VAR_SIZE], gold);
		// Inform the accelerator - ready for next iteration.
		UpdateSync((void*) &mem[local_prod_rdy_flag_offset], 1);
		t_acc_output += end_counter();
	}
#else
    for (i = 0; i < ITERATIONS; ++i) {
        start_counter();
        init_buffer(&mem[SYNC_VAR_SIZE], gold);
        t_acc_input += end_counter();

        start_counter();
        esp_run(cfg_000, NACC);
        t_acc += end_counter();

        start_counter();
		errors += validate_buffer(&mem[in_len + SYNC_VAR_SIZE], gold);
        t_acc_output += end_counter();
    }
#endif

    // free
    esp_free(mem);
    esp_free(gold);

#if (ENABLE_SM == 0)
	printf("Result: FFT SW %d = %lu\n", 2 * num_samples, (t_sw_input+t_sw+t_sw_output)/sw_iterations);
	printf("Result: FFT Linux %d Total = %lu\n\n", 2 * num_samples, (t_acc_input + t_acc + t_acc_output)/ITERATIONS);
#else
	#if (IS_ESP == 1)
		#if (COH_MODE == ESP_COHERENT_DMA)
			printf("Result: FFT ASI %d DMA CPU_Write = %lu\n", 2 * num_samples, (t_acc_input)/ITERATIONS);
			printf("Result: FFT ASI %d DMA CPU_Read = %lu\n", 2 * num_samples, (t_acc_output)/ITERATIONS);
			printf("Result: FFT ASI %d DMA Acc = %lu\n\n", 2 * num_samples, (t_acc)/ITERATIONS);
		#else
			printf("Result: FFT ASI %d MESI CPU_Write = %lu\n", 2 * num_samples, (t_acc_input)/ITERATIONS);
			printf("Result: FFT ASI %d MESI CPU_Read = %lu\n", 2 * num_samples, (t_acc_output)/ITERATIONS);
			printf("Result: FFT ASI %d MESI Acc = %lu\n", 2 * num_samples, (t_acc)/ITERATIONS);
			printf("Result: FFT ASI %d MESI Total = %lu\n\n", 2 * num_samples, (t_acc_input+t_acc+t_acc_output)/ITERATIONS);
		#endif
	#else
		#if (COH_MODE == SPX_WRITE_THROUGH_FWD)
			printf("Result: FFT ASI %d Spandex CPU_Write = %lu\n", 2 * num_samples, (t_acc_input)/ITERATIONS);
			printf("Result: FFT ASI %d Spandex CPU_Read = %lu\n", 2 * num_samples, (t_acc_output)/ITERATIONS);
			printf("Result: FFT ASI %d Spandex Acc = %lu\n\n", 2 * num_samples, (t_acc)/ITERATIONS);
		#endif
	#endif
#endif

	// printf("  Software Input = %lu\n", t_sw_input/(ITERATIONS/10));
	// printf("  Software = %lu\n", t_sw/(ITERATIONS/10));
	// printf("  Software Output = %lu\n", t_sw_output/(ITERATIONS/10));
	// printf("  Accel Input = %lu\n", t_acc_input/ITERATIONS);
	// printf("  Accel = %lu\n", t_acc/ITERATIONS);
	// printf("  Accel Output = %lu\n", t_acc_output/ITERATIONS);

    // printf("\n====== %s ======\n\n", cfg_000[0].devname);

	return errors;
}
