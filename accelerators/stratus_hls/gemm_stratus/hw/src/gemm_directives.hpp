// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __GEMM_DIRECTIVES_HPP__
#define __GEMM_DIRECTIVES_HPP__

// Macros
#if defined(STRATUS_HLS)

#if defined(HLS_DIRECTIVES_BASIC)

#define HLS_MAP_plm(_mem, _plm_block_name)      \
    HLS_MAP_TO_MEMORY(_mem, _plm_block_name)

#if (DMA_WIDTH == 64)
#if (WORD_SIZE == 32)

#if (DMA_CHUNK == 8)
#define OUT_DMA_CHUNK 8
#define OUT_PLM_NAME "plm_w32_d64_chk8"
#if (PARALLELISM == 1)
#define IN_PLM_NAME "plm_w32_d64_chk8_p1"
#elif (PARALLELISM == 4)
#define IN_PLM_NAME "plm_w32_d64_chk8_p4"
#elif (PARALLELISM == 8)
#define IN_PLM_NAME "plm_w32_d64_chk8_p8"
#else // (PARALLELISM == 16)
#define IN_PLM_NAME "plm_w32_d64_chk8_p16"
#endif

#elif (DMA_CHUNK == 16)
#define OUT_DMA_CHUNK 16
#define OUT_PLM_NAME "plm_w32_d64_chk16"
#if (PARALLELISM == 1)
#define IN_PLM_NAME "plm_w32_d64_chk16_p1"
#elif (PARALLELISM == 4)
#define IN_PLM_NAME "plm_w32_d64_chk16_p4"
#elif (PARALLELISM == 8)
#define IN_PLM_NAME "plm_w32_d64_chk16_p8"
#else // (PARALLELISM == 16)
#define IN_PLM_NAME "plm_w32_d64_chk16_p16"
#endif

#elif (DMA_CHUNK == 32)
#define OUT_DMA_CHUNK 32
#define OUT_PLM_NAME "plm_w32_d64_chk32"
#if (PARALLELISM == 1)
#define IN_PLM_NAME "plm_w32_d64_chk32_p1"
#elif (PARALLELISM == 4)
#define IN_PLM_NAME "plm_w32_d64_chk32_p4"
#elif (PARALLELISM == 8)
#define IN_PLM_NAME "plm_w32_d64_chk32_p8"
#else // (PARALLELISM == 16)
#define IN_PLM_NAME "plm_w32_d64_chk32_p16"
#endif

#elif (DMA_CHUNK == 64)
#define OUT_DMA_CHUNK 64
#define OUT_PLM_NAME "plm_w32_d64_chk64"
#if (PARALLELISM == 1)
#define IN_PLM_NAME "plm_w32_d64_chk64_p1"
#elif (PARALLELISM == 4)
#define IN_PLM_NAME "plm_w32_d64_chk64_p4"
#elif (PARALLELISM == 8)
#define IN_PLM_NAME "plm_w32_d64_chk64_p8"
#else // (PARALLELISM == 16)
#define IN_PLM_NAME "plm_w32_d64_chk64_p16"
#endif

#elif (DMA_CHUNK == 128)
#define OUT_DMA_CHUNK 128
#define OUT_PLM_NAME "plm_w32_d64_chk128"
#if (PARALLELISM == 1)
#define IN_PLM_NAME "plm_w32_d64_chk128_p1"
#elif (PARALLELISM == 4)
#define IN_PLM_NAME "plm_w32_d64_chk128_p4"
#elif (PARALLELISM == 8)
#define IN_PLM_NAME "plm_w32_d64_chk128_p8"
#else // (PARALLELISM == 16)
#define IN_PLM_NAME "plm_w32_d64_chk128_p16"
#endif

#elif (DMA_CHUNK == 512)
#define OUT_DMA_CHUNK 512
#define OUT_PLM_NAME "plm_w32_d64_chk512"
#if (PARALLELISM == 1)
#define IN_PLM_NAME "plm_w32_d64_chk512_p1"
#elif (PARALLELISM == 4)
#define IN_PLM_NAME "plm_w32_d64_chk512_p4"
#elif (PARALLELISM == 8)
#define IN_PLM_NAME "plm_w32_d64_chk512_p8"
#else // (PARALLELISM == 16)
#define IN_PLM_NAME "plm_w32_d64_chk512_p16"
#endif

#elif (DMA_CHUNK == 2048)
#define OUT_DMA_CHUNK 256
#define OUT_PLM_NAME "plm_w32_d64_chk256"
#if (PARALLELISM == 1)
#define IN_PLM_NAME "plm_w32_d64_chk2048_p1"
#elif (PARALLELISM == 4)
#define IN_PLM_NAME "plm_w32_d64_chk2048_p4"
#elif (PARALLELISM == 8)
#define IN_PLM_NAME "plm_w32_d64_chk2048_p8"
#else // (PARALLELISM == 16)
#define IN_PLM_NAME "plm_w32_d64_chk2048_p16"
#endif

#elif (DMA_CHUNK == 4096)
#define OUT_DMA_CHUNK 256
#define OUT_PLM_NAME "plm_w32_d64_chk256"
#if (PARALLELISM == 1)
#define IN_PLM_NAME "plm_w32_d64_chk4096_p1"
#elif (PARALLELISM == 4)
#define IN_PLM_NAME "plm_w32_d64_chk4096_p4"
#elif (PARALLELISM == 8)
#define IN_PLM_NAME "plm_w32_d64_chk4096_p8"
#else // (PARALLELISM == 16)
#define IN_PLM_NAME "plm_w32_d64_chk4096_p16"
#endif

#else // (DMA_CHUNK == 8192)
#define OUT_DMA_CHUNK 512
#define OUT_PLM_NAME "plm_w32_d64_chk512"
#if (PARALLELISM == 1)
#define IN_PLM_NAME "plm_w32_d64_chk8192_p1"
#elif (PARALLELISM == 2)
#define IN_PLM_NAME "plm_w32_d64_chk8192_p2"
#elif (PARALLELISM == 4)
#define IN_PLM_NAME "plm_w32_d64_chk8192_p4"
#elif (PARALLELISM == 8)
#define IN_PLM_NAME "plm_w32_d64_chk8192_p8"
#else // (PARALLELISM == 16)
#define IN_PLM_NAME "plm_w32_d64_chk8192_p16"
#endif

#endif

#endif
#endif

#if (DMA_WIDTH == 32)
#if (WORD_SIZE == 32)

#if (DMA_CHUNK == 8)
#define OUT_DMA_CHUNK 8
#define OUT_PLM_NAME "plm_w32_d32_chk8"
#if (PARALLELISM == 1)
#define IN_PLM_NAME "plm_w32_d32_chk8_p1"
#elif (PARALLELISM == 4)
#define IN_PLM_NAME "plm_w32_d32_chk8_p4"
#elif (PARALLELISM == 8)
#define IN_PLM_NAME "plm_w32_d32_chk8_p8"
#else // (PARALLELISM == 16)
#define IN_PLM_NAME "plm_w32_d32_chk8_p16"
#endif

#elif (DMA_CHUNK == 16)
#define OUT_DMA_CHUNK 16
#define OUT_PLM_NAME "plm_w32_d32_chk16"
#if (PARALLELISM == 1)
#define IN_PLM_NAME "plm_w32_d32_chk16_p1"
#elif (PARALLELISM == 4)
#define IN_PLM_NAME "plm_w32_d32_chk16_p4"
#elif (PARALLELISM == 8)
#define IN_PLM_NAME "plm_w32_d32_chk16_p8"
#else // (PARALLELISM == 16)
#define IN_PLM_NAME "plm_w32_d32_chk16_p16"
#endif

#elif (DMA_CHUNK == 32)
#define OUT_DMA_CHUNK 32
#define OUT_PLM_NAME "plm_w32_d32_chk32"
#if (PARALLELISM == 1)
#define IN_PLM_NAME "plm_w32_d32_chk32_p1"
#elif (PARALLELISM == 4)
#define IN_PLM_NAME "plm_w32_d32_chk32_p4"
#elif (PARALLELISM == 8)
#define IN_PLM_NAME "plm_w32_d32_chk32_p8"
#else // (PARALLELISM == 16)
#define IN_PLM_NAME "plm_w32_d32_chk32_p16"
#endif

#elif (DMA_CHUNK == 64)
#define OUT_DMA_CHUNK 64
#define OUT_PLM_NAME "plm_w32_d32_chk64"
#if (PARALLELISM == 1)
#define IN_PLM_NAME "plm_w32_d32_chk64_p1"
#elif (PARALLELISM == 4)
#define IN_PLM_NAME "plm_w32_d32_chk64_p4"
#elif (PARALLELISM == 8)
#define IN_PLM_NAME "plm_w32_d32_chk64_p8"
#else // (PARALLELISM == 16)
#define IN_PLM_NAME "plm_w32_d32_chk64_p16"
#endif

#elif (DMA_CHUNK == 128)
#define OUT_DMA_CHUNK 128
#define OUT_PLM_NAME "plm_w32_d32_chk128"
#if (PARALLELISM == 1)
#define IN_PLM_NAME "plm_w32_d32_chk128_p1"
#elif (PARALLELISM == 4)
#define IN_PLM_NAME "plm_w32_d32_chk128_p4"
#elif (PARALLELISM == 8)
#define IN_PLM_NAME "plm_w32_d32_chk128_p8"
#else // (PARALLELISM == 16)
#define IN_PLM_NAME "plm_w32_d32_chk128_p16"
#endif

#elif (DMA_CHUNK == 512)
#define OUT_DMA_CHUNK 512
#define OUT_PLM_NAME "plm_w32_d32_chk512"
#if (PARALLELISM == 1)
#define IN_PLM_NAME "plm_w32_d32_chk512_p1"
#elif (PARALLELISM == 4)
#define IN_PLM_NAME "plm_w32_d32_chk512_p4"
#elif (PARALLELISM == 8)
#define IN_PLM_NAME "plm_w32_d32_chk512_p8"
#else // (PARALLELISM == 16)
#define IN_PLM_NAME "plm_w32_d32_chk512_p16"
#endif

#elif (DMA_CHUNK == 2048)
#define OUT_DMA_CHUNK 256
#define OUT_PLM_NAME "plm_w32_d32_chk256"
#if (PARALLELISM == 1)
#define IN_PLM_NAME "plm_w32_d32_chk2048_p1"
#elif (PARALLELISM == 4)
#define IN_PLM_NAME "plm_w32_d32_chk2048_p4"
#elif (PARALLELISM == 8)
#define IN_PLM_NAME "plm_w32_d32_chk2048_p8"
#else // (PARALLELISM == 16)
#define IN_PLM_NAME "plm_w32_d32_chk2048_p16"
#endif

#elif (DMA_CHUNK == 4096)
#define OUT_DMA_CHUNK 256
#define OUT_PLM_NAME "plm_w32_d32_chk256"
#if (PARALLELISM == 1)
#define IN_PLM_NAME "plm_w32_d32_chk4096_p1"
#elif (PARALLELISM == 4)
#define IN_PLM_NAME "plm_w32_d32_chk4096_p4"
#elif (PARALLELISM == 8)
#define IN_PLM_NAME "plm_w32_d32_chk4096_p8"
#else // (PARALLELISM == 16)
#define IN_PLM_NAME "plm_w32_d32_chk4096_p16"
#endif

#else // (DMA_CHUNK == 8192)
#define OUT_DMA_CHUNK 512
#define OUT_PLM_NAME "plm_w32_d32_chk512"
#if (PARALLELISM == 1)
#define IN_PLM_NAME "plm_w32_d32_chk8192_p1"
#elif (PARALLELISM == 2)
#define IN_PLM_NAME "plm_w32_d32_chk8192_p2"
#elif (PARALLELISM == 4)
#define IN_PLM_NAME "plm_w32_d32_chk8192_p4"
#elif (PARALLELISM == 8)
#define IN_PLM_NAME "plm_w32_d32_chk8192_p8"
#else // (PARALLELISM == 16)
#define IN_PLM_NAME "plm_w32_d32_chk8192_p16"
#endif

#endif

#endif
#endif


#endif // HLS_DIRECTIVES_BASIC

#else /* !STRATUS_HLS */

#define HLS_MAP_plm(_mem, _plm_block_name)
#define OUT_DMA_CHUNK DMA_CHUNK

#endif /* STRATUS_HLS */

//
// Macros
//

// Load configuration
#define LESS_THAN_ROW 0
#define LESS_THAN_MATRIX2 1
#define MORE_THAN_MATRIX2 2

// log of chunk size
#define DMA_CHUNK_LOG (slog_2<DMA_CHUNK>::value)
#define OUT_DMA_CHUNK_LOG (slog_2<OUT_DMA_CHUNK>::value)

// floating/fixed point conversions
#define INT2FP(x) int2fp<FPDATA, WORD_SIZE>(x)
#define FP2INT(x) fp2int<FPDATA, WORD_SIZE>(x)

// arithmetic operators
#ifdef FIXED_POINT
#define MAC(add, mul1, mul2)					\
    add = add + (mul1 * mul2)

#define MULTIPLY(mul_out, mul1, mul2)		\
    mul_out = (mul1 * mul2)

#define ACCUMULATE(add1, add2)		\
    add1 = (add1 + add2)

#else // FLOAT_POINT

#define MAC(add, mul1, mul2)		\
    add = add + (mul1 * mul2)

#define MULTIPLY(mul_out, mul1, mul2)		\
    mul_out = (mul1 * mul2)

#define ACCUMULATE(add1, add2)		\
    add1 = (add1 + add2)

#endif



#define INPUT_ASI 1
#define OUTPUT_ASI 2



#define POLL_PROD_VALID_REQ 1
#define LOAD_DATA_REQ 2
#define POLL_CONS_READY_REQ 3
#define LOAD_DONE 5

#define UPDATE_CONS_VALID_REQ 6
#define UPDATE_PROD_VALID_REQ 7
#define UPDATE_CONS_READY_REQ 8
#define UPDATE_PROD_READY_REQ 9
#define STORE_DATA_REQ 10
#define STORE_DONE 11
#define STORE_FENCE 12

#define POLL_DONE 13
#define UPDATE_DONE 14

#define LOAD_CONFIG 15
#define UPDATE_CONFIG 16

#define COMPUTE 4

#define NINPUTS_OFFSET 0
#define MAT_D1_OFFSET 1
#define MAT_D2_OFFSET 2
#define MAT_D3_OFFSET 3
#define LD_OFFSET 4
#define ST_OFFSET 5
#define TRANSPOSE_OFFSET 6
#define DO_RELU_OFFSET 7



#endif /* __GEMM_DIRECTIVES_HPP_ */
