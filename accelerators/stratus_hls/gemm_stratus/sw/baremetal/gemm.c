/* Copyright (c) 2011-2022 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include <fixed_point.h>
#include <math.h>
#include "matmul.h"
// #include "fcn_input.h"
#include "../linux/app/gemm_directives.h"

#define D_COMMON 16
#define D3_VAL D_COMMON
#define D2_VAL D_COMMON
#define D1_VAL D_COMMON
#define NINPUTS (1000/D_COMMON)

#define PRINT_DEBUG

#ifdef PRINT_DEBUG
#define DBG_PRINT(x) printf(#x)
#else
#define DBG_PRINT(x)
#endif


/* User defined */

#define ITERATIONS 1

extern void sw_run(int32_t do_relu, int32_t transpose, int32_t ninputs,
		   int32_t d3, int32_t d2, int32_t d1,
		   native_t *in1, native_t *in2, native_t *out);



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

// static unsigned DMA_WORD_PER_BEAT(unsigned _st)
// {
//         return (sizeof(void *) / _st);
// }

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
#else
            val = out_arr[j]; //out[j];
			gold_val = gold[j];
#endif

		if ((fabs(gold[j] - val) / fabs(gold[j])) > ERR_TH)
		{
			errors++;
		// if(errors <= 256)
		    // printf("%d : %d : %d\n", j, (int) val, (int) gold_val);
		}
		
	}

	return errors;
}
extern void calculate_tiles(uint32_t ninputs,
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
	in_len = in_words_adj;
	out_len = out_words_adj;
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);

	// uint32_t size_mat1, size_mat2, size_mat_out, mat_chk_in,  mat_chk_out;
	// uint16_t load_cfg, mat_rem_in1, mat_rem_in2,mat_rem_out, loadable_rows, loadable_chunk, index_d1_incr;
	calculate_tiles(ninputs, d1,d2,d3,transpose,&size_mat1, &size_mat2, &size_mat_out, &mat_chk_in, &mat_rem_in1,
	&mat_rem_in2, &mat_chk_out, &mat_rem_out, &load_cfg, &loadable_rows, &loadable_chunk, &index_d1_incr);

	tiled_out_size = round_up(mat_chk_out, DMA_WORD_PER_BEAT(sizeof(token_t)));
	int tiled_in_len = SYNC_VAR_SIZE+round_up((2*loadable_chunk), DMA_WORD_PER_BEAT(sizeof(token_t)));
	out_offset  = tiled_in_len + SYNC_VAR_SIZE;


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

			tile_size = tiled_in_len; //d1*d2 + d2*d3;
			set_offsets(NUM_DEVICES, tile_size);

			cpu_prod_ready_offset = &accel_cons_ready_offset[0];
			cpu_cons_ready_offset = &accel_prod_ready_offset[0];
			cpu_cons_valid_offset = &accel_prod_valid_offset[0];
			cpu_prod_valid_offset = &accel_cons_valid_offset[0];

			#if(COMP_MODE!=MODE_REG_INV)
			iowrite32(dev, GEMM_PROD_VALID_REG, accel_prod_valid_offset[0]);
			iowrite32(dev, GEMM_CONS_READY_REG, accel_cons_ready_offset[0]);
			iowrite32(dev, GEMM_PROD_READY_REG, accel_prod_ready_offset[0]);
			iowrite32(dev, GEMM_CONS_VALID_REG, accel_cons_valid_offset[0]);
			iowrite32(dev, GEMM_INPUT_OFFSET_REG, input_buffer_offset[0]);

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


			printf(" rel_input_buffer_offset: %d\n",     input_buffer_offset    [0]);
			printf(" rel_output_buffer_offset: %d\n",    output_buffer_offset   [0]);
			printf(" rel_accel_prod_valid_offset: %d\n", accel_prod_valid_offset[0]);
			printf(" rel_accel_cons_ready_offset: %d\n", accel_cons_ready_offset[0]);
			printf(" rel_accel_prod_ready_offset: %d\n", accel_prod_ready_offset[0]);
			printf(" rel_accel_cons_valid_offset: %d\n", accel_cons_valid_offset[0]);

			printf(" n_input: %d\n", ninputs);
			printf(" d1: %d\n", d1);
			printf(" d2: %d\n", d2);
			printf(" d3: %d\n", d3);
			printf(" out_offset: %d\n", out_offset);
			printf(" out_len: %d\n", out_len);
			#endif

			printf("  Init sw buf...\n");
			// init_buf(sw_buf, (sw_buf + ld_offset2));
			init_buffer(mem, sw_buf, in_len);

			#if(COMP_MODE!=MODE_REG_INV)
			reset_sync(mem, NUM_DEVICES);
			asm volatile ("fence w, w");	
			#endif

			// Flush (customize coherence model here)
			esp_flush(coherence);
			// Start accelerators
			printf("  ===Start...\n");

    		hw_comp_start = get_counter();
			// printf("  counter start...\n");
			iowrite32(dev, CMD_REG, CMD_MASK_START);

			// printf("  dev cmd start...\n");
			// // Wait for completion
			#if(COMP_MODE==MODE_REG_INV)
			done = 0;
			while (!done) {
				done = ioread32(dev, STATUS_REG);
				done &= STATUS_MASK_DONE;
			}
			iowrite32(dev, CMD_REG, 0x0);
			#else
			// printf("  Update Prod Rdy...\n");
			in_main( ninputs,  d1,  d2,  d3,  transpose,  do_relu, ld_offset2, mem, sw_buf, out_arr);
			#endif
    		hw_comp_end = get_counter();



			printf("  Done\n");

			printf("SW Comp:\n");		
			
    		sw_comp_start = get_counter();
			for(int iter = 0; iter < ITERATIONS; iter++)
				sw_run(do_relu, transpose, ninputs, d3, d2, d1, sw_buf, (sw_buf + ld_offset2), gold);
    		sw_comp_end = get_counter();


    		printf("SW Comp Time: %lu\nHW Comp Time:%lu\n", (sw_comp_end-sw_comp_start)/(ITERATIONS*NINPUTS), (hw_comp_end-hw_comp_start)/NINPUTS);
			printf("HW Write Time: %lu\nHW Read Time: %lu\n", hw_write_time/NINPUTS, hw_read_time/NINPUTS);
			printf("  validating...\n");

			/* Validation */
			errors = validate_buf(&mem[rel_output_buffer_offset], gold, out_arr);

			if (errors)
				printf("  ... FAIL (%d errors)\n", errors);
			else
				printf("  ... PASS\n");

		}// coh
		aligned_free(ptable);
		aligned_free(mem);
		aligned_free(out_arr);
		aligned_free(gold);
		aligned_free(sw_buf);
	}//ndev

	return 0;
}


