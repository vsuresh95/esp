/* Copyright (c) 2011-2023 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include <fixed_point.h>

typedef int token_t;
typedef float native_t;

#define fx2float fixed32_to_float
#define float2fx float_to_fixed32
#define FX_IL 8

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}

#define SLD_VECTOR 0x070
#define DEV_NAME "sld,vector_stratus"
#define REL_ERROR_THRESHOLD 0.05

#define VECTOR_OP_ADD 0
#define VECTOR_OP_AVG_POOL 1
#define VECTOR_TEST VECTOR_OP_AVG_POOL

/* <<--params-->> */
#if (VECTOR_TEST == VECTOR_OP_ADD)
const int32_t vector_op = VECTOR_OP_ADD;
const int32_t input_len = 4096;
const int32_t stride = 0; // not used for add
const int32_t n_channel = 0; // not used for add
#else
const int32_t vector_op = VECTOR_OP_AVG_POOL;
const int32_t input_len = 8;
const int32_t stride = 8;
const int32_t n_channel = 64;
#endif

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
#define VECTOR_VECTOR_OP_REG		0x98
#define VECTOR_N_CHANNEL_REG		0x9C
#define VECTOR_INPUT_LEN_REG		0xA0
#define VECTOR_STRIDE_REG			0xA4
#define VECTOR_INPUT1_OFFSET_REG 	0xA8
#define VECTOR_INPUT2_OFFSET_REG 	0xAC
#define VECTOR_OUTPUT_OFFSET_REG 	0xB0
#define VECTOR_DO_RELU_REG			0xB4

static int validate_buf(token_t *out)
{
	int i;
	unsigned errors = 0;
	native_t val;

	for (i = 0; i < out_len; i++) {
		val = fx2float(out[i], FX_IL);
		#if (VECTOR_TEST == VECTOR_OP_ADD)
		native_t expected = 2 * (0.1 + (i % 8) * 0.1);
		#else
		native_t expected = 0.45;
		#endif
		native_t rel_err = (expected - val) / expected;
		if (rel_err > REL_ERROR_THRESHOLD || rel_err < -REL_ERROR_THRESHOLD)
			errors++;
	}

	return errors;
}

static void init_buf (token_t *in)
{
	int i;
	for (i = 0; i < in_len; i++) {
		native_t val = ((i % 8) * 0.1) + 0.1;
		in[i] = float2fx(val, FX_IL);
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
	unsigned **ptable;
	token_t *mem;
	unsigned errors = 0;
	unsigned coherence;

	#if (VECTOR_TEST == VECTOR_OP_ADD)
	in_words_adj = round_up(2 * input_len, DMA_WORD_PER_BEAT(sizeof(token_t)));
	out_words_adj = round_up(input_len, DMA_WORD_PER_BEAT(sizeof(token_t)));
	#else
	in_words_adj = round_up(n_channel * input_len * input_len, DMA_WORD_PER_BEAT(sizeof(token_t)));
	out_words_adj = round_up(in_words_adj / (stride * stride), DMA_WORD_PER_BEAT(sizeof(token_t)));
	#endif
	in_len = in_words_adj * (1);
	out_len = out_words_adj * (1);
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset = in_len;
	mem_size = (out_offset * sizeof(token_t)) + out_size;

	// Search for the device
	printf("Scanning device tree... \n");

	ndev = probe(&espdevs, VENDOR_SLD, SLD_VECTOR, DEV_NAME);
	if (ndev == 0) {
		printf("vector not found\n");
		return 0;
	}

	dev = &espdevs[0];

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
	mem = aligned_malloc(mem_size);
	printf("  memory buffer base-address = %p\n", mem);

	// Alocate and populate page table
	ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (i = 0; i < NCHUNK(mem_size); i++)
		ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];

	printf("  ptable = %p\n", ptable);
	printf("  nchunk = %lu\n", NCHUNK(mem_size));

	coherence = ACC_COH_RECALL;
	printf("  Generate input...\n");
	init_buf(mem);

	// Pass common configuration parameters

	iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	iowrite32(dev, COHERENCE_REG, coherence);

	iowrite32(dev, PT_ADDRESS_REG_0, (unsigned long long) ptable);
	iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

	// Use the following if input and output data are not allocated at the default offsets
	iowrite32(dev, SRC_OFFSET_REG, 0x0);
	iowrite32(dev, DST_OFFSET_REG, 0x0);

	// Pass accelerator-specific configuration parameters
	/* <<--regs-config-->> */
	iowrite32(dev, VECTOR_VECTOR_OP_REG, vector_op);
	iowrite32(dev, VECTOR_N_CHANNEL_REG, n_channel);
	iowrite32(dev, VECTOR_INPUT_LEN_REG, input_len);
	iowrite32(dev, VECTOR_STRIDE_REG, stride);
	iowrite32(dev, VECTOR_INPUT1_OFFSET_REG, 0);
	#if (VECTOR_TEST == VECTOR_OP_ADD)
	iowrite32(dev, VECTOR_INPUT2_OFFSET_REG, input_len);
	#else
	iowrite32(dev, VECTOR_INPUT2_OFFSET_REG, 0);
	#endif
	iowrite32(dev, VECTOR_OUTPUT_OFFSET_REG, out_offset);

	// Flush (customize coherence model here)
	esp_flush(coherence);

	// Start accelerators
	printf("  Start...\n");
	iowrite32(dev, CMD_REG, CMD_MASK_START);

	// Wait for completion
	done = 0;
	while (!done) {
		done = ioread32(dev, STATUS_REG);
		done &= STATUS_MASK_DONE;
	}
	iowrite32(dev, CMD_REG, 0x0);

	printf("  validating...\n");

	/* Validation */
	errors = validate_buf(&mem[out_offset]);
	if (errors)
		printf("  ... FAIL\n");
	else
		printf("  ... PASS\n");

	aligned_free(ptable);
	aligned_free(mem);

	return 0;
}
