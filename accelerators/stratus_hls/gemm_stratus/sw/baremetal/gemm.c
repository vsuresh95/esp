/* Copyright (c) 2011-2019 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include <fixed_point.h>
#include <utils/fft2_utils.h>

#include "sm.h"
#include <math.h>

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}

#define SLD_GEMM 0x051
#define DEV_NAME "sld,gemm_stratus"
#define FX_IL 16

/* <<--params-->> */
const unsigned dim_m = 20;
const unsigned dim_n = 20;
const unsigned dim_k = 20;

/* Size of the contiguous chunks for scatter/gather */
#define CHUNK_SHIFT 20
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK(_sz) ((_sz % CHUNK_SIZE == 0) ?		\
			(_sz / CHUNK_SIZE) :		\
			(_sz / CHUNK_SIZE) + 1)

/* User defined registers */
/* <<--regs-->> */
#define GEMM_DIM_M			0x70
#define GEMM_DIM_N			0x74
#define GEMM_DIM_K			0x78
#define GEMM_WEIGHT_BASE	0x7C
#define GEMM_INPUT_BASE		0x80
#define GEMM_OUTPUT_BASE	0x84

void gemm(const float* mat_a, const float* mat_b, float* mat_c, unsigned dim_m, unsigned dim_n, unsigned dim_k) {
    const unsigned block_size = 16;
    float sum;

	for (unsigned m = 0; m < dim_m; m += block_size) {
	    for (unsigned n = 0; n < dim_n; n += block_size) {
	        for (unsigned k = 0; k < dim_k; k += block_size) {

                unsigned m_rem = (m + block_size < dim_m) ? m + block_size : dim_m;
                unsigned n_rem = (n + block_size < dim_n) ? n + block_size : dim_n;
                unsigned k_rem = (k + block_size < dim_k) ? k + block_size : dim_k;

                for (unsigned m_ = m; m_ < m_rem; m_++) {
                    for (unsigned n_ = n; n_ < n_rem; n_++) {
                        if (k == 0) sum = 0;
                        else sum = mat_c[m_ * dim_n + n_];
                        for (unsigned k_ = k; k_ < k_rem; k_++) {
							sum += mat_a[m_ * dim_k + k_] * mat_b[k_ * dim_n + n_];
                        }
                        mat_c[m_ * dim_n + n_] = sum;
                    }
                }
            }
        }
    }
}

int validate_buffer(int *mem_c, float *gold_c)
{
    unsigned errors = 0;
    const unsigned len = dim_m * dim_n;
	const float ERR_TH = 0.05;

    for (unsigned j = 0; j < len; j++) {
		float val = fixed32_to_float(mem_c[j], FX_IL);
		if ((fabs(gold_c[j] - val) / fabs(gold_c[j])) > ERR_TH) {
            if (errors < 10) {
				uint32_t g = *((uint32_t *)&gold_c[j]);
				uint32_t v = *((uint32_t *)&val);
				printf("\tGOLD[%u] = 0x%x vs 0x%x = out[%u]\n", j, g, v, j);
			}
            errors++;
        }
    }

    printf("\tError for %d values out of %d\n", errors, len);

    return errors;
}

// Initialize input and calculate golden output
void init_buffer(int *mem_a, int *mem_b, float *gold_a, float *gold_b, float *gold_c)
{
    const float LO = -2.0;
    const float HI = 2.0;
    const unsigned len_a = dim_m * dim_k;
    const unsigned len_b = dim_n * dim_k;

    for (unsigned j = 0; j < len_a; j++) {
        float scaling_factor = (float) rand() / (float) RAND_MAX;
        gold_a[j] = LO + scaling_factor * (HI - LO);
        mem_a[j] = float_to_fixed32(gold_a[j], FX_IL);
    }

    for (unsigned j = 0; j < len_b; j++) {
        float scaling_factor = (float) rand() / (float) RAND_MAX;
        gold_b[j] = LO + scaling_factor * (HI - LO);
        mem_b[j] = float_to_fixed32(gold_b[j], FX_IL);
    }

    // Compute golden output
    gemm(gold_a, gold_b, gold_c, dim_m, dim_n, dim_k);
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
	int *mem0;
	int *mem1;
	int *mem2;
	float *gold0;
	float *gold1;
	float *gold2;
	unsigned errors0 = 0;
	unsigned errors1 = 0;
	unsigned errors2 = 0;
	unsigned coherence;

	printf("dim_m %u dim_n %u dim_k %u\n", dim_m, dim_n, dim_k);
    unsigned flag_len = PAYLOAD_OFFSET/sizeof(unsigned); // Number of unsigned elements reserved for flags
    unsigned mat_a_len = dim_m * dim_k;
    unsigned mat_b_len = dim_n * dim_k;
    unsigned mat_c_len = dim_m * dim_n;

    // Sync flag and data offsets
    unsigned mat_a_valid_offset = VALID_OFFSET;
    unsigned mat_a_offset = mat_a_valid_offset + flag_len;

    unsigned mat_b_offset = mat_a_offset + mat_a_len;

    unsigned mat_c_valid_offset = mat_b_offset + mat_b_len;
    unsigned mat_c_offset = mat_c_valid_offset + flag_len;

	// Descriptor size - 2 (STAT) + 2 * SM_INFO_SIZE (tasks)
	unsigned stat_len = 2;
	unsigned stat_offset = mat_c_offset + mat_c_len;
	unsigned descr_len = 2 * SM_INFO_SIZE;
	unsigned descr_offset = stat_offset + stat_len;

    unsigned mem_size = 4 * (descr_offset + descr_len) * sizeof(int);

	// Search for the device
	ndev = probe(&espdevs, VENDOR_SLD, SLD_GEMM, DEV_NAME);
	if (ndev == 0) {
		printf("%s not found\n", DEV_NAME);
		return 0;
	}

	printf("**************** %s.0 ****************\n", DEV_NAME);

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
	gold0 = aligned_malloc((mat_c_offset + mat_c_len) * sizeof(unsigned));
	gold1 = aligned_malloc((mat_c_offset + mat_c_len) * sizeof(unsigned));
	gold2 = aligned_malloc((mat_c_offset + mat_c_len) * sizeof(unsigned));
	mem0 = aligned_malloc(mem_size);
	mem1 = aligned_malloc(mem_size);
	mem2 = aligned_malloc(mem_size);

	// Allocate and populate page table
	ptable0 = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (i = 0; i < NCHUNK(mem_size); i++)
		ptable0[i] = (unsigned *) &mem0[i * (CHUNK_SIZE / sizeof(unsigned))];

	ptable1 = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (i = 0; i < NCHUNK(mem_size); i++)
		ptable1[i] = (unsigned *) &mem1[i * (CHUNK_SIZE / sizeof(unsigned))];

	ptable2 = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (i = 0; i < NCHUNK(mem_size); i++)
		ptable2[i] = (unsigned *) &mem2[i * (CHUNK_SIZE / sizeof(unsigned))];

	coherence = ACC_COH_RECALL;

	// Flush (customize coherence model here)
	esp_flush(coherence);

	///////////////////////////////////////////////////////
	/// Start first task
	///////////////////////////////////////////////////////
	init_buffer(&mem0[mat_a_offset], &mem0[mat_b_offset],
				&gold0[mat_a_offset], &gold0[mat_b_offset], &gold0[mat_c_offset]);

	iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	iowrite32(dev, COHERENCE_REG, coherence);
	iowrite32(dev, PT_ADDRESS_REG_0, (unsigned long long) ptable0);
	iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);
	iowrite32(dev, SRC_OFFSET_REG, 0x0);
	iowrite32(dev, DST_OFFSET_REG, 0x0);
	iowrite32(dev, GEMM_DIM_M, dim_m);
	iowrite32(dev, GEMM_DIM_N, dim_n);
	iowrite32(dev, GEMM_DIM_K, dim_k);
	iowrite32(dev, GEMM_WEIGHT_BASE, mat_b_offset);
	iowrite32(dev, GEMM_INPUT_BASE, mat_a_valid_offset);
	iowrite32(dev, GEMM_OUTPUT_BASE, mat_c_valid_offset);

	// Start accelerator
	iowrite32(dev, CMD_REG, CMD_MASK_START);
	
	printf("First context started\n");

	done = 0;
	while (!done) {
		done = ioread32(dev, STATUS_REG);
		done &= STATUS_MASK_DONE;
	}
	iowrite32(dev, CMD_REG, 0x0);

	printf("First context task done\n");

	errors0 += validate_buffer(&mem0[mat_c_offset], &gold0[mat_c_offset]);

	///////////////////////////////////////////////////////
	/// Start second task
	///////////////////////////////////////////////////////
	init_buffer(&mem1[mat_a_offset], &mem1[mat_b_offset],
				&gold1[mat_a_offset], &gold1[mat_b_offset], &gold1[mat_c_offset]);

	iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	iowrite32(dev, COHERENCE_REG, coherence);
	iowrite32(dev, PT_ADDRESS_REG_0, (unsigned long long) ptable1);
	iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);
	iowrite32(dev, SRC_OFFSET_REG, 0x0);
	iowrite32(dev, DST_OFFSET_REG, 0x0);
	iowrite32(dev, GEMM_DIM_M, dim_m);
	iowrite32(dev, GEMM_DIM_N, dim_n);
	iowrite32(dev, GEMM_DIM_K, dim_k);
	iowrite32(dev, GEMM_WEIGHT_BASE, mat_b_offset);
	iowrite32(dev, GEMM_INPUT_BASE, mat_a_valid_offset);
	iowrite32(dev, GEMM_OUTPUT_BASE, mat_c_valid_offset);

	// Start accelerator
	iowrite32(dev, CMD_REG, CMD_MASK_START);
	
	printf("Second context started\n");

	done = 0;
	while (!done) {
		done = ioread32(dev, STATUS_REG);
		done &= STATUS_MASK_DONE;
	}
	iowrite32(dev, CMD_REG, 0x0);

	printf("Second context task done\n");

	errors1 += validate_buffer(&mem1[mat_c_offset], &gold1[mat_c_offset]);

	///////////////////////////////////////////////////////
	/// Start third task
	///////////////////////////////////////////////////////
	init_buffer(&mem2[mat_a_offset], &mem2[mat_b_offset],
				&gold2[mat_a_offset], &gold2[mat_b_offset], &gold2[mat_c_offset]);

	iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	iowrite32(dev, COHERENCE_REG, coherence);
	iowrite32(dev, PT_ADDRESS_REG_0, (unsigned long long) ptable2);
	iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);
	iowrite32(dev, SRC_OFFSET_REG, 0x0);
	iowrite32(dev, DST_OFFSET_REG, 0x0);
	iowrite32(dev, GEMM_DIM_M, dim_m);
	iowrite32(dev, GEMM_DIM_N, dim_n);
	iowrite32(dev, GEMM_DIM_K, dim_k);
	iowrite32(dev, GEMM_WEIGHT_BASE, mat_b_offset);
	iowrite32(dev, GEMM_INPUT_BASE, mat_a_valid_offset);
	iowrite32(dev, GEMM_OUTPUT_BASE, mat_c_valid_offset);

	// Start accelerator
	iowrite32(dev, CMD_REG, CMD_MASK_START);
	
	printf("Third context started\n");

	done = 0;
	while (!done) {
		done = ioread32(dev, STATUS_REG);
		done &= STATUS_MASK_DONE;
	}
	iowrite32(dev, CMD_REG, 0x0);

	printf("Third context task done\n");

	errors2 += validate_buffer(&mem2[mat_c_offset], &gold2[mat_c_offset]);

	aligned_free(ptable0);
	aligned_free(ptable1);
	aligned_free(ptable2);
	aligned_free(mem0);
	aligned_free(mem1);
	aligned_free(mem2);
	aligned_free(gold0);
	aligned_free(gold1);
	aligned_free(gold2);

	printf("  Errors = %d %d %d\n", errors0, errors1, errors2);

	while(1);
	return 0;
}
