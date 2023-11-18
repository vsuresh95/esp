/* Copyright (c) 2011-2022 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include <fixed_point.h>
#include <math.h>
#include "matmul.h"
#include "fcn_input.h"

// Load configuration
#define LESS_THAN_ROW 0
#define LESS_THAN_MATRIX2 1
#define MORE_THAN_MATRIX2 2

#define DMA_WIDTH 64
#define DMA_CHUNK 2048
#define OUT_DMA_CHUNK 256
#define WORD_SIZE 32
#define PARALLELISM 8

// log of chunk size
#define DMA_CHUNK_LOG 11
//(log2<DMA_CHUNK>::value)
#define OUT_DMA_CHUNK_LOG 8
//(log2<OUT_DMA_CHUNK>::value)

#define NINPUTS 1
#define D3_VAL 256
#define D2_VAL 256
#define D1_VAL 256

#define PRINT_DEBUG

#ifdef PRINT_DEBUG
#define DBG_PRINT(x) printf(#x)
#else
#define DBG_PRINT(x)
#endif


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

#define LINE_WIDTH 128
#define WORDS_PER_LINE (LINE_WIDTH/BITWIDTH)

//SYNC FLAGS
#define RDY_OFFSET (0*WORDS_PER_LINE)
#define VLD_OFFSET (1*WORDS_PER_LINE)
#define LAST_OFFSET (VLD_OFFSET+1)
#define NUM_FLAG_PAIRS 2
#define NUM_FLAGS 2

#define SYNC_VAR_SIZE (NUM_FLAGS*WORDS_PER_LINE)
//10

#define NINPUTS_OFFSET 0
#define MAT_D1_OFFSET 1
#define MAT_D2_OFFSET 2
#define MAT_D3_OFFSET 3
#define LD_OFFSET 4
#define ST_OFFSET 5
#define TRANSPOSE_OFFSET 6
#define DO_RELU_OFFSET 7

static uint64_t get_counter() {
  uint64_t counter;
  asm volatile (
    "li t0, 0;"
    "csrr t0, mcycle;"
    "mv %0, t0"
    : "=r" ( counter )
    :
    : "t0"
  );

  return counter;
}

// static uint64_t get_counter() {
//   uint64_t counter;
//   asm volatile (
//     "li t0, 0;"
//     "csrr t0, mcycle;"
//     "mv %0, t0"
//     : "=r" ( counter )
//     :
//     : "t0"
//   );

//   return counter;
// }

// #define ITERATIONS 1000
#define ITERATIONS 1

extern void sw_run(int32_t do_relu, int32_t transpose, int32_t ninputs,
		   int32_t d3, int32_t d2, int32_t d1,
		   native_t *in1, native_t *in2, native_t *out);

uint64_t sw_comp_start = 0;
uint64_t sw_comp_end = 0;
uint64_t hw_comp_start = 0;
uint64_t hw_comp_end = 0;
uint64_t hw_write_time = 0;
uint64_t hw_read_time = 0;


token_t *mem;
const int num_devices = 1;
int32_t tile_size;
int32_t rel_accel_prod_ready_offset;
int32_t rel_accel_prod_valid_offset;
int32_t rel_accel_prod_last_offset;
int32_t rel_input_buffer_offset;
int32_t rel_accel_cons_ready_offset;
int32_t rel_accel_cons_valid_offset;
int32_t rel_accel_con_last_offset;
int32_t rel_output_buffer_offset;


int32_t* cpu_prod_ready_offset;
int32_t* cpu_prod_valid_offset;
int32_t* cpu_cons_ready_offset;
int32_t* cpu_cons_valid_offset;


#include "../linux/app/coh_func.h"
#include "../linux/app/gemm.h"

const float ERR_TH = 0.05;

typedef float native_t;

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}

#define MAX_PRINTED_ERRORS 10

#define SLD_GEMM 0x051
#define DEV_NAME "sld,gemm_stratus"

/* <<--params-->> */
const int32_t do_relu = 0;
const int32_t transpose = 1;
const int32_t ninputs = NINPUTS;
// const int32_t ninputs = 1;
// const int32_t d3 = 3;
// const int32_t d3 = 16;
const int32_t d3 = D3_VAL;
// const int32_t d2 = 3;
const int32_t d2 = D2_VAL;
const int32_t d1 = D1_VAL;
// const int32_t d3 = 4;
// const int32_t d2 = 2;
// const int32_t d1 = 2;
int32_t st_offset;
const int32_t ld_offset1 = 0;
int32_t ld_offset2;


// int32_t tile_size;

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned tiled_out_size;
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
#define GEMM_PROD_VALID_REG 0x74
#define GEMM_CONS_READY_REG 0x70
#define GEMM_PROD_READY_REG 0x6C
#define GEMM_CONS_VALID_REG 0x68
#define GEMM_INPUT_OFFSET_REG 0x64
#define GEMM_TRANSPOSE_REG 0x60
#define GEMM_DO_RELU_REG 0x5c
#define GEMM_ST_OFFSET_REG 0x58
#define GEMM_LD_OFFSET2_REG 0x54
#define GEMM_LD_OFFSET1_REG 0x50
#define GEMM_D3_REG 0x4c
#define GEMM_D2_REG 0x48
#define GEMM_D1_REG 0x44
#define GEMM_NINPUTS_REG 0x40

static int validate_buf(token_t *out, native_t *gold, native_t* out_arr)
{
	int j;
	native_t val, gold_val;
	unsigned errors = 0;
		int temp_len = ninputs * d1 * d3;
        for (j = 0; j < temp_len; j++) {
#ifdef __FIXED
	    val = out_arr[j]; //fx2float(out[j], FX_IL);
			gold_val = gold[j];
		// gold_val = fx2float(gold[j], FX_IL);
		// printf("Output[%d] = %d (orig %d)\n", j, (int)val, out[j]);
#else
            val = out_arr[j]; //out[j];
			gold_val = gold[j];
#endif

		if ((fabs(gold[j] - val) / fabs(gold[j])) > ERR_TH)
		// {
			errors++;
        //     if (gold[j] != val) {
        //         errors++;
        //         // if (errors <= MAX_PRINTED_ERRORS) {
		//     // printf("%d : %d : %d\n", j, (int) val, (int) gold[j]);
		// // }
        //     }
		// if(errors <= 256)
		    // printf("%d : %d : %d\n", j, (int) val, (int) gold_val);
		// }
		
	}

	return errors;
}
void calculate_tiles(uint32_t ninputs,
				   uint32_t matrix_d1,
				   uint32_t matrix_d2,
				   uint32_t matrix_d3,
				   uint32_t transpose,
				   uint32_t* size_matrix1,
				   uint32_t* size_matrix2,
				   uint32_t* size_matrix_out,
				   uint32_t* matrix_chk_in,
				   uint16_t* matrix_rem_in1,
				   uint16_t* matrix_rem_in2,
				   uint32_t* matrix_chk_out,
				   uint16_t* matrix_rem_out,
				   uint16_t* load_cfg,
				   uint16_t* loadable_rows,
				   uint16_t* loadable_chunk,
				   uint16_t* index_d1_incrr);

// static void sw_comp(native_t* gold){
// 		// #include "fcn_gold.h"
// 	#include "fcn_input.h"
// 	int i = 0;
// 	const int offset = round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t)));
// 	for (i = 0; i < ninputs * (d1*d3); i++) gold[i] = 0.0;
//  	for (i = 0; i < ninputs; i++) {
// 		native_t* input_a = &input[i * offset];
// 		native_t* input_b = &input[ninputs * offset + i*d2*d3];
// 		const int output_offset = i*d1*d3;
// 		//weight stationary
//     // sw_comp_start = get_counter();
// 		for(int z = 0; z < d2; z++){
// 			int input_b_offset = z*d3;
// 			for(int x = 0; x < d1; x++){
// 					// round_up(ninputs * (d1*d2 + d2*d3), DMA_WORD_PER_BEAT(sizeof(token_t)));
// 					// native_t temp_wt = input[i * d1*d2 + x*d2 + z]; 
// 					// native_t temp_wt = input[i *offset  + x*d2 + z] ;
// 					const int output_offset2 = output_offset+x*d3;
// 					native_t temp_wt = input_a[x*d2 + z];
// 					// int64_t temp_wt = in[ninputs * d1*d2 + i*d2*d3 + d2*z + y]; 
// 				for (int y = 0; y < d3; y++){
// 					// gold[i*d1*d3 + x*d3 + y] += ((temp_wt * (input[ninputs * offset  + i*d2*d3 + z*d3 + y]))); //>>FX_IL
// 					gold[output_offset2 + y] += ((temp_wt * (input_b[input_b_offset + y]))); //>>FX_IL
// 					// printf("gold[%d] (%d) += in[%d] (%d) * in[%d] (%d) [%d]\n", (i*d1*d3 + x*d3 + y), (int)gold[i*d1*d3 + x*d3 + y], 
// 					// 															(i * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t))) + x*d2 + z),(int)input[i * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t)))  + x*d2 + z], 
// 					// 															(ninputs * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t)))  + i*d2*d3 + z*d3 + y), (int)input[ninputs * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t))) + i*d2*d3 + z*d3 + y], 
// 					// 															((int)(temp_wt * (input[ninputs * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t))) + i*d2*d3 + z*d3 + y]))));
// 				}
// 			}
// 		}
// 	}

// }

// static void init_buf (token_t *in, native_t * gold)
static void init_buf (native_t *sw_buf,native_t *sw_buf2)
{
    int i;

#ifdef __FIXED
	for( int ni = 0; ni < ninputs; ni++){
		for (i = 0; i < (round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t)))); i++) {
			sw_buf[ni*(round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t)))) + i] = i%17-8;//input[i];
			if(i%64==0)
			 printf("sw[%d] = %d\n", (ni*(round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t)))) + i), (int)sw_buf[ni*(round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t)))) + i]);
		}
	}

	for( int ni = 0; ni < ninputs; ni++){
		for (i = 0; i < d2*d3; i++) {
			sw_buf2[ni*d2*d3 + i] = i%17-5; //input[3*(round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t)))) + i];

			if(i%64==0)
			printf("sw2[%d] = %d\n", (ni*d2*d3 + i), (int)sw_buf2[ni*d2*d3 + i]);
		}
	}

#else 
#include "gold.h"
// #include "fcn_gold.h"
#endif

}

static void init_buf_input1 (int ninput, token_t *in, int mat1size, int mat2size, native_t* sw_buf, native_t* sw_buf2, int32_t offset, int32_t offset2)
{
    int i;
	
	static int tile_num = 1;
	// printf("init_buf_input1 Tile %d A[%d - %d]\n", tile_num, offset, mat1size);
		
#ifdef __FIXED
	// *offset_n = offset + mat1size; 
	for (i = 0; i < mat2size; i++) {
		in[i] = float2fx(sw_buf[ninput*d2*d1 + offset+ i], FX_IL); 
    	// printf("in2[%d] = %d, sw[%d] = %d\n", mat1size+i, in[mat1size+i], ninput*d2*d3 + offset2+ i, (int)sw_buf2[ninput*d2*d3 + offset2+ i]);
    }
	// *offset_n2 = offset2 + mat1size; 

#else 
#include "gold.h"
// #include "fcn_gold.h"
#endif
	tile_num++;
}

static void init_buf_input2 (int ninput, token_t *in, int mat1size, int mat2size, native_t* sw_buf, native_t* sw_buf2, int32_t offset, int32_t offset2)
{
    int i;
	
	static int tile_num = 1;
	// printf("init_buf_input2 Tile %d B[%d - %d]\n", tile_num, offset2, mat2size);
		
#ifdef __FIXED
	for (i = 0; i < mat2size; i++) {
		in[mat1size+i] = float2fx(sw_buf2[ninput*d2*d3 + offset2+ i], FX_IL); 
    	// printf("in2[%d] = %d, sw[%d] = %d\n", mat1size+i, in[mat1size+i], ninput*d2*d3 + offset2+ i, (int)sw_buf2[ninput*d2*d3 + offset2+ i]);
    }
	// *offset_n2 = offset2 + mat1size; 

#else 
#include "gold.h"
// #include "fcn_gold.h"
#endif
	tile_num++;
}


static void init_buf_output (int ninput, int offset2, int len, token_t *res, native_t* out_arr)
{
    int i;
#ifdef __FIXED

	int offset = offset2+ninput*(round_up(d1*d3, DMA_WORD_PER_BEAT(sizeof(token_t))));
	// printf("output tile O[%d - %d]\n", offset2, len);
	for (i = 0; i < len; i++) { //round_up(len, DMA_WORD_PER_BEAT(sizeof(token_t)))
		#ifdef __FIXED
		out_arr[offset+i] = fx2float(res[i], FX_IL);
		// printf("res[%d] = %u (%d)\n", i, res[i], (int)out_arr[offset+i]);
		#else
		out_arr[offset+i] = res[i], FX_IL;
		#endif
    }
#else 
#include "gold.h"
// #include "fcn_gold.h"
#endif
}

int main(int argc, char * argv[])
{
	int i;
	int n;
	int ndev;
	struct esp_device *espdevs;
	struct esp_device *dev;
	unsigned done;
	unsigned **ptable;
	native_t *gold;
	native_t *out_arr;
	native_t *sw_buf;
	unsigned errors = 0;
	unsigned coherence;

	
	if (DMA_WORD_PER_BEAT(sizeof(token_t)) <= 1) {
		in_words_adj = ninputs * (d1*d2 + d2*d3);
		out_words_adj = ninputs * d1 * d3;
		st_offset = ninputs * (d1 * d2 + d2 * d3);
		ld_offset2 = ninputs * (d1 * d2);
	} else {
		in_words_adj = round_up(ninputs * (d1*d2 + d2*d3), DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(ninputs * d1 * d3, DMA_WORD_PER_BEAT(sizeof(token_t)));

		st_offset = in_words_adj; 
		ld_offset2 = round_up(ninputs * (d1 * d2), DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = SYNC_VAR_SIZE + in_words_adj;
	out_len = out_words_adj;
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);

	uint32_t size_mat1, size_mat2, size_mat_out, mat_chk_in,  mat_chk_out;
	uint16_t load_cfg, mat_rem_in1, mat_rem_in2,mat_rem_out, loadable_rows, loadable_chunk, index_d1_incr;
	calculate_tiles(ninputs, d1,d2,d3,transpose,&size_mat1, &size_mat2, &size_mat_out, &mat_chk_in, &mat_rem_in1,
	&mat_rem_in2, &mat_chk_out, &mat_rem_out, &load_cfg, &loadable_rows, &loadable_chunk, &index_d1_incr);

	tiled_out_size = round_up(mat_chk_out, DMA_WORD_PER_BEAT(sizeof(token_t)));
	in_len = SYNC_VAR_SIZE+round_up((2*loadable_chunk), DMA_WORD_PER_BEAT(sizeof(token_t)));
	out_offset  = in_len + SYNC_VAR_SIZE;


	mem_size = (out_offset * sizeof(token_t)) + tiled_out_size;
	
	// Search for the device
	printf("Scanning device tree... \n");

	ndev = probe(&espdevs, VENDOR_SLD, SLD_GEMM, DEV_NAME);
	if (ndev == 0) {
		printf("gemm not found\n");
		return 0;
	}

	n = 0;
	// for (n = 0; n < ndev; n++) 
	{

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
		sw_buf = aligned_malloc(in_size);
		gold = aligned_malloc(out_size);
		out_arr = aligned_malloc(out_size);
		mem = aligned_malloc(mem_size);
		printf("  memory buffer base-address = %p\n", mem);
		printf("  memory buffer base-address for gold = %p\n", gold);

		// Allocate and populate page table
		ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
		for (i = 0; i < NCHUNK(mem_size); i++)
			ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];

		printf("  ptable = %p\n", ptable);
		printf("  nchunk = %lu\n", NCHUNK(mem_size));

// #ifndef __riscv
// 		for (coherence = ACC_COH_NONE; coherence <= ACC_COH_FULL; coherence++) {
// #else
		{
			/* TODO: Restore full test once ESP caches are integrated */
			coherence = ACC_COH_FULL;
// #endif
		
			// init_buf(mem, gold);

			// init_buf(mem, sw_buf);

			// Pass common configuration parameters

			iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
			iowrite32(dev, COHERENCE_REG, coherence);
			// If Spandex Caches
			#ifndef ESP
			// set_spandex_config_reg();
			// #if(COH_MODE==3)
			// 	spandex_config.w_cid = (n+2)%(NUM_DEVICES+1);
			// #endif
			printf("Writing spandex :%d\n", spandex_config.spandex_reg);
			iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);
			#endif

			iowrite32(dev, PT_ADDRESS_REG, (unsigned long) ptable);

			iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
			iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

			// Use the following if input and output data are not allocated at the default offsets
			iowrite32(dev, SRC_OFFSET_REG, 0);
			iowrite32(dev, DST_OFFSET_REG, 0);

			// Pass accelerator-specific configuration parametxers
			/* <<--regs-config-->> */
			iowrite32(dev, GEMM_DO_RELU_REG, do_relu);
			iowrite32(dev, GEMM_TRANSPOSE_REG, transpose);
			iowrite32(dev, GEMM_NINPUTS_REG, ninputs);
			iowrite32(dev, GEMM_D3_REG, d3);
			iowrite32(dev, GEMM_D2_REG, d2);
			iowrite32(dev, GEMM_D1_REG, d1);
			iowrite32(dev, GEMM_ST_OFFSET_REG, st_offset);
			iowrite32(dev, GEMM_LD_OFFSET1_REG, ld_offset1);
			iowrite32(dev, GEMM_LD_OFFSET2_REG, ld_offset2);

			tile_size = in_len; //d1*d2 + d2*d3;
			rel_accel_prod_ready_offset = RDY_OFFSET;//0;
			rel_accel_prod_valid_offset = VLD_OFFSET; //rel_accel_prod_ready_offset+2;
			rel_accel_prod_last_offset = LAST_OFFSET; //rel_accel_prod_valid_offset+1;
			rel_input_buffer_offset = SYNC_VAR_SIZE;
			rel_accel_cons_ready_offset = tile_size + RDY_OFFSET;;
			rel_accel_cons_valid_offset = tile_size + VLD_OFFSET; //rel_accel_cons_ready_offset+2;
			rel_accel_con_last_offset = tile_size + LAST_OFFSET;//rel_accel_cons_valid_offset+1;
			rel_output_buffer_offset = out_offset;

			cpu_prod_ready_offset = &rel_accel_cons_ready_offset;
			cpu_cons_ready_offset = &rel_accel_prod_ready_offset;
			cpu_cons_valid_offset = &rel_accel_prod_valid_offset;
			cpu_prod_valid_offset = &rel_accel_cons_valid_offset;

			iowrite32(dev, GEMM_PROD_VALID_REG, rel_accel_prod_valid_offset);
			iowrite32(dev, GEMM_CONS_READY_REG, rel_accel_cons_ready_offset);
			iowrite32(dev, GEMM_PROD_READY_REG, rel_accel_prod_ready_offset);
			iowrite32(dev, GEMM_CONS_VALID_REG, rel_accel_cons_valid_offset);
			iowrite32(dev, GEMM_INPUT_OFFSET_REG, rel_input_buffer_offset);

			printf(" GEMM_DO_RELU_REG: %d\n", do_relu);
			printf(" GEMM_TRANSPOSE_REG: %d\n", transpose);
			printf(" GEMM_NINPUTS_REG: %d\n", ninputs);
			printf(" GEMM_D3_REG: %d\n", d3);
			printf(" GEMM_D2_REG: %d\n", d2);
			printf(" GEMM_D1_REG: %d\n", d1);
			printf(" GEMM_ST_OFFSET_REG: %d\n", st_offset);
			printf(" GEMM_LD_OFFSET1_REG: %d\n", ld_offset1);
			printf(" GEMM_LD_OFFSET2_REG: %d\n", ld_offset2);
			// printf(" GEMM_LD_OFFSET2_REG: %d\n", ld_offset2);


			printf(" rel_input_buffer_offset: %d\n", rel_input_buffer_offset);
			printf(" rel_output_buffer_offset: %d\n", rel_output_buffer_offset);
			printf(" rel_accel_prod_valid_offset: %d\n", rel_accel_prod_valid_offset);
			printf(" rel_accel_cons_ready_offset: %d\n", rel_accel_cons_ready_offset);
			printf(" rel_accel_prod_ready_offset: %d\n", rel_accel_prod_ready_offset);
			printf(" rel_accel_cons_valid_offset: %d\n", rel_accel_cons_valid_offset);

			printf(" n_input: %d\n", ninputs);
			printf(" d1: %d\n", d1);
			printf(" d2: %d\n", d2);
			printf(" d3: %d\n", d3);
			printf(" min len: %d\n", round_up(3,DMA_WORD_PER_BEAT(sizeof(token_t))));
			printf(" out_offset: %d\n", out_offset);
			printf(" out_len: %d\n", out_len);


			// reset_sync();
			mem[rel_accel_prod_ready_offset] = 1;//BM
			printf("reset accel_prod_ready mem[%d]: %d\n", rel_accel_prod_ready_offset, *(mem + rel_accel_prod_ready_offset));
			mem[rel_accel_cons_valid_offset] = 0;
			printf("reset accel_cons_valid mem[%d]: %d\n", rel_accel_cons_valid_offset, *(mem + rel_accel_cons_valid_offset));
			mem[rel_accel_con_last_offset] =0;
			printf("reset accel_con_last mem[%d]: %d\n", rel_accel_con_last_offset, *(mem + rel_accel_con_last_offset));
			mem[rel_accel_prod_valid_offset] =0;
			printf("reset accel_prod_valid mem[%d]: %d\n", rel_accel_prod_valid_offset, *(mem + rel_accel_prod_valid_offset));
			mem[rel_accel_cons_ready_offset] = 0; 
			printf("reset accel_cons_ready mem[%d]: %d\n", rel_accel_cons_ready_offset, *(mem + rel_accel_cons_ready_offset));
	
			asm volatile ("fence w, w");	
			// mem[0] = 65536;
			// mem[1] = 2<<16;
			// for(int iter = 0; iter < mem_size; iter++){
			// 	if(iter==out_offset) printf("OUTPUTS:\n");
			// 	printf("mem[%d]: %d\n", iter, mem[iter]>>16);
			// }


			printf("  Init sw buf...\n");
			init_buf(sw_buf, (sw_buf + ld_offset2));

			// Flush (customize coherence model here)
			esp_flush(coherence);
			// Start accelerators
			printf("  ===Start...\n");

    		hw_comp_start = get_counter();
			// printf("  counter start...\n");
			iowrite32(dev, CMD_REG, CMD_MASK_START);

			// printf("  dev cmd start...\n");
			// // Wait for completion
			// done = 0;
			// while (!done) {
			// 	done = ioread32(dev, STATUS_REG);
			// 	done &= STATUS_MASK_DONE;
			// }
			// iowrite32(dev, CMD_REG, 0x0);
			
			// printf("  Update Prod Rdy...\n");
			update_prod_rdy();

			
			int tinput = 0;
			// while(tinput<ninputs){
			printf("  Poll Cons Rdy...\n");
			while(!poll_cons_rdy()); // wait for cons ready
			// printf("  Found Cons Rdy...\n");

			asm volatile ("fence w, w");	//release semantics
			mem[rel_input_buffer_offset+NINPUTS_OFFSET] = ninputs;
			mem[rel_input_buffer_offset+MAT_D1_OFFSET] = d1;
			mem[rel_input_buffer_offset+MAT_D2_OFFSET] = d2;
			mem[rel_input_buffer_offset+MAT_D3_OFFSET] = d3;
			mem[rel_input_buffer_offset+LD_OFFSET] = rel_input_buffer_offset;
			mem[rel_input_buffer_offset+ST_OFFSET] = rel_output_buffer_offset;
			mem[rel_input_buffer_offset+TRANSPOSE_OFFSET] = transpose;
			mem[rel_input_buffer_offset+DO_RELU_OFFSET] = do_relu;
			printf("  Provided config. Update Cons Rdy and last...\n");

			asm volatile ("fence w, w");	//release semantics
			update_cons_rdy();
			update_cons_valid(0);

			hw_write_time = 0;
			hw_read_time = 0;
			int out_iters = ((d1*d3)/mat_chk_out);
			int in_iters = (d2*d3/(loadable_chunk));
			while(tinput<ninputs){
				uint32_t index_d1 = 0;
				int offset = 0;
				int offseti2 = 0;
				int offset2 = 0;
				int offset_n = 0;
				int offset_n2 = 0;
				int8_t load_a = 1;
				uint32_t in_chk = 0;
				uint32_t out_chk = 0;
				uint32_t out_row = 0;
				uint32_t out_tile = 0;
				int chk_per_tile = mat_chk_out; //(d1*d3/loadable_rows);
				uint32_t md1 = 0, md2 = 0;
				while(in_chk < mat_chk_in || out_chk < chk_per_tile){//
					if(out_chk < chk_per_tile && poll_prod_valid())
					{
						int mat1size = OUT_DMA_CHUNK;
						
						// If true the next is the last (smaller) chunk
						if (load_cfg == LESS_THAN_MATRIX2 && loadable_rows != 1) {
							mat1size = loadable_rows;
						} else {
							if (out_chk == mat_chk_out - 1 && mat_rem_out != 0)
							mat1size = mat_rem_out;
						}

						uint64_t hw_read_time_start = get_counter();
						init_buf_output(tinput, out_tile+out_row*d3+offset2, mat1size, mem+rel_output_buffer_offset,out_arr);
						update_prod_valid();
						update_prod_rdy();
						// offset2 += mat_chk_out; //loadable_rows; //loadable_rows;
						out_row++;
						if(out_row == loadable_rows){
							out_row = 0;
							if(load_cfg == LESS_THAN_MATRIX2){
								offset2 += loadable_rows;
								if(offset2 >= d3){
									offset2 = 0;
									out_tile += loadable_rows*d3;
								}
							} else {
								offset2 += OUT_DMA_CHUNK;
								if(offset2 >= d3){
									offset2 = 0;
									out_tile += loadable_rows*d3;
								}
							}
						}
						uint64_t hw_read_time_end = get_counter();
						hw_read_time += (hw_read_time_end-hw_read_time_start);
						// printf("  out chk:%d/%d\n", out_chk,mat_chk_out);//(d1*d3/loadable_rows));
						out_chk++;
					}
					else if((in_chk < mat_chk_in ) && poll_cons_rdy()){//|| out_chk < mat_chk_out)
						int mat1size = loadable_chunk; //(round_up(d1*loadable_rows, DMA_WORD_PER_BEAT(sizeof(token_t))));
						int mat2size = loadable_chunk;
						if (load_cfg == LESS_THAN_ROW && in_chk == mat_chk_in - 1 && mat_rem_in2 != 0) {
							mat1size = mat_rem_in1;
							mat2size = mat_rem_in2;
						} else if (load_cfg != LESS_THAN_ROW) {
							if (md1 + loadable_rows > d1)
								mat1size = mat_rem_in1;
							if (md2 + loadable_rows > d3)
								mat2size = mat_rem_in2;
						}

						uint64_t hw_write_time_start = get_counter();
						// init_buf_input(0, mem+rel_input_buffer_offset, loadable_rows);
						if(load_a)//(in_chk==0)
						{
							init_buf_input1 (tinput, mem+rel_input_buffer_offset, mat1size, mat2size, sw_buf, (sw_buf + ld_offset2), offset,offseti2);
							load_a = !load_a;
						}
						init_buf_input2 (tinput, mem+rel_input_buffer_offset, mat1size, mat2size, sw_buf, (sw_buf + ld_offset2), offset,offseti2);
						uint64_t hw_write_time_end = get_counter();
						hw_write_time += (hw_write_time_end-hw_write_time_start);

						// offset = offset + mat1size;
						// offseti2 = offseti2+mat2size;
						// printf("  Update Cons Rdy and last...\n");
						update_cons_rdy();
						update_cons_valid(0);
						// printf("  in chk:%d/%d\n", in_chk,mat_chk_in);
						// in_chk ++;

						if(in_chk == mat_chk_in - 1){
							load_a = 1;
							if (md1 + loadable_rows >= d1 && md2 + loadable_rows >= d3){
								md1 += loadable_rows;
								offset+= loadable_chunk;
								md2 += loadable_rows;
								offseti2 += loadable_chunk;
								in_chk++;
							}else if(md2 + loadable_rows >= d3){
								md2 = 0;
								offset += loadable_chunk;
								md1 += loadable_rows;
								offseti2 = 0;
								in_chk = 0;
							} else
							{
								// md1 += loadable_rows;
								// offset+= loadable_chunk;
								// md2 = 0;

								in_chk = 0;
								md2 += loadable_rows;
								offseti2 += loadable_chunk;
							}
						} else {
							in_chk++;
							md2 += loadable_rows;
							offseti2 += loadable_chunk;
						}
					}
				}

				// for (uint32_t md1 = 0; md1 < d1; md1 += loadable_rows)
				// {
				// 	printf("tinput:%d/%d\nmd1:%d/%d + %d\n",tinput, ninputs, md1,d1, loadable_rows);
				// 	uint16_t loaded_rows_d1 = (loadable_rows< d1 - md1)?loadable_rows:d1 - md1;
				// 	// uint32_t index_d2 = index_d1;
				// 	for (uint32_t md2 = 0; md2 < d3; )
				// 	{
				// 		printf("md2:%d/%d + %d\n", md2, d3, loadable_rows);
				// 		// uint16_t loaded_rows_d3 = min(loadable_rows, matrix_d3 - d2);
				// 		// uint32_t index_d1i = index_d2;
				// 		// for (uint16_t d1i = 0; d1i < loaded_rows_d1; d1i++)
				// 		{
				// 			// for (uint24_t chk = 0; chk < matrix_chk_out; ++chk)
				// 			// {
				// 			// printf("d1i:%d/%d\n", d1i,loaded_rows_d1);
				// 			uint32_t in_chk = 0;
				// 			// uint32_t out_chk = 0;
				// 			while(in_chk < mat_chk_in || out_chk < chk_per_tile){//
									

				// 				if(out_chk < chk_per_tile && poll_prod_valid())
				// 				{
				// 					int mat1size = OUT_DMA_CHUNK;
									
				// 					// If true the next is the last (smaller) chunk
				// 					if (load_cfg == LESS_THAN_MATRIX2 && loadable_rows != 1) {
				// 						mat1size = loadable_rows;
				// 					} else {
				// 						if (out_chk == mat_chk_out - 1 && mat_rem_out != 0)
				// 						mat1size = mat_rem_out;
				// 					}

				// 					// if (load_cfg == LESS_THAN_ROW && out_chk == mat_chk_out - 1 && mat_rem_out != 0) {
				// 					// 	mat1size = mat_rem_out;
				// 					// } else if (load_cfg != LESS_THAN_ROW) {
				// 					// 	if (md1 + loadable_rows > d1)
				// 					// 		mat1size = mat_rem_out;
				// 					// 	if (md2 + loadable_rows > d3)
				// 					// 		mat1size = mat_rem_out;
				// 					// }
				// 					uint64_t hw_read_time_start = get_counter();
				// 					init_buf_output(tinput, offset2, mat1size, mem+rel_output_buffer_offset,out_arr);
				// 					update_prod_valid();
				// 					update_prod_rdy();
				// 					offset2 += loadable_rows; //loadable_rows;
				// 					uint64_t hw_read_time_end = get_counter();
				// 					hw_read_time += (hw_read_time_end-hw_read_time_start);
				// 					printf("  out chk:%d/%d\n", out_chk,(d1*d3/loadable_rows));
				// 					out_chk++;
				// 				}
				// 				else if((in_chk < mat_chk_in ) && poll_cons_rdy()){//|| out_chk < mat_chk_out)
				// 					int mat1size = loadable_chunk; //(round_up(d1*loadable_rows, DMA_WORD_PER_BEAT(sizeof(token_t))));
				// 					int mat2size = loadable_chunk;
				// 					if (load_cfg == LESS_THAN_ROW && in_chk == mat_chk_in - 1 && mat_rem_in2 != 0) {
				// 						mat1size = mat_rem_in1;
				// 						mat2size = mat_rem_in2;
				// 					} else if (load_cfg != LESS_THAN_ROW) {
				// 						if (md1 + loadable_rows > d1)
				// 							mat1size = mat_rem_in1;
				// 						if (md2 + loadable_rows > d3)
				// 							mat2size = mat_rem_in2;
				// 					}

				// 					uint64_t hw_write_time_start = get_counter();
				// 					// init_buf_input(0, mem+rel_input_buffer_offset, loadable_rows);
				// 					init_buf_input (tinput, mem+rel_input_buffer_offset, mat1size, mat2size, sw_buf, (sw_buf + ld_offset2), md1,md1+mat1size);
				// 					uint64_t hw_write_time_end = get_counter();
				// 					hw_write_time += (hw_write_time_end-hw_write_time_start);

				// 					// offset = offset + mat1size;
				// 					// offseti2 = offseti2+mat2size;
				// 					// printf("  Update Cons Rdy and last...\n");
				// 					update_cons_rdy();
				// 					update_cons_valid(0);
				// 					printf("  in chk:%d/%d\n", in_chk,mat_chk_in);
				// 					in_chk ++;
				// 				}
				// 			}
				// 		}
				// 	}
				// }

				tinput++;
			}
    		hw_comp_end = get_counter();



			printf("  Done\n");

			printf("SW Comp:\n");		
			
    		sw_comp_start = get_counter();
			for(int iter = 0; iter < ITERATIONS; iter++)
				sw_run(do_relu, transpose, ninputs, d3, d2, d1, sw_buf, (sw_buf + ld_offset2), gold);
    		sw_comp_end = get_counter();


    		printf("SW Comp Time: %lu\nHW Comp Time:%lu\n", (sw_comp_end-sw_comp_start)/ITERATIONS, (hw_comp_end-hw_comp_start));
			printf("HW Write Time: %lu\nHW Read Time: %lu\n", hw_write_time, hw_read_time);
			printf("  validating...\n");

			/* Validation */
			// for(int iter = 0; iter < (out_offset+out_len); iter++){
			// 	if(iter==out_offset) printf("OUTPUTS:\n");
			// 	printf("mem[%d]: %d\n", iter, mem[iter]);
			// }

			errors = validate_buf(&mem[rel_output_buffer_offset], gold, out_arr);

			if (errors)
				printf("  ... FAIL (%d errors)\n", errors);
			else
				printf("  ... PASS\n");

		}// coh
		aligned_free(ptable);
		aligned_free(mem);
		aligned_free(gold);
		aligned_free(sw_buf);
	}//ndev

	return 0;
}


void calculate_tiles(uint32_t ninputs,
				   uint32_t matrix_d1,
				   uint32_t matrix_d2,
				   uint32_t matrix_d3,
				   uint32_t transpose,
				   uint32_t* size_matrix1,
				   uint32_t* size_matrix2,
				   uint32_t* size_matrix_out,
				   uint32_t* matrix_chk_in,
				   uint16_t* matrix_rem_in1,
				   uint16_t* matrix_rem_in2,
				   uint32_t* matrix_chk_out,
				   uint16_t* matrix_rem_out,
				   uint16_t* load_cfg,
				   uint16_t* loadable_rows,
				   uint16_t* loadable_chunk,
				   uint16_t* index_d1_incr)
{
				// 	,
				//    uint16_t& m2_loop_iters,
				//    uint16_t& m2_plm_incr){
	*size_matrix1 = matrix_d1 * matrix_d2;
    *size_matrix2 = matrix_d2 * matrix_d3;
    *size_matrix_out = matrix_d1 * matrix_d3;// * ninputs;

	printf("sizem1:%d sizem2:%d sizeout:%d \n", *size_matrix1, *size_matrix2, *size_matrix_out);

    // m2_loop_iters = 1;
    // m2_plm_incr = 1;

    uint8_t d3_odd = matrix_d3 % 2;
    uint8_t is_less_than_matrix2 = (*size_matrix2 > DMA_CHUNK || !transpose);

    if ((matrix_d2 > DMA_CHUNK) || (is_less_than_matrix2 && d3_odd)) {
		*load_cfg = LESS_THAN_ROW;
		*loadable_rows = 1;
		*loadable_chunk = DMA_CHUNK;
		uint32_t matrix_mul;
		// calculate_chunks(matrix_chk_in, matrix_rem_in1, matrix_d2, 0);
		*matrix_chk_in = matrix_d2 >> DMA_CHUNK_LOG;
		// calculating the number of cols (covered the by the chunks)
		matrix_mul = *matrix_chk_in << DMA_CHUNK_LOG;
		*matrix_rem_in1 = matrix_d2 - matrix_mul;

		// adding the last chunk if it is necessary
		if (*matrix_rem_in1 != 0) { ++(*matrix_chk_in); }

		*matrix_rem_in2 = *matrix_rem_in1;
		*index_d1_incr = matrix_d2;
    } else if (is_less_than_matrix2) {
		*load_cfg = LESS_THAN_MATRIX2;
		if (*size_matrix2 > DMA_CHUNK) {
			*loadable_rows = DMA_CHUNK / matrix_d2;
			if (*loadable_rows != 1)
			*loadable_rows = ((*loadable_rows) >> 1) << 1;
		} else {
			*loadable_rows = matrix_d3;
		}
		*loadable_chunk = *loadable_rows * matrix_d2;
		*matrix_chk_in = 1;
		*matrix_rem_in1 = *size_matrix1 % *loadable_chunk;
		*matrix_rem_in2 = *size_matrix2 % *loadable_chunk;
		*index_d1_incr = *loadable_chunk;
	// 	if (!transpose) {
	// 		// m2_loop_iters = matrix_d2;
	// 		// m2_plm_incr = matrix_d2;
	// 	}
    } else 
		{
		*load_cfg = MORE_THAN_MATRIX2;
		*loadable_rows = matrix_d3;
		*loadable_chunk = *size_matrix2;
		*matrix_chk_in = 1;
		*matrix_rem_in1 = *size_matrix1 % *loadable_chunk;
		*matrix_rem_in2 = *size_matrix2;
		*index_d1_incr = *loadable_chunk;
		}
	// calculate_chunks(matrix_chk_out, matrix_rem_out, size_matrix_out, 1);
// calculating the number of chunks (ceil)
 		if (*load_cfg == LESS_THAN_MATRIX2 && *loadable_rows != 1) 
		{
			*matrix_chk_out = (*size_matrix_out) / *loadable_rows;
			uint32_t matrix_mul = (*matrix_chk_out) * (*loadable_rows); 
			*matrix_rem_out = *(size_matrix_out) - matrix_mul;
			// adding the last chunk if it is necessary
			if (*matrix_rem_out > 0) { ++(*matrix_chk_out); 
			}
		}
		else
		{
			*matrix_chk_out = (*size_matrix_out) >> OUT_DMA_CHUNK_LOG;
			// calculating the number of cols (covered the by the chunks)
			uint32_t matrix_mul = (*matrix_chk_out) << OUT_DMA_CHUNK_LOG; 
			*matrix_rem_out = *(size_matrix_out) - matrix_mul;
			// adding the last chunk if it is necessary
			if (*matrix_rem_out > 0) { ++(*matrix_chk_out); 
			}
		}
	printf("cfg: %d loadable rows: %d\nloadable chunk:%d\nmatrix rem in2:%d\nmatrix rem in1:%d\nmatrix rem out:%d\nmatrix chnk in:%d\nmatrix chnk out:%d\n", (int)*load_cfg, *loadable_rows, *loadable_chunk
	, *matrix_rem_in1, *matrix_rem_in2, *matrix_rem_out, *matrix_chk_in, *matrix_chk_out);
}
