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
const unsigned dim_n = 18;
const unsigned dim_k = 20;

/* Size of the contiguous chunks for scatter/gather */
#define CHUNK_SHIFT 20
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK(_sz) ((_sz % CHUNK_SIZE == 0) ?		\
			(_sz / CHUNK_SIZE) :		\
			(_sz / CHUNK_SIZE) + 1)

/* User defined registers */
/* <<--regs-->> */
#define GEMM_CONTEXT_BASE_PTR_0		0x70
#define GEMM_CONTEXT_BASE_PTR_1		0x74
#define GEMM_CONTEXT_BASE_PTR_2		0x78
#define GEMM_CONTEXT_BASE_PTR_3		0x7C
#define GEMM_CONTEXT_NPRIO_0		0x80
#define GEMM_CONTEXT_NPRIO_1		0x84
#define GEMM_CONTEXT_NPRIO_2		0x88
#define GEMM_CONTEXT_NPRIO_3		0x8C
#define GEMM_VALID_CONTEXTS			0x90
#define GEMM_SCHED_PERIOD			0x94

static uint64_t t_start = 0;
static uint64_t t_end = 0;

uint64_t t_sw;
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
	start_counter();
    gemm(gold_a, gold_b, gold_c, dim_m, dim_n, dim_k);
	t_sw += end_counter();
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
	int *mem;
	float *gold;
	unsigned errors = 0;
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

    unsigned mem_size = (descr_offset + descr_len) * sizeof(int);

	// Search for the device
	ndev = probe(&espdevs, VENDOR_SLD, SLD_GEMM, DEV_NAME);
	if (ndev == 0) {
		printf("%s not found\n", DEV_NAME);
		return 0;
	}

	t_sw = 0;
	t_acc = 0;

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
	gold = aligned_malloc((mat_c_offset + mat_c_len) * sizeof(unsigned));
	mem = aligned_malloc(mem_size);

	// Allocate and populate page table
	ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (i = 0; i < NCHUNK(mem_size); i++)
		ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(unsigned))];

	coherence = ACC_COH_RECALL;

    // We will cast the synchronization flags from *mem to custom atomic flags
    atomic_flag_t input_flag;
    atomic_flag_t output_flag;
	atomic_flag_init(&input_flag, (volatile uint64_t *) &mem[mat_a_valid_offset]);
	atomic_flag_init(&output_flag, (volatile uint64_t *) &mem[mat_c_valid_offset]);

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
	iowrite32(dev, GEMM_CONTEXT_BASE_PTR_0, stat_offset);
	iowrite32(dev, PT_ADDRESS_REG_0, (unsigned long long) ptable);
	iowrite32(dev, GEMM_CONTEXT_NPRIO_0, 1);
	iowrite32(dev, GEMM_VALID_CONTEXTS, 0x1);
	iowrite32(dev, GEMM_SCHED_PERIOD, 0x100000);

	// Set up two descriptors: 1 GEMM, 1 JUMP
	mem[stat_offset + 0] = 1;
	mem[stat_offset + 1] = descr_offset;

	mem[descr_offset + 0 * SM_INFO_SIZE + 0] = 1;
	mem[descr_offset + 0 * SM_INFO_SIZE + 1] = dim_m;
	mem[descr_offset + 0 * SM_INFO_SIZE + 2] = dim_n;
	mem[descr_offset + 0 * SM_INFO_SIZE + 3] = dim_k;
	mem[descr_offset + 0 * SM_INFO_SIZE + 4] = mat_b_offset;
	mem[descr_offset + 0 * SM_INFO_SIZE + 5] = mat_a_valid_offset;
	mem[descr_offset + 0 * SM_INFO_SIZE + 6] = mat_c_valid_offset;

	mem[descr_offset + 1 * SM_INFO_SIZE + 0] = 2;
	mem[descr_offset + 1 * SM_INFO_SIZE + 1] = descr_offset;

	// Flush (customize coherence model here)
	esp_flush(coherence);

	// Start accelerators
	iowrite32(dev, CMD_REG, CMD_MASK_START);

    const unsigned iterations = 20;

    for (unsigned i = 0; i < iterations; i++) {
        printf("[APP] Starting iteration %d!\n", i);

		// Accelerator is implicitly ready because computation is chained
        init_buffer(&mem[mat_a_offset], &mem[mat_b_offset],
                    &gold[mat_a_offset], &gold[mat_b_offset], &gold[mat_c_offset]);
		// Inform the accelerator to start.
		atomic_flag_store(&input_flag, 1);

		// Wait for the accelerator to send output.
		start_counter();
		while(atomic_flag_load(&output_flag) != 1);
		t_acc += end_counter();
		errors += validate_buffer(&mem[mat_c_offset], &gold[mat_c_offset]);

		// Reset for next iteration.
		atomic_flag_store(&output_flag, 0);
    }

	// Wait for completion
	iowrite32(dev, CMD_REG, 0x0);

	aligned_free(ptable);
	aligned_free(mem);
	aligned_free(gold);

	printf("  Errors = %d\n", errors);
	printf("  Software = %lu\n", t_sw/iterations);
	printf("  Accel = %lu\n", t_acc/iterations);

	return 0;
}
