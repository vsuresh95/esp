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
const unsigned dim_m = 10;
const unsigned dim_n = 8;
const unsigned dim_k = 10;

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

    // We will cast the synchronization flags from *mem to custom atomic flags
    atomic_flag_t input_flag0, input_flag1, input_flag2;
    atomic_flag_t output_flag0, output_flag1, output_flag2;
	atomic_flag_init(&input_flag0, (volatile uint64_t *) &mem0[mat_a_valid_offset]);
	atomic_flag_init(&output_flag0, (volatile uint64_t *) &mem0[mat_c_valid_offset]);
	atomic_flag_init(&input_flag1, (volatile uint64_t *) &mem1[mat_a_valid_offset]);
	atomic_flag_init(&output_flag1, (volatile uint64_t *) &mem1[mat_c_valid_offset]);
	atomic_flag_init(&input_flag2, (volatile uint64_t *) &mem2[mat_a_valid_offset]);
	atomic_flag_init(&output_flag2, (volatile uint64_t *) &mem2[mat_c_valid_offset]);

	// Pass common configuration parameters
	iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	iowrite32(dev, COHERENCE_REG, coherence);

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
	iowrite32(dev, GEMM_CONTEXT_BASE_PTR_0, stat_offset);
	iowrite32(dev, PT_ADDRESS_REG_0, (unsigned long long) ptable0);
	iowrite32(dev, GEMM_CONTEXT_NPRIO_0, 1);
	iowrite32(dev, GEMM_VALID_CONTEXTS, 0x1);
	iowrite32(dev, GEMM_SCHED_PERIOD, 0x100000);

	// Set up two descriptors: 1 GEMM, 1 JUMP
	mem0[stat_offset + 0] = 1;
	mem0[stat_offset + 1] = descr_offset;

	mem0[descr_offset + 0 * SM_INFO_SIZE + 0] = 1;
	mem0[descr_offset + 0 * SM_INFO_SIZE + 1] = dim_m;
	mem0[descr_offset + 0 * SM_INFO_SIZE + 2] = dim_n;
	mem0[descr_offset + 0 * SM_INFO_SIZE + 3] = dim_k;
	mem0[descr_offset + 0 * SM_INFO_SIZE + 4] = mat_b_offset;
	mem0[descr_offset + 0 * SM_INFO_SIZE + 5] = mat_a_valid_offset;
	mem0[descr_offset + 0 * SM_INFO_SIZE + 6] = mat_c_valid_offset;

	mem0[descr_offset + 1 * SM_INFO_SIZE + 0] = 2;
	mem0[descr_offset + 1 * SM_INFO_SIZE + 1] = descr_offset;

	printf("First context configured\n");

	// Start accelerators
	iowrite32(dev, CMD_REG, CMD_MASK_START);

	///////////////////////////////////////////////////////
	/// Send first context task
	///////////////////////////////////////////////////////
	// Wait for the accelerator to be ready
	while(atomic_flag_load(&input_flag0) != 0);
	// When the accelerator is ready, we write the input data to it
	init_buffer(&mem0[mat_a_offset], &mem0[mat_b_offset],
				&gold0[mat_a_offset], &gold0[mat_b_offset], &gold0[mat_c_offset]);
	// Inform the accelerator to start.
	atomic_flag_store(&input_flag0, 1);

	printf("First context task sent\n");

	printf("PT_ADDRESS_REG_0 = %x\n", ioread32(dev, PT_ADDRESS_REG_0));

	for (i = 0; i < 3; i++) {
		iowrite32(dev, PT_ADDRESS_REG_0, (unsigned long long) ptable1);
		printf("PT_ADDRESS_REG_0 = %x\n", ioread32(dev, PT_ADDRESS_REG_0));
	}

	///////////////////////////////////////////////////////
	/// Configure second context
	///////////////////////////////////////////////////////
	iowrite32(dev, GEMM_CONTEXT_BASE_PTR_1, stat_offset);
	iowrite32(dev, PT_ADDRESS_REG_1, (unsigned long long) ptable1);
	iowrite32(dev, GEMM_CONTEXT_NPRIO_1, 1);
	iowrite32(dev, GEMM_VALID_CONTEXTS, 0x3);

	// Set up two descriptors: 1 GEMM, 1 JUMP
	mem1[stat_offset + 0] = 1;
	mem1[stat_offset + 1] = descr_offset;

	mem1[descr_offset + 0 * SM_INFO_SIZE + 0] = 1;
	mem1[descr_offset + 0 * SM_INFO_SIZE + 1] = dim_m;
	mem1[descr_offset + 0 * SM_INFO_SIZE + 2] = dim_n;
	mem1[descr_offset + 0 * SM_INFO_SIZE + 3] = dim_k;
	mem1[descr_offset + 0 * SM_INFO_SIZE + 4] = mat_b_offset;
	mem1[descr_offset + 0 * SM_INFO_SIZE + 5] = mat_a_valid_offset;
	mem1[descr_offset + 0 * SM_INFO_SIZE + 6] = mat_c_valid_offset;

	mem1[descr_offset + 1 * SM_INFO_SIZE + 0] = 2;
	mem1[descr_offset + 1 * SM_INFO_SIZE + 1] = descr_offset;

	printf("Second context configured\n");

	///////////////////////////////////////////////////////
	/// Get first context output
	///////////////////////////////////////////////////////
	while(atomic_flag_load(&output_flag0) != 1);
	errors0 += validate_buffer(&mem0[mat_c_offset], &gold0[mat_c_offset]);

	// Reset for next iteration.
	atomic_flag_store(&output_flag0, 0);

	printf("First context task done\n");

	///////////////////////////////////////////////////////
	/// Send second context task
	///////////////////////////////////////////////////////
	// Wait for the accelerator to be ready
	while(atomic_flag_load(&input_flag1) != 0);
	// When the accelerator is ready, we write the input data to it
	init_buffer(&mem1[mat_a_offset], &mem1[mat_b_offset],
				&gold1[mat_a_offset], &gold1[mat_b_offset], &gold1[mat_c_offset]);
	// Inform the accelerator to start.
	atomic_flag_store(&input_flag1, 1);

	printf("Second context task sent\n");

	///////////////////////////////////////////////////////
	/// Configure third context
	///////////////////////////////////////////////////////
	iowrite32(dev, GEMM_CONTEXT_BASE_PTR_2, stat_offset);
	iowrite32(dev, PT_ADDRESS_REG_2, (unsigned long long) ptable2);
	iowrite32(dev, GEMM_CONTEXT_NPRIO_2, 1);
	iowrite32(dev, GEMM_VALID_CONTEXTS, 0x7);

	// Set up two descriptors: 1 GEMM, 1 JUMP
	mem2[stat_offset + 0] = 1;
	mem2[stat_offset + 1] = descr_offset;

	mem2[descr_offset + 0 * SM_INFO_SIZE + 0] = 1;
	mem2[descr_offset + 0 * SM_INFO_SIZE + 1] = dim_m;
	mem2[descr_offset + 0 * SM_INFO_SIZE + 2] = dim_n;
	mem2[descr_offset + 0 * SM_INFO_SIZE + 3] = dim_k;
	mem2[descr_offset + 0 * SM_INFO_SIZE + 4] = mat_b_offset;
	mem2[descr_offset + 0 * SM_INFO_SIZE + 5] = mat_a_valid_offset;
	mem2[descr_offset + 0 * SM_INFO_SIZE + 6] = mat_c_valid_offset;

	mem2[descr_offset + 1 * SM_INFO_SIZE + 0] = 2;
	mem2[descr_offset + 1 * SM_INFO_SIZE + 1] = descr_offset;

	printf("Third context configured\n");

	///////////////////////////////////////////////////////
	/// Send first context task
	///////////////////////////////////////////////////////
	// Wait for the accelerator to be ready
	while(atomic_flag_load(&input_flag0) != 0);
	// When the accelerator is ready, we write the input data to it
	init_buffer(&mem0[mat_a_offset], &mem0[mat_b_offset],
				&gold0[mat_a_offset], &gold0[mat_b_offset], &gold0[mat_c_offset]);
	// Inform the accelerator to start.
	atomic_flag_store(&input_flag0, 1);

	printf("First context task sent\n");

	///////////////////////////////////////////////////////
	/// Get second context output
	///////////////////////////////////////////////////////
	while(atomic_flag_load(&output_flag1) != 1);
	errors1 += validate_buffer(&mem1[mat_c_offset], &gold1[mat_c_offset]);

	// Reset for next iteration.
	atomic_flag_store(&output_flag1, 0);

	printf("Second context task done\n");

	///////////////////////////////////////////////////////
	/// Get first context output
	///////////////////////////////////////////////////////
	while(atomic_flag_load(&output_flag0) != 1);
	errors0 += validate_buffer(&mem0[mat_c_offset], &gold0[mat_c_offset]);

	// Reset for next iteration.
	atomic_flag_store(&output_flag0, 0);

	printf("First context task done\n");

	///////////////////////////////////////////////////////
	/// New test
	///////////////////////////////////////////////////////
	printf("-----------------------\n");	

	///////////////////////////////////////////////////////
	/// Send first context task
	///////////////////////////////////////////////////////
	// Wait for the accelerator to be ready
	while(atomic_flag_load(&input_flag0) != 0);
	// When the accelerator is ready, we write the input data to it
	init_buffer(&mem0[mat_a_offset], &mem0[mat_b_offset],
				&gold0[mat_a_offset], &gold0[mat_b_offset], &gold0[mat_c_offset]);
	// Inform the accelerator to start.
	atomic_flag_store(&input_flag0, 1);

	printf("First context task sent\n");

	///////////////////////////////////////////////////////
	/// Send second context task
	///////////////////////////////////////////////////////
	// Wait for the accelerator to be ready
	while(atomic_flag_load(&input_flag1) != 0);
	// When the accelerator is ready, we write the input data to it
	init_buffer(&mem1[mat_a_offset], &mem1[mat_b_offset],
				&gold1[mat_a_offset], &gold1[mat_b_offset], &gold1[mat_c_offset]);
	// Inform the accelerator to start.
	atomic_flag_store(&input_flag1, 1);

	printf("Second context task sent\n");

	///////////////////////////////////////////////////////
	/// Send third context task
	///////////////////////////////////////////////////////
	// Wait for the accelerator to be ready
	while(atomic_flag_load(&input_flag2) != 0);
	// When the accelerator is ready, we write the input data to it
	init_buffer(&mem2[mat_a_offset], &mem2[mat_b_offset],
				&gold2[mat_a_offset], &gold2[mat_b_offset], &gold2[mat_c_offset]);
	// Inform the accelerator to start.
	atomic_flag_store(&input_flag2, 1);

	printf("Third context task sent\n");

	///////////////////////////////////////////////////////
	/// Get all contexts output
	///////////////////////////////////////////////////////
	// Check for the accelerator to send output
	bool context_0_done = false;
	bool context_1_done = false;
	bool context_2_done = false;
	while (!(context_0_done & context_1_done & context_2_done)) {
		bool context_0_ready = (atomic_flag_load(&output_flag0) == 1);
		bool context_1_ready = (atomic_flag_load(&output_flag1) == 1);
		bool context_2_ready = (atomic_flag_load(&output_flag2) == 1);

		if (context_0_ready) {
			// When the output is ready, we read it
			errors0 += validate_buffer(&mem0[mat_c_offset], &gold0[mat_c_offset]);

			// Reset for next iteration.
			atomic_flag_store(&output_flag0, 0);

			context_0_done = true;

			printf("First context task done\n");
		} else if (context_1_ready) {
			// When the output is ready, we read it
			errors1 += validate_buffer(&mem1[mat_c_offset], &gold1[mat_c_offset]);

			// Reset for next iteration.
			atomic_flag_store(&output_flag1, 0);

			context_1_done = true;

			printf("Second context task done\n");
		} else if (context_2_ready) {
			// When the output is ready, we read it
			errors2 += validate_buffer(&mem2[mat_c_offset], &gold2[mat_c_offset]);

			// Reset for next iteration.
			atomic_flag_store(&output_flag2, 0);

			context_2_done = true;

			printf("Third context task done\n");
		}
	}
	
	for (i = 0; i < 3; i++) {
		printf("MON_UTIL_REG_%d_LO = %x\n", i, ioread32(dev, MON_UTIL_REG_0_LO + 0x8*i));
		printf("MON_UTIL_REG_%d_HI = %x\n", i, ioread32(dev, MON_UTIL_REG_0_HI + 0x8*i));
	}

	iowrite32(dev, GEMM_VALID_CONTEXTS, 0);
	while(ioread32(dev, VALID_CONTEXTS_ACK_REG) != 0);
	iowrite32(dev, CMD_REG, 0x0);

	// Reset all sync variables to default values.
	atomic_flag_store(&input_flag0, 0);
	atomic_flag_store(&output_flag0, 0);
	atomic_flag_store(&input_flag1, 0);
	atomic_flag_store(&output_flag1, 0);
	atomic_flag_store(&input_flag2, 0);
	atomic_flag_store(&output_flag2, 0);

	///////////////////////////////////////////////////////
	/// New test
	///////////////////////////////////////////////////////
	printf("-----------------------\n");	

	// Initialize registers of accelerator and start it.
	iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	iowrite32(dev, COHERENCE_REG, coherence);

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
	iowrite32(dev, GEMM_CONTEXT_BASE_PTR_0, stat_offset);
	iowrite32(dev, PT_ADDRESS_REG_0, (unsigned long long) ptable0);
	iowrite32(dev, GEMM_CONTEXT_NPRIO_0, 1);
	iowrite32(dev, GEMM_VALID_CONTEXTS, 0x1);
	iowrite32(dev, GEMM_SCHED_PERIOD, 0x100000);

	// Set context as available
	mem0[stat_offset + 0] = 1;

	printf("First context configured\n");

	// Start accelerator
	iowrite32(dev, CMD_REG, CMD_MASK_START);

	///////////////////////////////////////////////////////
	/// Configure second context
	///////////////////////////////////////////////////////
	iowrite32(dev, GEMM_CONTEXT_BASE_PTR_1, stat_offset);
	iowrite32(dev, PT_ADDRESS_REG_1, (unsigned long long) ptable1);
	iowrite32(dev, GEMM_CONTEXT_NPRIO_1, 1);
	iowrite32(dev, GEMM_VALID_CONTEXTS, 0x3);

	// Set context as available
	mem1[stat_offset + 0] = 1;

	printf("Second context configured\n");

	///////////////////////////////////////////////////////
	/// Configure third context
	///////////////////////////////////////////////////////
	iowrite32(dev, GEMM_CONTEXT_BASE_PTR_2, stat_offset);
	iowrite32(dev, PT_ADDRESS_REG_2, (unsigned long long) ptable2);
	iowrite32(dev, GEMM_CONTEXT_NPRIO_2, 1);
	iowrite32(dev, GEMM_VALID_CONTEXTS, 0x7);

	// Set context as available
	mem2[stat_offset + 0] = 1;

	printf("Third context configured\n");

	///////////////////////////////////////////////////////
	/// Send first context task
	///////////////////////////////////////////////////////
	// Wait for the accelerator to be ready
	while(atomic_flag_load(&input_flag0) != 0);
	// When the accelerator is ready, we write the input data to it
	init_buffer(&mem0[mat_a_offset], &mem0[mat_b_offset],
				&gold0[mat_a_offset], &gold0[mat_b_offset], &gold0[mat_c_offset]);
	// Inform the accelerator to start.
	atomic_flag_store(&input_flag0, 1);

	printf("First context task sent\n");	

	///////////////////////////////////////////////////////
	/// Send second context task
	///////////////////////////////////////////////////////
	// Wait for the accelerator to be ready
	while(atomic_flag_load(&input_flag1) != 0);
	// When the accelerator is ready, we write the input data to it
	init_buffer(&mem1[mat_a_offset], &mem1[mat_b_offset],
				&gold1[mat_a_offset], &gold1[mat_b_offset], &gold1[mat_c_offset]);
	// Inform the accelerator to start.
	atomic_flag_store(&input_flag1, 1);

	printf("Second context task sent\n");

	///////////////////////////////////////////////////////
	/// Send third context task
	///////////////////////////////////////////////////////
	// Wait for the accelerator to be ready
	while(atomic_flag_load(&input_flag2) != 0);
	// When the accelerator is ready, we write the input data to it
	init_buffer(&mem2[mat_a_offset], &mem2[mat_b_offset],
				&gold2[mat_a_offset], &gold2[mat_b_offset], &gold2[mat_c_offset]);
	// Inform the accelerator to start.
	atomic_flag_store(&input_flag2, 1);

	printf("Third context task sent\n");	

	///////////////////////////////////////////////////////
	/// Get first context output
	///////////////////////////////////////////////////////
	while(atomic_flag_load(&output_flag0) != 1);
	errors0 += validate_buffer(&mem0[mat_c_offset], &gold0[mat_c_offset]);

	// Reset for next iteration.
	atomic_flag_store(&output_flag0, 0);

	printf("First context task done\n");	

	///////////////////////////////////////////////////////
	/// Get second context output
	///////////////////////////////////////////////////////
	while(atomic_flag_load(&output_flag1) != 1);
	errors1 += validate_buffer(&mem1[mat_c_offset], &gold1[mat_c_offset]);

	// Reset for next iteration.
	atomic_flag_store(&output_flag1, 0);

	printf("Second context task done\n");

	///////////////////////////////////////////////////////
	/// Get third context output
	///////////////////////////////////////////////////////
	while(atomic_flag_load(&output_flag2) != 1);
	errors2 += validate_buffer(&mem2[mat_c_offset], &gold2[mat_c_offset]);

	// Reset for next iteration.
	atomic_flag_store(&output_flag2, 0);

	printf("Third context task done\n");

	for (i = 0; i < 3; i++) {
		printf("MON_UTIL_REG_%d_LO = %x\n", i, ioread32(dev, MON_UTIL_REG_0_LO + 0x8*i));
		printf("MON_UTIL_REG_%d_HI = %x\n", i, ioread32(dev, MON_UTIL_REG_0_HI + 0x8*i));
	}

	///////////////////////////////////////////////////////
	/// New test
	///////////////////////////////////////////////////////
	printf("-----------------------\n");	

	///////////////////////////////////////////////////////
	/// Delete second context
	///////////////////////////////////////////////////////
	iowrite32(dev, GEMM_VALID_CONTEXTS, 0x5);
	while(ioread32(dev, VALID_CONTEXTS_ACK_REG) != 0x5);

	printf("Second context deleted\n");

	///////////////////////////////////////////////////////
	/// Send first context task
	///////////////////////////////////////////////////////
	// Wait for the accelerator to be ready
	while(atomic_flag_load(&input_flag0) != 0);
	// When the accelerator is ready, we write the input data to it
	init_buffer(&mem0[mat_a_offset], &mem0[mat_b_offset],
				&gold0[mat_a_offset], &gold0[mat_b_offset], &gold0[mat_c_offset]);
	// Inform the accelerator to start.
	atomic_flag_store(&input_flag0, 1);

	printf("First context task sent\n");	

	///////////////////////////////////////////////////////
	/// Send third context task
	///////////////////////////////////////////////////////
	// Wait for the accelerator to be ready
	while(atomic_flag_load(&input_flag2) != 0);
	// When the accelerator is ready, we write the input data to it
	init_buffer(&mem2[mat_a_offset], &mem2[mat_b_offset],
				&gold2[mat_a_offset], &gold2[mat_b_offset], &gold2[mat_c_offset]);
	// Inform the accelerator to start.
	atomic_flag_store(&input_flag2, 1);

	printf("Third context task sent\n");	

	///////////////////////////////////////////////////////
	/// Get first context output
	///////////////////////////////////////////////////////
	while(atomic_flag_load(&output_flag0) != 1);
	errors0 += validate_buffer(&mem0[mat_c_offset], &gold0[mat_c_offset]);

	// Reset for next iteration.
	atomic_flag_store(&output_flag0, 0);

	printf("First context task done\n");	

	///////////////////////////////////////////////////////
	/// Get third context output
	///////////////////////////////////////////////////////
	while(atomic_flag_load(&output_flag2) != 1);
	errors2 += validate_buffer(&mem2[mat_c_offset], &gold2[mat_c_offset]);

	// Reset for next iteration.
	atomic_flag_store(&output_flag2, 0);

	printf("Third context task done\n");

	for (i = 0; i < 3; i++) {
		printf("MON_UTIL_REG_%d_LO = %x\n", i, ioread32(dev, MON_UTIL_REG_0_LO + 0x8*i));
		printf("MON_UTIL_REG_%d_HI = %x\n", i, ioread32(dev, MON_UTIL_REG_0_HI + 0x8*i));
	}

	///////////////////////////////////////////////////////
	/// New test
	///////////////////////////////////////////////////////
	printf("-----------------------\n");	

	///////////////////////////////////////////////////////
	/// Configure second context
	///////////////////////////////////////////////////////
	iowrite32(dev, GEMM_CONTEXT_BASE_PTR_3, stat_offset);
	iowrite32(dev, PT_ADDRESS_REG_3, (unsigned long long) ptable1);
	iowrite32(dev, GEMM_CONTEXT_NPRIO_3, 1);
	iowrite32(dev, GEMM_VALID_CONTEXTS, 0xD);

	printf("Second context configured\n");

	///////////////////////////////////////////////////////
	/// Send first context task
	///////////////////////////////////////////////////////
	// Wait for the accelerator to be ready
	while(atomic_flag_load(&input_flag0) != 0);
	// When the accelerator is ready, we write the input data to it
	init_buffer(&mem0[mat_a_offset], &mem0[mat_b_offset],
				&gold0[mat_a_offset], &gold0[mat_b_offset], &gold0[mat_c_offset]);
	// Inform the accelerator to start.
	atomic_flag_store(&input_flag0, 1);

	printf("First context task sent\n");	

	///////////////////////////////////////////////////////
	/// Send first context task
	///////////////////////////////////////////////////////
	// Wait for the accelerator to be ready
	while(atomic_flag_load(&input_flag0) != 0);
	// When the accelerator is ready, we write the input data to it
	init_buffer(&mem0[mat_a_offset], &mem0[mat_b_offset],
				&gold0[mat_a_offset], &gold0[mat_b_offset], &gold0[mat_c_offset]);
	// Inform the accelerator to start.
	atomic_flag_store(&input_flag0, 1);

	printf("First context task sent\n");	

	///////////////////////////////////////////////////////
	/// Send second context task
	///////////////////////////////////////////////////////
	// Wait for the accelerator to be ready
	while(atomic_flag_load(&input_flag1) != 0);
	// When the accelerator is ready, we write the input data to it
	init_buffer(&mem1[mat_a_offset], &mem1[mat_b_offset],
				&gold1[mat_a_offset], &gold1[mat_b_offset], &gold1[mat_c_offset]);
	// Inform the accelerator to start.
	atomic_flag_store(&input_flag1, 1);

	printf("Second context task sent\n");

	///////////////////////////////////////////////////////
	/// Send third context task
	///////////////////////////////////////////////////////
	// Wait for the accelerator to be ready
	while(atomic_flag_load(&input_flag2) != 0);
	// When the accelerator is ready, we write the input data to it
	init_buffer(&mem2[mat_a_offset], &mem2[mat_b_offset],
				&gold2[mat_a_offset], &gold2[mat_b_offset], &gold2[mat_c_offset]);
	// Inform the accelerator to start.
	atomic_flag_store(&input_flag2, 1);

	printf("Third context task sent\n");

	for (int i = 0; i < 10; i++) {
		printf("Idle loop...\n");
	}

	///////////////////////////////////////////////////////
	/// Get first context output
	///////////////////////////////////////////////////////
	while(atomic_flag_load(&output_flag0) != 1);
	errors0 += validate_buffer(&mem0[mat_c_offset], &gold0[mat_c_offset]);

	// Reset for next iteration.
	atomic_flag_store(&output_flag0, 0);

	printf("First context task done\n");	

	///////////////////////////////////////////////////////
	/// Get second context output
	///////////////////////////////////////////////////////
	while(atomic_flag_load(&output_flag1) != 1);
	errors1 += validate_buffer(&mem1[mat_c_offset], &gold1[mat_c_offset]);

	// Reset for next iteration.
	atomic_flag_store(&output_flag1, 0);

	printf("Second context task done\n");

	///////////////////////////////////////////////////////
	/// Get third context output
	///////////////////////////////////////////////////////
	while(atomic_flag_load(&output_flag2) != 1);
	errors2 += validate_buffer(&mem2[mat_c_offset], &gold2[mat_c_offset]);

	// Reset for next iteration.
	atomic_flag_store(&output_flag2, 0);

	printf("Third context task done\n");

	///////////////////////////////////////////////////////
	/// Get first context output
	///////////////////////////////////////////////////////
	while(atomic_flag_load(&output_flag0) != 1);
	errors0 += validate_buffer(&mem0[mat_c_offset], &gold0[mat_c_offset]);

	// Reset for next iteration.
	atomic_flag_store(&output_flag0, 0);

	printf("First context task done\n");	
	
	for (i = 0; i < 3; i++) {
		printf("MON_UTIL_REG_%d_LO = %x\n", i, ioread32(dev, MON_UTIL_REG_0_LO + 0x8*i));
		printf("MON_UTIL_REG_%d_HI = %x\n", i, ioread32(dev, MON_UTIL_REG_0_HI + 0x8*i));
	}

	///////////////////////////////////////////////////////
	/// New test
	///////////////////////////////////////////////////////
	printf("-----------------------\n");	

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

	return 0;
}
