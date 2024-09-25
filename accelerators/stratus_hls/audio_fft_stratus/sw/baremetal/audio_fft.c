/* Copyright (c) 2011-2019 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include "utils/fft2_utils.h"

typedef int token_t;
typedef float native_t;
#define fx2float fixed32_to_float
#define float2fx float_to_fixed32
#define FX_IL 14

#include "coh_func.h"
#include "sm.h"

const float ERR_TH = 0.05;

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}

#define SLD_AUDIO_FFT 0x063
#define DEV_NAME "sld,audio_fft_stratus"

/* <<--params-->> */
const int32_t logn_samples = LOG_LEN;
const int32_t num_samples = (1 << logn_samples);
const int32_t do_inverse = 0;
const int32_t do_shift = 0;

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned mem_size;

/* Size of the contiguous chunks for scatter/gather */
#define CHUNK_SHIFT 20
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK(_sz) ((_sz % CHUNK_SIZE == 0) ?		\
			(_sz / CHUNK_SIZE) :		\
			(_sz / CHUNK_SIZE) + 1)

/* User defined registers */
/* <<--regs-->> */
#define AUDIO_FFT_DO_INVERSE_REG 0x48
#define AUDIO_FFT_LOGN_SAMPLES_REG 0x44
#define AUDIO_FFT_DO_SHIFT_REG 0x40

#define AUDIO_FFT_PROD_VALID_OFFSET 0x4C
#define AUDIO_FFT_PROD_READY_OFFSET 0x50
#define AUDIO_FFT_CONS_VALID_OFFSET 0x54
#define AUDIO_FFT_CONS_READY_OFFSET 0x58
#define AUDIO_FFT_INPUT_OFFSET 0x5C
#define AUDIO_FFT_OUTPUT_OFFSET 0x60

static uint64_t t_start = 0;
static uint64_t t_end = 0;

uint64_t t_sw_input;
uint64_t t_sw;
uint64_t t_sw_output;
uint64_t t_acc_input;
uint64_t t_acc;
uint64_t t_acc_output;

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

int validate_buf(token_t *out, float *gold)
{
	int j;
	unsigned errors = 0;
	unsigned len = 2 * num_samples;

    spandex_token_t out_data;
    void* src = (void*) out;

	for (j = 0; j < len; j+=2, src+=8) {
        out_data.value_64 = read_mem_reqodata(src);
		if (fx2float(out_data.value_32_1, FX_IL) == 0x11223344) errors++;
		if (fx2float(out_data.value_32_2, FX_IL) == 0x11223344) errors++;
	}

	return errors;
}

void sw_run(float *gold)
{
	int j;
	unsigned len = 2 * num_samples;

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
	fft2_comp(gold, 1, num_samples, logn_samples, do_inverse, do_shift);
	t_sw += end_counter();

	start_counter();
	for (j = 0; j < len; j+=2, src+=8) {
        gold_data.value_64 = read_mem(src);
	}
	t_sw_output += end_counter();
}

void init_buf(token_t *in, float *gold)
{
	int j;
	unsigned len = 2 * num_samples;

    spandex_token_t in_data;
    void* src = (void*) in;

	for (j = 0; j < len; j+=2, src+=8) {
		in_data.value_32_1 = float2fx((native_t) (j % 100), FX_IL);
		in_data.value_32_2 = float2fx((native_t) (j % 100), FX_IL);
        write_mem_wtfwd(src, in_data.value_64);
	}
}

int main(int argc, char * argv[])
{
	int i;
	int n;
	int ndev;
	struct esp_device *espdevs;
	struct esp_device *dev;
	unsigned done;
	unsigned spin_ct;
	unsigned **ptable = NULL;
	token_t *mem;
	float *gold;
	unsigned errors = 0;
    const float ERROR_COUNT_TH = 0.001;
	unsigned len = num_samples;

	printf("logn %u nsmp %u nfft %u inv %u shft %u len %u\n", logn_samples, num_samples, 1, do_inverse, do_shift, len);
	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = (2 * len) + SYNC_VAR_SIZE;
		out_words_adj = (2 * len) + SYNC_VAR_SIZE;
	} else {
		in_words_adj = round_up((2 * len) + SYNC_VAR_SIZE, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up((2 * len) + SYNC_VAR_SIZE, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj;
	out_len = out_words_adj;
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset  = in_len;
	mem_size = (out_offset * sizeof(token_t)) + out_size;

	printf("ilen %u isize %u o_off %u olen %u osize %u msize %u\n", in_len, out_len, in_size, out_size, out_offset, mem_size);
	// Search for the device
	ndev = probe(&espdevs, VENDOR_SLD, SLD_AUDIO_FFT, DEV_NAME);
	if (ndev == 0) {
		printf("%s not found\n", DEV_NAME);
		return 0;
	}

	t_sw_input = 0;
	t_sw = 0;
	t_sw_output = 0;
	t_acc_input = 0;
	t_acc = 0;
	t_acc_output = 0;

	n = 0;
	{
		printf("**************** %s.%d ****************\n", DEV_NAME, n);

		dev = &espdevs[n];

		// Check DMA capabilities
		if (ioread32(dev, PT_NCHUNK_MAX_REG) == 0) {
			printf("  -> scatter-gather DMA is disabled. Abort.\n");
			return 0;
		}

		if (ioread32(dev, PT_NCHUNK_MAX_REG) < NCHUNK(mem_size)) {
			printf("  -> Not enough TLB entries available. Abort.\n");
			return 0;
		}

		// Allocate memory
		gold = aligned_malloc(out_len * sizeof(float));
		mem = aligned_malloc(mem_size);

		// Allocate and populate page table
		ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
		for (i = 0; i < NCHUNK(mem_size); i++)
			ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];
			
		// Program sync flags
		unsigned cons_rdy_flag_offset = 0*in_len + READY_FLAG_OFFSET;
		unsigned cons_vld_flag_offset = 0*in_len + VALID_FLAG_OFFSET;
		unsigned prod_rdy_flag_offset = 1*in_len + READY_FLAG_OFFSET;
		unsigned prod_vld_flag_offset = 1*in_len + VALID_FLAG_OFFSET;

		for (i = 0; i < ITERATIONS/10 + 1; i++)
		{
			sw_run(gold);
		}

#if (ENABLE_SM == 1)
		// Reset all sync variables to default values.
		UpdateSync((void*) &mem[cons_vld_flag_offset], 0);
		UpdateSync((void*) &mem[cons_rdy_flag_offset], 1);
		UpdateSync((void*) &mem[END_FLAG_OFFSET], 0);
		UpdateSync((void*) &mem[prod_vld_flag_offset], 0);
		UpdateSync((void*) &mem[prod_rdy_flag_offset], 1);

		// Initialize registers of accelerator and start it.
		iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
		iowrite32(dev, COHERENCE_REG, coherence);
		iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);

		iowrite32(dev, PT_ADDRESS_REG, (unsigned long long) ptable);
		iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
		iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

		// Use the following if input and output data are not allocated at the default offsets
		iowrite32(dev, SRC_OFFSET_REG, 0x0);
		iowrite32(dev, DST_OFFSET_REG, 0x0);

		// Pass accelerator-specific configuration parameters
		/* <<--regs-config-->> */
		iowrite32(dev, AUDIO_FFT_LOGN_SAMPLES_REG, logn_samples);
		iowrite32(dev, AUDIO_FFT_DO_SHIFT_REG, do_shift);
		iowrite32(dev, AUDIO_FFT_DO_INVERSE_REG, do_inverse);

		iowrite32(dev, AUDIO_FFT_PROD_VALID_OFFSET, cons_vld_flag_offset);
		iowrite32(dev, AUDIO_FFT_PROD_READY_OFFSET, cons_rdy_flag_offset);
		iowrite32(dev, AUDIO_FFT_CONS_VALID_OFFSET, prod_vld_flag_offset);
		iowrite32(dev, AUDIO_FFT_CONS_READY_OFFSET, prod_rdy_flag_offset);
		iowrite32(dev, AUDIO_FFT_INPUT_OFFSET, SYNC_VAR_SIZE);
		iowrite32(dev, AUDIO_FFT_OUTPUT_OFFSET, in_len + SYNC_VAR_SIZE);

		// Flush (customize coherence model here)
		esp_flush(coherence);

		// Start accelerator
		iowrite32(dev, CMD_REG, CMD_MASK_START);

		for (i = 0; i < ITERATIONS; ++i) {
			// printf("SM Enabled\n");
			start_counter();
			// Wait for the accelerator to be ready
			SpinSync((void*) &mem[cons_rdy_flag_offset], 1);
			// Reset flag for the next iteration
			UpdateSync((void*) &mem[cons_rdy_flag_offset], 0);
			// When the accelerator is ready, we write the input data to it
			init_buf(&mem[SYNC_VAR_SIZE], gold);

			if (i == ITERATIONS - 1) {
				UpdateSync((void*) &mem[END_FLAG_OFFSET], 1);
			}

			// Inform the accelerator to start.
			UpdateSync((void*) &mem[cons_vld_flag_offset], 1);
			t_acc_input += end_counter();

			start_counter();
			// Wait for the accelerator to send output.
			SpinSync((void*) &mem[prod_vld_flag_offset], 1);
			// Reset flag for next iteration.
			UpdateSync((void*) &mem[prod_vld_flag_offset], 0);
			t_acc += end_counter();

			start_counter();
			errors += validate_buf(&mem[in_len + SYNC_VAR_SIZE], gold);
			// Inform the accelerator - ready for next iteration.
			UpdateSync((void*) &mem[prod_rdy_flag_offset], 1);
			t_acc_output += end_counter();
		}

		// Wait for completion
		done = 0;
		spin_ct = 0;
		while (!done) {
			done = ioread32(dev, STATUS_REG);
			done &= STATUS_MASK_DONE;
			spin_ct++;
		}
		iowrite32(dev, CMD_REG, 0x0);
#else
		for (i = 0; i < ITERATIONS; ++i) {
			/* TODO: Restore full test once ESP caches are integrated */
			coherence = ACC_COH_FULL;

			// Pass common configuration parameters
			iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
			iowrite32(dev, COHERENCE_REG, coherence);

			iowrite32(dev, PT_ADDRESS_REG, (unsigned long long) ptable);
			iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
			iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

			// Use the following if input and output data are not allocated at the default offsets
			iowrite32(dev, SRC_OFFSET_REG, 0x0);
			iowrite32(dev, DST_OFFSET_REG, 0x0);

			// Pass accelerator-specific configuration parameters
			/* <<--regs-config-->> */
			iowrite32(dev, AUDIO_FFT_LOGN_SAMPLES_REG, logn_samples);
			iowrite32(dev, AUDIO_FFT_DO_SHIFT_REG, do_shift);
			iowrite32(dev, AUDIO_FFT_DO_INVERSE_REG, do_inverse);

			iowrite32(dev, AUDIO_FFT_PROD_VALID_OFFSET, cons_vld_flag_offset);
			iowrite32(dev, AUDIO_FFT_PROD_READY_OFFSET, cons_rdy_flag_offset);
			iowrite32(dev, AUDIO_FFT_CONS_VALID_OFFSET, prod_vld_flag_offset);
			iowrite32(dev, AUDIO_FFT_CONS_READY_OFFSET, prod_rdy_flag_offset);
			iowrite32(dev, AUDIO_FFT_INPUT_OFFSET, SYNC_VAR_SIZE);
			iowrite32(dev, AUDIO_FFT_OUTPUT_OFFSET, in_len + SYNC_VAR_SIZE);

			// printf("SM Enabled\n");
			start_counter();
			// When the accelerator is ready, we write the input data to it
			init_buf(&mem[SYNC_VAR_SIZE], gold);
			t_acc_input += end_counter();

			// Flush (customize coherence model here)
			esp_flush(coherence);

			// Start accelerators
			start_counter();
			iowrite32(dev, CMD_REG, CMD_MASK_START);

			// Wait for completion
			done = 0;
			spin_ct = 0;
			while (!done) {
				done = ioread32(dev, STATUS_REG);
				done &= STATUS_MASK_DONE;
				spin_ct++;
			}
			iowrite32(dev, CMD_REG, 0x0);
			t_acc += end_counter();

			start_counter();
			errors += validate_buf(&mem[in_len + SYNC_VAR_SIZE], gold);
			t_acc_output += end_counter();
		}
#endif		

		aligned_free(ptable);
		aligned_free(mem);
		aligned_free(gold);

		printf("  Software Input = %lu\n", t_sw_input/(ITERATIONS/10 + 1));
		printf("  Software = %lu\n", t_sw/(ITERATIONS/10 + 1));
		printf("  Software Output = %lu\n", t_sw_output/(ITERATIONS/10 + 1));
		printf("  Accel Input = %lu\n", t_acc_input/ITERATIONS);
		printf("  Accel = %lu\n", t_acc/ITERATIONS);
		printf("  Accel Output = %lu\n", t_acc_output/ITERATIONS);
	}

	return 0;
}
