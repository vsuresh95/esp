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

#define SYNC_VAR_SIZE 4

#define NINPUTS_OFFSET 0
#define MAT_D1_OFFSET 1
#define MAT_D2_OFFSET 2
#define MAT_D3_OFFSET 3
#define LD_OFFSET 4
#define ST_OFFSET 5
#define TRANSPOSE_OFFSET 6
#define DO_RELU_OFFSET 7

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

#define ITERATIONS 1000

extern void sw_run(int32_t do_relu, int32_t transpose, int32_t ninputs,
		   int32_t d3, int32_t d2, int32_t d1,
		   native_t *in1, native_t *in2, native_t *out);

uint64_t sw_comp_start = 0;
uint64_t sw_comp_end = 0;
uint64_t hw_comp_start = 0;
uint64_t hw_comp_end = 0;

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
const int32_t transpose = 0;
// const int32_t ninputs = 2;
const int32_t ninputs = 1;
// const int32_t d3 = 3;
// const int32_t d3 = 16;
const int32_t d3 = 2;
// const int32_t d2 = 3;
const int32_t d2 = 2;
const int32_t d1 = 2;
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
#define GEMM_PROD_VALID_REG 0x70
#define GEMM_CONS_READY_REG 0x6C
#define GEMM_PROD_READY_REG 0x68
#define GEMM_CONS_VALID_REG 0x64
#define GEMM_INPUT_OFFSET_REG 0x60
#define GEMM_TRANSPOSE_REG 0x60
#define GEMM_DO_RELU_REG 0x5c
#define GEMM_ST_OFFSET_REG 0x58
#define GEMM_LD_OFFSET2_REG 0x54
#define GEMM_LD_OFFSET1_REG 0x50
#define GEMM_D3_REG 0x4c
#define GEMM_D2_REG 0x48
#define GEMM_D1_REG 0x44
#define GEMM_NINPUTS_REG 0x40

static int validate_buf(token_t *out, native_t *gold)
{
	int j;
	native_t val, gold_val;
	unsigned errors = 0;
		int temp_len = ninputs * d1 * d3;
        for (j = 0; j < temp_len; j++) {
#ifdef __FIXED
	    val = fx2float(out[j], FX_IL);
			gold_val = gold[j];
		// gold_val = fx2float(gold[j], FX_IL);
		printf("Output[%d] = %d (orig %d)\n", j, (int)val, out[j]);
#else
            val = out[j];
			gold_val = gold[j];
#endif

		if ((fabs(gold[j] - val) / fabs(gold[j])) > ERR_TH)
			errors++;
        //     if (gold[j] != val) {
        //         errors++;
        //         // if (errors <= MAX_PRINTED_ERRORS) {
		//     // printf("%d : %d : %d\n", j, (int) val, (int) gold[j]);
		// // }
        //     }
		    printf("%d : %d : %d\n", j, (int) val, (int) gold_val);
	}

	return errors;
}

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
static void init_buf (token_t *in, native_t *sw_buf)
{
    int i;
	
// #include "input.h"
// #include "fcn_input.h"

// #ifdef __FIXED
//     for (i = 0; i < ninputs * (d1*d2 + d2*d3); i++) {
//         in[i] = float2fx(in[i], FX_IL);
// 		printf("input in[%d]: %d\n", i, in[i]);
//     }
// #endif

// #include "gold.h"
// #include "fcn_gold.h"
// in[0] = 1;
// in[1] = 2;
// in[3] = 4;

#ifdef __FIXED

	// // #include "fcn_gold.h"
	// for (i = 0; i < ninputs * (d1*d3); i++) gold[i] = 0.0;
 	// for (i = 0; i < ninputs; i++) {
	// 	//weight stationary
    // sw_comp_start = get_counter();
	// 	for(int z = 0; z < d2; z++){
	// 		for(int x = 0; x < d1; x++){
	// 				// round_up(ninputs * (d1*d2 + d2*d3), DMA_WORD_PER_BEAT(sizeof(token_t)));
	// 				// native_t temp_wt = input[i * d1*d2 + x*d2 + z]; 
	// 				native_t temp_wt = input[i * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t))) + x*d2 + z] ;
	// 				// int64_t temp_wt = in[ninputs * d1*d2 + i*d2*d3 + d2*z + y]; 
	// 			for (int y = 0; y < d3; y++){
	// 				gold[i*d1*d3 + x*d3 + y] += ((temp_wt * (input[ninputs * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t)))  + i*d2*d3 + z*d3 + y]))); //>>FX_IL
	// 				printf("gold[%d] (%d) += in[%d] (%d) * in[%d] (%d) [%d]\n", (i*d1*d3 + x*d3 + y), (int)gold[i*d1*d3 + x*d3 + y], 
	// 																			(i * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t))) + x*d2 + z),(int)input[i * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t)))  + x*d2 + z], 
	// 																			(ninputs * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t)))  + i*d2*d3 + z*d3 + y), (int)input[ninputs * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t))) + i*d2*d3 + z*d3 + y], 
	// 																			((int)(temp_wt * (input[ninputs * round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t))) + i*d2*d3 + z*d3 + y]))));
	// 			}
	// 		}
	// 	}
	// }

    // sw_comp_end = get_counter();

	for (i = 0; i < ninputs * (round_up(d1*d2, DMA_WORD_PER_BEAT(sizeof(token_t))) + d2*d3); i++) {
		sw_buf[i] = input[i];
		in[i] = float2fx(input[i], FX_IL);
		if(i%128==0)
			printf("input in[%d] (%d): %d\n", i, (int)input[i], in[i]);
    }

#else 
// #include "gold.h"
#include "fcn_gold.h"
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

		st_offset = in_words_adj; //ninputs * (d1 * d2 + d2 * d3);
		ld_offset2 = round_up(ninputs * (d1 * d2), DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj;
	out_len = out_words_adj;
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset  = in_len;
	mem_size = (out_offset * sizeof(token_t)) + out_size;
	
	// Search for the device
	printf("Scanning device tree... \n");

	ndev = probe(&espdevs, VENDOR_SLD, SLD_GEMM, DEV_NAME);
	if (ndev == 0) {
		printf("gemm not found\n");
		return 0;
	}

	for (n = 0; n < ndev; n++) {

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
		
			printf("  Generate input...\n");

			reset_sync();
			// init_buf(mem, gold);
			init_buf(mem, sw_buf);


    		sw_comp_start = get_counter();
			for(int iter = 0; iter < ITERATIONS; iter++)
				sw_run(do_relu, transpose, ninputs, d3, d2, d1, sw_buf, (sw_buf + ld_offset2), gold);
    		sw_comp_end = get_counter();
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

			tile_size = d1*d2 + d2*d3;
			rel_accel_prod_ready_offset = 0;
			rel_accel_prod_valid_offset = rel_accel_prod_ready_offset+2;
			rel_accel_prod_last_offset = rel_accel_prod_valid_offset+1;
			rel_input_buffer_offset = SYNC_VAR_SIZE;
			rel_accel_cons_ready_offset = tile_size + SYNC_VAR_SIZE;
			rel_accel_cons_valid_offset = rel_accel_cons_ready_offset+2;
			rel_accel_con_last_offset = rel_accel_cons_valid_offset+1;
			rel_output_buffer_offset = tile_size + 2*SYNC_VAR_SIZE;

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

			printf(" n_input: %d\n", ninputs);
			printf(" d1: %d\n", d1);
			printf(" d2: %d\n", d2);
			printf(" d3: %d\n", d3);
			printf(" min len: %d\n", round_up(3,DMA_WORD_PER_BEAT(sizeof(token_t))));
			printf(" out_offset: %d\n", out_offset);
			printf(" out_len: %d\n", out_len);


			// mem[0] = 65536;
			// mem[1] = 2<<16;
			// for(int iter = 0; iter < mem_size; iter++){
			// 	if(iter==out_offset) printf("OUTPUTS:\n");
			// 	printf("mem[%d]: %d\n", iter, mem[iter]>>16);
			// }

			// Flush (customize coherence model here)
			esp_flush(coherence);
			// Start accelerators
			printf("  Start...\n");

    		hw_comp_start = get_counter();
			iowrite32(dev, CMD_REG, CMD_MASK_START);

			// // Wait for completion
			// done = 0;
			// while (!done) {
			// 	done = ioread32(dev, STATUS_REG);
			// 	done &= STATUS_MASK_DONE;
			// }
			// iowrite32(dev, CMD_REG, 0x0);


			update_prod_rdy();
			while(!poll_cons_rdy()); // wait for cons ready
			mem[rel_input_buffer_offset+NINPUTS_OFFSET] = ninputs;
			mem[rel_input_buffer_offset+MAT_D1_OFFSET] = d1;
			mem[rel_input_buffer_offset+MAT_D2_OFFSET] = d2;
			mem[rel_input_buffer_offset+MAT_D3_OFFSET] = d3;
			mem[rel_input_buffer_offset+LD_OFFSET] = rel_input_buffer_offset;
			mem[rel_input_buffer_offset+ST_OFFSET] = rel_output_buffer_offset;
			mem[rel_input_buffer_offset+TRANSPOSE_OFFSET] = transpose;
			mem[rel_input_buffer_offset+DO_RELU_OFFSET] = do_relu;
			update_cons_rdy();
			update_cons_valid(1);

			while(!poll_prod_valid()); //wait for prod 
    		hw_comp_end = get_counter();

			printf("  Done\n");

    		printf("SW Comp Time: %lu\nHW Comp Time:%lu\n", (sw_comp_end-sw_comp_start)/ITERATIONS, (hw_comp_end-hw_comp_start));
			printf("  validating...\n");

			/* Validation */
			// for(int iter = 0; iter < (out_offset+out_len); iter++){
			// 	if(iter==out_offset) printf("OUTPUTS:\n");
			// 	printf("mem[%d]: %d\n", iter, mem[iter]);
			// }
			errors = validate_buf(&mem[rel_output_buffer_offset], gold);

			if (errors)
				printf("  ... FAIL (%d errors)\n", errors);
			else
				printf("  ... PASS\n");

		}
		aligned_free(ptable);
		aligned_free(mem);
		aligned_free(gold);
		aligned_free(sw_buf);
	}

	return 0;
}
