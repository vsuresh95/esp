/* Copyright (c) 2011-2019 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include "utils/fft2_utils.h"

#include "cfg.h"
#include "sw_func.h"
#include "acc_func.h"

static int validate_buf(token_t *out, float *gold)
{
	int j;
	unsigned errors = 0;

	for (j = 0; j < 2 * len; j++) {
		native_t val = fx2float(out[j+SYNC_VAR_SIZE], FX_IL);
		uint32_t ival = *((uint32_t*)&val);
		printf("%u G %08x O %08x\n", j, ((uint32_t*)gold)[j], ival);
		if ((fabs(gold[j] - val) / fabs(gold[j])) > ERR_TH) {
			errors++;
        }
	}

	//printf("  %u errors\n", errors);
	return errors;
}

static void init_buf(token_t *in, float *gold, token_t *in_filter, float *gold_filter, token_t *in_twiddle, float *gold_twiddle)
{
	int j;

	// convert input to fixed point
	for (j = 0; j < 2 * len; j++)
		in[j+SYNC_VAR_SIZE] = float2fx((native_t) gold[j], FX_IL);

	// convert input to fixed point
	for (j = 0; j < 2 * (len+1); j++)
		in_filter[j] = float2fx((native_t) gold_filter[j], FX_IL);

	// convert input to fixed point
	for (j = 0; j < 2 * len; j++)
		in_twiddle[j] = float2fx((native_t) gold_twiddle[j], FX_IL);
}

int main(int argc, char * argv[])
{
	int i;

	///////////////////////////////////////////////////////////////
	// Init data size parameters
	///////////////////////////////////////////////////////////////
	init_params();

	printf("ilen %u isize %u o_off %u olen %u osize %u msize %u\n", in_len, out_len, in_size, out_size, out_offset, mem_size);

	///////////////////////////////////////////////////////////////
	// Allocate memory pointers
	///////////////////////////////////////////////////////////////
	gold = aligned_malloc((6 * out_len) * sizeof(float));
	mem = aligned_malloc(mem_size);
	printf("  memory buffer base-address = %p\n", mem);

    volatile token_t* sm_sync = (volatile token_t*) mem;

	// Allocate and populate page table
	ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (i = 0; i < NCHUNK(mem_size); i++)
		ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];

	printf("  ptable = %p\n", ptable);
	printf("  nchunk = %lu\n", NCHUNK(mem_size));

	///////////////////////////////////////////////////////////////
	// Initialize golden buffers, transfer to accelerator memory
	// and compute golden output
	///////////////////////////////////////////////////////////////
	printf("  --------------------\n");
	printf("  Generate input...\n");

	golden_data_init(gold, (gold + out_len) /* gold_filter */, (gold + 3 * out_len) /* gold_twiddle */);

	printf("  Init buffers...\n");

	init_buf(mem, gold, (mem + 5 * acc_offset) /* in_filter */, (gold + out_len) /* gold_filter */,
			(mem + 7 * acc_offset) /* in_twiddle */, (gold + 3 * out_len) /* gold_twiddle */);

	printf("  SW implementation...\n");
	printf("  --------------------\n");

	chain_sw_impl (gold, (gold + out_len) /* gold_filter */, (gold + 3 * out_len) /* gold_twiddle */, (gold + 4 * out_len) /* gold_freqdata */);

	///////////////////////////////////////////////////////////////
	// Start all accelerators
	///////////////////////////////////////////////////////////////
	start_acc();

	sm_sync[0] = 0;
	sm_sync[acc_offset] = 0;
	sm_sync[2*acc_offset] = 0;
	sm_sync[3*acc_offset] = 0;

	sm_sync[2] = 0;
	sm_sync[acc_offset+2] = 0;
	sm_sync[2*acc_offset+2] = 0;

	sm_sync[0] = 1;
	while(sm_sync[NUM_DEVICES*acc_offset] != 1);
	sm_sync[NUM_DEVICES*acc_offset] = 0;

	sm_sync[2] = 1;
	sm_sync[acc_offset+2] = 1;
	sm_sync[2*acc_offset+2] = 1;

	sm_sync[0] = 1;
	while(sm_sync[NUM_DEVICES*acc_offset] != 1);
	sm_sync[NUM_DEVICES*acc_offset] = 0;

	///////////////////////////////////////////////////////////////
	// Terminate all accelerators
	///////////////////////////////////////////////////////////////
	terminate_acc();

	printf("  validating...\n");

	/* Validation */
	unsigned errors = validate_buf(&mem[NUM_DEVICES*acc_offset], gold);
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
