
#ifndef __ESP_GEMM_DIR_H__
#define __ESP_GEMM_DIR_H__

#define MAX_DEVICES 3
#define NUM_DEVICES 3
#define ITERATIONS 1000

// Comp Mode 0: Reg Inv
// Comp Mode 1: Chaining
// Comp Mode 2: Pipelining
#define MODE_REG_INV 0
#define MODE_CHAIN 1
#define MODE_PIPE 2

#define COMP_MODE MODE_PIPE

// Load configuration
#define LESS_THAN_ROW 0
#define LESS_THAN_MATRIX2 1
#define MORE_THAN_MATRIX2 2


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
// #define BITWIDTH 64
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

#define DMA_WIDTH 64
// #define DMA_CHUNK 2048
#define DMA_CHUNK 4096
#define OUT_DMA_CHUNK 256
#define WORD_SIZE 32
#define PARALLELISM 8

// log of chunk size
// #define DMA_CHUNK_LOG 11
#define DMA_CHUNK_LOG 12
//(log2<DMA_CHUNK>::value)
#define OUT_DMA_CHUNK_LOG 8
//(log2<OUT_DMA_CHUNK>::value)


#define LINE_WIDTH 128
#define WORDS_PER_LINE (LINE_WIDTH/BITWIDTH)

//SYNC FLAGS
#define RDY_OFFSET (0*WORDS_PER_LINE)
#define VLD_OFFSET (1*WORDS_PER_LINE)
#define LAST_OFFSET (2*WORDS_PER_LINE)
#define NUM_FLAG_PAIRS 2
#define NUM_FLAGS 2

#define SYNC_VAR_SIZE (NUM_FLAGS*WORDS_PER_LINE)

#define NINPUTS_OFFSET 0
#define MAT_D1_OFFSET 1
#define MAT_D2_OFFSET 2
#define MAT_D3_OFFSET 3
#define LD_OFFSET 4
#define ST_OFFSET 5
#define TRANSPOSE_OFFSET 6
#define DO_RELU_OFFSET 7

/* <<--params-def-->> */
#define DO_RELU 0
#define TRANSPOSE 1
// #define NINPUTS 2
// #define D_COMMON 8

// #define SW_ONLY

#define D_COMMON 64
#define D3_VAL D_COMMON
#define D2_VAL D_COMMON
#define D1_VAL 1
#define NINPUTS 1
//(1000/D_COMMON)

#define D3 D3_VAL
#define D2 D2_VAL
#define D1 D1_VAL
#define ST_OFFSET0 (NINPUTS * (D1 * D2 + D2 * D3))
#define LD_OFFSET1 0
#define LD_OFFSET2 (NINPUTS * (D1 * D2))


typedef float native_t;
#ifndef __linux__
static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}
#endif

#endif
