// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include "libesp.h"
#include "cfg.h"
#include "utils/fft2_utils.h"

const float ERR_TH = 0.05;

#define ENABLE_SM
#define SPX

#ifdef SPX
	#define COH_MODE 2
#else
	#define IS_ESP 1
	#define COH_MODE 1
#endif

#define ITERATIONS 1000

#include "coh_func.h"
#include "sm.h"

static uint64_t t_start = 0;
static uint64_t t_end = 0;

uint64_t t_sw;
uint64_t t_cpu_write;
uint64_t t_acc;
uint64_t t_cpu_read;

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

/* User-defined code */
static int validate_buffer(token_t *acc_buf, native_t *sw_buf)
{
	int i;
	unsigned errors = 0;
	const unsigned num_samples = 1<<logn_samples;

    spandex_native_t gold_data;
    spandex_token_t out_data;
    void* src;  
	void* dst;

    src = (void*) sw_buf;
    dst = (void*) acc_buf;

    for (i = 0; i < 2 * num_samples; i+= 2, src += 8, dst += 8) {
        gold_data.value_64 = read_mem_reqv(src);
        out_data.value_64 = read_mem_reqodata(dst);

        native_t val = fx2float(out_data.value_32_1, FX_IL);
        if ((fabs(gold_data.value_32_1 - val) / fabs(gold_data.value_32_1)) > ERR_TH) {
            errors++;
        }
        val = fx2float(out_data.value_32_2, FX_IL);
        if ((fabs(gold_data.value_32_2 - val) / fabs(gold_data.value_32_2)) > ERR_TH) {
            errors++;
        }
	}

	return errors;
}


/* User-defined code */
static void init_buffer(token_t *acc_buf, native_t *in_buf)
{
	int i;
    spandex_native_t gold_data;
    spandex_token_t in_data;
    void* src;  
    void* dst;
	const unsigned num_samples = (1 << logn_samples);

    src = (void*) in_buf;
    dst = (void*) acc_buf;

    for (i = 0; i < 2 * num_samples; i += 2, src += 8, dst += 8) {
        gold_data.value_64 = read_mem_reqv(src);
        in_data.value_32_1 = float2fx(gold_data.value_32_1, FX_IL);
        in_data.value_32_2 = float2fx(gold_data.value_32_2, FX_IL);
        write_mem_wtfwd(dst, in_data.value_64);
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
}

int main(int argc, char **argv)
{
	int i, j;
	int errors = 0;

    native_t *in_buf;
    token_t *acc_buf;
    native_t *sw_buf;

    t_sw = 0;
    t_cpu_write = 0;
    t_acc = 0;
    t_cpu_read = 0;

    audio_fft_cfg_000[0].esp.coherence = coherence;
	audio_fft_cfg_000[0].spandex_conf = spandex_config.spandex_reg;

    printf("\n====== %s ======\n\n", cfg_000[0].devname);
	printf("	Coherence = %s\n", CohPrintHeader);
	printf("	ITERATIONS = %u\n", ITERATIONS);

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
    printf("  Allocations\n");
    acc_buf = (token_t *) esp_alloc(size);
    cfg_000[0].hw_buf = acc_buf;

	// Initialize IN buf and SW buf
	const float LO = -2.0;
	const float HI = 2.0;
	const unsigned num_samples = (1 << logn_samples);

    in_buf = (native_t*) esp_alloc(2 * num_samples);
    sw_buf = (native_t*) esp_alloc(2 * num_samples);

	srand((unsigned int) time(NULL));

	for (j = 0; j < 2 * num_samples; j++) {
		float scaling_factor = (float) rand () / (float) RAND_MAX;
		in_buf[j] = LO + scaling_factor * (HI - LO);
		sw_buf[j] = LO + scaling_factor * (HI - LO);
	}

	// Perform the software computation
    for (i = 0; i < ITERATIONS/10; ++i) {
        start_counter();
		fft2_comp(sw_buf, 1, (1 << logn_samples), logn_samples, do_inverse, do_shift);
        t_sw += end_counter();
	}

#ifdef ENABLE_SM
	// Reset all sync variables to default values.
	UpdateSync((void*) &acc_buf[local_cons_vld_flag_offset], 0);
	UpdateSync((void*) &acc_buf[local_cons_rdy_flag_offset], 1);
	UpdateSync((void*) &acc_buf[END_FLAG_OFFSET], 0);
	UpdateSync((void*) &acc_buf[local_prod_vld_flag_offset], 0);
	UpdateSync((void*) &acc_buf[local_prod_rdy_flag_offset], 1);

    audio_fft_cfg_000[0].esp.start_stop = 1;
	esp_run(cfg_000, NACC);

	for (i = 0; i < ITERATIONS; ++i) {
		// printf("SM Enabled\n");
		start_counter();
		// Wait for the accelerator to be ready
		SpinSync((void*) &acc_buf[local_cons_rdy_flag_offset], 1);
		// Reset flag for the next iteration
		UpdateSync((void*) &acc_buf[local_cons_rdy_flag_offset], 0);
		// When the accelerator is ready, we write the input data to it
		init_buffer(&acc_buf[SYNC_VAR_SIZE], in_buf);

		if (i == ITERATIONS - 1) {
			UpdateSync((void*) &acc_buf[END_FLAG_OFFSET], 1);
		}

		// Inform the accelerator to start.
		UpdateSync((void*) &acc_buf[local_cons_vld_flag_offset], 1);
		t_cpu_write += end_counter();

		start_counter();
		// Wait for the accelerator to send output.
		SpinSync((void*) &acc_buf[local_prod_vld_flag_offset], 1);
		// Reset flag for next iteration.
		UpdateSync((void*) &acc_buf[local_prod_vld_flag_offset], 0);
		t_acc += end_counter();

		start_counter();
		errors += validate_buffer(&acc_buf[in_len + SYNC_VAR_SIZE], sw_buf);
		// Inform the accelerator - ready for next iteration.
		UpdateSync((void*) &acc_buf[local_prod_rdy_flag_offset], 1);
		t_cpu_read += end_counter();
	}
#else
    for (i = 0; i < ITERATIONS; ++i) {
        start_counter();
        init_buffer(&acc_buf[SYNC_VAR_SIZE], in_buf);
        t_cpu_write += end_counter();

        start_counter();
        esp_run(cfg_000, NACC);
        t_acc += end_counter();

        start_counter();
		errors += validate_buffer(&acc_buf[in_len + SYNC_VAR_SIZE], sw_buf);
        t_cpu_read += end_counter();
    }
#endif

    // free
    esp_free(in_buf);
    esp_free(acc_buf);
    esp_free(sw_buf);

    printf("    SW Time: %lu\n", t_sw/(ITERATIONS/10));
    printf("    CPU Write Time: %lu\n", t_cpu_write/ITERATIONS);
    printf("    FFT Time: %lu\n", t_acc/ITERATIONS);
    printf("    CPU Read Time: %lu\n", t_cpu_read/ITERATIONS);
    printf("    Errors = %u\n", errors);

    printf("\n====== %s ======\n\n", cfg_000[0].devname);

	return errors;
}
