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

#ifdef FCN

int32_t input_buffer_offset[MAX_DEVICES];
int32_t output_buffer_offset[MAX_DEVICES];

int32_t accel_prod_last_element[MAX_DEVICES];
int32_t accel_cons_last_element[MAX_DEVICES];

int32_t accel_prod_valid_offset[MAX_DEVICES];
int32_t accel_cons_ready_offset[MAX_DEVICES];
int32_t accel_prod_ready_offset[MAX_DEVICES];
int32_t accel_cons_valid_offset[MAX_DEVICES];

int32_t * cpu_cons_ready_offset = &accel_prod_ready_offset[0];
int32_t * cpu_cons_valid_offset = &accel_prod_valid_offset[0];
int32_t * cpu_prod_ready_offset;// = &accel_cons_ready_offset[NUM_DEVICES-1];
int32_t * cpu_prod_valid_offset;// = &accel_cons_valid_offset[NUM_DEVICES-1];

static inline void set_offsets(int num_devices, int tile_size){

	int32_t rel_accel_prod_ready_offset = RDY_OFFSET;
	int32_t rel_accel_prod_valid_offset = VLD_OFFSET;
	int32_t rel_accel_prod_last_offset = LAST_OFFSET;
	int32_t rel_input_buffer_offset = SYNC_VAR_SIZE;
	int32_t rel_accel_cons_ready_offset = tile_size + RDY_OFFSET;
	int32_t rel_accel_cons_valid_offset = tile_size + VLD_OFFSET;
	int32_t rel_accel_con_last_offset = tile_size + LAST_OFFSET;
	int32_t rel_output_buffer_offset = tile_size + SYNC_VAR_SIZE;

    for(int dev_id = 0; dev_id < num_devices; dev_id++){
		accel_prod_valid_offset[dev_id] = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_prod_valid_offset;
		accel_cons_ready_offset[dev_id] = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_cons_ready_offset;
		accel_prod_ready_offset[dev_id] = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_prod_ready_offset;
		accel_cons_valid_offset[dev_id] = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_cons_valid_offset;
		accel_cons_last_element[dev_id] = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_con_last_offset;
		accel_prod_last_element[dev_id] = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_prod_last_offset;
		input_buffer_offset[dev_id]  	= dev_id*(tile_size + SYNC_VAR_SIZE) + rel_input_buffer_offset;
		output_buffer_offset[dev_id] 	= dev_id*(tile_size + SYNC_VAR_SIZE) + rel_output_buffer_offset;
    }
	cpu_prod_ready_offset = &accel_cons_ready_offset[num_devices-1];
	cpu_prod_valid_offset = &accel_cons_valid_offset[num_devices-1];
}


static inline void reset_sync(token_t *buf, int num_devices){
	int n;
    // printf("Inside reset sync %d\n", num_devices);
	for(n = 0; n< num_devices; n++){
		write_mem_wtfwd(((void*)(buf + accel_prod_ready_offset[n])), (int64_t)1);//BM
		// printf("reset accel_prod_ready acc_buf[%d]: %d\n", accel_prod_ready_offset, *(acc_buf + accel_prod_ready_offset));
		write_mem_wtfwd(((void*)(buf + accel_cons_valid_offset[n])), (int64_t)0);
		// DBG_PRINT("reset accel_cons_valid acc_buf[%d]: %d\n", accel_cons_valid_offset, *(acc_buf + accel_cons_valid_offset));
		// write_mem(((void*)(buf + accel_cons_last_element[n])), 0);
		// DBG_PRINT("reset accel_con_last acc_buf[%d]: %d\n", accel_con_last_offset, *(acc_buf + accel_con_last_offset));
		write_mem_wtfwd(((void*)(buf + accel_prod_valid_offset[n])), 0);
		// DBG_PRINT("reset accel_prod_valid acc_buf[%d]: %d\n", accel_prod_valid_offset, *(acc_buf + accel_prod_valid_offset));
		write_mem_wtfwd(((void*)(buf + accel_cons_ready_offset[n])), 0); 
		// DBG_PRINT("reset accel_cons_ready acc_buf[%d]: %d\n", accel_cons_ready_offset, *(acc_buf + accel_cons_ready_offset));
	}
	asm volatile ("fence w, w");	

    // printf("end of reset sync\n");
}


static void init_buf_output (int ninput, int offset2, int len, token_t *res, native_t* out_arr, int size_mat_out)
{
    int i;
#ifdef __FIXED

	int offset = offset2;//+ninput*(round_up(size_mat_out, DMA_WORD_PER_BEAT(sizeof(token_t))));

	// #ifdef __linux__
	// printf("output tile O[%d - %d]\n", offset, len);
	// #endif
	#ifdef __FIXED
	for (i = 0; i < len; i+=2) { //round_up(len, DMA_WORD_PER_BEAT(sizeof(token_t)))
		spandex_token_t val;
		val.value_64 = read_mem(res + i);
		// out_arr[offset+i] = fx2float(res[i], FX_IL);
		// out_arr[offset+i] = fx2float(val.value_32_1, FX_IL);
		// out_arr[offset+i+1] = fx2float(val.value_32_2, FX_IL);
		// printf("res[%d] = %u (%d)\n", i, res[i], (int)out_arr[offset+i]);
    }
	#else
	for (i = 0; i < len; i++)
	out_arr[offset+i] = res[i], FX_IL;
	#endif
#else 
#include "gold.h"
// #include "fcn_gold.h"
#endif
}
static void init_weights(native_t *sw_buf, native_t *sw_buf1,native_t *sw_buf2,unsigned in_len)
{
    int i;

    // printf("  Initialize inputs\n");

    for (i = 0; i < in_len; i+=2) {
	// for (i = 0; i < matsize; i+=1) {
		// in[i] = float2fx(sw_buf[ninput + offset+ i], FX_IL); 
		// in[i] = float2fx(2.0, FX_IL); 
		int32_t temp = float2fx(2.0, FX_IL); 
		spandex_token_t val;
		val.value_32_1 = temp;
		val.value_32_2 = temp;
		write_mem(&sw_buf[i], val.value_64);
    	// printf("in2[%d] = %d, sw[%d] = %d\n", mat1size+i, in[mat1size+i], ninput*d2*d3 + offset2+ i, (int)sw_buf2[ninput*d2*d3 + offset2+ i]);
    }
    for (i = 0; i < in_len; i+=2) {
	// for (i = 0; i < matsize; i+=1) {
		// in[i] = float2fx(sw_buf[ninput + offset+ i], FX_IL); 
		// in[i] = float2fx(2.0, FX_IL); 
		int32_t temp = float2fx(2.0, FX_IL); 
		spandex_token_t val;
		val.value_32_1 = temp;
		val.value_32_2 = temp;
		write_mem(&sw_buf1[i], val.value_64);
    	// printf("in2[%d] = %d, sw[%d] = %d\n", mat1size+i, in[mat1size+i], ninput*d2*d3 + offset2+ i, (int)sw_buf2[ninput*d2*d3 + offset2+ i]);
    }
    for (i = 0; i < in_len; i+=2) {
	// for (i = 0; i < matsize; i+=1) {
		// in[i] = float2fx(sw_buf[ninput + offset+ i], FX_IL); 
		// in[i] = float2fx(2.0, FX_IL); 
		int32_t temp = float2fx(2.0, FX_IL); 
		spandex_token_t val;
		val.value_32_1 = temp;
		val.value_32_2 = temp;
		write_mem(&sw_buf2[i], val.value_64);
    	// printf("in2[%d] = %d, sw[%d] = %d\n", mat1size+i, in[mat1size+i], ninput*d2*d3 + offset2+ i, (int)sw_buf2[ninput*d2*d3 + offset2+ i]);
    }
}

static inline void in_main(int ninputs, int d1, int d2, int d3, int transpose, int do_relu, int ld_offset2, token_t* mem,native_t *sw_buf, native_t * out_arr)
{
		UpdateSync((void*) &acc_buf[cpu_prod_ready_offset], 1);
	int tinput = 0;
	// printf("  Poll Cons Rdy...\n");
	#if (COMP_MODE==MODE_PIPE)
	//fill pipe
	for(tinput = 0; tinput<2; tinput++){
		start_counter();
		SpinSync((void*) &acc_buf[cpu_cons_ready_offset], 1); // wait for cons ready
		// printf("  Found Cons Rdy...\n");

		//write input
		init_buffer(&acc_buf[INPUT_OFFSET], sw_buf, 64);
		

		// printf("  Provided config. Update Cons Rdy and last...\n");
		asm volatile ("fence w, w");	//release semantics
		UpdateSync((void*) &acc_buf[cpu_cons_ready_offset], 0);
		UpdateSync((void*) &acc_buf[cpu_cons_valid_offset], 1);
		t_cpu_write += end_counter();
	}
	#endif

	hw_write_time = 0;
	hw_read_time = 0;
	int out_iters = ((d1*d3)/mat_chk_out);
	int in_iters = (d2*d3/(loadable_chunk));
	while(tinput<ninputs){
		
		start_counter();
		SpinSync((void*) &acc_buf[cpu_cons_ready_offset], 1); // wait for cons ready
		// printf("  Found Cons Rdy...\n");

		//write input
		init_buffer(&acc_buf[INPUT_OFFSET], sw_buf, 64);
		

		// printf("  Provided config. Update Cons Rdy and last...\n");
		asm volatile ("fence w, w");	//release semantics
		UpdateSync((void*) &acc_buf[cpu_cons_ready_offset], 0);
		UpdateSync((void*) &acc_buf[cpu_cons_valid_offset], 1);
		t_cpu_write += end_counter();
		start_counter();
		// Inform the accelerator - ready for next iteration.
		UpdateSync((void*) &acc_buf[cpu_prod_ready_offset], 1);
		// Wait for the accelerator to send output.
		RevSpinSync((void*) &acc_buf[cpu_prod_valid_offset], 0);
		
		// Reset flag for next iteration.
		//printf("Prod Valid : iteration %d: %d\n", i, acc_buf[cpu_prod_valid_offset]);
	
		t_gemm += end_counter();
	     
		start_counter();
		err += validate_buffer(&acc_buf[st_offset], &sw_buf[in_len], out_len);		
		t_cpu_read += end_counter();	
		UpdateSync((void*) &acc_buf[cpu_prod_valid_offset], 0);
		UpdateSync((void*) &acc_buf[cpu_prod_ready_offset], 1);
			
		tinput++;
	}

	#if (COMP_MODE==MODE_PIPE)
	for(tinput = 0; tinput<2; tinput++){
		RevSpinSync((void*) &acc_buf[cpu_prod_valid_offset], 0);
		
		// Reset flag for next iteration.
		//printf("Prod Valid : iteration %d: %d\n", i, acc_buf[cpu_prod_valid_offset]);
	
		t_gemm += end_counter();
	     
		start_counter();
		err += validate_buffer(&acc_buf[st_offset], &sw_buf[in_len], out_len);		
		t_cpu_read += end_counter();	
		UpdateSync((void*) &acc_buf[cpu_prod_valid_offset], 0);
		UpdateSync((void*) &acc_buf[cpu_prod_ready_offset], 1);
	}
	#endif
}

#endif


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
	// // *offset_n = offset + mat1size; 
	// for (i = 0; i < matsize; i++) {
	// 	in[i] = float2fx(2.0, FX_IL); 
    // }

	for (i = 0; i < matsize; i+=2) {
	// for (i = 0; i < matsize; i+=1) {
		// in[i] = float2fx(sw_buf[ninput + offset+ i], FX_IL); 
		// in[i] = float2fx(2.0, FX_IL); 
		int32_t temp = float2fx(2.0, FX_IL); 
		spandex_token_t val;
		val.value_32_1 = temp;
		val.value_32_2 = temp;
		write_mem(&in[i], val.value_64);
    	// printf("in2[%d] = %d, sw[%d] = %d\n", mat1size+i, in[mat1size+i], ninput*d2*d3 + offset2+ i, (int)sw_buf2[ninput*d2*d3 + offset2+ i]);
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