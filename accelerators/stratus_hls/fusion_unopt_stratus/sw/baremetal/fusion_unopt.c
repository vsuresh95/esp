/* Copyright (c) 2011-2022 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include <fixed_point.h>

#define COH_MODE 0
#define ESP
#define ITERATIONS 100

#include "cfg.h"
#include "sw_func.h"
#include "coh_func.h"
#include "acc_func.h"

static uint64_t t_start = 0;
static uint64_t t_end = 0;

uint64_t t_cpu;
uint64_t t_acc;

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

static int validate_buf(token_t *out, token_t *gold)
{
	int i;
	int j;
	unsigned errors = 0;

	for (i = 0; i < 1; i++)
		for (j = 0; j < veno * sdf_block_size * sdf_block_size * sdf_block_size * 3; j++)
			if (gold[i * out_words_adj + j] != out[i * out_words_adj + j])
				errors++;

	return errors;
}


static void init_buf (token_t *in, token_t * gold)
{
	int i;
	int j;

	for (i = 0; i < 1; i++)
		for (j = 0; j < 2+1+4*4+4+1+1+imgwidth*imgheight+sdf_block_size3 * 2+htdim * 5+veno; j++)
			in[i * in_words_adj + j] = (token_t) j - 1000;

	// for (i = 0; i < 1; i++)
	// 	for (j = 0; j < veno * sdf_block_size * sdf_block_size * sdf_block_size * 3; j++)
	// 		gold[i * out_words_adj + j] = (token_t) j;
}


int main(int argc, char * argv[])
{
	int i;
	unsigned errors = 0;
	t_acc = 0;
	t_cpu = 0;

	///////////////////////////////////////////////////////////////
	// Init data size parameters
	///////////////////////////////////////////////////////////////
	init_params();

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

	printf("CPU starts\n");
	for (i = 0; i < ITERATIONS; i++) {
		start_counter();
		IntegrateIntoScene_depth_s(mem,
									*(mem + 2),
									mem + 2 + 1,
									mem + 2 + 1 + 4 * 4,
									*(mem + 2 + 1 + 4 * 4 + 4),
									*(mem + 2 + 1 + 4 * 4 + 4 + 1),
									mem + 2 + 1 + 4 * 4 + 4 + 1 + 1,
									mem + 2 + 1 + 4 * 4 + 4 + 1 + 1 + imgwidth * imgheight,
									mem + 2 + 1 + 4 * 4 + 4 + 1 + 1 + imgwidth * imgheight + sdf_block_size3 * 2,
									mem + 2 + 1 + 4 * 4 + 4 + 1 + 1 + imgwidth * imgheight + sdf_block_size3 * 2 + htdim * 5,
									gold);
		t_cpu += end_counter();
	}
	printf("  Mode: %s\n", print_coh);

	probe_acc();


	printf("Acc starts\n");

	init_buf(mem, gold);

	for (i = 0; i < ITERATIONS; i++) {
		start_counter();
		start_fusion(dev);
		// printf("In the middle of acc run\n");
		terminate_fusion(dev);
		t_acc += end_counter();
		
	}

	/* Validation */
	errors = validate_buf(&mem[out_offset], gold);
	// if (errors)
	// 	printf("  ... FAIL\n");
	// else
	// 	printf("  ... PASS\n");

	aligned_free(ptable);
	aligned_free(mem);
	aligned_free(gold);

	printf("  CPU = %lu\n", t_cpu/ITERATIONS);
	printf("  ACC = %lu\n", t_acc/ITERATIONS);

	return 0;
}
