/* Copyright (c) 2011-2019 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include "utils/fir_utils.h"

#if (FIR_FX_WIDTH == 64)
typedef long long token_t;
typedef double native_t;
#define fx2float fixed64_to_double
#define float2fx double_to_fixed64
#define FX_IL 42
#else // (FIR_FX_WIDTH == 32)
typedef int token_t;
typedef float native_t;
#define fx2float fixed32_to_float
#define float2fx float_to_fixed32
#define FX_IL 14
#endif /* FIR_FX_WIDTH */

const float ERR_TH = 0.05;

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}


#define SLD_FIR 0x057
#define DEV_NAME "sld,fir_stratus"

/* <<--params-->> */
const int32_t logn_samples = 10;
const int32_t num_samples = (1 << logn_samples);
const int32_t num_ffts = 1;
const int32_t do_inverse = 0;
const int32_t do_shift = 0;
const int32_t scale_factor = 1;
int32_t len;

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
#define FIR_LOGN_SAMPLES_REG 0x40
#define FIR_NUM_FFTS_REG 0x44
#define FIR_DO_INVERSE_REG 0x48
#define FIR_DO_SHIFT_REG 0x4c
#define FIR_SCALE_FACTOR_REG 0x50

#define SYNC_VAR_SIZE 2

#define NUM_DEVICES 2

static int validate_buf(token_t *out, float *gold)
{
	int j;
	unsigned errors = 0;

	for (j = 0; j < 2 * len; j++) {
		native_t val = fx2float(out[j+SYNC_VAR_SIZE], FX_IL);
		uint32_t ival = *((uint32_t*)&val);
		// printf("  GOLD[%u] = 0x%08x  :  OUT[%u] = 0x%08x\n", j, ((uint32_t*)gold)[j], j, ival);
		if ((fabs(gold[j] - val) / fabs(gold[j])) > ERR_TH)
			errors++;
	}

	//printf("  %u errors\n", errors);
	return errors;
}


static void init_buf(token_t *in, float *gold)
{
	int j;
	const float LO = -10.0;
	const float HI = 10.0;

	/* srand((unsigned int) time(NULL)); */

	for (j = 0; j < 2 * len; j++) {
		float scaling_factor = (float) rand () / (float) RAND_MAX;
		gold[j] = LO + scaling_factor * (HI - LO);
		uint32_t ig = ((uint32_t*)gold)[j];
		// printf("  IN[%u] = 0x%08x\n", j, ig);
	}

	// convert input to fixed point
	for (j = 0; j < 2 * len; j++)
		in[j+SYNC_VAR_SIZE] = float2fx((native_t) gold[j], FX_IL);

	// Compute golden output
	fir_comp(gold, num_ffts, num_samples, logn_samples, 0 /* do_inverse */, do_shift);

	// for (j = 0; j < 2 * len; j++) {
	// 	printf("  INT GOLD[%u] = 0x%08x\n", j, ((uint32_t*)gold)[j]);
    // }

	fir_comp(gold, num_ffts, num_samples, logn_samples, 1 /* do_inverse */, do_shift);
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
	unsigned coherence;
        const float ERROR_COUNT_TH = 0.001;

	len = num_ffts * (1 << logn_samples);
	printf("logn %u nsmp %u nfft %u inv %u shft %u len %u\n", logn_samples, num_samples, num_ffts, do_inverse, do_shift, len);
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

    unsigned acc_offset = mem_size;
    mem_size *= NUM_DEVICES+1;

	printf("ilen %u isize %u o_off %u olen %u osize %u msize %u\n", in_len, out_len, in_size, out_size, out_offset, mem_size);

	// Allocate memory
	gold = aligned_malloc(out_len * sizeof(float));
	mem = aligned_malloc(mem_size);
	printf("  memory buffer base-address = %p\n", mem);

    volatile token_t* sm_sync = (volatile token_t*) mem;

	// Allocate and populate page table
	ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (i = 0; i < NCHUNK(mem_size); i++)
		ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];

	printf("  ptable = %p\n", ptable);
	printf("  nchunk = %lu\n", NCHUNK(mem_size));

	coherence = ACC_COH_FULL;
	printf("  --------------------\n");
	printf("  Generate input...\n");
	init_buf(mem, gold);

	printf("Scanning device tree... \n");

	ndev = probe(&espdevs, VENDOR_SLD, SLD_FIR, DEV_NAME);
	if (ndev == 0) {
		printf("%s not found\n", DEV_NAME);
		return 0;
	}

	for (n = 0; n < ndev; n++) {

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

		// Pass common configuration parameters
		iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
		iowrite32(dev, COHERENCE_REG, coherence);

		iowrite32(dev, PT_ADDRESS_REG, (unsigned long long) ptable);
		iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
		iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

		// Use the following if input and output data are not allocated at the default offsets
		iowrite32(dev, SRC_OFFSET_REG, n * acc_offset);
		iowrite32(dev, DST_OFFSET_REG, n * acc_offset);

		// Pass accelerator-specific configuration parameters
		/* <<--regs-config-->> */
		iowrite32(dev, FIR_LOGN_SAMPLES_REG, logn_samples);
		iowrite32(dev, FIR_NUM_FFTS_REG, num_ffts);
		iowrite32(dev, FIR_SCALE_FACTOR_REG, scale_factor);
		iowrite32(dev, FIR_DO_SHIFT_REG, do_shift);
		iowrite32(dev, FIR_DO_INVERSE_REG, n);

		// Flush (customize coherence model here)
		esp_flush(coherence);

		// Start accelerators
		printf("  Start...\n");
		iowrite32(dev, CMD_REG, CMD_MASK_START);
    }

    unsigned cpu_arr_offset = out_offset + out_len + SYNC_VAR_SIZE;

	sm_sync[0] = 1;
	while(sm_sync[2*cpu_arr_offset] != 1);

	for (n = 0; n < ndev; n++) {

		printf("**************** %s.%d ****************\n", DEV_NAME, n);

		dev = &espdevs[n];

	    // Wait for completion
	    done = 0;
	    spin_ct = 0;
	    while (!done) {
	        done = ioread32(dev, STATUS_REG);
	        done &= STATUS_MASK_DONE;
	        spin_ct++;
	    }
	    iowrite32(dev, CMD_REG, 0x0);

        printf("  Done : spin_count = %u\n", spin_ct);
    }

	printf("  validating...\n");

	/* Validation */
	errors = validate_buf(&mem[(2*cpu_arr_offset)], gold);
	if ((float)((float)errors / (2.0 * (float)len)) > ERROR_COUNT_TH)
		printf("  ... FAIL : %u errors out of %u\n", errors, 2*len);
	else
		printf("  ... PASS : %u errors out of %u\n", errors, 2*len);

	aligned_free(ptable);
	aligned_free(mem);
	aligned_free(gold);

    // while(1);

	return 0;
}
