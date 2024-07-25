
//#define ENABLE_SM
//#define SPX

//#ifdef SPX
//	#define COH_MODE 2
//#else
//	#define IS_ESP 1
//	#define COH_MODE 0 
//#endif

//#define ENABLE_SM
#define SYNC_VAR_LEN 0x4
#define PROD_VALID_OFFSET 0x404
#define PROD_READY_OFFSET 0x400
#define CONS_VALID_OFFSET SYNC_VAR_LEN
#define CONS_READY_OFFSET 0x0
#define INPUT_OFFSET 0x8
#define OUTPUT_OFFSET 0x108

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <math.h>

#include <esp_accelerator.h>
#include <esp_probe.h>
#include <fixed_point.h>

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

typedef float native_t;

#include "coh_func.h"
#include "sm.h"
#include "sw_func.h"

#define NINPUTS 1
//#define D 64  
//#define Dx2 (D*2)
//#define D3 D
//#define D2 D
//#define D1 1
#define T 1
#define ITERATIONS 1000
//10

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}

#define MAX_PRINTED_ERRORS 10

#define SLD_GEMM 0x051
#define DEV_NAME "sld,gemm_stratus"

/* <<--params-->> */
const int32_t do_relu = 0;
const int32_t transpose = T;
const int32_t ninputs = NINPUTS;
const int32_t d3 = D3;
const int32_t d2 = D2;
const int32_t d1 = D1;
int32_t st_offset;
#ifdef ENABLE_SM
const int32_t ld_offset1 = INPUT_OFFSET;
#else
const int32_t ld_offset1 = 0;
#endif
int32_t ld_offset2;

static unsigned in_words_adj;
static unsigned in2_words_adj;
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
#define GEMM_TRANSPOSE_REG 0x60
#define GEMM_DO_RELU_REG 0x5c
#define GEMM_ST_OFFSET_REG 0x58
#define GEMM_LD_OFFSET2_REG 0x54
#define GEMM_LD_OFFSET1_REG 0x50
#define GEMM_D3_REG 0x4c
#define GEMM_D2_REG 0x48
#define GEMM_D1_REG 0x44
#define GEMM_NINPUTS_REG 0x40
#define PROD_VALID_OFFSET_REG 0x64
#define CONS_READY_OFFSET_REG 0x68
#define PROD_READY_OFFSET_REG 0x6C
#define CONS_VALID_OFFSET_REG 0x70
// #define INPUT_OFFSET 0x64
// #define OUTPUT_OFFSET 0x68


uint64_t t_sw_gemm;
uint64_t t_cpu_write;
uint64_t t_gemm;
uint64_t t_cpu_read;

static uint64_t t_start = 0;
static uint64_t t_end = 0;

// static inline
void start_counter() {
    asm volatile (
		"li t0, 0;"
		"csrr t0, mcycle;"
		"mv %0, t0"
		: "=r" (t_start)
		:
		: "t0"
	);
}

// static inline
uint64_t end_counter() {
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

static int validate_buf(token_t *out, native_t *gold)
{
	int j;
	unsigned errors = 0;

#if 0
 	native_t val;
         for (j = 0; j < out_len; j++) {
 #ifdef __FIXED
 	    val = fx2float(out[j], FX_IL);
 #else
             val = out[j];
 #endif

             if (gold[j] != val) {
                 errors++;
                // if (errors <= MAX_PRINTED_ERRORS) {
 		    printf("%d : %d : %d\n", j, (int) val, (int) gold[j]);
 		//}
             }
 	}
#endif


	spandex_native_t gold_data;
    spandex_token_t out_data;
    void* src;  
	void* dst;

    src = (void*) gold;
    dst = (void*) out;
	for (j = 0; j < out_len; j += 2, src += 8, dst += 8) {
		//out_data.value_64 = read_mem_reqv(dst);
		out_data.value_64 = read_mem_reqodata(dst);
		//errors += out_data.value_64;
	}

	return errors;
}


static void init_buf (token_t *in, native_t * gold)
{
    int i;

    spandex_native_t gold_data;
    spandex_token_t in_data;
    void* src;  
    void* dst;

    src = (void*) gold;
    dst = (void*) in;

	for (i = 0; i < in_len; i += 2, src += 8, dst += 8) {
	in_data.value_32_1 = i % 17 - 8;
	in_data.value_32_2 = (i+1) % 17 - 8;

        write_mem_wtfwd(dst, in_data.value_64);
    }
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
	native_t *gold;
	unsigned errors = 0;
	
	ld_offset2 = ld_offset1+ ninputs * (d1 * d2);
	
	if (DMA_WORD_PER_BEAT(sizeof(token_t)) <= 1) {
		in_words_adj = ninputs * (d1*d2 + d2*d3);
		in2_words_adj = ninputs * (d1*d2);
		out_words_adj = ninputs * d1 * d3;
	} else {
		in_words_adj = round_up(ninputs * (d1*d2 + d2*d3), DMA_WORD_PER_BEAT(sizeof(token_t)));
		in2_words_adj = round_up(ninputs * (d1*d2), DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(ninputs * d1 * d3, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj;
	out_len = out_words_adj;
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset  = in_len;
	#ifdef ENABLE_SM
	out_offset  += 4*SYNC_VAR_LEN; //OUTPUT_OFFSET;
	#endif
	st_offset = out_offset;
	mem_size = (out_offset * sizeof(token_t)) + out_size;

	const int prod_rdy_offset = ld_offset1 + in_len + CONS_READY_OFFSET;
	const int prod_vld_offset = ld_offset1 + in_len + CONS_VALID_OFFSET;
	const int cons_rdy_offset = CONS_READY_OFFSET;
	const int cons_vld_offset = CONS_VALID_OFFSET;

	// Search for the device
	//printf("Scanning device tree... \n");

	ndev = probe(&espdevs, VENDOR_SLD, SLD_GEMM, DEV_NAME);
	if (ndev == 0) {
		printf("gemm not found\n");
		return 0;
	}

	//printf("Test parameters: [D, T, NINPUTS] = [%u %u %u, %u, %u]\n\n", d1,d2,d3, T, NINPUTS);
	// #ifdef ENABLE_SM
	// printf("ASI mode\n");
	// #endif

	//for (n = 0; n < 3; n++) 
	#ifdef ENABLE_SM
	 n = 0;
	 #else
	 n = 3; 
	#endif
	{
		//printf("**************** %s.%d ****************\n", DEV_NAME, n);

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
		gold = aligned_malloc(in_len * sizeof(native_t) + out_len * sizeof(native_t));
		mem = aligned_malloc(mem_size);
		// printf("  memory buffer base-address = %p\n", mem);
		// printf("  memory buffer base-address for gold = %p\n", gold);
		// printf("  coherence = %u\n", coherence);
		// printf("  Coherence Mode: %s\n", CohPrintHeader);

		// Allocate and populate page table
		ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
		for (i = 0; i < NCHUNK(mem_size); i++)
			ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];

		// printf("  ptable = %p\n", ptable);
		// printf("  nchunk = %lu\n", NCHUNK(mem_size));

#ifndef __riscv
//		for (coherence = ACC_COH_NONE; coherence <= ACC_COH_FULL; coherence++) {
#else
		{
			/* TODO: Restore full test once ESP caches are integrated */
			// coherence = ACC_COH_NONE;
#endif
		
			// printf("  Generate input...\n");

			// Pass common configuration parameters
			iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);
			iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
			iowrite32(dev, COHERENCE_REG, coherence);

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
			#ifdef ENABLE_SM
			iowrite32(dev, GEMM_NINPUTS_REG, ITERATIONS);
			#else
			iowrite32(dev, GEMM_NINPUTS_REG, ninputs);
			#endif
			iowrite32(dev, GEMM_D3_REG, d3);
			iowrite32(dev, GEMM_D2_REG, d2);
			iowrite32(dev, GEMM_D1_REG, d1);
			iowrite32(dev, GEMM_ST_OFFSET_REG, st_offset);
			iowrite32(dev, GEMM_LD_OFFSET1_REG, ld_offset1);
			iowrite32(dev, GEMM_LD_OFFSET2_REG, ld_offset2);
			iowrite32(dev, PROD_VALID_OFFSET_REG, CONS_VALID_OFFSET);
			iowrite32(dev, PROD_READY_OFFSET_REG, CONS_READY_OFFSET);
			iowrite32(dev, CONS_VALID_OFFSET_REG, PROD_VALID_OFFSET);
			iowrite32(dev, CONS_READY_OFFSET_REG, PROD_READY_OFFSET);
			

			// Flush (customize coherence model here)
#ifdef ENABLE_SM
				asm volatile ("fence w, rw");
				//write_mem_wtfwd(&mem[cons_vld_offset],0);//write consumer valid
				write_mem_wtfwd(&mem[CONS_VALID_OFFSET],0);//write consumer valid
				asm volatile ("fence w, rw");
				//write_mem_wtfwd(&mem[cons_rdy_offset],1);//write consumer ready
				write_mem_wtfwd(&mem[CONS_READY_OFFSET],1);//write consumer ready
				asm volatile ("fence w, rw");
				//write_mem_wtfwd(&mem[prod_vld_offset],0);//write producer valid
				write_mem_wtfwd(&mem[PROD_VALID_OFFSET],0);//write producer valid
				asm volatile ("fence w, rw");
				//write_mem_wtfwd(&mem[prod_rdy_offset],1);//write producer ready
				write_mem_wtfwd(&mem[PROD_READY_OFFSET],1);//write producer ready
				asm volatile ("fence w, rw");
				
#endif
			esp_flush(coherence);

			t_sw_gemm = 0;
			t_cpu_write = 0;
			t_gemm = 0;
			t_cpu_read = 0;

			for (i = 0; i < in_len; ++i) {
				gold[i] = i % 17 - 8;
			//	printf("gold[%d] = %d\n", i, (int) gold[i]);
			}
			// Start accelerators

			for (i = 0; i < ITERATIONS; ++i) {
#ifdef ENABLE_SM
				// asm volatile ("fence w, rw");
				// write_mem_wtfwd(&mem[CONS_READY_OFFSET],1);//write consumer ready
				// asm volatile ("fence w, rw");
				// write_mem_wtfwd(&mem[PROD_VALID_OFFSET],0);//write producer valid
				// asm volatile ("fence w, rw");
				// write_mem_wtfwd(&mem[PROD_READY_OFFSET],1);//write producer ready
				// asm volatile ("fence w, rw");
				start_counter();

				//printf("Iter %d Write for prodrdy \n", i);
				//write_mem_wtfwd(&mem[PROD_READY_OFFSET],1);//write producer ready
				//asm volatile ("fence w, rw");
				//printf("Iter %d/%d \n==========\nWait for consrdy at address %x\n", i, (int)(ITERATIONS),&mem[CONS_READY_OFFSET]);
				while (read_mem_reqodata(&mem[CONS_READY_OFFSET])==0) {}
				write_mem_wtfwd(&mem[CONS_READY_OFFSET],0);//write consumer ready
				asm volatile ("fence w, w");
				init_buf(&mem[INPUT_OFFSET], gold);
				//printf("inp addr: %x\n",&mem[INPUT_OFFSET]);
				//printf("op addr: %x\n",&mem[OUTPUT_OFFSET]);
				asm volatile ("fence w, w");
				//printf("Iter %d Write for consvld %x\n", i, &mem[CONS_VALID_OFFSET]);
				if(i == ITERATIONS-1)
					write_mem_wtfwd(&mem[CONS_VALID_OFFSET],2);//write consumer valid
				else
					write_mem_wtfwd(&mem[CONS_VALID_OFFSET],1);//write consumer valid
				// write_mem_wtfwd(&mem[CONS_VALID_OFFSET],1);//write consumer valid
				asm volatile ("fence w, w");
				write_mem_wtfwd(&mem[PROD_READY_OFFSET],1);//write producer ready
				asm volatile ("fence w, rw");
#else
				start_counter();
				init_buf(mem, gold);
#endif
				t_cpu_write += end_counter();

				// printf("  Start...\n");
			// for (i = 0; i < ITERATIONS; ++i) {
				start_counter();
				#ifdef ENABLE_SM
				if(i==0)
				#endif
				iowrite32(dev, CMD_REG, CMD_MASK_START);

				// Wait for completion
				done = 0;
#ifdef ENABLE_SM
				while (read_mem_reqodata(&mem[PROD_VALID_OFFSET])==0) {}
				 //printf("  prod vld:...%d\n",mem[PROD_VALID_OFFSET]);
				write_mem_wtfwd (&mem[PROD_VALID_OFFSET], 0); // reset cons ready
			
#else
				while (!done) {
					done = ioread32(dev, STATUS_REG);
					done &= STATUS_MASK_DONE;
				}
				iowrite32(dev, CMD_REG, 0x0);
#endif
				t_gemm += end_counter();
			// }

				// printf("  Done\n");
				// printf("  validating...\n");

				/* Validation */
				start_counter();
				errors += validate_buf(&mem[out_offset], &gold[in_len]);
				t_cpu_read += end_counter();
			}

			// if (errors)
			// 	printf("  ... FAIL\n");
			// else
			// 	printf("  ... PASS\n");

			// printf("	Errors = %u\n", errors);
			// printf("	SW Time = %lu\n", t_sw_gemm);
			//printf("	CPU Write Time = %lu\n", t_cpu_write);
			//printf("	GEMM Time = %lu\n", t_gemm);
			//printf("	CPU Read Time = %lu\n", t_cpu_read);
			printf("Result: GEMM Baremetal %dx%dx%d Total = %lu\n", d1,d2,d3,t_gemm+t_cpu_write+t_cpu_read);

		}
		aligned_free(ptable);
		aligned_free(mem);
		aligned_free(gold);
	}

	while(1);

	return errors;
}
