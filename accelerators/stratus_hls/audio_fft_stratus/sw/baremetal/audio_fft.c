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

#define ITERATIONS 5
#define COH_MODE 1
#define IS_ESP 1

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
const int32_t logn_samples = 6;
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
#define AUDIO_FFT_DO_SHIFT_REG_0		0x60
#define AUDIO_FFT_DO_SHIFT_REG_1		0x64
#define AUDIO_FFT_DO_SHIFT_REG_2		0x68
#define AUDIO_FFT_DO_SHIFT_REG_3		0x6C
#define AUDIO_FFT_LOGN_SAMPLES_REG_0	0x70
#define AUDIO_FFT_LOGN_SAMPLES_REG_1	0x74
#define AUDIO_FFT_LOGN_SAMPLES_REG_2	0x78
#define AUDIO_FFT_LOGN_SAMPLES_REG_3	0x7C
#define AUDIO_FFT_DO_INVERSE_REG_0		0x80
#define AUDIO_FFT_DO_INVERSE_REG_1		0x84
#define AUDIO_FFT_DO_INVERSE_REG_2		0x88
#define AUDIO_FFT_DO_INVERSE_REG_3		0x8C
#define AUDIO_FFT_INPUT_QUEUE_BASE_0	0x90
#define AUDIO_FFT_INPUT_QUEUE_BASE_1	0x94
#define AUDIO_FFT_INPUT_QUEUE_BASE_2	0x98
#define AUDIO_FFT_INPUT_QUEUE_BASE_3	0x9C
#define AUDIO_FFT_OUTPUT_QUEUE_BASE_0	0xA0
#define AUDIO_FFT_OUTPUT_QUEUE_BASE_1	0xA4
#define AUDIO_FFT_OUTPUT_QUEUE_BASE_2	0xA8
#define AUDIO_FFT_OUTPUT_QUEUE_BASE_3	0xAC
#define AUDIO_FFT_CONTEXT_QUOTA_0		0xB0
#define AUDIO_FFT_CONTEXT_QUOTA_1		0xB4
#define AUDIO_FFT_CONTEXT_QUOTA_2		0xB8
#define AUDIO_FFT_CONTEXT_QUOTA_3		0xBC
#define AUDIO_FFT_VALID_CONTEXTS		0xC0

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
	unsigned **ptable0 = NULL;
	unsigned **ptable1 = NULL;
	unsigned **ptable2 = NULL;
	token_t *mem0;
	token_t *mem1;
	token_t *mem2;
	float *gold;
	unsigned errors = 0;
    const float ERROR_COUNT_TH = 0.001;
	unsigned len = num_samples;

	// printf("logn %u nsmp %u nfft %u inv %u shft %u len %u\n", logn_samples, num_samples, 1, do_inverse, do_shift, len);
	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = (2 * len) + PAYLOAD_OFFSET;
		out_words_adj = (2 * len) + PAYLOAD_OFFSET;
	} else {
		in_words_adj = round_up((2 * len) + PAYLOAD_OFFSET, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up((2 * len) + PAYLOAD_OFFSET, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj;
	out_len = out_words_adj;
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset  = in_len;
	mem_size = 4 * ((out_offset * sizeof(token_t)) + out_size);

	// printf("ilen %u isize %u o_off %u olen %u osize %u msize %u\n", in_len, out_len, in_size, out_size, out_offset, mem_size);
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
		mem0 = aligned_malloc(mem_size);
		mem1 = aligned_malloc(mem_size);
		mem2 = aligned_malloc(mem_size);

		// Allocate and populate page table
		ptable0 = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
		for (i = 0; i < NCHUNK(mem_size); i++)
			ptable0[i] = (unsigned *) &mem0[i * (CHUNK_SIZE / sizeof(token_t))];
			
		ptable1 = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
		for (i = 0; i < NCHUNK(mem_size); i++)
			ptable1[i] = (unsigned *) &mem1[i * (CHUNK_SIZE / sizeof(token_t))];
			
		ptable2 = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
		for (i = 0; i < NCHUNK(mem_size); i++)
			ptable2[i] = (unsigned *) &mem2[i * (CHUNK_SIZE / sizeof(token_t))];
			
		// Program sync flags
		unsigned input_valid_offset_0 = 0*in_len + VALID_OFFSET;
		unsigned input_data_offset_0 = 0*in_len + PAYLOAD_OFFSET;
		unsigned output_valid_offset_0 = 1*in_len + VALID_OFFSET;
		unsigned output_data_offset_0 = 1*in_len + PAYLOAD_OFFSET;

		unsigned input_valid_offset_1 = 2*in_len + VALID_OFFSET;
		unsigned input_data_offset_1 = 2*in_len + PAYLOAD_OFFSET;
		unsigned output_valid_offset_1 = 3*in_len + VALID_OFFSET;
		unsigned output_data_offset_1 = 3*in_len + PAYLOAD_OFFSET;

		unsigned input_valid_offset_2 = 4*in_len + VALID_OFFSET;
		unsigned input_data_offset_2 = 4*in_len + PAYLOAD_OFFSET;
		unsigned output_valid_offset_2 = 5*in_len + VALID_OFFSET;
		unsigned output_data_offset_2 = 5*in_len + PAYLOAD_OFFSET;

		// Reset all sync variables to default values.
		UpdateSync((void*) &mem0[input_valid_offset_0], 0);
		UpdateSync((void*) &mem0[output_valid_offset_0], 0);
		UpdateSync((void*) &mem1[input_valid_offset_1], 0);
		UpdateSync((void*) &mem1[output_valid_offset_1], 0);
		UpdateSync((void*) &mem2[input_valid_offset_2], 0);
		UpdateSync((void*) &mem2[output_valid_offset_2], 0);

		// Initialize registers of accelerator and start it.
		iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
		iowrite32(dev, COHERENCE_REG, coherence);
		iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);

		iowrite32(dev, PT_ADDRESS_REG, (unsigned long long) ptable0);
		iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
		iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

		// Use the following if input and output data are not allocated at the default offsets
		iowrite32(dev, SRC_OFFSET_REG, 0x0);
		iowrite32(dev, DST_OFFSET_REG, 0x0);

		// Flush (customize coherence model here)
		esp_flush(coherence);

		///////////////////////////////////////////////////////
		/// Configure first context
		///////////////////////////////////////////////////////
		iowrite32(dev, AUDIO_FFT_LOGN_SAMPLES_REG_0, logn_samples);
		iowrite32(dev, AUDIO_FFT_DO_SHIFT_REG_0, do_shift);
		iowrite32(dev, AUDIO_FFT_DO_INVERSE_REG_0, do_inverse);
		iowrite32(dev, AUDIO_FFT_INPUT_QUEUE_BASE_0, input_valid_offset_0);
		iowrite32(dev, AUDIO_FFT_OUTPUT_QUEUE_BASE_0, output_valid_offset_0);
		iowrite32(dev, AUDIO_FFT_CONTEXT_QUOTA_0, 50000);
		iowrite32(dev, PT_ADDRESS_REG_0, (unsigned long long) ptable0);
		iowrite32(dev, AUDIO_FFT_VALID_CONTEXTS, 0x1);

		printf("First context configured\n");

		// Start accelerator
		iowrite32(dev, CMD_REG, CMD_MASK_START);

		///////////////////////////////////////////////////////
		/// Send first context task
		///////////////////////////////////////////////////////
		// Wait for the accelerator to be ready
		SpinSync((void*) &mem0[input_valid_offset_0], 0);
		// When the accelerator is ready, we write the input data to it
		init_buf(&mem0[input_data_offset_0], gold);
		// Inform the accelerator to start.
		UpdateSync((void*) &mem0[input_valid_offset_0], 1);

		printf("First context task sent\n");

		printf("PT_ADDRESS_REG_0 = %x\n", ioread32(dev, PT_ADDRESS_REG_0));

		for (i = 0; i < 3; i++) {
			iowrite32(dev, PT_ADDRESS_REG_0, (unsigned long long) ptable1);
			printf("PT_ADDRESS_REG_0 = %x\n", ioread32(dev, PT_ADDRESS_REG_0));
		}

		///////////////////////////////////////////////////////
		/// Configure second context
		///////////////////////////////////////////////////////
		iowrite32(dev, AUDIO_FFT_LOGN_SAMPLES_REG_1, logn_samples);
		iowrite32(dev, AUDIO_FFT_DO_SHIFT_REG_1, do_shift);
		iowrite32(dev, AUDIO_FFT_DO_INVERSE_REG_1, do_inverse);
		iowrite32(dev, AUDIO_FFT_INPUT_QUEUE_BASE_1, input_valid_offset_1);
		iowrite32(dev, AUDIO_FFT_OUTPUT_QUEUE_BASE_1, output_valid_offset_1);
		iowrite32(dev, AUDIO_FFT_CONTEXT_QUOTA_1, 50000);
		iowrite32(dev, PT_ADDRESS_REG_1, (unsigned long long) ptable1);
		iowrite32(dev, AUDIO_FFT_VALID_CONTEXTS, 0x3);

		printf("Second context configured\n");

		///////////////////////////////////////////////////////
		/// Get first context output
		///////////////////////////////////////////////////////
		// Wait for the accelerator to send output
		SpinSync((void*) &mem0[output_valid_offset_0], 1);

		// When the output is ready, we read it
		errors += validate_buf(&mem0[output_data_offset_0], gold);
		// Inform the accelerator - ready for next iteration.
		UpdateSync((void*) &mem0[output_valid_offset_0], 0);

		printf("First context task done\n");

		///////////////////////////////////////////////////////
		/// Send second context task
		///////////////////////////////////////////////////////
		// Wait for the accelerator to be ready
		SpinSync((void*) &mem1[input_valid_offset_1], 0);
		// When the accelerator is ready, we write the input data to it
		init_buf(&mem1[input_data_offset_1], gold);
		// Inform the accelerator to start.
		UpdateSync((void*) &mem1[input_valid_offset_1], 1);

		printf("Second context task sent\n");

		///////////////////////////////////////////////////////
		/// Configure third context
		///////////////////////////////////////////////////////
		iowrite32(dev, AUDIO_FFT_LOGN_SAMPLES_REG_2, logn_samples);
		iowrite32(dev, AUDIO_FFT_DO_SHIFT_REG_2, do_shift);
		iowrite32(dev, AUDIO_FFT_DO_INVERSE_REG_2, do_inverse);
		iowrite32(dev, AUDIO_FFT_INPUT_QUEUE_BASE_2, input_valid_offset_2);
		iowrite32(dev, AUDIO_FFT_OUTPUT_QUEUE_BASE_2, output_valid_offset_2);
		iowrite32(dev, AUDIO_FFT_CONTEXT_QUOTA_2, 50000);
		iowrite32(dev, PT_ADDRESS_REG_2, (unsigned long long) ptable2);
		iowrite32(dev, AUDIO_FFT_VALID_CONTEXTS, 0x7);

		printf("Third context configured\n");

		///////////////////////////////////////////////////////
		/// Send first context task
		///////////////////////////////////////////////////////
		// Wait for the accelerator to be ready
		SpinSync((void*) &mem0[input_valid_offset_0], 0);
		// When the accelerator is ready, we write the input data to it
		init_buf(&mem0[input_data_offset_0], gold);
		// Inform the accelerator to start.
		UpdateSync((void*) &mem0[input_valid_offset_0], 1);

		printf("First context task sent\n");

		///////////////////////////////////////////////////////
		/// Get second context output
		///////////////////////////////////////////////////////
		// Wait for the accelerator to send output
		SpinSync((void*) &mem1[output_valid_offset_1], 1);

		// When the output is ready, we read it
		errors += validate_buf(&mem1[output_data_offset_1], gold);
		// Inform the accelerator - ready for next iteration.
		UpdateSync((void*) &mem1[output_valid_offset_1], 0);

		printf("Second context task done\n");

		///////////////////////////////////////////////////////
		/// Get first context output
		///////////////////////////////////////////////////////
		// Wait for the accelerator to send output
		SpinSync((void*) &mem0[output_valid_offset_0], 1);

		// When the output is ready, we read it
		errors += validate_buf(&mem0[output_data_offset_0], gold);
		// Inform the accelerator - ready for next iteration.
		UpdateSync((void*) &mem0[output_valid_offset_0], 0);

		printf("First context task done\n");

		///////////////////////////////////////////////////////
		/// Send first context task
		///////////////////////////////////////////////////////
		// Wait for the accelerator to be ready
		SpinSync((void*) &mem0[input_valid_offset_0], 0);
		// When the accelerator is ready, we write the input data to it
		init_buf(&mem0[input_data_offset_0], gold);
		// Inform the accelerator to start.
		UpdateSync((void*) &mem0[input_valid_offset_0], 1);

		printf("First context task sent\n");

		///////////////////////////////////////////////////////
		/// Send second context task
		///////////////////////////////////////////////////////
		// Wait for the accelerator to be ready
		SpinSync((void*) &mem1[input_valid_offset_1], 0);
		// When the accelerator is ready, we write the input data to it
		init_buf(&mem1[input_data_offset_1], gold);
		// Inform the accelerator to start.
		UpdateSync((void*) &mem1[input_valid_offset_1], 1);

		printf("Second context task sent\n");

		///////////////////////////////////////////////////////
		/// Send third context task
		///////////////////////////////////////////////////////
		// Wait for the accelerator to be ready
		SpinSync((void*) &mem2[input_valid_offset_2], 0);
		// When the accelerator is ready, we write the input data to it
		init_buf(&mem2[input_data_offset_2], gold);
		// Inform the accelerator to start.
		UpdateSync((void*) &mem2[input_valid_offset_2], 1);

		printf("Third context task sent\n");

		///////////////////////////////////////////////////////
		/// Get all contexts output
		///////////////////////////////////////////////////////
		// Check for the accelerator to send output
		bool context_0_done = false;
		bool context_1_done = false;
		bool context_2_done = false;
		while (!(context_0_done & context_1_done & context_2_done)) {
			bool context_0_ready = TestSync((void*) &mem0[output_valid_offset_0], 1);
			bool context_1_ready = TestSync((void*) &mem1[output_valid_offset_1], 1);
			bool context_2_ready = TestSync((void*) &mem2[output_valid_offset_2], 1);

			if (context_0_ready) {
				// When the output is ready, we read it
				errors += validate_buf(&mem0[output_data_offset_0], gold);
				// Inform the accelerator - ready for next iteration.
				UpdateSync((void*) &mem0[output_valid_offset_0], 0);

				context_0_done = true;

				printf("First context task done\n");
			} else if (context_1_ready) {
				// When the output is ready, we read it
				errors += validate_buf(&mem1[output_data_offset_1], gold);
				// Inform the accelerator - ready for next iteration.
				UpdateSync((void*) &mem1[output_valid_offset_1], 0);

				context_1_done = true;

				printf("Second context task done\n");
			} else if (context_2_ready) {
				// When the output is ready, we read it
				errors += validate_buf(&mem2[output_data_offset_2], gold);
				// Inform the accelerator - ready for next iteration.
				UpdateSync((void*) &mem2[output_valid_offset_2], 0);

				context_2_done = true;

				printf("Third context task done\n");
			}
		}
		
		for (i = 0; i < 3; i++) {
			printf("MON_UTIL_REG_%d = %x\n", i, ioread32(dev, MON_UTIL_REG_0 + 0x4*i));
		}

		iowrite32(dev, CMD_REG, 0x0);
	
		// Reset all sync variables to default values.
		UpdateSync((void*) &mem0[input_valid_offset_0], 0);
		UpdateSync((void*) &mem0[output_valid_offset_0], 0);
		UpdateSync((void*) &mem1[input_valid_offset_1], 0);
		UpdateSync((void*) &mem1[output_valid_offset_1], 0);
		UpdateSync((void*) &mem2[input_valid_offset_2], 0);
		UpdateSync((void*) &mem2[output_valid_offset_2], 0);

		// Initialize registers of accelerator and start it.
		iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
		iowrite32(dev, COHERENCE_REG, coherence);
		iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);

		iowrite32(dev, PT_ADDRESS_REG, (unsigned long long) ptable0);
		iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
		iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

		// Use the following if input and output data are not allocated at the default offsets
		iowrite32(dev, SRC_OFFSET_REG, 0x0);
		iowrite32(dev, DST_OFFSET_REG, 0x0);

		// Flush (customize coherence model here)
		esp_flush(coherence);

		///////////////////////////////////////////////////////
		/// Configure first context
		///////////////////////////////////////////////////////
		iowrite32(dev, AUDIO_FFT_LOGN_SAMPLES_REG_0, logn_samples);
		iowrite32(dev, AUDIO_FFT_DO_SHIFT_REG_0, do_shift);
		iowrite32(dev, AUDIO_FFT_DO_INVERSE_REG_0, do_inverse);
		iowrite32(dev, AUDIO_FFT_INPUT_QUEUE_BASE_0, input_valid_offset_0);
		iowrite32(dev, AUDIO_FFT_OUTPUT_QUEUE_BASE_0, output_valid_offset_0);
		iowrite32(dev, AUDIO_FFT_CONTEXT_QUOTA_0, 50000);
		iowrite32(dev, PT_ADDRESS_REG_0, (unsigned long long) ptable0);
		iowrite32(dev, AUDIO_FFT_VALID_CONTEXTS, 0x1);

		printf("First context configured\n");

		// Start accelerator
		iowrite32(dev, CMD_REG, CMD_MASK_START);

		///////////////////////////////////////////////////////
		/// Configure second context
		///////////////////////////////////////////////////////
		iowrite32(dev, AUDIO_FFT_LOGN_SAMPLES_REG_1, logn_samples);
		iowrite32(dev, AUDIO_FFT_DO_SHIFT_REG_1, do_shift);
		iowrite32(dev, AUDIO_FFT_DO_INVERSE_REG_1, do_inverse);
		iowrite32(dev, AUDIO_FFT_INPUT_QUEUE_BASE_1, input_valid_offset_1);
		iowrite32(dev, AUDIO_FFT_OUTPUT_QUEUE_BASE_1, output_valid_offset_1);
		iowrite32(dev, AUDIO_FFT_CONTEXT_QUOTA_1, 50000);
		iowrite32(dev, PT_ADDRESS_REG_1, (unsigned long long) ptable1);
		iowrite32(dev, AUDIO_FFT_VALID_CONTEXTS, 0x3);

		printf("Second context configured\n");

		///////////////////////////////////////////////////////
		/// Configure third context
		///////////////////////////////////////////////////////
		iowrite32(dev, AUDIO_FFT_LOGN_SAMPLES_REG_2, logn_samples);
		iowrite32(dev, AUDIO_FFT_DO_SHIFT_REG_2, do_shift);
		iowrite32(dev, AUDIO_FFT_DO_INVERSE_REG_2, do_inverse);
		iowrite32(dev, AUDIO_FFT_INPUT_QUEUE_BASE_2, input_valid_offset_2);
		iowrite32(dev, AUDIO_FFT_OUTPUT_QUEUE_BASE_2, output_valid_offset_2);
		iowrite32(dev, AUDIO_FFT_CONTEXT_QUOTA_2, 50000);
		iowrite32(dev, PT_ADDRESS_REG_2, (unsigned long long) ptable2);
		iowrite32(dev, AUDIO_FFT_VALID_CONTEXTS, 0x7);

		printf("Third context configured\n");

		///////////////////////////////////////////////////////
		/// Send first context task
		///////////////////////////////////////////////////////
		// Wait for the accelerator to be ready
		SpinSync((void*) &mem0[input_valid_offset_0], 0);
		// When the accelerator is ready, we write the input data to it
		init_buf(&mem0[input_data_offset_0], gold);
		// Inform the accelerator to start.
		UpdateSync((void*) &mem0[input_valid_offset_0], 1);

		printf("First context task sent\n");	

		///////////////////////////////////////////////////////
		/// Send second context task
		///////////////////////////////////////////////////////
		// Wait for the accelerator to be ready
		SpinSync((void*) &mem1[input_valid_offset_1], 0);
		// When the accelerator is ready, we write the input data to it
		init_buf(&mem1[input_data_offset_1], gold);
		// Inform the accelerator to start.
		UpdateSync((void*) &mem1[input_valid_offset_1], 1);

		printf("Second context task sent\n");

		///////////////////////////////////////////////////////
		/// Send third context task
		///////////////////////////////////////////////////////
		// Wait for the accelerator to be ready
		SpinSync((void*) &mem2[input_valid_offset_2], 0);
		// When the accelerator is ready, we write the input data to it
		init_buf(&mem2[input_data_offset_2], gold);
		// Inform the accelerator to start.
		UpdateSync((void*) &mem2[input_valid_offset_2], 1);

		printf("Third context task sent\n");	

		///////////////////////////////////////////////////////
		/// Get first context output
		///////////////////////////////////////////////////////
		// Wait for the accelerator to send output
		SpinSync((void*) &mem0[output_valid_offset_0], 1);

		// When the output is ready, we read it
		errors += validate_buf(&mem0[output_data_offset_0], gold);
		// Inform the accelerator - ready for next iteration.
		UpdateSync((void*) &mem0[output_valid_offset_0], 0);

		printf("First context task done\n");	

		///////////////////////////////////////////////////////
		/// Get second context output
		///////////////////////////////////////////////////////
		// Wait for the accelerator to send output
		SpinSync((void*) &mem1[output_valid_offset_1], 1);

		// When the output is ready, we read it
		errors += validate_buf(&mem1[output_data_offset_1], gold);
		// Inform the accelerator - ready for next iteration.
		UpdateSync((void*) &mem1[output_valid_offset_1], 0);

		printf("Second context task done\n");

		///////////////////////////////////////////////////////
		/// Get second context output
		///////////////////////////////////////////////////////
		// Wait for the accelerator to send output
		SpinSync((void*) &mem2[output_valid_offset_2], 1);

		// When the output is ready, we read it
		errors += validate_buf(&mem2[output_data_offset_2], gold);
		// Inform the accelerator - ready for next iteration.
		UpdateSync((void*) &mem2[output_valid_offset_2], 0);

		printf("Third context task done\n");

		for (i = 0; i < 3; i++) {
			printf("MON_UTIL_REG_%d = %x\n", i, ioread32(dev, MON_UTIL_REG_0 + 0x4*i));
		}

		aligned_free(ptable0);
		aligned_free(ptable1);
		aligned_free(ptable2);
		aligned_free(mem0);
		aligned_free(mem1);
		aligned_free(mem2);
		aligned_free(gold);

		printf("Result: FFT Baremetal %d Total = %lu\n\n", 2 * num_samples, (t_acc_input+t_acc+t_acc_output)/ITERATIONS);

		// printf("  Software Input = %lu\n", t_sw_input/(ITERATIONS/10));
		// printf("  Software = %lu\n", t_sw/(ITERATIONS/10));
		// printf("  Software Output = %lu\n", t_sw_output/(ITERATIONS/10));
		// printf("  Accel Input = %lu\n", t_acc_input/ITERATIONS);
		// printf("  Accel = %lu\n", t_acc/ITERATIONS);
		// printf("  Accel Output = %lu\n", t_acc_output/ITERATIONS);
	}

	// while(1);
	return 0;
}