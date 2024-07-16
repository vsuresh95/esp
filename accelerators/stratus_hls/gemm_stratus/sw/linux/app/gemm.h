#ifndef __GEMM_H_
#define __GEMM_H_
#include "coh_func.h"
#include "sw_func.h"
#include "sm.h"
static inline uint64_t get_counter() {
    uint64_t t_end = 0;
#ifndef __linux__
	asm volatile (
		"li t0, 0;"
		"csrr t0, mcycle;"
		"mv %0, t0"
		: "=r" (t_end)
		:
		: "t0"
	);
#else
	asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (t_end)
		:
		: "t0"
	);
#endif
	return t_end;
}


uint64_t sw_start = 0;
uint64_t sw_end = 0;
uint64_t sw_comp_time = 0;
uint64_t hw_comp_start = 0;
uint64_t hw_comp_end = 0;
uint64_t hw_write_time = 0;
uint64_t hw_read_time = 0;
uint64_t cfg_write_time = 0;
uint64_t weight_write_time = 0;

static inline native_t accumulate_fx(token_t *res, int len){
	native_t valout = 0.0;
	for (int i = 0; i < len; i+=2) { //round_up(len, DMA_WORD_PER_BEAT(sizeof(token_t)))
		spandex_token_t val;
		val.value_64 = read_mem_reqv(res + i);
		// out_arr[offset+i] = fx2float(res[i], FX_IL);
		valout += fx2float(val.value_32_1, FX_IL);
		valout += fx2float(val.value_32_2, FX_IL);
		// printf("res[%d] = %u (%d)\n", i, res[i], (int)out_arr[offset+i]);
    }
	return valout;
}

static inline native_t accumulate(native_t *res, int len){
	native_t valout = 0.0;
	for (int i = 0; i < len; i++) {
		valout += res[i];
    }
	return valout;
}

static void init_weight_buffer(native_t *sw_buf, native_t *sw_buf1,native_t *sw_buf2,unsigned in_len)
{
    int i;

    // printf("  Initialize inputs\n");

    for (i = 0; i < in_len; i++) {
		native_t val = i % 17 - 8;
		sw_buf[i] = val;
    }
    for (i = 0; i < in_len; i++) {
		native_t val = i % 17 - 8;
		sw_buf1[i] = val;
    }
    for (i = 0; i < in_len; i++) {
		native_t val = i % 17 - 8;
		sw_buf2[i] = val;
    }
}

/* User-defined code */

static inline void init_buf_input1 (int ninput, token_t *in, int matsize, native_t* sw_buf, int32_t offset)
{
    int i;
#ifdef __FIXED
	// *offset_n = offset + mat1size; 
	for (i = 0; i < matsize; i++) {
		in[i] = float2fx(2.0, FX_IL); 
    }

#else 
#include "gold.h"
// #include "fcn_gold.h"
#endif
	// tile_num++;
}

static void init_buffer(token_t *acc_buf, native_t *sw_buf, unsigned in_len)
{
    int i;

    // printf("  Initialize inputs\n");

    for (i = 0; i < in_len; i++) {
	native_t val = i % 17 - 8;
    #if(COMP_MODE==MODE_REG_INV)
    	// printf("  Initialize HW inputs\n");
        #ifdef __FIXED
                acc_buf[i] = float2fx(val, FX_IL);
        #else
                acc_buf[i] = val;
        #endif
    #endif
	sw_buf[i] = val;
    }
}

static inline void sw_fcn(int ninputs, int d1, int d2, int d3, int transpose, int do_relu, native_t *sw_buf){
	int tinput = 0;
	int in1_len = d1*d2;
	int in_len = d1*d2+d2*d3;
	int out_len = d1*d3;
	sw_comp_time = 0;
	init_weight_buffer(&sw_buf[in1_len], &sw_buf[in_len+out_len+in1_len], &sw_buf[2*(in_len+out_len)+in1_len], d2*d3);
	while(tinput < ninputs){
		init_buffer(NULL, sw_buf, in1_len);
		int64_t sw_comp_start = get_counter();
		sw_run(1, transpose, 1, d3, d2, d1, sw_buf, &sw_buf[in1_len], &sw_buf[in_len]);
		sw_run(1, transpose, 1, d3, d2, d1, &sw_buf[in_len], &sw_buf[in_len+out_len+in1_len], &sw_buf[2*(in_len)+out_len]);
		sw_run(1, transpose, 1, d3, d2, d1, &sw_buf[2*in_len+out_len], &sw_buf[2*(in_len)+out_len+in1_len], &sw_buf[3*(in_len)+2*out_len]);
		int64_t sw_comp_end = get_counter();
		sw_comp_time += sw_comp_end - sw_comp_start;
		native_t val = accumulate(&sw_buf[3*in_len], d3);
		tinput++;
	}
}

#endif