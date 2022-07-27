/* Copyright (c) 2011-2021 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include <fixed_point.h>

#include "data.h"	// for the new test

#define SOC_COLS 2
#define SOC_ROWS 4
#define REQ_PLANE 0
#define FWD_PLANE 1
#define RSP_PLANE 2

#define ITERATION 1

typedef int32_t token_t;

typedef union
{
  struct
  {
    unsigned char r_en   : 1;
    unsigned char r_op   : 1;
    unsigned char r_type : 2;
    unsigned char r_cid  : 4;
    unsigned char w_en   : 1;
    unsigned char w_op   : 1;
    unsigned char w_type : 2;
    unsigned char w_cid  : 4;
	uint16_t reserved: 16;
  };
  uint32_t spandex_reg;
} spandex_config_t;

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
    return (sizeof(void *) / _st);
}

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

uint64_t start_write;
uint64_t stop_write;
uint64_t intvl_write;
uint64_t start_flush;
uint64_t stop_flush;
uint64_t intvl_flush;
uint64_t start_read;
uint64_t stop_read;
uint64_t intvl_read;

uint64_t start_acc;
uint64_t stop_acc;
uint64_t intvl_acc;

uint64_t start_sw;
uint64_t stop_sw;
uint64_t intvl_sw;

#define SLD_HU_AUDIODEC 0x040
#define DEV_NAME "sld,hu_audiodec_rtl"

/* <<--params-->> */
const int32_t conf_0  =  0;   // 0: dummy
const int32_t conf_1  =  0;   // 1: dummy, channel is fixed a 16
// const int32_t conf_2  =  8;   // audio block size					// for the original test
const int32_t conf_2  =  1024;   // audio block size					// for the new test
// const int32_t conf_2  =  16;   // audio block size					// debug
// FIXME need to set dma_read_index and dma_write_index properly
const int32_t conf_3  =  0;   // dma_read_index  
// const int32_t conf_4  =  64;  // dma_write_index 128*32b/64b = 64 	// for the original test
const int32_t conf_4  =  8192;  // dma_write_index 16*1024*32b/64b = 8192 	// for the new test
// const int32_t conf_4  =  128;  // dma_write_index 256*32b/64b = 128 	// debug
const int32_t conf_5  =  0;   // 5-07: dummy
const int32_t conf_6  =  0;
const int32_t conf_7  =  0;

/*
// for the original test   
const int32_t conf_8  =  39413;   // 08: cfg_cos_alpha;  
const int32_t conf_9  =  60968;   // 09: cfg_sin_alpha;
const int32_t conf_10 =  -46013;   // 10: cfg_cos_beta;
const int32_t conf_11 =  -56152;   // 11: cfg_sin_beta;
const int32_t conf_12 =  -35750;   // 12: cfg_cos_gamma;
const int32_t conf_13 =  -22125;   // 13: cfg_sin_gamma;
const int32_t conf_14 =  -39414;   // 14: cfg_cos_2_alpha;
const int32_t conf_15 =  57035;   // 15: cfg_sin_2_alpha;
const int32_t conf_16 =  -15211;   // 16: cfg_cos_2_beta;
const int32_t conf_17 =  10276;   // 17: cfg_sin_2_beta;
const int32_t conf_18 =  27688;   // 18: cfg_cos_2_gamma;
const int32_t conf_19 =  -48707;   // 19: cfg_sin_2_gamma;
const int32_t conf_20 =  -42691;   // 20: cfg_cos_3_alpha;
const int32_t conf_21 =  -11292;   // 21: cfg_sin_3_alpha;
const int32_t conf_22 =  48018;   // 22: cfg_cos_3_beta;
const int32_t conf_23 =  23121;   // 23: cfg_sin_3_beta;
const int32_t conf_24 =  -53190;   // 24: cfg_cos_3_gamma;
const int32_t conf_25 =  22878;   // 25: cfg_sin_3_gamma;
*/

// for the new test
// Block 0
// const int32_t conf_8 = 0;	// 08: cfg_cos_alpha;
// const int32_t conf_9 = -65536;	// 09: cfg_sin_alpha;
// const int32_t conf_10 = 65535;	// 10: cfg_cos_beta;
// const int32_t conf_11 = 274;	// 11: cfg_sin_beta;
// const int32_t conf_12 = 0;	// 12: cfg_cos_gamma;
// const int32_t conf_13 = 65536;	// 13: cfg_sin_gamma;
// const int32_t conf_14 = -65536;	// 14: cfg_cos_2_alpha;
// const int32_t conf_15 = 0;	// 15: cfg_sin_2_alpha;
// const int32_t conf_16 = 65534;	// 16: cfg_cos_2_beta;
// const int32_t conf_17 = 549;	// 17: cfg_sin_2_beta;
// const int32_t conf_18 = -65536;	// 18: cfg_cos_2_gamma;
// const int32_t conf_19 = 0;	// 19: cfg_sin_2_gamma;
// const int32_t conf_20 = 0;	// 20: cfg_cos_3_alpha;
// const int32_t conf_21 = 65536;	// 21: cfg_sin_3_alpha;
// const int32_t conf_22 = 65531;	// 22: cfg_cos_3_beta;
// const int32_t conf_23 = 823;	// 23: cfg_sin_3_beta;
// const int32_t conf_24 = 0;	// 24: cfg_cos_3_gamma;
// const int32_t conf_25 = -65536;	// 25: cfg_sin_3_gamma;

// Block 1
// const int32_t conf_8 = 0;	// 08: cfg_cos_alpha;
// const int32_t conf_9 = -65536;	// 09: cfg_sin_alpha;
// const int32_t conf_10 = 65531;	// 10: cfg_cos_beta;
// const int32_t conf_11 = 823;	// 11: cfg_sin_beta;
// const int32_t conf_12 = 0;	// 12: cfg_cos_gamma;
// const int32_t conf_13 = 65536;	// 13: cfg_sin_gamma;
// const int32_t conf_14 = -65536;	// 14: cfg_cos_2_alpha;
// const int32_t conf_15 = 0;	// 15: cfg_sin_2_alpha;
// const int32_t conf_16 = 65515;	// 16: cfg_cos_2_beta;
// const int32_t conf_17 = 1646;	// 17: cfg_sin_2_beta;
// const int32_t conf_18 = -65536;	// 18: cfg_cos_2_gamma;
// const int32_t conf_19 = 0;	// 19: cfg_sin_2_gamma;
// const int32_t conf_20 = 0;	// 20: cfg_cos_3_alpha;
// const int32_t conf_21 = 65536;	// 21: cfg_sin_3_alpha;
// const int32_t conf_22 = 65489;	// 22: cfg_cos_3_beta;
// const int32_t conf_23 = 2469;	// 23: cfg_sin_3_beta;
// const int32_t conf_24 = 0;	// 24: cfg_cos_3_gamma;
// const int32_t conf_25 = -65536;	// 25: cfg_sin_3_gamma;

// Block 2
// const int32_t conf_8 = 0;	// 08: cfg_cos_alpha;
// const int32_t conf_9 = -65536;	// 09: cfg_sin_alpha;
// const int32_t conf_10 = 65522;	// 10: cfg_cos_beta;
// const int32_t conf_11 = 1372;	// 11: cfg_sin_beta;
// const int32_t conf_12 = 0;	// 12: cfg_cos_gamma;
// const int32_t conf_13 = 65536;	// 13: cfg_sin_gamma;
// const int32_t conf_14 = -65536;	// 14: cfg_cos_2_alpha;
// const int32_t conf_15 = 0;	// 15: cfg_sin_2_alpha;
// const int32_t conf_16 = 65479;	// 16: cfg_cos_2_beta;
// const int32_t conf_17 = 2743;	// 17: cfg_sin_2_beta;
// const int32_t conf_18 = -65536;	// 18: cfg_cos_2_gamma;
// const int32_t conf_19 = 0;	// 19: cfg_sin_2_gamma;
// const int32_t conf_20 = 0;	// 20: cfg_cos_3_alpha;
// const int32_t conf_21 = 65536;	// 21: cfg_sin_3_alpha;
// const int32_t conf_22 = 65407;	// 22: cfg_cos_3_beta;
// const int32_t conf_23 = 4113;	// 23: cfg_sin_3_beta;
// const int32_t conf_24 = 0;	// 24: cfg_cos_3_gamma;
// const int32_t conf_25 = -65536;	// 25: cfg_sin_3_gamma;

// Block 3
const int32_t conf_8 = 0;	// 08: cfg_cos_alpha;
const int32_t conf_9 = -65536;	// 09: cfg_sin_alpha;
const int32_t conf_10 = 65508;	// 10: cfg_cos_beta;
const int32_t conf_11 = 1920;	// 11: cfg_sin_beta;
const int32_t conf_12 = 0;	// 12: cfg_cos_gamma;
const int32_t conf_13 = 65536;	// 13: cfg_sin_gamma;
const int32_t conf_14 = -65536;	// 14: cfg_cos_2_alpha;
const int32_t conf_15 = 0;	// 15: cfg_sin_2_alpha;
const int32_t conf_16 = 65423;	// 16: cfg_cos_2_beta;
const int32_t conf_17 = 3839;	// 17: cfg_sin_2_beta;
const int32_t conf_18 = -65536;	// 18: cfg_cos_2_gamma;
const int32_t conf_19 = 0;	// 19: cfg_sin_2_gamma;
const int32_t conf_20 = 0;	// 20: cfg_cos_3_alpha;
const int32_t conf_21 = 65536;	// 21: cfg_sin_3_alpha;
const int32_t conf_22 = 65283;	// 22: cfg_cos_3_beta;
const int32_t conf_23 = 5754;	// 23: cfg_sin_3_beta;
const int32_t conf_24 = 0;	// 24: cfg_cos_3_gamma;
const int32_t conf_25 = -65536;	// 25: cfg_sin_3_gamma;


const int32_t conf_26 =  0;		// 26-31: dummy
const int32_t conf_27 =  0;
const int32_t conf_28 =  0;
const int32_t conf_29 =  0;
const int32_t conf_30 =  0;
const int32_t conf_31 =  0; 

const int32_t CFG_REGS_10_REG = 4*16;
const int32_t CFG_REGS_11_REG = 4*17;
const int32_t CFG_REGS_12_REG = 4*18;
const int32_t CFG_REGS_13_REG = 4*19;
const int32_t CFG_REGS_14_REG = 4*20;
const int32_t CFG_REGS_15_REG = 4*21;
const int32_t CFG_REGS_16_REG = 4*22;
const int32_t CFG_REGS_17_REG = 4*23;
const int32_t CFG_REGS_18_REG = 4*24;
const int32_t CFG_REGS_19_REG = 4*25;
const int32_t CFG_REGS_29_REG = 4*26;
const int32_t CFG_REGS_1_REG = 4*27;
const int32_t CFG_REGS_28_REG = 4*28;
const int32_t CFG_REGS_0_REG = 4*29;
const int32_t CFG_REGS_3_REG = 4*30;
const int32_t CFG_REGS_2_REG = 4*31;
const int32_t CFG_REGS_5_REG = 4*32;
const int32_t CFG_REGS_4_REG = 4*33;
const int32_t CFG_REGS_7_REG = 4*34;
const int32_t CFG_REGS_6_REG = 4*35;
const int32_t CFG_REGS_21_REG = 4*36;
const int32_t CFG_REGS_9_REG = 4*37;
const int32_t CFG_REGS_20_REG = 4*38;
const int32_t CFG_REGS_8_REG = 4*39;
const int32_t CFG_REGS_23_REG = 4*40;
const int32_t CFG_REGS_22_REG = 4*41;
const int32_t CFG_REGS_25_REG = 4*42;
const int32_t CFG_REGS_24_REG = 4*43;
const int32_t CFG_REGS_27_REG = 4*44;
const int32_t CFG_REGS_26_REG = 4*45;
const int32_t CFG_REGS_30_REG = 4*46;
const int32_t CFG_REGS_31_REG = 4*47;

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned mem_size;

// #define ESP
#define COH_MODE 0
/* 3 - Owner Prediction, 2 - Write-through forwarding, 1 - Baseline Spandex (ReqV), 0 - Baseline Spandex (MESI) */
/* 3 - Non-Coherent DMA, 2 - LLC Coherent DMA, 1 - Coherent DMA, 0 - Fully Coherent MESI */

/* Size of the contiguous chunks for scatter/gather */
#define CHUNK_SHIFT 20
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK(_sz) ((_sz % CHUNK_SIZE == 0) ?	\
		     (_sz / CHUNK_SIZE) :	\
		     (_sz / CHUNK_SIZE) + 1)

void load_aq() {
#ifndef ESP
	#if(COH_MODE == 1)
		asm volatile ("fence r, r");
	#endif
#endif
}

/* User defined registers */
/* <<--regs-->> */

static int validate_buf(token_t* out, token_t* gold, unsigned* coherence_addr, struct esp_device *dev)
{
    int i, j;
    unsigned errors = 0;
	int32_t value_32_1, value_32_2;

	// replace the following 8 lines of code with "read" in the cache coherence part
    // for (i = 0; i < 128; i++) {
	// if (gold[i] != out[i]) {
	//     printf("%d : %d <-- error \n", gold[i], out[i]);
	//     errors++;
	// } else {
	//     printf("%d : %d\n", gold[i], out[i]);
	// }
    // }

	// the cache coherence part
	int64_t value_64;
	void* dst = (void*) out;
	printf("accOut = [\n");	// for the new testing
#ifndef ESP
	spandex_config_t spandex_config;
	*coherence_addr = ACC_COH_FULL;

	#if (COH_MODE == 3)
		/* ********************************************************** */
		/* Owner Prediction */
		/* ********************************************************** */
		printf("Owner Prediction\n");

		spandex_config.spandex_reg = 0;
		spandex_config.r_en = 1;
		spandex_config.r_type = 2;
		spandex_config.w_en = 1;
		spandex_config.w_op = 1;
		spandex_config.w_type = 1;
		iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);

		// read
		start_read = get_counter();
		// for (j = 0; j < 128; j += 2)	// for the original test
		for (j = 0; j < 16384; j += 2)	// for the new test
 	   	{
			asm volatile (
				"mv t0, %1;"
				".word 0x4002B30B;"
				"mv %0, t1"
				: "=r" (value_64)	// take 2 int32 values, de-coallase them, and do the comparison
				: "r" (dst)
				: "t0", "t1", "memory"
			);

			value_32_1 = value_64 & 0xFFFFFFFF;
			value_32_2 = (value_64 >> 32) & 0xFFFFFFFF;

			printf("%ld, %ld, ", value_32_1, value_32_2);	// for the new test
			if ((j + 2) % 32 == 0) {						// for the new test
				printf("\n");								// for the new test
			}												// for the new test

			if (gold[j] != value_32_1) {
				// printf("%d : %d <-- error \n", gold[j], value_32_1);
				++errors;
			}
			else {
				// printf("%d : %d \n", gold[j], value_32_1);
			}
			if (gold[j + 1] != value_32_2) {
				// printf("%d : %d <-- error \n", gold[j + 1], value_32_2);
				++errors;
			}
			else {
				// printf("%d : %d \n", gold[j + 1], value_32_2);
			}

			dst += 8;
 	   	}
		stop_read = get_counter();
		intvl_read += stop_read - start_read;
	
	#elif (COH_MODE == 2)
		/* ********************************************************** */
		/* Write-through forwarding */
		/* ********************************************************** */
		printf("Write-through forwarding\n");

		spandex_config.spandex_reg = 0;
		spandex_config.r_en = 1;
		spandex_config.r_type = 2;
		spandex_config.w_en = 1;
		spandex_config.w_type = 1;
		iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);

		// read
		start_read = get_counter();
 	   	// for (j = 0; j < 128; j += 2)	// for the original test
		for (j = 0; j < 16384; j += 2)	// for the new test
 	   	{
			asm volatile (
				"mv t0, %1;"
				".word 0x4002B30B;"
				"mv %0, t1"
				: "=r" (value_64)
				: "r" (dst)
				: "t0", "t1", "memory"
			);

			value_32_1 = value_64 & 0xFFFFFFFF;
			value_32_2 = (value_64 >> 32) & 0xFFFFFFFF;

			printf("%ld, %ld, ", value_32_1, value_32_2);	// for the new test
			if ((j + 2) % 32 == 0) {						// for the new test
				printf("\n");								// for the new test
			}												// for the new test

			if (gold[j] != value_32_1) {
				// printf("%d : %d <-- error \n", gold[j], value_32_1);
				++errors;
			}
			else {
				// printf("%d : %d \n", gold[j], value_32_1);
			}
			if (gold[j + 1] != value_32_2) {
				// printf("%d : %d <-- error \n", gold[j + 1], value_32_2);
				++errors;
			}
			else {
				// printf("%d : %d \n", gold[j + 1], value_32_2);
			}

			dst += 8;
 	   	}
      	stop_read = get_counter();
		intvl_read += stop_read - start_read;

	#elif (COH_MODE == 1)
		/* ********************************************************** */
		/* Baseline Spandex (ReqV) */
		/* ********************************************************** */
		printf("Baseline Spandex (ReqV)\n");

		spandex_config.spandex_reg = 0;
		spandex_config.r_en = 1;
		spandex_config.r_type = 1;
		iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);

		// read
		start_read = get_counter();
		load_aq();
 	   	// for (j = 0; j < 128; j += 2)	// for the original test
		for (j = 0; j < 16384; j += 2)	// for the new test
 	   	{
			asm volatile (
				"mv t0, %1;"
				".word 0x2002B30B;"
				"mv %0, t1"
				: "=r" (value_64)
				: "r" (dst)
				: "t0", "t1", "memory"
			);

			value_32_1 = value_64 & 0xFFFFFFFF;
			value_32_2 = (value_64 >> 32) & 0xFFFFFFFF;

			printf("%ld, %ld, ", value_32_1, value_32_2);	// for the new test
			if ((j + 2) % 32 == 0) {						// for the new test
				printf("\n");								// for the new test
			}												// for the new test

			if (gold[j] != value_32_1) {
				// printf("%d : %d <-- error \n", gold[j], value_32_1);
				++errors;
			}
			else {
				// printf("%d : %d \n", gold[j], value_32_1);
			}
			if (gold[j + 1] != value_32_2) {
				// printf("%d : %d <-- error \n", gold[j + 1], value_32_2);
				++errors;
			}
			else {
				// printf("%d : %d \n", gold[j + 1], value_32_2);
			}

			dst += 8;
 	   	}
      	stop_read = get_counter();
		intvl_read += stop_read - start_read;

	#else
		/* ********************************************************** */
		/* Baseline Spandex (MESI) */
		/* ********************************************************** */
		printf("Baseline Spandex (MESI)\n");

		spandex_config.spandex_reg = 0;
		iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);

		// read
		start_read = get_counter();
 	   	// for (j = 0; j < 128; j += 2)	// for the original test
		for (j = 0; j < 16384; j += 2)	// for the new test
		// for (j = 0; j < 256; j += 2)	// debug
 	   	{
			asm volatile (
				"mv t0, %1;"
				".word 0x0002B30B;"
				"mv %0, t1"
				: "=r" (value_64)
				: "r" (dst)
				: "t0", "t1", "memory"
			);

			value_32_1 = value_64 & 0xFFFFFFFF;
			value_32_2 = (value_64 >> 32) & 0xFFFFFFFF;

			printf("%ld, %ld, ", value_32_1, value_32_2);	// for the new test
			if ((j + 2) % 32 == 0) {						// for the new test
				printf("\n");								// for the new test
			}												// for the new test

			if (gold[j] != value_32_1) {
				// printf("%d : %d <-- error \n", gold[j], value_32_1);
				++errors;
			}
			else {
				// printf("%d : %d \n", gold[j], value_32_1);
			}
			if (gold[j + 1] != value_32_2) {
				// printf("%d : %d <-- error \n", gold[j + 1], value_32_2);
				++errors;
			}
			else {
				// printf("%d : %d \n", gold[j + 1], value_32_2);
			}
			dst += 8;
 	   	}
      	stop_read = get_counter();
		intvl_read += stop_read - start_read;

	#endif
#else
	#if (COH_MODE == 3)
		/* ********************************************************** */
		/* Non-Coherent DMA */
		/* ********************************************************** */
		printf("Non-Coherent DMA\n");
		*coherence_addr = ACC_COH_NONE;

		// read
		start_read = get_counter();
 	   	// for (j = 0; j < 128; j += 2)	// for the original test
		for (j = 0; j < 16384; j += 2)	// for the new test
 	   	{
			asm volatile (
				"mv t0, %1;"
				".word 0x0002B30B;"
				"mv %0, t1"
				: "=r" (value_64)
				: "r" (dst)
				: "t0", "t1", "memory"
			);

			value_32_1 = value_64 & 0xFFFFFFFF;
			value_32_2 = (value_64 >> 32) & 0xFFFFFFFF;

			printf("%ld, %ld, ", value_32_1, value_32_2);	// for the new test
			if ((j + 2) % 32 == 0) {						// for the new test
				printf("\n");								// for the new test
			}												// for the new test

			if (gold[j] != value_32_1) {
				// printf("%d : %d <-- error \n", gold[j], value_32_1);
				++errors;
			}
			else {
				// printf("%d : %d \n", gold[j], value_32_1);
			}
			if (gold[j + 1] != value_32_2) {
				// printf("%d : %d <-- error \n", gold[j + 1], value_32_2);
				++errors;
			}
			else {
				// printf("%d : %d \n", gold[j + 1], value_32_2);
			}

			dst += 8;
 	   	}
      	stop_read = get_counter();
		intvl_read += stop_read - start_read;
	
	#elif (COH_MODE == 2)
		/* ********************************************************** */
		/* LLC Coherent DMA */
		/* ********************************************************** */
		printf("LLC Coherent DMA\n");
		*coherence_addr = ACC_COH_LLC;

		//read
		start_read = get_counter();
 	   	// for (j = 0; j < 128; j += 2)	// for the original test
		for (j = 0; j < 16384; j += 2)	// for the new test
 	   	{
			asm volatile (
				"mv t0, %1;"
				".word 0x0002B30B;"
				"mv %0, t1"
				: "=r" (value_64)
				: "r" (dst)
				: "t0", "t1", "memory"
			);

			value_32_1 = value_64 & 0xFFFFFFFF;
			value_32_2 = (value_64 >> 32) & 0xFFFFFFFF;

			printf("%ld, %ld, ", value_32_1, value_32_2);	// for the new test
			if ((j + 2) % 32 == 0) {						// for the new test
				printf("\n");								// for the new test
			}												// for the new test

			if (gold[j] != value_32_1) {
				// printf("%d : %d <-- error \n", gold[j], value_32_1);
				++errors;
			}
			else {
				// printf("%d : %d \n", gold[j], value_32_1);
			}
			if (gold[j + 1] != value_32_2) {
				// printf("%d : %d <-- error \n", gold[j + 1], value_32_2);
				++errors;
			}
			else {
				// printf("%d : %d \n", gold[j + 1], value_32_2);
			}

			dst += 8;
 	   	}
      	stop_read = get_counter();
		intvl_read += stop_read - start_read;
	
	#elif (COH_MODE == 1)
		/* ********************************************************** */
		/* Coherent DMA */
		/* ********************************************************** */
		printf("Coherent DMA\n");
		*coherence_addr = ACC_COH_RECALL;

		// read
		start_read = get_counter();
 	   	// for (j = 0; j < 128; j += 2)	// for the original test
		for (j = 0; j < 16384; j += 2)	// for the new test
 	   	{
			asm volatile (
				"mv t0, %1;"
				".word 0x0002B30B;"
				"mv %0, t1"
				: "=r" (value_64)
				: "r" (dst)
				: "t0", "t1", "memory"
			);

			value_32_1 = value_64 & 0xFFFFFFFF;
			value_32_2 = (value_64 >> 32) & 0xFFFFFFFF;

			printf("%ld, %ld, ", value_32_1, value_32_2);	// for the new test
			if ((j + 2) % 32 == 0) {						// for the new test
				printf("\n");								// for the new test
			}												// for the new test

			if (gold[j] != value_32_1) {
				// printf("%d : %d <-- error \n", gold[j], value_32_1);
				++errors;
			}
			else {
				// printf("%d : %d \n", gold[j], value_32_1);
			}
			if (gold[j + 1] != value_32_2) {
				// printf("%d : %d <-- error \n", gold[j + 1], value_32_2);
				++errors;
			}
			else {
				// printf("%d : %d \n", gold[j + 1], value_32_2);
			}

			dst += 8;
 	   	}
      	stop_read = get_counter();
		intvl_read += stop_read - start_read;
	
	#else
		/* ********************************************************** */
		/* Fully Coherent MESI */
		/* ********************************************************** */
		printf("Fully Coherent MESI\n");
		*coherence_addr = ACC_COH_FULL;

		// read
		start_read = get_counter();
 	   	// for (j = 0; j < 128; j += 2)	// for the original test
		for (j = 0; j < 16384; j += 2)	// for the new test
 	   	{
			asm volatile (
				"mv t0, %1;"
				".word 0x0002B30B;"
				"mv %0, t1"
				: "=r" (value_64)	// take 2 int32 values, de-coallase them, and do the comparison
				: "r" (dst)
				: "t0", "t1", "memory"
			);

			value_32_1 = value_64 & 0xFFFFFFFF;
			value_32_2 = (value_64 >> 32) & 0xFFFFFFFF;

			printf("%ld, %ld, ", value_32_1, value_32_2);	// for the new test
			if ((j + 2) % 32 == 0) {						// for the new test
				printf("\n");								// for the new test
			}												// for the new test

			if (gold[j] != value_32_1) {
				// printf("%d : %d <-- error \n", gold[j], value_32_1);
				++errors;
			}
			else {
				// printf("%d : %d \n", gold[j], value_32_1);
			}
			if (gold[j + 1] != value_32_2) {
				// printf("%d : %d <-- error \n", gold[j + 1], value_32_2);
				++errors;
			}
			else {
				// printf("%d : %d \n", gold[j + 1], value_32_2);
			}

			dst += 8;
 	   	}
      	stop_read = get_counter();
		intvl_read += stop_read - start_read;

	#endif
#endif

	printf("]\n");	// for the new test

    return errors;
}


static void init_buf (token_t* in, token_t* gold, unsigned* coherence_addr, struct esp_device *dev)
{
    int i;
    int j;
	int32_t value_32_1, value_32_2;

    // audio_in[8][16] block size of 8 and 16 channels
    // int32_t audio_in[128] = {	// for the original test
	// 0, 46956, 20486, -7767, 8080, -15972, 48876, -61539, 17530, 6186, 55830, 1599, -17846, -46806, -22604, -48556,
	// 0, 14640, -6849, 39524, -39538, 49604, -62424, 30140, 39773, 57357, 30140, -20913, 1893, 5786, 6343, -39217,
	// 0, 31444, 59546, -24019, -39073, 52264, 10492, -53281, 4469, -21024, -14012, -17243, -37146, 19890, 14424, -2648,
	// 0, 39511, 45462, 1795, -55876, 31378, 2549, -48556, 31496, -8206, 12537, -6489, -35705, -32139, -1967, 4338,
	// 0, -8546, 52, 18920, -23613, -62404, 4194, -41072, 14693, 16495, 11429, -10211, -25297, -55149, -14956, 51098,
	// 0, 42283, -45751, -5604, 44479, 5767, -45574, 21889, -8278, -39728, 2824, 58903, -17204, 1644, -25900, -16666,
	// 0, 35730, -60104, 14955, -60195, -9982, 45278, -25055, -52849, 63714, 144, -28004, 20289, 57271, -8363, -2471,
	// 0, -603, -17682, 63740, -17636, 62023, 17897, 29314, 17137, 43312, -2255, 23802, 5426, 9083, 20270, 2097
    // };

    // audio_out[8][16] block size of 8, and 16 channels 
    // reference output
    // int32_t audio_out[128] = {	// for the original test
	// 0, -34519, -47810, 12544, 67691, -41491, -52565, 5253, -15552, -39352, -23718, -20251, -95005, 14864, 33952, -12760, 
	// 0, 8373, -27227, 20559, -84367, -675, 25144, -11761, -56951, 10040, -12294, 116, -30892, 15657, 2782, -19321, 
	// 0, -43239, -54495, 19574, 2178, -34299, -5258, 6577, -53470, 20580, 44490, -2974, -11615, -3359, -12250, 24679, 
	// 0, -34170, -64338, 28282, -12197, -36253, -42777, 10233, -56617, -22730, 24930, -12662, -42978, 1135, 32743, 5401, 
	// 0, 11577, -2974, 9014, 23693, -559, -104591, 1797, 13873, -37102, 28600, -4373, -26764, 3208, 61310, -26930, 
	// 0, -12007, 1306, -17954, -7669, 20787, 43619, -39543, -3618, -5218, -23187, 6950, -32588, -16500, 6610, 35079, 
	// 0, 3242, 6013, -14474, 3569, 13586, -35038, 49086, 44199, 57690, 1437, 31054, 16639, 14147, -32750, -39608, 
	// 0, 28706, -19949, 26343, -33510, -23263, 63536, 24505, -30473, 4608, -14731, 9331, 26619, -11471, 13574, 13357
    // };

    // pack int16_t into int32_t data, little endian
	// replaced the following 3 lines of code with "write" in the cache coherence part
    // for (i = 0; i < 128; i++) {
	// in[i] = audio_in[i + 1];
    // }

	// the cache coherence part
	int64_t value_64;
	void* dst = (void*) in;
#ifndef ESP
	spandex_config_t spandex_config;
	*coherence_addr = ACC_COH_FULL;

	#if (COH_MODE == 3)
		/* ********************************************************** */
		/* Owner Prediction */
		/* ********************************************************** */
		printf("Owner Prediction\n");

		spandex_config.spandex_reg = 0;
		spandex_config.r_en = 1;
		spandex_config.r_type = 2;
		spandex_config.w_en = 1;
		spandex_config.w_op = 1;
		spandex_config.w_type = 1;
		iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);
		// write
		start_write = get_counter();
		// for (j = 0; j < 128; j += 2)	// for the original test
		for (j = 0; j < 16384; j += 2)	// for the new test
    	{
			// value_32_1 = audio_in[j];	// for the original test
			// value_32_2 = audio_in[j + 1];	// for the original test
			value_32_1 = new_audio_in[j];	// for the new test
			value_32_2 = new_audio_in[j + 1];	// for the new test
			value_64 = ((int64_t) value_32_1) & 0xFFFFFFFF;
			value_64 |= (((int64_t) value_32_2) << 32) & 0xFFFFFFFF00000000;
			asm volatile (
				"mv t0, %0;"
				"mv t1, %1;"
				".word 0x2262B82B"
				: 
				: "r" (dst), "r" (value_64)
				: "t0", "t1", "memory"
			);

			dst += 8;
    	}
		stop_write = get_counter();
		intvl_write += stop_write - start_write;
	
	#elif (COH_MODE == 2)
		/* ********************************************************** */
		/* Write-through forwarding */
		/* ********************************************************** */
		printf("Write-through forwarding\n");

		spandex_config.spandex_reg = 0;
		spandex_config.r_en = 1;
		spandex_config.r_type = 2;
		spandex_config.w_en = 1;
		spandex_config.w_type = 1;
		iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);

		// write
		start_write = get_counter();
    	// for (j = 0; j < 128; j += 2)	// for the original test
		for (j = 0; j < 16384; j += 2)	// for the new test
    	{
			// value_32_1 = audio_in[j];	// for the original test
			// value_32_2 = audio_in[j + 1];	// for the original test
			value_32_1 = new_audio_in[j];	// for the new test
			value_32_2 = new_audio_in[j + 1];	// for the new test
			value_64 = ((int64_t) value_32_1) & 0xFFFFFFFF;
			value_64 |= (((int64_t) value_32_2) << 32) & 0xFFFFFFFF00000000;
			asm volatile (
				"mv t0, %0;"
				"mv t1, %1;"
				".word 0x2062B02B"
				: 
				: "r" (dst), "r" (value_64)
				: "t0", "t1", "memory"
			);

			dst += 8;
    	}
      	stop_write = get_counter();
		intvl_write += stop_write - start_write;

	#elif (COH_MODE == 1)
		/* ********************************************************** */
		/* Baseline Spandex (ReqV) */
		/* ********************************************************** */
		printf("Baseline Spandex (ReqV)\n");

		spandex_config.spandex_reg = 0;
		spandex_config.r_en = 1;
		spandex_config.r_type = 1;
		iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);

		// write
		start_write = get_counter();
    	// for (j = 0; j < 128; j += 2)	// for the original test
		for (j = 0; j < 16384; j += 2)	// for the new test
    	{
			// value_32_1 = audio_in[j];	// for the original test
			// value_32_2 = audio_in[j + 1];	// for the original test
			value_32_1 = new_audio_in[j];	// for the new test
			value_32_2 = new_audio_in[j + 1];	// for the new test
			value_64 = ((int64_t) value_32_1) & 0xFFFFFFFF;
			value_64 |= (((int64_t) value_32_2) << 32) & 0xFFFFFFFF00000000;
			asm volatile (
				"mv t0, %0;"
				"mv t1, %1;"
				".word 0x0062B02B"
				: 
				: "r" (dst), "r" (value_64)
				: "t0", "t1", "memory"
			);

			dst += 8;
    	}
      	stop_write = get_counter();
		intvl_write += stop_write - start_write;

	#else
		/* ********************************************************** */
		/* Baseline Spandex (MESI) */
		/* ********************************************************** */
		printf("Baseline Spandex (MESI)\n");

		spandex_config.spandex_reg = 0;
		iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);
		
		// write
		start_write = get_counter();
    	// for (j = 0; j < 128; j += 2)	// for the original test
		for (j = 0; j < 16384; j += 2)	// for the new test
		// for (j = 0; j < 256; j += 2)	// debug
    	{
			// value_32_1 = audio_in[j];	// for the original test
			// value_32_2 = audio_in[j + 1];	// for the original test
			value_32_1 = new_audio_in[j];	// for the new test
			value_32_2 = new_audio_in[j + 1];	// for the new test
			value_64 = ((int64_t) value_32_1) & 0xFFFFFFFF;
			value_64 |= (((int64_t) value_32_2) << 32) & 0xFFFFFFFF00000000;
			asm volatile (
				"mv t0, %0;"
				"mv t1, %1;"
				".word 0x0062B02B"
				: 
				: "r" (dst), "r" (value_64)
				: "t0", "t1", "memory"
			);
			dst += 8;
    	}
      	stop_write = get_counter();
		intvl_write += stop_write - start_write;

	#endif
#else
	#if (COH_MODE == 3)
		/* ********************************************************** */
		/* Non-Coherent DMA */
		/* ********************************************************** */
		printf("Non-Coherent DMA\n");
		*coherence_addr = ACC_COH_NONE;

		// write
		start_write = get_counter();
    	// for (j = 0; j < 128; j += 2)	// for the original test
		for (j = 0; j < 16384; j += 2)	// for the new test
    	{
			// value_32_1 = audio_in[j];	// for the original test
			// value_32_2 = audio_in[j + 1];	// for the original test
			value_32_1 = new_audio_in[j];	// for the new test
			value_32_2 = new_audio_in[j + 1];	// for the new test
			value_64 = ((int64_t) value_32_1) & 0xFFFFFFFF;
			value_64 |= (((int64_t) value_32_2) << 32) & 0xFFFFFFFF00000000;
			asm volatile (
				"mv t0, %0;"
				"mv t1, %1;"
				".word 0x0062B02B"
				: 
				: "r" (dst), "r" (value_64)
				: "t0", "t1", "memory"
			);

			dst += 8;
    	}
		stop_write = get_counter();
		start_flush = get_counter();
		esp_flush(*coherence_addr);
      	stop_flush = get_counter();
		intvl_write += stop_write - start_write;
		intvl_flush += stop_flush - start_flush;
	
	#elif (COH_MODE == 2)
		/* ********************************************************** */
		/* LLC Coherent DMA */
		/* ********************************************************** */
		printf("LLC Coherent DMA\n");
		*coherence_addr = ACC_COH_LLC;

		// write
		start_write = get_counter();
    	// for (j = 0; j < 128; j += 2)	// for the original test
		for (j = 0; j < 16384; j += 2)	// for the new test
    	{
			// value_32_1 = audio_in[j];	// for the original test
			// value_32_2 = audio_in[j + 1];	// for the original test
			value_32_1 = new_audio_in[j];	// for the new test
			value_32_2 = new_audio_in[j + 1];	// for the new test
			value_64 = ((int64_t) value_32_1) & 0xFFFFFFFF;
			value_64 |= (((int64_t) value_32_2) << 32) & 0xFFFFFFFF00000000;
			asm volatile (
				"mv t0, %0;"
				"mv t1, %1;"
				".word 0x0062B02B"
				: 
				: "r" (dst), "r" (value_64)
				: "t0", "t1", "memory"
			);
			dst += 8;
    	}
		stop_write = get_counter();
		start_flush = get_counter();
		esp_flush(*coherence_addr);
      	stop_flush = get_counter();
		intvl_write += stop_write - start_write;
		intvl_flush += stop_flush - start_flush;
	
	#elif (COH_MODE == 1)
		/* ********************************************************** */
		/* Coherent DMA */
		/* ********************************************************** */
		printf("Coherent DMA\n");
		*coherence_addr = ACC_COH_RECALL;

		// write
		start_write = get_counter();
    	// for (j = 0; j < 128; j += 2)	// for the original test
		for (j = 0; j < 16384; j += 2)	// for the new test
    	{
			// value_32_1 = audio_in[j];	// for the original test
			// value_32_2 = audio_in[j + 1];	// for the original test
			value_32_1 = new_audio_in[j];	// for the new test
			value_32_2 = new_audio_in[j + 1];	// for the new test
			value_64 = ((int64_t) value_32_1) & 0xFFFFFFFF;
			value_64 |= (((int64_t) value_32_2) << 32) & 0xFFFFFFFF00000000;
			asm volatile (
				"mv t0, %0;"
				"mv t1, %1;"
				".word 0x0062B02B"
				: 
				: "r" (dst), "r" (value_64)
				: "t0", "t1", "memory"
			);
			dst += 8;
    	}
		stop_write = get_counter();
		start_flush = get_counter();
		esp_flush(*coherence_addr);
      	stop_flush = get_counter();
		intvl_write += stop_write - start_write;
		intvl_flush += stop_flush - start_flush;
	
	#else
		/* ********************************************************** */
		/* Fully Coherent MESI */
		/* ********************************************************** */
		printf("Fully Coherent MESI\n");
		*coherence_addr = ACC_COH_FULL;

		// write
		start_write = get_counter();
    	// for (j = 0; j < 128; j += 2)	// for the original test
		for (j = 0; j < 16384; j += 2)	// for the new test
    	{
			// value_32_1 = audio_in[j];	// for the original test
			// value_32_2 = audio_in[j + 1];	// for the original test
			value_32_1 = new_audio_in[j];	// for the new test
			value_32_2 = new_audio_in[j + 1];	// for the new test
			value_64 = ((int64_t) value_32_1) & 0xFFFFFFFF;
			value_64 |= (((int64_t) value_32_2) << 32) & 0xFFFFFFFF00000000;
			asm volatile (
				"mv t0, %0;"
				"mv t1, %1;"
				".word 0x0062B02B"
				: 
				: "r" (dst), "r" (value_64)
				: "t0", "t1", "memory"
			);
			dst += 8;
    	}
		stop_write = get_counter();
		start_flush = get_counter();
		esp_flush(*coherence_addr);
      	stop_flush = get_counter();
		intvl_write += stop_write - start_write;
		intvl_flush += stop_flush - start_flush;
	
	#endif
#endif

    // for (i = 0; i < 128; i++) {	// for the original test
	for (i = 0; i < 16384; i++) {	// for the new test
	// for (i = 0; i < 256; i++) {	// debug
	// gold[i] = audio_out[i];		// for the original test
	gold[i] = new_audio_out[i];		// for the new test
    }
}

void rotateOrder_sw() {
	int i, error;
	/* The original code */
	// int32_t output[128];
	// int32_t valueX, valueY, valueZ, valueR, valueS, valueT, valueU, valueV, valueK, valueL, valueM, valueN, valueO, valueP, valueQ;
	// int32_t vX, vY, vZ, vR, vS, vT, vU, vV, vK, vL, vM, vN, vO, vP, vQ;

	/* The new code */
	// float output[128];	// for the original test
	float output[16384];	// for the new test
	// float output[256];	// debug
	float valueX, valueY, valueZ, valueR, valueS, valueT, valueU, valueV, valueK, valueL, valueM, valueN, valueO, valueP, valueQ;
	float vX, vY, vZ, vR, vS, vT, vU, vV, vK, vL, vM, vN, vO, vP, vQ;

	/* The new code */	// for the original test
	// float conf_8_f = 0.6013946533203125;
	// float conf_9_f = 0.9302978515625;
	// float conf_10_f = -0.7021026611328125;
	// float conf_11_f = -0.8568115234375;
	// float conf_12_f = -0.545501708984375;
	// float conf_13_f = -0.3376007080078125;
	// float conf_14_f = -0.601409912109375;
	// float conf_15_f = 0.8702850341796875;
	// float conf_16_f = -0.2321014404296875;
	// float conf_17_f = 0.15679931640625;
	// float conf_18_f = 0.4224853515625;
	// float conf_19_f = -0.7432098388671875;
	// float conf_20_f = -0.6514129638671875;
	// float conf_21_f = -0.17230224609375;
	// float conf_22_f = 0.732696533203125;
	// float conf_23_f = 0.3527984619140625;
	// float conf_24_f = -0.811614990234375;
	// float conf_25_f = 0.349090576171875;

	/* The original code */
	// int32_t sinBetaSq = conf_11 * conf_11;
	// int32_t sinBetaCb = conf_11 * conf_11 * conf_11;
	// int32_t fSqrt3 = 333333;		// random numbers just for timing testing
	// int32_t fSqrt3_2 = 323232;	// random numbers just for timing testing
	// int32_t fSqrt15 = 151515;	// random numbers just for timing testing
	// int32_t fSqrt5_2 = 525252;	// random numbers just for timing testing
	// int32_t fxp025 = 25252;		// random numbers just for timing testing
	// int32_t fxp050 = 50505;		// random numbers just for timing testing
	// int32_t fxp075 = 75757;		// random numbers just for timing testing
	// int32_t fxp00625 = 62562;	// random numbers just for timing testing
	// int32_t fxp0125 = 12512;	// random numbers just for timing testing

	/* The new code */
	float sinBetaSq = conf_11_f * conf_11_f;
	float sinBetaCb = conf_11_f * conf_11_f * conf_11_f;
	float fSqrt3 = 1.7320508;
	float fSqrt3_2 = 1.2247448;
	float fSqrt15 = 3.8729833;
	float fSqrt5_2 = 1.5811388;
	float fxp025 = 0.25;
	float fxp050 = 0.5;
	float fxp075 = 0.75;
	float fxp00625 = 0.0625;
	float fxp0125 = 0.125;
	
	// audio_in[8][16] block size of 8 and 16 channels
	/* The original code */
	// int32_t audio_in[128] = {
	// 0, 46956, 20486, -7767, 8080, -15972, 48876, -61539, 17530, 6186, 55830, 1599, -17846, -46806, -22604, -48556,
	// 0, 14640, -6849, 39524, -39538, 49604, -62424, 30140, 39773, 57357, 30140, -20913, 1893, 5786, 6343, -39217,
	// 0, 31444, 59546, -24019, -39073, 52264, 10492, -53281, 4469, -21024, -14012, -17243, -37146, 19890, 14424, -2648,
	// 0, 39511, 45462, 1795, -55876, 31378, 2549, -48556, 31496, -8206, 12537, -6489, -35705, -32139, -1967, 4338,
	// 0, -8546, 52, 18920, -23613, -62404, 4194, -41072, 14693, 16495, 11429, -10211, -25297, -55149, -14956, 51098,
	// 0, 42283, -45751, -5604, 44479, 5767, -45574, 21889, -8278, -39728, 2824, 58903, -17204, 1644, -25900, -16666,
	// 0, 35730, -60104, 14955, -60195, -9982, 45278, -25055, -52849, 63714, 144, -28004, 20289, 57271, -8363, -2471,
	// 0, -603, -17682, 63740, -17636, 62023, 17897, 29314, 17137, 43312, -2255, 23802, 5426, 9083, 20270, 2097
	// };

	/* The new code */	// for the original test
	// float audio_in[128] = {
	// 	0.0, 0.71649169921875, 0.312591552734375, -0.1185150146484375, 0.123291015625, -0.24371337890625, 0.74578857421875, -0.9390106201171875, 0.267486572265625, 0.094390869140625, 0.851898193359375, 0.0243988037109375, -0.272308349609375, -0.714202880859375, -0.34490966796875, -0.74090576171875, 
	// 	0.0, 0.223388671875, -0.1045074462890625, 0.60308837890625, -0.603302001953125, 0.75689697265625, -0.9525146484375, 0.45989990234375, 0.6068878173828125, 0.8751983642578125, 0.45989990234375, -0.3191070556640625, 0.0288848876953125, 0.088287353515625, 0.0967864990234375, -0.5984039306640625, 
	// 	0.0, 0.47979736328125, 0.908599853515625, -0.3665008544921875, -0.5962066650390625, 0.7974853515625, 0.16009521484375, -0.8130035400390625, 0.0681915283203125, -0.32080078125, -0.21380615234375, -0.2631072998046875, -0.566802978515625, 0.303497314453125, 0.2200927734375, -0.0404052734375, 
	// 	0.0, 0.6028900146484375, 0.693695068359375, 0.0273895263671875, -0.85260009765625, 0.478790283203125, 0.0388946533203125, -0.74090576171875, 0.4805908203125, -0.125213623046875, 0.1912994384765625, -0.0990142822265625, -0.5448150634765625, -0.4904022216796875, -0.0300140380859375, 0.066192626953125, 
	// 	0.0, -0.130401611328125, 0.00079345703125, 0.2886962890625, -0.3603057861328125, -0.95220947265625, 0.063995361328125, -0.626708984375, 0.2241973876953125, 0.2516937255859375, 0.1743927001953125, -0.1558074951171875, -0.3860015869140625, -0.8415069580078125, -0.22821044921875, 0.779693603515625, 
	// 	0.0, 0.6451873779296875, -0.6981048583984375, -0.08551025390625, 0.6786956787109375, 0.0879974365234375, -0.695404052734375, 0.3339996337890625, -0.126312255859375, -0.606201171875, 0.0430908203125, 0.8987884521484375, -0.26251220703125, 0.02508544921875, -0.39520263671875, -0.254302978515625, 
	// 	0.0, 0.545196533203125, -0.9171142578125, 0.2281951904296875, -0.9185028076171875, -0.152313232421875, 0.690887451171875, -0.3823089599609375, -0.8064117431640625, 0.972198486328125, 0.002197265625, -0.42730712890625, 0.3095855712890625, 0.8738861083984375, -0.1276092529296875, -0.0377044677734375, 
	// 	0.0, -0.0092010498046875, -0.269805908203125, 0.97259521484375, -0.26910400390625, 0.9463958740234375, 0.2730865478515625, 0.447296142578125, 0.2614898681640625, 0.660888671875, -0.0344085693359375, 0.363189697265625, 0.082794189453125, 0.1385955810546875, 0.309295654296875, 0.0319976806640625
	// };

	// audio_out[8][16] block size of 8, and 16 channels 
    // reference output
	/* The original code */
    // int32_t audio_out[128] = {
	// 0, -34519, -47810, 12544, 67691, -41491, -52565, 5253, -15552, -39352, -23718, -20251, -95005, 14864, 33952, -12760, 
	// 0, 8373, -27227, 20559, -84367, -675, 25144, -11761, -56951, 10040, -12294, 116, -30892, 15657, 2782, -19321, 
	// 0, -43239, -54495, 19574, 2178, -34299, -5258, 6577, -53470, 20580, 44490, -2974, -11615, -3359, -12250, 24679, 
	// 0, -34170, -64338, 28282, -12197, -36253, -42777, 10233, -56617, -22730, 24930, -12662, -42978, 1135, 32743, 5401, 
	// 0, 11577, -2974, 9014, 23693, -559, -104591, 1797, 13873, -37102, 28600, -4373, -26764, 3208, 61310, -26930, 
	// 0, -12007, 1306, -17954, -7669, 20787, 43619, -39543, -3618, -5218, -23187, 6950, -32588, -16500, 6610, 35079, 
	// 0, 3242, 6013, -14474, 3569, 13586, -35038, 49086, 44199, 57690, 1437, 31054, 16639, 14147, -32750, -39608, 
	// 0, 28706, -19949, 26343, -33510, -23263, 63536, 24505, -30473, 4608, -14731, 9331, 26619, -11471, 13574, 13357
    // };

	/* The new code */	// for the original test
	// float audio_out[128] = {
	// 	0.0, -0.5267181396484375, -0.729522705078125, 0.19140625, 1.0328826904296875, -0.6331024169921875, -0.8020782470703125, 0.0801544189453125, -0.2373046875, -0.6004638671875, -0.361907958984375, -0.3090057373046875, -1.4496612548828125, 0.226806640625, 0.51806640625, -0.1947021484375, 
	// 	0.0, 0.1277618408203125, -0.4154510498046875, 0.3137054443359375, -1.2873382568359375, -0.0102996826171875, 0.3836669921875, -0.1794586181640625, -0.8690032958984375, 0.1531982421875, -0.187591552734375, 0.00177001953125, -0.47137451171875, 0.2389068603515625, 0.042449951171875, -0.2948150634765625, 
	// 	0.0, -0.6597747802734375, -0.8315277099609375, 0.298675537109375, 0.033233642578125, -0.5233612060546875, -0.080230712890625, 0.1003570556640625, -0.815887451171875, 0.31402587890625, 0.678863525390625, -0.045379638671875, -0.1772308349609375, -0.0512542724609375, -0.186920166015625, 0.3765716552734375, 
	// 	0.0, -0.521392822265625, -0.981719970703125, 0.431549072265625, -0.1861114501953125, -0.5531768798828125, -0.6527252197265625, 0.1561431884765625, -0.8639068603515625, -0.346832275390625, 0.380401611328125, -0.193206787109375, -0.655792236328125, 0.0173187255859375, 0.4996185302734375, 0.0824127197265625, 
	// 	0.0, 0.1766510009765625, -0.045379638671875, 0.137542724609375, 0.3615264892578125, -0.0085296630859375, -1.5959320068359375, 0.0274200439453125, 0.2116851806640625, -0.566131591796875, 0.4364013671875, -0.0667266845703125, -0.40838623046875, 0.0489501953125, 0.935516357421875, -0.410919189453125, 
	// 	0.0, -0.1832122802734375, 0.019927978515625, -0.273956298828125, -0.1170196533203125, 0.3171844482421875, 0.6655731201171875, -0.6033782958984375, -0.055206298828125, -0.079620361328125, -0.3538055419921875, 0.106048583984375, -0.49725341796875, -0.25177001953125, 0.100860595703125, 0.5352630615234375, 
	// 	0.0, 0.049468994140625, 0.0917510986328125, -0.220855712890625, 0.0544586181640625, 0.207305908203125, -0.534637451171875, 0.748992919921875, 0.6744232177734375, 0.880279541015625, 0.0219268798828125, 0.473846435546875, 0.2538909912109375, 0.2158660888671875, -0.499725341796875, -0.6043701171875, 
	// 	0.0, 0.438018798828125, -0.3043975830078125, 0.4019622802734375, -0.511322021484375, -0.3549652099609375, 0.969482421875, 0.3739166259765625, -0.4649810791015625, 0.0703125, -0.2247772216796875, 0.1423797607421875, 0.4061737060546875, -0.1750335693359375, 0.207122802734375, 0.2038116455078125
	// };

	error = 0;
	start_sw = get_counter();

	/* The original code */
	// for (i = 0; i < 128; i += 16) {
	// 	// Rotate Order 1
	// 	valueY = -audio_in[i + 3] * conf_9 + audio_in[i + 1] * conf_8;
	// 	valueZ = audio_in[i + 2];
	// 	valueX = audio_in[i + 3] * conf_8 + audio_in[i + 1] * conf_9;

	// 	vY = valueY;
	// 	vZ = valueX * conf_11 + valueZ * conf_10;
	// 	vX = valueX * conf_10 + valueZ * conf_11;

	// 	output[i + 1] = -vX * conf_13 + vY * conf_12;
	// 	output[i + 2] = vZ;
    //  	output[i + 3] = vX * conf_12 + vY * conf_13;

	// 	// Rotate Order 2
	// 	valueV = -audio_in[i + 8] * conf_15 + audio_in[i + 4] * conf_14;
    //     valueT = -audio_in[i + 7] * conf_9 + audio_in[i + 5] * conf_8;
    //     valueR = audio_in[i + 6];
    //     valueS = audio_in[i + 7] * conf_8 + audio_in[i + 5] * conf_9;
    //     valueU = audio_in[i + 8] * conf_14 + audio_in[i + 4] * conf_15;

    //     vV = -conf_11 * valueT + conf_10 * valueV;
    // 	vT = -conf_10 * valueT + conf_11 * valueV;
	// 	vR = (fxp075 * conf_16 + fxp025) * valueR + (fxp050 * fSqrt3 * sinBetaSq) * valueU + (fSqrt3 * conf_11 * conf_10) * valueS;
	// 	vS = conf_16 * valueS - fSqrt3 * conf_10 * conf_11 * valueR + conf_10 * conf_11 * valueU;
	// 	vU = (fxp025 * conf_16 + fxp075) * valueU - conf_10 * conf_11 * valueS + fxp050 * fSqrt3 * sinBetaSq * valueR;

	// 	output[i + 4] = -vU * conf_19 + vV * conf_18;
	// 	output[i + 5] = -vS * conf_13 + vT * conf_12;
	// 	output[i + 6] = vR;
	// 	output[i + 7] = vS * conf_12 + vT * conf_13;
	// 	output[i + 8] = vU * conf_18 + vV * conf_19;

	// 	// Rotate Order 3
	// 	valueQ = -audio_in[i + 15] * conf_21 + audio_in[i + 9] * conf_20;
	// 	valueO = -audio_in[i + 14] * conf_15 + audio_in[i + 10] * conf_14;
	// 	valueM = -audio_in[i + 13] * conf_9 + audio_in[i + 11] * conf_8;
	// 	valueK = audio_in[i + 12];
	// 	valueL = audio_in[i + 13] * conf_8 + audio_in[i + 11] * conf_9;
	// 	valueN = audio_in[i + 14] * conf_14 + audio_in[i + 10] * conf_15;
	// 	valueP = audio_in[i + 15] * conf_20 + audio_in[i + 9] * conf_21;

	// 	vQ = fxp0125 * valueQ * (5 + 3 * conf_16) - fSqrt3_2 * valueO * conf_10 * conf_11 + fxp025 * fSqrt15 * valueM * sinBetaSq;
	// 	vO = valueO * conf_16 - fSqrt5_2 * valueM * conf_10 * conf_11 + fSqrt3_2 * valueQ * conf_10 * conf_11;
	// 	vM = fxp0125 * valueM * (3 + 5 * conf_16) - fSqrt5_2 * valueO * conf_10 * conf_11 + fxp025 * fSqrt15 * valueQ * sinBetaSq;
	// 	vK = fxp025 * valueK * conf_10 * (-1 + 15 * conf_16) + fxp050 * fSqrt15 * valueN * conf_10 * sinBetaSq + fxp050 * fSqrt5_2 * valueP * sinBetaCb + fxp0125 * fSqrt3_2 * valueL * (conf_11 + 5 * conf_23);
	// 	vL = fxp00625 * valueL * (conf_10 + 15 * conf_22) + fxp025 * fSqrt5_2 * valueN * (1 + 3 * conf_16) * conf_11 + fxp025 * fSqrt15 * valueP * conf_10 * sinBetaSq - fxp0125 * fSqrt3_2 * valueK * (conf_11 + 5 * conf_23);
	// 	vN = fxp0125 * valueN * (5 * conf_10 + 3 * conf_22) + fxp025 * fSqrt3_2 * valueP * (3 + conf_16) * conf_11 + fxp050 * fSqrt15 * valueK * conf_10 * sinBetaSq + fxp0125 * fSqrt5_2 * valueL * (conf_11 - 3 * conf_23);
	// 	vP = fxp00625 * valueP * (15 * conf_10 + conf_22) - fxp025 * fSqrt3_2 * valueN * (3 + conf_16) * conf_11 + fxp025 * fSqrt15 * valueL * conf_10 * sinBetaSq - fxp050 * fSqrt5_2 * valueK * sinBetaCb;

	// 	output[i + 9] = -vP * conf_25 + vQ * conf_24;
	// 	output[i + 10] = -vN * conf_19 + vO * conf_18;
	// 	output[i + 11] = -vL * conf_13 + vM * conf_12;
	// 	output[i + 12] = vK;
	// 	output[i + 13] = vL * conf_12 + vM * conf_13;
	// 	output[i + 14] = vN * conf_18 + vO * conf_19;
	// 	output[i + 15] = vP * conf_24 + vQ * conf_25;
	// }

	/* The new code */
	// for (i = 0; i < 128; i += 16) {	// for the original test
	for (i = 0; i < 16384; i += 16) {	// for the new test
	// for (i = 0; i < 256; i += 16) {	// debug
		// Rotate Order 1
		valueY = -audio_in[i + 3] * conf_9_f + audio_in[i + 1] * conf_8_f;
		valueZ = audio_in[i + 2];
		valueX = audio_in[i + 3] * conf_8_f + audio_in[i + 1] * conf_9_f;

		vY = valueY;
		vZ = valueX * conf_11_f + valueZ * conf_10_f;
		vX = valueX * conf_10_f + valueZ * conf_11_f;

		output[i + 1] = -vX * conf_13_f + vY * conf_12_f;
		output[i + 2] = vZ;
        output[i + 3] = vX * conf_12_f + vY * conf_13_f;

		// Rotate Order 2
		valueV = -audio_in[i + 8] * conf_15_f + audio_in[i + 4] * conf_14_f;
        valueT = -audio_in[i + 7] * conf_9_f + audio_in[i + 5] * conf_8_f;
        valueR = audio_in[i + 6];
        valueS = audio_in[i + 7] * conf_8_f + audio_in[i + 5] * conf_9_f;
        valueU = audio_in[i + 8] * conf_14_f + audio_in[i + 4] * conf_15_f;

        vV = -conf_11_f * valueT + conf_10_f * valueV;
    	vT = -conf_10_f * valueT + conf_11_f * valueV;
		vR = (fxp075 * conf_16_f + fxp025) * valueR + (fxp050 * fSqrt3 * sinBetaSq) * valueU + (fSqrt3 * conf_11_f * conf_10_f) * valueS;
		vS = conf_16_f * valueS - fSqrt3 * conf_10_f * conf_11_f * valueR + conf_10_f * conf_11_f * valueU;
		vU = (fxp025 * conf_16_f + fxp075) * valueU - conf_10_f * conf_11_f * valueS + fxp050 * fSqrt3 * sinBetaSq * valueR;

		output[i + 4] = -vU * conf_19_f + vV * conf_18_f;
		output[i + 5] = -vS * conf_13_f + vT * conf_12_f;
		output[i + 6] = vR;
		output[i + 7] = vS * conf_12_f + vT * conf_13_f;
		output[i + 8] = vU * conf_18_f + vV * conf_19_f;

		// Rotate Order 3
		valueQ = -audio_in[i + 15] * conf_21_f + audio_in[i + 9] * conf_20_f;
		valueO = -audio_in[i + 14] * conf_15_f + audio_in[i + 10] * conf_14_f;
		valueM = -audio_in[i + 13] * conf_9_f + audio_in[i + 11] * conf_8_f;
		valueK = audio_in[i + 12];
		valueL = audio_in[i + 13] * conf_8_f + audio_in[i + 11] * conf_9_f;
		valueN = audio_in[i + 14] * conf_14_f + audio_in[i + 10] * conf_15_f;
		valueP = audio_in[i + 15] * conf_20_f + audio_in[i + 9] * conf_21_f;

		vQ = fxp0125 * valueQ * (5 + 3 * conf_16_f) - fSqrt3_2 * valueO * conf_10_f * conf_11_f + fxp025 * fSqrt15 * valueM * sinBetaSq;
		vO = valueO * conf_16_f - fSqrt5_2 * valueM * conf_10_f * conf_11_f + fSqrt3_2 * valueQ * conf_10_f * conf_11_f;
		vM = fxp0125 * valueM * (3 + 5 * conf_16_f) - fSqrt5_2 * valueO * conf_10_f * conf_11_f + fxp025 * fSqrt15 * valueQ * sinBetaSq;
		vK = fxp025 * valueK * conf_10_f * (-1 + 15 * conf_16_f) + fxp050 * fSqrt15 * valueN * conf_10_f * sinBetaSq + fxp050 * fSqrt5_2 * valueP * sinBetaCb + fxp0125 * fSqrt3_2 * valueL * (conf_11_f + 5 * conf_23_f);
		vL = fxp00625 * valueL * (conf_10_f + 15 * conf_22_f) + fxp025 * fSqrt5_2 * valueN * (1 + 3 * conf_16_f) * conf_11_f + fxp025 * fSqrt15 * valueP * conf_10_f * sinBetaSq - fxp0125 * fSqrt3_2 * valueK * (conf_11_f + 5 * conf_23_f);
		vN = fxp0125 * valueN * (5 * conf_10_f + 3 * conf_22_f) + fxp025 * fSqrt3_2 * valueP * (3 + conf_16_f) * conf_11_f + fxp050 * fSqrt15 * valueK * conf_10_f * sinBetaSq + fxp0125 * fSqrt5_2 * valueL * (conf_11_f - 3 * conf_23_f);
		vP = fxp00625 * valueP * (15 * conf_10_f + conf_22_f) - fxp025 * fSqrt3_2 * valueN * (3 + conf_16_f) * conf_11_f + fxp025 * fSqrt15 * valueL * conf_10_f * sinBetaSq - fxp050 * fSqrt5_2 * valueK * sinBetaCb;

		output[i + 9] = -vP * conf_25_f + vQ * conf_24_f;
		output[i + 10] = -vN * conf_19_f + vO * conf_18_f;
		output[i + 11] = -vL * conf_13_f + vM * conf_12_f;
		output[i + 12] = vK;
		output[i + 13] = vL * conf_12_f + vM * conf_13_f;
		output[i + 14] = vN * conf_18_f + vO * conf_19_f;
		output[i + 15] = vP * conf_24_f + vQ * conf_25_f;
	}

	stop_sw = get_counter();
	intvl_sw += stop_sw - start_sw;

	// The 0th channel has no calculation
	// for (i = 0; i < 128; i += 16) {	// for the original test
	for (i = 0; i < 16384; i += 16) {	// for the new test
	// for (i = 0; i < 256; i += 16) {	// debug
		output[i] = audio_in[i];
	}

	/*
	// Print out the baseline for error percentage checking
	// for the new test
	printf("baseline = [\n");
	for (i = 0; i < 16384; ++i) {
		printf("%f, ", output[i]);
		if ((i + i) % 32 == 0) {
			printf("\n");
		}
	}
	printf("]\n");
	*/

	// for (i = 0; i < 128; ++i) {
	// 	if (output[i] - audio_out[i] > 0.0001 || output[i] - audio_out[i] < -0.0001) {
	// 		++error;
	// 		printf("Discrepancy found at the %d element!\n", i);
	// 		printf("The golden output: %d\n", audio_out[i]);
	// 		printf("The actual output: %d\n", output[i]);
	// 	}
	// }

	// printf("Number of errors: %d\n", error);
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
    token_t *mem;
    token_t *gold;
    unsigned errors = 0;
    unsigned coherence;

    // in_len = 128;	// for the original test
    // out_len = 128;	// for the original test
	in_len = 16384;		// for the new test
    out_len = 16384;	// for the new test
	// in_len = 256;	// debug
    // out_len = 256;	// debug

	intvl_sw = 0;
	intvl_write = 0;
	intvl_flush = 0;
	intvl_read = 0;
	intvl_acc = 0;
	
    in_size = in_len * sizeof(token_t);
    out_size = out_len * sizeof(token_t);	
    out_offset = in_len;
    mem_size = in_size + out_size;
    // Search for the device
    printf("Scanning device tree... \n");

    ndev = probe(&espdevs, VENDOR_SLD, SLD_HU_AUDIODEC, DEV_NAME);
    if (ndev == 0) {
	printf("device not found\n");
	return 0;
    }

    for (n = 0; n < ndev; n++) {

	printf("**************** %s.%d ****************\n", DEV_NAME, n);

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
	gold = aligned_malloc(out_size);
	mem = aligned_malloc(mem_size);
	printf("  memory buffer base-address = %p\n", mem);

	// Alocate and populate page table
	ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (i = 0; i < NCHUNK(mem_size); i++)
	    ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];

	printf("  ptable = %p\n", ptable);
	printf("  nchunk = %lu\n", NCHUNK(mem_size));

	/* TODO: Restore full test once ESP caches are integrated */
	// coherence = ACC_COH_RECALL;	// Spandex: ACC_COH_FULL, ESP: check the sensor_dma.c file

	printf("  --------------------\n");
	printf("  Generate input...\n");

	// Many iterations for the timing measurement
	for (i = 0; i < ITERATION; ++i) {
	// Software execution for the accelerator
	// rotateOrder_sw();

	init_buf(mem, gold, &coherence, dev);

	// Pass common configuration parameters

	iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	iowrite32(dev, COHERENCE_REG, coherence);

#ifndef __sparc
	iowrite32(dev, PT_ADDRESS_REG, (unsigned long long) ptable);
#else
	iowrite32(dev, PT_ADDRESS_REG, (unsigned) ptable);
#endif
	iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

	// Use the following if input and output data are not allocated at the default offsets
	iowrite32(dev, SRC_OFFSET_REG, 0x0);
	// TODO need to add output offset?
	iowrite32(dev, DST_OFFSET_REG, 0x0);

	// Pass accelerator-specific configuration parameters
	/* <<--regs-config-->> */
	iowrite32(dev, CFG_REGS_0_REG,  conf_0 );
	iowrite32(dev, CFG_REGS_1_REG,  conf_1 );
	iowrite32(dev, CFG_REGS_2_REG,  conf_2 );
	iowrite32(dev, CFG_REGS_3_REG,  conf_3 );
	iowrite32(dev, CFG_REGS_4_REG,  conf_4 );
	iowrite32(dev, CFG_REGS_5_REG,  conf_5 );
	iowrite32(dev, CFG_REGS_6_REG,  conf_6 );
	iowrite32(dev, CFG_REGS_7_REG,  conf_7 );
	iowrite32(dev, CFG_REGS_8_REG,  conf_8 );
	iowrite32(dev, CFG_REGS_9_REG,  conf_9 );
	iowrite32(dev, CFG_REGS_10_REG, conf_10);
	iowrite32(dev, CFG_REGS_11_REG, conf_11);
	iowrite32(dev, CFG_REGS_12_REG, conf_12);
	iowrite32(dev, CFG_REGS_13_REG, conf_13);
	iowrite32(dev, CFG_REGS_14_REG, conf_14);
	iowrite32(dev, CFG_REGS_15_REG, conf_15);
	iowrite32(dev, CFG_REGS_16_REG, conf_16);
	iowrite32(dev, CFG_REGS_17_REG, conf_17);
	iowrite32(dev, CFG_REGS_18_REG, conf_18);
	iowrite32(dev, CFG_REGS_19_REG, conf_19);
	iowrite32(dev, CFG_REGS_20_REG, conf_20);
	iowrite32(dev, CFG_REGS_21_REG, conf_21);
	iowrite32(dev, CFG_REGS_22_REG, conf_22);
	iowrite32(dev, CFG_REGS_23_REG, conf_23);
	iowrite32(dev, CFG_REGS_24_REG, conf_24);
	iowrite32(dev, CFG_REGS_25_REG, conf_25);
	iowrite32(dev, CFG_REGS_26_REG, conf_26);
	iowrite32(dev, CFG_REGS_27_REG, conf_27);
	iowrite32(dev, CFG_REGS_28_REG, conf_28);
	iowrite32(dev, CFG_REGS_29_REG, conf_29);
	iowrite32(dev, CFG_REGS_30_REG, conf_30);
	iowrite32(dev, CFG_REGS_31_REG, conf_31);


	// Flush (customize coherence model here)
	esp_flush(coherence);	// Do nothing when using the Spandex protocol

	// Start accelerators
	printf("  Start...\n");
	start_acc = get_counter();
	iowrite32(dev, CMD_REG, CMD_MASK_START);
	// load_aq();

	// Wait for completion
	done = 0;
	while (!done) {
	    done = ioread32(dev, STATUS_REG);
	    done &= STATUS_MASK_DONE;
	}
	iowrite32(dev, CMD_REG, 0x0);
	stop_acc = get_counter();
	intvl_acc += stop_acc - start_acc;

	printf("  Done\n");
	printf("  validating...\n");

	/* Validation */
	errors = validate_buf(&mem[out_offset], gold, &coherence, dev);
	if (errors)
	    printf("  ... FAIL\n");
	else
	    printf("  ... PASS\n");
    }
	}

	// Print out the cycle numbers
	printf("Coherence protocol: %u\n", coherence);
	printf("Coherence mode: %d\n", COH_MODE);
	printf("Software time: %lu\n", intvl_sw / ITERATION);
	printf("Write time: %lu\n", intvl_write / ITERATION);
	printf("Flush time: %lu\n", intvl_flush / ITERATION);
	printf("Acc time: %lu\n", intvl_acc / ITERATION);
	printf("Read time: %lu\n", intvl_read / ITERATION);

    aligned_free(ptable);
    aligned_free(mem);
    aligned_free(gold);

    return 0;
}
