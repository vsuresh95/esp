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

#define SLD_GEMM_SM 0x053
#define DEV_NAME "sld,gemm_sm_stratus"
#define FX_IL 16
#define N_THREADS 2

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
#define GEMM_SM_CONTEXT_QUEUE_PTR_0		0x70
#define GEMM_SM_CONTEXT_QUEUE_PTR_1		0x74
#define GEMM_SM_CONTEXT_QUEUE_PTR_2		0x78
#define GEMM_SM_CONTEXT_QUEUE_PTR_3		0x7C
#define GEMM_SM_CONTEXT_NPRIO_0			0x80
#define GEMM_SM_CONTEXT_NPRIO_1			0x84
#define GEMM_SM_CONTEXT_NPRIO_2			0x88
#define GEMM_SM_CONTEXT_NPRIO_3			0x8C
#define GEMM_SM_VALID_CONTEXTS			0x90
#define GEMM_SM_SCHED_PERIOD			0x94

void gemm_sm(const float* mat_a, const float* mat_b, float* mat_c, unsigned dim_m, unsigned dim_n, unsigned dim_k) {
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
    gemm_sm(gold_a, gold_b, gold_c, dim_m, dim_n, dim_k);
}

int main(int argc, char * argv[])
{
	int i, n;
	int ndev;
	struct esp_device *espdevs;
	struct esp_device *dev;
	unsigned done;
	unsigned spin_ct;
	unsigned **ptable[N_THREADS] = {NULL};
	unsigned *mem[N_THREADS] = {NULL};
	float *gold[N_THREADS] = {NULL};
	unsigned errors[N_THREADS] = {0};
	unsigned coherence;

	printf("dim_m %u dim_n %u dim_k %u\n", dim_m, dim_n, dim_k);
    unsigned flag_len = PAYLOAD_OFFSET/sizeof(unsigned); // Number of nn_token_t elements reserved for flags
    unsigned mat_a_len = dim_m * dim_k;
    unsigned mat_b_len = dim_n * dim_k;
    unsigned mat_c_len = dim_m * dim_n;
    // Sync flag and data offsets
    unsigned mat_a_valid_offset = VALID_OFFSET;
    unsigned mat_a_offset = mat_a_valid_offset + flag_len;
    unsigned mat_b_offset = mat_a_offset + mat_a_len;
    unsigned mat_c_valid_offset = mat_b_offset + mat_b_len;
    unsigned mat_c_offset = mat_c_valid_offset + flag_len;
    unsigned input_queue_offset = mat_c_offset + mat_c_len;
    unsigned mem_size = N_THREADS * ((input_queue_offset) * sizeof(int) + sizeof(gemm_queue_t));
	// Flag offsets
	uint64_t *input_flag[N_THREADS] = {NULL};
	uint64_t *output_flag[N_THREADS] = {NULL};
	// SM queue pointers
	gemm_queue_t *q[N_THREADS] = {NULL};
	// Queue entry (same for all)
	gemm_queue_entry_t e;

	// Search for the device
	ndev = probe(&espdevs, VENDOR_SLD, SLD_GEMM_SM, DEV_NAME);
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
	for (i = 0; i < N_THREADS; i++) {
		gold[i] = aligned_malloc((mat_c_offset + mat_c_len) * sizeof(float));
		mem[i] = aligned_malloc(mem_size);

		// Allocate and populate page table
		ptable[i] = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
		for (n = 0; n < NCHUNK(mem_size); n++)
			ptable[i][n] = (unsigned *) &mem[i][n * (CHUNK_SIZE / sizeof(unsigned))];
		
		input_flag[i] = (uint64_t *) &mem[i][mat_a_valid_offset];
		__atomic_store_n(input_flag[i], 0, __ATOMIC_RELEASE);
		output_flag[i] = (uint64_t *) &mem[i][mat_c_valid_offset];
		__atomic_store_n(output_flag[i], 0, __ATOMIC_RELEASE);

		q[i] = (gemm_queue_t *) &mem[i][input_queue_offset];
		sm_queue_init((sm_queue_t *) q[i]);
	}
	// Prepare queue entry ahead of time
    e.gemm_params.dim_m = dim_m;
	e.gemm_params.dim_n = dim_n;
	e.gemm_params.dim_k = dim_k;
	e.gemm_params.weight_base = mat_b_offset;
	e.gemm_params.input_base = mat_a_valid_offset;
	e.gemm_params.output_base = mat_c_valid_offset;

	// Pass common configuration parameters
	coherence = ACC_COH_RECALL;
	iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	iowrite32(dev, COHERENCE_REG, coherence);

	iowrite32(dev, PT_ADDRESS_REG, (unsigned long long) ptable[0]);
	iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

	// Use the following if input and output data are not allocated at the default offsets
	iowrite32(dev, SRC_OFFSET_REG, 0x0);
	iowrite32(dev, DST_OFFSET_REG, 0x0);

	// Flush (customize coherence model here)
	esp_flush(coherence);

	// Configure first context
	iowrite32(dev, GEMM_SM_CONTEXT_QUEUE_PTR_0, input_queue_offset);
	iowrite32(dev, PT_ADDRESS_REG_0, (unsigned long long) ptable[0]);
	iowrite32(dev, GEMM_SM_CONTEXT_NPRIO_0, 1);
	iowrite32(dev, GEMM_SM_VALID_CONTEXTS, 0x1);
	iowrite32(dev, GEMM_SM_SCHED_PERIOD, 40000);
	// Start accelerators
	iowrite32(dev, CMD_REG, CMD_MASK_START);
	printf("First context configured\n");

	// Configure second context
	iowrite32(dev, GEMM_SM_CONTEXT_QUEUE_PTR_1, input_queue_offset);
	iowrite32(dev, PT_ADDRESS_REG_1, (unsigned long long) ptable[1]);
	iowrite32(dev, GEMM_SM_CONTEXT_NPRIO_1, 2);
	iowrite32(dev, GEMM_SM_VALID_CONTEXTS, 0x3);
	printf("Second context configured\n");

	unsigned t_id = 0;
	unsigned iterations[N_THREADS];
	iterations[0] = 20;
	iterations[1] = 200;

	unsigned inputs_remaining[N_THREADS];
	unsigned outputs_remaining[N_THREADS];
	unsigned input_tasks_remaining[N_THREADS];
	for (i = 0; i < N_THREADS; i++) {
		inputs_remaining[i] = iterations[i];
		outputs_remaining[i] = iterations[i];
		input_tasks_remaining[i] = iterations[i];
	}

    unsigned thread_status[N_THREADS];
    for (unsigned i = 0; i < N_THREADS; i++) {
        thread_status[i] = 0;
    }
    unsigned threads_done = 0;

	// Main processing loop
    while (threads_done < N_THREADS) {
		// Yield to other threads if applicable
		if (thread_status[t_id] == 1) {
			t_id = (t_id + 1) % N_THREADS;
			continue;
		}
		// Check if thread is done
		if (input_tasks_remaining[t_id] + inputs_remaining[t_id] + outputs_remaining[t_id] == 0) {
			threads_done++;
			thread_status[t_id] = 1;
			printf("Thread %d done\n", t_id);
			t_id = (t_id + 1) % N_THREADS;
			continue;
		}
		// Check if input queue is full
		if (input_tasks_remaining[t_id] > 0) {
			if (!gemm_queue_full(q[t_id])) {
				gemm_queue_push(q[(t_id)], &e);
				input_tasks_remaining[t_id]--;
			}
		}
		if (input_tasks_remaining[t_id] % 50 == 0 && input_tasks_remaining[t_id] > 0) {
			for (i = 0; i < 5; i++)
				printf("Thread %d: input tasks remaining %d\n", t_id, input_tasks_remaining[t_id]);
		}
        // Check if input queue is not empty and input data is invalid
		if (inputs_remaining[t_id] > 0) {
			bool input_is_invaid = (__atomic_load_n(input_flag[t_id], __ATOMIC_ACQUIRE) == 0);
			if (!gemm_queue_empty(q[t_id]) && input_is_invaid) {
				// Set input flag valid
				__atomic_store_n(input_flag[t_id], 1, __ATOMIC_RELEASE);
				inputs_remaining[t_id]--;
			}
		}
		// Check if output data is valid
		if (outputs_remaining[t_id] > 0) {
			bool output_is_valid = (__atomic_load_n(output_flag[t_id], __ATOMIC_ACQUIRE) == 1);
			if (output_is_valid) {
				// Reset for next iteration.
				__atomic_store_n(output_flag[t_id], 0, __ATOMIC_RELEASE);
				outputs_remaining[t_id]--;
			}
		}
		t_id = (t_id + 1) % N_THREADS;
	}

	for (i = 0; i < N_THREADS; i++) {
		printf("Freeing resources for thread %d\n", i);
		aligned_free(ptable[i]);
		aligned_free(mem[i]);
		aligned_free(gold[i]);
	}

	printf("  Errors = \n");
	for (i = 0; i < N_THREADS; i++)
		printf("%d, ", errors[i]);
	printf("\n");

	return 0;
}
