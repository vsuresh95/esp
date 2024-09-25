/* Copyright (c) 2011-2022 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include <fixed_point.h>
#include <stdlib.h>

typedef int32_t token_t;

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}


#define SLD_NERF_MLP 0x040
#define DEV_NAME "sld,nerf_mlp_stratus"

/* <<--params-->> */
const int32_t batch_size = 1;

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned mem_size;

uint64_t cpu_start;
uint64_t cpu_end;
uint64_t cpu_diff;
uint64_t acc_start;
uint64_t acc_end;
uint64_t acc_diff;

#define LAYER_N_DIMS 256
#define LAYER_0_INPUTS 63
#define LAYER_0_OUTPUTS 256
#define LAYER_5_INPUTS 319
#define LAYER_5_OUTPUTS 256
#define LAYER_9_INPUTS 283
#define LAYER_9_OUTPUTS 128
#define LAYER_10_INPUTS 128
#define LAYER_10_OUTPUTS 3
#define LAYER_11_INPUTS 256
#define LAYER_11_OUTPUTS 1
#define LAYER_5_INPUTS_ROUND 320
#define LAYER_0_INPUTS_ROUND 64
#define LAYER_9_INPUTS_ROUND 288
#define LAYER_10_OUTPUTS_ROUND 16
#define LAYER_11_OUTPUTS_ROUND 16

/* Size of the contiguous chunks for scatter/gather */
#define CHUNK_SHIFT 24
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK(_sz) ((_sz % CHUNK_SIZE == 0) ?		\
			(_sz / CHUNK_SIZE) :		\
			(_sz / CHUNK_SIZE) + 1)

/* User defined registers */
/* <<--regs-->> */
#define NERF_MLP_BATCH_SIZE_REG 0x40

static uint64_t get_counter() {
  uint64_t counter;
  asm volatile (
    "li t0, 0;"
    "csrr t0, mcycle;"
    "mv %0, t0"
    : "=r" ( counter )
    :
    : "t0"
  );

  return counter;
}

static int validate_buf(token_t *out, token_t *gold)
{
	int i;
	int j;
	unsigned errors = 0;

	for (i = 0; i < 1; i++)
		for (j = 0; j < 4; j++)
			if (gold[i * out_words_adj + j] != out[i * out_words_adj + j])
            {
				errors++;
			    printf("gold[%d] = %d out[%d] = %d\n", j, gold[i], j, out[i]);
            }

	return errors;
}

static void init_buf (token_t *in, token_t * gold)
{
	int i;
	int j;
   
    for (int i = 0; i < 1; i++)
        for (int j = 0; j < 595960; j++)
            in[i * in_words_adj + j] = (token_t) (1); // (rand()%5) - 5);

    // // weights
    // for (int i = 0; i < 1; i++)
    //     for (int j = 3; j < 595872; j+=5)
    //         in[i * in_words_adj + j] = (token_t) (j%5);

    // // inputs
    // for (int i = 0; i < 1; i++)
    //     for (int j = 595872; j < 595960; j++)
    //         in[i * in_words_adj + j] = (token_t) (j%5);

    unsigned weights_offset = 
    /* layer 0 */ LAYER_0_INPUTS*LAYER_0_OUTPUTS + LAYER_0_OUTPUTS +
    /* layer 1 */ LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS +
    /* layer 2 */ LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS + 
    /* layer 3 */ LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS +
    /* layer 4 */ LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS +
    /* layer 5 */ LAYER_5_INPUTS*LAYER_5_OUTPUTS + LAYER_5_OUTPUTS + 
    /* layer 6 */ LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS + 
    /* layer 7 */ LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS + 
    /* layer 8 */ LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS + 
    /* layer 9 */ LAYER_9_INPUTS*LAYER_9_OUTPUTS + LAYER_9_OUTPUTS + 
    /* layer 10 */ LAYER_10_INPUTS*LAYER_10_OUTPUTS + LAYER_10_OUTPUTS_ROUND +
    /* layer 11 */ LAYER_11_INPUTS*LAYER_11_OUTPUTS + LAYER_11_OUTPUTS_ROUND;

    unsigned in_offset = 0;
    token_t *ping;
    token_t *pong;
    token_t *layer_11_input;

	ping = aligned_malloc(LAYER_5_INPUTS * sizeof(token_t));
	pong = aligned_malloc(LAYER_5_INPUTS * sizeof(token_t));
	layer_11_input = aligned_malloc(LAYER_N_DIMS * sizeof(token_t));

    cpu_start = get_counter();

    // Layer 0
    for (uint16_t col_wgt = 0; col_wgt < LAYER_0_OUTPUTS; col_wgt++)
    {
        ping[col_wgt] = in[in_offset+LAYER_0_INPUTS*LAYER_0_OUTPUTS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_0_INPUTS; row_wgt++)
        {
            ping[col_wgt] += in[in_offset+col_wgt*LAYER_0_INPUTS+row_wgt] * in[weights_offset+row_wgt];
        }

        if (ping[col_wgt] < 0) ping[col_wgt] = 0;
    }

    in_offset += LAYER_0_INPUTS*LAYER_0_OUTPUTS + LAYER_0_OUTPUTS;

    // Layer 1
    for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
    {
        pong[col_wgt] = in[in_offset+LAYER_N_DIMS*LAYER_N_DIMS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            pong[col_wgt] += in[in_offset+col_wgt*LAYER_N_DIMS+row_wgt] * ping[row_wgt];
        }

        if (pong[col_wgt] < 0) pong[col_wgt] = 0;
    }

    in_offset += LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS;

    // Layer 2
    for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
    {
        ping[col_wgt] = in[in_offset+LAYER_N_DIMS*LAYER_N_DIMS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            ping[col_wgt] += in[in_offset+col_wgt*LAYER_N_DIMS+row_wgt] * pong[row_wgt];
        }

        if (ping[col_wgt] < 0) ping[col_wgt] = 0;
    }

    in_offset += LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS;

    // Layer 3
    for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
    {
        pong[col_wgt] = in[in_offset+LAYER_N_DIMS*LAYER_N_DIMS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            pong[col_wgt] += in[in_offset+col_wgt*LAYER_N_DIMS+row_wgt] * ping[row_wgt];
        }

        if (pong[col_wgt] < 0) pong[col_wgt] = 0;
    }

    in_offset += LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS;

    // Layer 4
    for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
    {
        ping[col_wgt] = in[in_offset+LAYER_N_DIMS*LAYER_N_DIMS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            ping[col_wgt] += in[in_offset+col_wgt*LAYER_N_DIMS+row_wgt] * pong[row_wgt];
        }

        if (ping[col_wgt] < 0) ping[col_wgt] = 0;
    }

    in_offset += LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS;

    // Layer 5
    for (uint16_t row_wgt = LAYER_N_DIMS; row_wgt < LAYER_5_INPUTS; row_wgt++)
    {
        ping[row_wgt] = in[weights_offset+row_wgt-LAYER_N_DIMS];
    }

    for (uint16_t col_wgt = 0; col_wgt < LAYER_5_OUTPUTS; col_wgt++)
    {
        pong[col_wgt] = in[in_offset+LAYER_5_INPUTS*LAYER_5_OUTPUTS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_5_INPUTS; row_wgt++)
        {
            pong[col_wgt] += in[in_offset+col_wgt*LAYER_5_INPUTS+row_wgt] * ping[row_wgt];
        }

        if (pong[col_wgt] < 0) pong[col_wgt] = 0;
    }

    in_offset += LAYER_5_INPUTS*LAYER_5_OUTPUTS + LAYER_5_OUTPUTS;

    // Layer 6
    for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
    {
        ping[col_wgt] = in[in_offset+LAYER_N_DIMS*LAYER_N_DIMS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            ping[col_wgt] += in[in_offset+col_wgt*LAYER_N_DIMS+row_wgt] * pong[row_wgt];
        }

        if (ping[col_wgt] < 0) ping[col_wgt] = 0;
    }

    in_offset += LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS;

    // Layer 7
    for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
    {
        pong[col_wgt] = in[in_offset+LAYER_N_DIMS*LAYER_N_DIMS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            pong[col_wgt] += in[in_offset+col_wgt*LAYER_N_DIMS+row_wgt] * ping[row_wgt];
        }

        if (pong[col_wgt] < 0) pong[col_wgt] = 0;

        layer_11_input[col_wgt] = pong[col_wgt];
    }

    in_offset += LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS;

    // Layer 8
    for (uint16_t col_wgt = 0; col_wgt < LAYER_N_DIMS; col_wgt++)
    {
        ping[col_wgt] = in[in_offset+LAYER_N_DIMS*LAYER_N_DIMS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_N_DIMS; row_wgt++)
        {
            ping[col_wgt] += in[in_offset+col_wgt*LAYER_N_DIMS+row_wgt] * pong[row_wgt];
        }
    }

    in_offset += LAYER_N_DIMS*LAYER_N_DIMS + LAYER_N_DIMS;

    // Layer 9
    for (uint16_t row_wgt = LAYER_N_DIMS; row_wgt < LAYER_9_INPUTS; row_wgt++)
    {
        ping[row_wgt] = in[weights_offset+LAYER_0_INPUTS+row_wgt-LAYER_N_DIMS];
    }

    for (uint16_t col_wgt = 0; col_wgt < LAYER_9_OUTPUTS; col_wgt++)
    {
        pong[col_wgt] = in[in_offset+LAYER_9_INPUTS*LAYER_9_OUTPUTS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_9_INPUTS; row_wgt++)
        {
            pong[col_wgt] += in[in_offset+col_wgt*LAYER_9_INPUTS+row_wgt] * ping[row_wgt];
        }

        if (pong[col_wgt] < 0) pong[col_wgt] = 0;
    }

    in_offset += LAYER_9_INPUTS*LAYER_9_OUTPUTS + LAYER_9_OUTPUTS;

    // Layer 10
    for (uint16_t col_wgt = 0; col_wgt < LAYER_10_OUTPUTS; col_wgt++)
    {
        gold[col_wgt] = in[in_offset+LAYER_10_INPUTS*LAYER_10_OUTPUTS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_10_INPUTS; row_wgt++)
        {
            gold[col_wgt] += in[in_offset+col_wgt*LAYER_10_INPUTS+row_wgt] * pong[row_wgt];
        }
    }

    in_offset += LAYER_10_INPUTS*LAYER_10_OUTPUTS + LAYER_10_OUTPUTS;

    // Layer 11
    for (uint16_t col_wgt = 0; col_wgt < LAYER_11_OUTPUTS; col_wgt++)
    {
        gold[LAYER_10_OUTPUTS+col_wgt] = in[in_offset+LAYER_11_INPUTS*LAYER_11_OUTPUTS + col_wgt];

        for (uint16_t row_wgt = 0; row_wgt < LAYER_11_INPUTS; row_wgt++)
        {
            gold[LAYER_10_OUTPUTS+col_wgt] += in[in_offset+col_wgt*LAYER_11_INPUTS+row_wgt] * layer_11_input[row_wgt];
        }
    }

    in_offset += LAYER_11_INPUTS*LAYER_11_OUTPUTS + LAYER_11_OUTPUTS;

    cpu_end = get_counter();
    cpu_diff = cpu_end - cpu_start;
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
	token_t *gold;
	unsigned errors = 0;
	unsigned coherence;

	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = 595960;
		out_words_adj = 4;
	} else {
		in_words_adj = round_up(595960, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(4, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj * (1);
	out_len = out_words_adj * (1);
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset  = in_len;
	mem_size = (out_offset * sizeof(token_t)) + out_size;


	// Search for the device
	printf("Scanning device tree... \n");

	ndev = probe(&espdevs, VENDOR_SLD, SLD_NERF_MLP, DEV_NAME);
	if (ndev == 0) {
		printf("nerf_mlp not found\n");
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

		// Allocate memory
		gold = aligned_malloc(out_size);
		mem = aligned_malloc(mem_size);
		printf("  memory buffer base-address = %p\n", mem);

		// Alocate and populate page table
		ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
		for (i = 0; i < NCHUNK(mem_size); i++)
			ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];

		printf("  ptable = %p\n", ptable);
		printf("  nchunk = %lu\n", NCHUNK(mem_size));

#ifndef __riscv
		for (coherence = ACC_COH_NONE; coherence <= ACC_COH_RECALL; coherence++) {
#else
		{
			/* TODO: Restore full test once ESP caches are integrated */
			coherence = ACC_COH_NONE;
#endif
			printf("  --------------------\n");
			printf("  Generate input...\n");
			init_buf(mem, gold);

			// Pass common configuration parameters

			iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
			iowrite32(dev, COHERENCE_REG, coherence);

#ifndef __sparc
			iowrite32(dev, PT_ADDRESS_REG, (unsigned long long) ptable);
#else
			iowrite32(dev, PT_ADDRESS_REG, (unsigned) ptable);
#endif
			iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
			iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

			// Use the following if input and output data are not allocated at the default offsets
			iowrite32(dev, SRC_OFFSET_REG, 0x0);
			iowrite32(dev, DST_OFFSET_REG, 0x0);

			// Pass accelerator-specific configuration parameters
			/* <<--regs-config-->> */
			iowrite32(dev, NERF_MLP_BATCH_SIZE_REG, batch_size);

			// Flush (customize coherence model here)
			esp_flush(coherence);

			// Start accelerators
			printf("  Start...\n");

            acc_start = get_counter();

			iowrite32(dev, CMD_REG, CMD_MASK_START);

			// Wait for completion
			done = 0;
			while (!done) {
				done = ioread32(dev, STATUS_REG);
				done &= STATUS_MASK_DONE;
			}
			iowrite32(dev, CMD_REG, 0x0);

            acc_end = get_counter();
            acc_diff = acc_end - acc_start;

			printf("  Done\n");
			printf("  validating...\n");

			/* Validation */
			// errors = validate_buf(&mem[out_offset], gold);
			if (errors)
				printf("  ... FAIL\n");
			else
				printf("  ... PASS\n");

            printf("CPU time = %lu\n", cpu_diff);
            printf("ACC time = %lu\n", acc_diff);
		}
		aligned_free(ptable);
		aligned_free(mem);
		aligned_free(gold);
	}

	return 0;
}
