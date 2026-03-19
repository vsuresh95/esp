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

#include <math.h>

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}

#define SLD_GEMM 0x051
#define DEV_NAME "sld,gemm_stratus"
#define FX_IL 8

/* <<--params-->> */
const unsigned dim_m = 20;
const unsigned dim_n = 20;
const unsigned dim_k = 20;
const unsigned do_bias = 1;

/* Size of the contiguous chunks for scatter/gather */
#define CHUNK_SHIFT 20
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK(_sz) ((_sz % CHUNK_SIZE == 0) ?		\
			(_sz / CHUNK_SIZE) :		\
			(_sz / CHUNK_SIZE) + 1)

/* User defined registers */
/* <<--regs-->> */
#define GEMM_NINPUTS_REG    0x98
#define GEMM_D1_REG         0x9c
#define GEMM_D2_REG         0xA0
#define GEMM_D3_REG         0xA4
#define GEMM_LD_OFFSET1_REG 0xA8
#define GEMM_LD_OFFSET2_REG 0xAC
#define GEMM_BIAS_OFFSET_REG 0xB0
#define GEMM_ST_OFFSET_REG  0xB4
#define GEMM_DO_RELU_REG    0xB8
#define GEMM_DO_BIAS_REG    0xBC
#define GEMM_TRANSPOSE_REG  0xC0

void gemm(const float* mat_a, const float* mat_b, float* mat_c, const float *bias, unsigned dim_m, unsigned dim_n, unsigned dim_k) {
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
    if (do_bias) {
        for (unsigned m = 0; m < dim_m; m++) {
            for (unsigned n = 0; n < dim_n; n++) {
                mat_c[m * dim_n + n] += bias[n];
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
void init_buffer(int *mem_a, int *mem_b, int *mem_bias,
		 float *gold_a, float *gold_b, float *gold_bias, float *gold_c)
{
    const float LO = -1.0;
    const float HI = 1.0;
    const unsigned len_a = dim_m * dim_k;
    const unsigned len_b = dim_n * dim_k;
    const unsigned len_bias = dim_n;

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

    for (unsigned j = 0; j < len_bias; j++) {
        float scaling_factor = (float) rand() / (float) RAND_MAX;
        gold_bias[j] = LO + scaling_factor * (HI - LO);
        mem_bias[j] = float_to_fixed32(gold_bias[j], FX_IL);
    }

    // Compute golden output
    gemm(gold_a, gold_b, gold_c, gold_bias, dim_m, dim_n, dim_k);
}

#include "gemm_baseline.h"
#include "gemm_amu.h"

int main(int argc, char * argv[])
{
#if (ENABLE_AMU == 0)
	gemm_baseline();
#else
	gemm_amu();
#endif
	return 0;
}
