/* Copyright (c) 2011-2023 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include <fixed_point.h>
#include <stdbool.h>

/* User defined */

// Define data type (decomment the one needed)
// #define __UINT
// #define __INT
#define __FIXED
// #define __FLOAT

// Define bit width (decomment the one needed)
#ifndef __riscv
#define BITWIDTH 32
// #define BITWIDTH 64
#else
#define BITWIDTH 32
//#define BITWIDTH 64
#endif

/* End of user defined */

#ifdef __UINT
#if (BITWIDTH == 32)
typedef unsigned token_t;
#elif (BITWIDTH == 64)
typedef long long unsigned token_t;
#endif
#endif

#ifdef __INT
#if (BITWIDTH == 32)
typedef int token_t;
#elif (BITWIDTH == 64)
typedef long long token_t;
#endif
#endif

#ifdef __FIXED
#if (BITWIDTH == 32)
typedef int token_t;
#define fx2float fixed32_to_float
#define float2fx float_to_fixed32
#define FX_IL 16
#elif (BITWIDTH == 64)
typedef long long token_t;
#define fx2float fixed64_to_double
#define float2fx double_to_fixed64
#define FX_IL 32
#endif
#endif

#ifdef __FLOAT
#if (BITWIDTH == 32)
typedef float token_t;
#elif (BITWIDTH == 64)
typedef double token_t;
#endif
#endif

#if (BITWIDTH == 32)
typedef float native_t;
#elif (BITWIDTH == 64)
typedef double native_t;
#endif

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}

#define MAX_PRINTED_ERRORS 10
#define REL_ERROR_THRESHOLD 0.01
#define SLD_CONV2D 0x052
#define DEV_NAME "sld,conv2d_stratus"

/* <<--params-->> */
const int32_t n_channels = 2;
const int32_t feature_map_height = 6;
const int32_t feature_map_width = 6;
const int32_t n_filters = 2;
const int32_t filter_height = 3;
const int32_t filter_width = 3;
const int32_t pad_h = 1;
const int32_t pad_w = 1;
const int32_t is_padded = 1;
const int32_t stride_h = 1;
const int32_t stride_w = 1;
const int32_t dilation_h = 1;
const int32_t dilation_w = 1;
const int32_t do_relu = 0;
const int32_t pool_type = 0;
const int32_t batch_size = 1;

static unsigned in_words_adj;
static unsigned weights_words_adj;
static unsigned bias_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned weights_len;
static unsigned bias_len;
static unsigned out_len;
static unsigned in_size;
static unsigned weights_size;
static unsigned bias_size;
static unsigned out_size;
static unsigned weights_offset;
static unsigned bias_offset;
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
#define CONV2D_N_CHANNELS_REG 0x98
#define CONV2D_FEATURE_MAP_HEIGHT_REG 0x9C
#define CONV2D_FEATURE_MAP_WIDTH_REG 0xA0
#define CONV2D_N_FILTERS_REG 0xA4
#define CONV2D_FILTER_DIM_REG 0xA8
#define CONV2D_IS_PADDED_REG 0xAC
#define CONV2D_STRIDE_REG 0xB0
#define CONV2D_DO_RELU_REG 0xB4
#define CONV2D_POOL_TYPE_REG 0xB8
#define CONV2D_BATCH_SIZE_REG 0xBC

static int validate_buf(token_t *out, native_t *gold)
{
	int j;
	native_t val;
	unsigned errors = 0;

        for (j = 0; j < out_len; j++) {
#ifdef __FIXED
	    val = fx2float(out[j], FX_IL);
#else
            val = out[j];
#endif
            if (!gold[j] && val ||
		(((gold[j] - val) / gold[j]) > REL_ERROR_THRESHOLD ||
		((gold[j] - val) / gold[j]) < -REL_ERROR_THRESHOLD))
		{
                errors++;
                if (errors <= MAX_PRINTED_ERRORS) {
                    printf("%d : %d\n", (int) val, (int) gold[j]);
                }
            }
	}

	return errors;
}


static void init_buf (token_t *in, native_t * gold)
{
#include "input.h"
#include "gold.h"

	int i, size;
    size = (n_channels * feature_map_height * feature_map_width) +
	(n_channels * n_filters * filter_height * filter_width) +
	n_filters;
}

#include "conv2d_baseline.h"
#include "conv2d_amu.h"

int main(int argc, char * argv[])
{
#if (ENABLE_AMU == 0)
	return conv2d_baseline();
#else
	return conv2d_amu();
#endif
}
