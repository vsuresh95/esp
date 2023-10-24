// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
/**
 * Baremetal device driver for SORT
 *
 * Select Scatter-Gather in ESP configuration
 */

#include <stdio.h>
#include <stdlib.h>
#include <esp_accelerator.h>
#include <esp_probe.h>

// #define ENABLE_SM
// #define SPX

#ifdef SPX
	#define COH_MODE 0
#else
	#define IS_ESP 1
	#define COH_MODE 1
#endif

#include "sm.h"
#define ITERATIONS 10

#define SLD_SORT   0x0B
#define DEV_NAME "sld,sort_stratus"

#define SORT_LEN 64
#define SORT_BATCH 1

#define SYNC_VAR_SIZE 6
#define UPDATE_VAR_SIZE 2
#define VALID_FLAG_OFFSET 0
#define END_FLAG_OFFSET 2
#define READY_FLAG_OFFSET 4

#define SORT_BUF_SIZE (2 * SORT_LEN * SORT_BATCH * sizeof(unsigned) + 2 * SYNC_VAR_SIZE * sizeof(unsigned))

/* Size of the contiguous chunks for scatter/gather */
#define CHUNK_SHIFT 11
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK ((SORT_BUF_SIZE % CHUNK_SIZE == 0) ?			\
			(SORT_BUF_SIZE / CHUNK_SIZE) :			\
			(SORT_BUF_SIZE / CHUNK_SIZE) + 1)

// User defined registers
#define SORT_LEN_REG		0x40
#define SORT_BATCH_REG		0x44
#define SORT_LEN_MIN_REG	0x48
#define SORT_LEN_MAX_REG	0x4c
#define SORT_BATCH_MAX_REG	0x50

#define SORT_PROD_VALID_OFFSET 0x54
#define SORT_PROD_READY_OFFSET 0x58
#define SORT_CONS_VALID_OFFSET 0x5c
#define SORT_CONS_READY_OFFSET 0x60
#define SORT_INPUT_OFFSET 0x64
#define SORT_OUTPUT_OFFSET 0x68

static int validate_sorted(float *array, int len)
{
	int i;
	int rtn = 0;
	for (i = 0; i < len; i++) {
		if (i > 0 && array[i] < array[i-1]){
			printf("	Error at %d\n", i);
			printf("	mem[%d] = %llx\n", i, array[i]);
			printf("	mem[%d] = %llx\n", i - 1, array[i-1]);
			rtn++;
		}
		printf("A[%d]: array=%llx\n", i, array[i]);
	}
	return rtn;
}

static void init_buf (float *buf, unsigned sort_size, unsigned sort_batch)
{
	int i, j, k;

	float A[SORT_LEN];
	#include "debug.h"

	printf("  Generate random input...\n");

	/* srand(time(NULL)); */
	for (j = 0; j < sort_batch; j++)
		for (i = 0; i < sort_size; i++) {
			/* TAV rand between 0 and 1 */
#ifndef __riscv
			buf[sort_size * j + i] = ((float) rand () / (float) RAND_MAX);
#else
			// buf[sort_size * j + i] = 1.0 / ((float) i + 1);
			if (sort_size == 32) {
				buf[sort_size * j + i] = A[i];
			}
			else {
				for (k = 0; k < sort_size / 64; ++k) {
					buf[sort_size * j + i] = A[i - 64 * k];
				}
			}
			
#endif
			// printf("A[%d]: array=%llx\n", i, buf[sort_size * j + i]);
			
			/* /\* More general testbench *\/ */
			/* float M = 100000.0; */
			/* buf[sort_size * j + i] =  M * ((float) rand() / (float) RAND_MAX) - M/2; */
			/* /\* Easyto debug...! *\/ */
			/* buf[sort_size * j + i] = (float) (sort_size - i);; */
		}
}


int main(int argc, char * argv[])
{
	int n;
	int ndev;
	struct esp_device *espdevs = NULL;
	// unsigned coherence;

	ndev = probe(&espdevs, VENDOR_SLD, SLD_SORT, DEV_NAME);
	if (!ndev) {
		printf("Error: %s device not found!\n", DEV_NAME);
		exit(EXIT_FAILURE);
	}

	printf("Test parameters: [LEN, BATCH] = [%d, %d]\n\n", SORT_LEN, SORT_BATCH);
	for (n = 0; n < ndev; n++) {
// #ifndef __riscv
// 		for (coherence = ACC_COH_NONE; coherence <= ACC_COH_FULL; coherence++) {
// #else
// 		{
// 			/* TODO: Restore full test once ESP caches are integrated */
// 			coherence = ACC_COH_FULL;
// #if (IS_ESP == 1 && COH_MODE == 0)
// 			coherence = ACC_COH_RECALL;
// #endif
// #endif
			struct esp_device *dev = &espdevs[n];
			unsigned sort_batch_max;
			unsigned sort_len_max;
			unsigned sort_len_min;
			unsigned done;
			int i, j;
			unsigned **ptable = NULL;
			unsigned *mem;
			unsigned errors = 0;
			int scatter_gather = 1;

			sort_batch_max = ioread32(dev, SORT_BATCH_MAX_REG);
			sort_len_min = ioread32(dev, SORT_LEN_MIN_REG);
			sort_len_max = ioread32(dev, SORT_LEN_MAX_REG);

			printf("******************** %s.%d ********************\n", DEV_NAME, n);
			// Check access ok
			if (SORT_LEN < sort_len_min ||
				SORT_LEN > sort_len_max ||
				SORT_BATCH < 1 ||
				SORT_BATCH > sort_batch_max) {
				printf("  Error: unsopported configuration parameters for %s.%d\n", DEV_NAME, n);
				printf("         device can sort up to %d fp-vectors of size [%d, %d]\n",
					sort_batch_max, sort_len_min, sort_len_max);
				break;
			}

			// Check if scatter-gather DMA is disabled
			if (ioread32(dev, PT_NCHUNK_MAX_REG) == 0) {
			    printf("  -> scatter-gather DMA is disabled. Abort.\n");
			    scatter_gather = 0;
			}

			if (scatter_gather)
				if (ioread32(dev, PT_NCHUNK_MAX_REG) < NCHUNK) {
				    printf("  -> Not enough TLB entries available. Abort.\n");
				    break;
				}

			// Allocate memory (will be contigous anyway in baremetal)
			mem = aligned_malloc(SORT_BUF_SIZE);

			printf("  memory buffer base-address = %p\n", mem);
			printf("  coherence = %u\n", coherence);
			printf("  Coherence Mode: %s\n", CohPrintHeader);

			if (scatter_gather) {
				//Alocate and populate page table
				ptable = aligned_malloc(NCHUNK * sizeof(unsigned *));
				for (i = 0; i < NCHUNK; i++)
					ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(unsigned))];

				printf("  ptable = %p\n", ptable);
				printf("  nchunk = %lu\n", NCHUNK);
			}

			// Configure device
			iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
			iowrite32(dev, COHERENCE_REG, coherence);

			if (scatter_gather) {
				iowrite32(dev, PT_ADDRESS_REG, (unsigned long) ptable);
				iowrite32(dev, PT_NCHUNK_REG, NCHUNK);
				iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);
				iowrite32(dev, SRC_OFFSET_REG, 0);
				iowrite32(dev, DST_OFFSET_REG, 0); // Sort runs in place
			} else {
				iowrite32(dev, SRC_OFFSET_REG, (unsigned long) mem);
				iowrite32(dev, DST_OFFSET_REG, (unsigned long) mem); // Sort runs in place
			}
			iowrite32(dev, SORT_LEN_REG, SORT_LEN);
			iowrite32(dev, SORT_BATCH_REG, SORT_BATCH);

			iowrite32(dev, SORT_PROD_VALID_OFFSET, VALID_FLAG_OFFSET);
			iowrite32(dev, SORT_PROD_READY_OFFSET, READY_FLAG_OFFSET);
			iowrite32(dev, SORT_CONS_VALID_OFFSET, SYNC_VAR_SIZE + SORT_LEN + VALID_FLAG_OFFSET);
			iowrite32(dev, SORT_CONS_READY_OFFSET, SYNC_VAR_SIZE + SORT_LEN + READY_FLAG_OFFSET);
			iowrite32(dev, SORT_INPUT_OFFSET, SYNC_VAR_SIZE);
			iowrite32(dev, SORT_OUTPUT_OFFSET, SYNC_VAR_SIZE + SORT_LEN + SYNC_VAR_SIZE);

#ifdef ENABLE_SM
			// Reset all sync variables to default values.
			UpdateSync((void*) &mem[VALID_FLAG_OFFSET], 0);
			UpdateSync((void*) &mem[READY_FLAG_OFFSET], 1);
			UpdateSync((void*) &mem[END_FLAG_OFFSET], 0);
			UpdateSync((void*) &mem[SYNC_VAR_SIZE + SORT_LEN + VALID_FLAG_OFFSET], 0);
			UpdateSync((void*) &mem[SYNC_VAR_SIZE + SORT_LEN + READY_FLAG_OFFSET], 1);
			UpdateSync((void*) &mem[SYNC_VAR_SIZE + SORT_LEN + END_FLAG_OFFSET], 0);
#endif

			// Flush for non-coherent DMA
			esp_flush(coherence);

#ifndef ENABLE_SM
			for (i = 0; i < ITERATIONS; ++i) {
			// Initialize input: write floating point hex values (simpler to debug)
			init_buf((float *) &mem[SYNC_VAR_SIZE], SORT_LEN, SORT_BATCH);
#endif

			// Start accelerator
			printf("  Start..\n");
			iowrite32(dev, CMD_REG, CMD_MASK_START);

#ifdef ENABLE_SM
			for (i = 0; i < ITERATIONS; ++i) {

			printf("SM Enabled\n");

			// Initialize input: write floating point hex values (simpler to debug)
			WriteScratchReg(0x100);
			// Wait for the accelerator to be ready
			SpinSync((void*) &mem[READY_FLAG_OFFSET], 1);
			// Reset flag for the next iteration
			UpdateSync((void*) &mem[READY_FLAG_OFFSET], 0);
			// When the accelerator is ready, we write the input data to it
			init_buf((float *) &mem[SYNC_VAR_SIZE], SORT_LEN, SORT_BATCH);
			WriteScratchReg(0);

			WriteScratchReg(0x200);
			if (i == ITERATIONS - 1) {
				WriteScratchReg(0x500);
				UpdateSync((void*) &mem[END_FLAG_OFFSET], 1);
				UpdateSync((void*) &mem[SYNC_VAR_SIZE + SORT_LEN + END_FLAG_OFFSET], 1);
				
				// SpinSync((void*) &mem[READY_FLAG_OFFSET], 1);
				// UpdateSync((void*) &mem[READY_FLAG_OFFSET], 0);
				// UpdateSync((void*) &mem[VALID_FLAG_OFFSET], 1);

				// SpinSync((void*) &mem[SYNC_VAR_SIZE + SORT_LEN + VALID_FLAG_OFFSET], 1);
				// UpdateSync((void*) &mem[SYNC_VAR_SIZE + SORT_LEN + VALID_FLAG_OFFSET], 0);
				// UpdateSync((void*) &mem[SYNC_VAR_SIZE + SORT_LEN + READY_FLAG_OFFSET], 1);
				WriteScratchReg(0);
			}
			// Inform the accelerator to start.
			UpdateSync((void*) &mem[VALID_FLAG_OFFSET], 1);
			WriteScratchReg(0);

			WriteScratchReg(0x300);
			// Wait for the accelerator to send output.
			SpinSync((void*) &mem[SYNC_VAR_SIZE + SORT_LEN + VALID_FLAG_OFFSET], 1);
			// Reset flag for next iteration.
			UpdateSync((void*) &mem[SYNC_VAR_SIZE + SORT_LEN + VALID_FLAG_OFFSET], 0);
			WriteScratchReg(0);

			WriteScratchReg(0x400);
			errors += validate_sorted((float *) &mem[SYNC_VAR_SIZE + SORT_LEN + SYNC_VAR_SIZE], SORT_LEN);
			// Inform the accelerator - ready for next iteration.
			UpdateSync((void*) &mem[SYNC_VAR_SIZE + SORT_LEN + READY_FLAG_OFFSET], 1);
			WriteScratchReg(0);
			}
#endif

			// End accelerator
			done = 0;
			while (!done) {
				done = ioread32(dev, STATUS_REG);
				done &= STATUS_MASK_DONE;
			}
			iowrite32(dev, CMD_REG, 0x0);
			printf("  Done\n");

			/* /\* Print output *\/ */
			/* printf("  output:\n"); */
			/* for (j = 0; j < SORT_BATCH; j++) */
			/* 	for (i = 0; i < SORT_LEN; i++) */
			/* 		printf("    mem[%d][%d] = %08x\n", j, i, mem[j*SORT_LEN + i]); */

			/* Validation */
			printf("  validating...\n");

#ifndef ENABLE_SM
			for (j = 0; j < SORT_BATCH; j++) {
				int err = validate_sorted((float *) &mem[j * SORT_BUF_SIZE + SYNC_VAR_SIZE + SORT_LEN + SYNC_VAR_SIZE], SORT_LEN);
				/* if (err != 0) */
				/* 	printf("  Error: %s.%d mismatch on batch %d\n", DEV_NAME, n, j); */
				errors += err;
			}
			}
#endif
			if (errors)
				printf("  ... FAIL\n");
			else
				printf("  ... PASS\n");
			printf("**************************************************\n\n");
			printf("errors = %d\n", errors);

			if (scatter_gather)
				aligned_free(ptable);
			aligned_free(mem);

		// }
	}
	return 0;
}
