/* Copyright (c) 2011-2019 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include "utils/fft2_utils.h"

#define COH_MODE 1
#define IS_ESP 1

#include "coh_func.h"
#include "sm.h"

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}

#define SLD_GEMM 0x051
#define DEV_NAME "sld,gemm_stratus"

/* <<--params-->> */
const unsigned dim_m = 16;
const unsigned dim_n = 16;
const unsigned dim_k = 16;

static unsigned in_1_words_adj;
static unsigned in_2_words_adj;
static unsigned out_words_adj;
static unsigned in_1_len;
static unsigned in_2_len;
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
#define GEMM_DIMM_M_REG 0x40
#define GEMM_DIMM_N_REG 0x44
#define GEMM_DIMM_K_REG 0x48

#define GEMM_PROD_VALID_OFFSET 0x4C
#define GEMM_PROD_READY_OFFSET 0x50
#define GEMM_CONS_VALID_OFFSET 0x54
#define GEMM_CONS_READY_OFFSET 0x58
#define GEMM_INPUT_1_OFFSET 0x5C
#define GEMM_INPUT_2_OFFSET 0x60
#define GEMM_OUTPUT_OFFSET 0x64

#define ITERATIONS 1

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

void sw_run(unsigned *gold)
{
	int j, m, n, k, m_, n_, k_;
	spandex_token_t in_data;
    void* src = (void*) gold;
	const unsigned block_size = 16;

	for (j = 0; j < dim_m * dim_k; j+=2, src+=8) {
		in_data.value_32_1 = (j+1) % 100;
		in_data.value_32_2 = (j+2) % 100;
        write_mem_wtfwd(src, in_data.value_64);
	}

    src = (void*) (gold + in_1_words_adj);

	for (j = 0; j < dim_n * dim_k; j+=2, src+=8) {
		in_data.value_32_1 = (j+1) % 100;
		in_data.value_32_2 = (j+2) % 100;
        write_mem_wtfwd(src, in_data.value_64);
	}

	// Compute golden output
	start_counter();
	for (m = 0; m < dim_m/block_size; m++) {
		unsigned in_1_offset = (m * block_size) * dim_k;
		unsigned local_out_offset = in_1_words_adj + in_2_words_adj + (m * block_size) * dim_n;
		for (n = 0; n < dim_n/block_size; n++) {
			unsigned in_2_offset = in_1_words_adj + (n * block_size) * dim_k;
			unsigned out_block_offset = local_out_offset + n * block_size;
			for (k = 0; k < dim_k/block_size; k++) {
				unsigned in_1_block_offset = in_1_offset + k * block_size;
				unsigned in_2_block_offset = in_2_offset + k * block_size;
				for (m_ = 0; m_ < block_size; m_++) {
					unsigned in_1_elem_offset = in_1_block_offset + m_ * dim_k;
					unsigned out_elem_offset = out_block_offset + m_ * dim_n;
					for (n_ = 0; n_ < block_size; n_++) {
						unsigned in_2_elem_offset = in_2_block_offset + n_ * dim_k;
						for (k_ = 0; k_ < block_size; k_++) {
							gold[out_elem_offset + n_] += gold[in_1_elem_offset + k_] * gold[in_2_elem_offset + k_];
						}
					}
				}
			}
		}
	}
	t_sw += end_counter();
}

int validate_buf(unsigned *out, unsigned *gold)
{
	int j;
	unsigned errors = 0;

	spandex_token_t out_data;
    void* src = (void*) out;

	for (j = 0; j < dim_m * dim_n; j+=2, src+=8) {
        out_data.value_64 = read_mem_reqodata(src);
		if (out_data.value_32_1 != gold[in_1_words_adj + in_2_words_adj + j]) errors++;
		if (out_data.value_32_2 != gold[in_1_words_adj + in_2_words_adj + j + 1]) errors++;
	}

	return errors;
}

void init_buf(unsigned *in)
{
	int j;
	
	spandex_token_t in_data;
    void* src = (void*) in;

	for (j = 0; j < dim_m * dim_k; j+=2, src+=8) {
		in_data.value_32_1 = (j+1) % 100;
		in_data.value_32_2 = (j+2) % 100;
        write_mem_wtfwd(src, in_data.value_64);
	}

    src = (void*) (in + in_1_words_adj);

	for (j = 0; j < dim_n * dim_k; j+=2, src+=8) {
		in_data.value_32_1 = (j+1) % 100;
		in_data.value_32_2 = (j+2) % 100;
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
	unsigned *mem;
	unsigned *gold;
	unsigned errors = 0;
	unsigned coherence;

	printf("dim_m %u dim_n %u dim_k %u\n", dim_m, dim_n, dim_k);
	if (DMA_WORD_PER_BEAT(sizeof(unsigned)) == 0) {
		in_1_words_adj = dim_m * dim_k;
		in_2_words_adj = dim_n * dim_k;
		out_words_adj = dim_m * dim_n;
	} else {
		in_1_words_adj = round_up(dim_m * dim_k, DMA_WORD_PER_BEAT(sizeof(unsigned)));
		in_2_words_adj = round_up(dim_n * dim_k, DMA_WORD_PER_BEAT(sizeof(unsigned)));
		out_words_adj = round_up(dim_m * dim_n, DMA_WORD_PER_BEAT(sizeof(unsigned)));
	}
	in_len = in_1_words_adj + in_2_words_adj + SYNC_VAR_SIZE;
	out_len = out_words_adj + SYNC_VAR_SIZE;
	in_size = in_len * sizeof(unsigned);
	out_size = out_len * sizeof(unsigned);
	out_offset  = in_len;
	mem_size = (out_offset * sizeof(unsigned)) + out_size;

	printf("ilen %u isize %u o_off %u olen %u osize %u msize %u\n", in_len, out_len, in_size, out_size, out_offset, mem_size);
	// Search for the device
	ndev = probe(&espdevs, VENDOR_SLD, SLD_GEMM, DEV_NAME);
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

	// Program sync flags
	unsigned cons_rdy_flag_offset = 0*in_len + READY_FLAG_OFFSET;
	unsigned cons_vld_flag_offset = 0*in_len + VALID_FLAG_OFFSET;
	unsigned prod_rdy_flag_offset = 1*in_len + READY_FLAG_OFFSET;
	unsigned prod_vld_flag_offset = 1*in_len + VALID_FLAG_OFFSET;

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
		gold = aligned_malloc((in_len + out_len) * sizeof(unsigned));
		mem = aligned_malloc(mem_size);

		// Allocate and populate page table
		ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
		for (i = 0; i < NCHUNK(mem_size); i++)
			ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(unsigned))];

		// Reset all sync variables to default values.
		UpdateSync((void*) &mem[cons_vld_flag_offset], 0);
		UpdateSync((void*) &mem[cons_rdy_flag_offset], 1);
		UpdateSync((void*) &mem[END_FLAG_OFFSET], 0);
		UpdateSync((void*) &mem[prod_vld_flag_offset], 0);
		UpdateSync((void*) &mem[prod_rdy_flag_offset], 1);

		sw_run(gold);

		{
			/* TODO: Restore full test once ESP caches are integrated */
			coherence = ACC_COH_RECALL;

			// Pass common configuration parameters
			start_counter();
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
			iowrite32(dev, GEMM_DIMM_M_REG, dim_m);
			iowrite32(dev, GEMM_DIMM_N_REG, dim_n);
			iowrite32(dev, GEMM_DIMM_K_REG, dim_k);

			iowrite32(dev, GEMM_PROD_VALID_OFFSET, cons_vld_flag_offset);
			iowrite32(dev, GEMM_PROD_READY_OFFSET, cons_rdy_flag_offset);
			iowrite32(dev, GEMM_CONS_VALID_OFFSET, prod_vld_flag_offset);
			iowrite32(dev, GEMM_CONS_READY_OFFSET, prod_rdy_flag_offset);
			iowrite32(dev, GEMM_INPUT_1_OFFSET, SYNC_VAR_SIZE);
			iowrite32(dev, GEMM_INPUT_2_OFFSET, in_1_words_adj + SYNC_VAR_SIZE);
			iowrite32(dev, GEMM_OUTPUT_OFFSET, in_len + SYNC_VAR_SIZE);

			// Flush (customize coherence model here)
			esp_flush(coherence);

			// Start accelerators
			iowrite32(dev, CMD_REG, CMD_MASK_START);

			for (i = 0; i < ITERATIONS; i++)
			{
				// Wait for the accelerator to be ready
				SpinSync((void*) &mem[cons_rdy_flag_offset], 1);
				// Reset flag for the next iteration
				UpdateSync((void*) &mem[cons_rdy_flag_offset], 0);
				// When the accelerator is ready, we write the input data to it
				init_buf(&mem[SYNC_VAR_SIZE]);

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
			t_acc += end_counter();
		}

		aligned_free(ptable);
		aligned_free(mem);
		aligned_free(gold);

		printf("  Errors = %d\n", errors);
		printf("  Software Input = %lu\n", t_sw_input);
		printf("  Software = %lu\n", t_sw);
		printf("  Software Output = %lu\n", t_sw_output);
		printf("  Accel Input = %lu\n", t_acc_input/ITERATIONS);
		printf("  Accel = %lu\n", t_acc/ITERATIONS);
		printf("  Accel Output = %lu\n", t_acc_output/ITERATIONS);
	}

	return 0;
}
