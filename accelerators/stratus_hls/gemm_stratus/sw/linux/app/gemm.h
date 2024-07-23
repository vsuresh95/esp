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


// static inline void set_offsets(int num_devices, int in1_len, int in_len, int out_len){

// 	int32_t tile_len = in1_len + in_len;

//     for(int dev_id = 0; dev_id < num_devices; dev_id++){
// 		accel_prod_valid_offset[dev_id] = dev_id*(tile_size + INPUT_OFFSET) + CONS_VALID_OFFSET;
// 		accel_cons_ready_offset[dev_id] = dev_id*(tile_size + INPUT_OFFSET) + tile_len +CONS_READY_OFFSET;
// 		accel_prod_ready_offset[dev_id] = dev_id*(tile_size + INPUT_OFFSET) + tile_len + CONS_READY_OFFSET;
// 		accel_cons_valid_offset[dev_id] = dev_id*(tile_size + INPUT_OFFSET) + CONS_VALID_OFFSET;
// 		input_buffer_offset[dev_id]  	= dev_id*(tile_size + INPUT_OFFSET);
// 		output_buffer_offset[dev_id] 	= dev_id*(tile_size + INPUT_OFFSET) + tile_len + INPUT_OFFSET;
//     }
// 	cpu_prod_ready_offset = &accel_cons_ready_offset[num_devices-1];
// 	cpu_prod_valid_offset = &accel_cons_valid_offset[num_devices-1];
// }

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

void print_flags(int ndev, token_t* acc_buf){
	for(int dev_id = 0; dev_id< ndev; dev_id++){
		printf("gemm_cfg_000[%d].cons_valid_offset [%d] = %d\n", dev_id,accel_cons_valid_offset[dev_id], acc_buf[accel_cons_valid_offset[dev_id]]);
		printf("gemm_cfg_000[%d].prod_rdy_offset [%d] =  %d\n", dev_id,accel_prod_ready_offset[dev_id], acc_buf[accel_prod_ready_offset[dev_id]]);
		printf("gemm_cfg_000[%d].cons_rdy_offset [%d] =  %d\n", dev_id,accel_cons_ready_offset[dev_id], acc_buf[accel_cons_ready_offset[dev_id]]);
		printf("gemm_cfg_000[%d].prod_valid_offset [%d] =  %d\n", dev_id,accel_prod_valid_offset[dev_id], acc_buf[accel_prod_valid_offset[dev_id]]);
	}
}

void update_gemm_cfg(int num_devices,unsigned coherence,  spandex_config_t spande_config, token_t* acc_buf){
	int dev_id;
	for(dev_id = 0; dev_id < 3; dev_id++)
	{
		/* <<--descriptor-->> */
		// gemm_cfg_000[dev_id].do_relu = DO_RELU;
		// gemm_cfg_000[dev_id].transpose = TRANSPOSE;
		// gemm_cfg_000[dev_id].ninputs = NINPUTS;
		// gemm_cfg_000[dev_id].d3 = D3;
		// gemm_cfg_000[dev_id].d2 = D2;
		// gemm_cfg_000[dev_id].d1 = D1;
		// gemm_cfg_000[dev_id].st_offset = ST_OFFSET0;
		// gemm_cfg_000[dev_id].ld_offset1 = LD_OFFSET1;
		// gemm_cfg_000[dev_id].ld_offset2 = LD_OFFSET2;
		// gemm_cfg_000[dev_id].src_offset = 0;
		// gemm_cfg_000[dev_id].dst_offset = 0;
		gemm_cfg_000[dev_id].esp = esp;
		gemm_cfg_000[dev_id].esp.coherence = coherence;
		#ifndef ENABLE_SM
		gemm_cfg_000[dev_id].esp.start_stop = 0;
		#else
		gemm_cfg_000[dev_id].esp.start_stop = 1;
		#endif

		gemm_cfg_000[dev_id].spandex_conf = spandex_config.spandex_reg;
		//gemm_cfg_000[dev_id].input_offset = input_buffer_offset[dev_id];
		// gemm_cfg_000[dev_id].cons_valid_offset = accel_cons_valid_offset[dev_id];
		// gemm_cfg_000[dev_id].prod_rdy_offset = accel_prod_ready_offset[dev_id];
		// gemm_cfg_000[dev_id].cons_rdy_offset = accel_cons_ready_offset[dev_id];
		// gemm_cfg_000[dev_id].prod_valid_offset = accel_prod_valid_offset[dev_id];

		cfg_000[dev_id].run = true;
		#ifdef ENABLE_SM
		if(dev_id == 0)
			cfg_000[dev_id].devname = "gemm_stratus.0";
		if(dev_id == 1)
			cfg_000[dev_id].devname = "gemm_stratus.1";
		if(dev_id == 2)
			cfg_000[dev_id].devname = "gemm_stratus.2";
		#else
		if(dev_id == 0)
			cfg_000[dev_id].devname = "gemm_stratus.3";
		if(dev_id == 1)
			cfg_000[dev_id].devname = "gemm_stratus.4";
		if(dev_id == 2)
			cfg_000[dev_id].devname = "gemm_stratus.5";
		#endif
		cfg_000[dev_id].ioctl_req = GEMM_STRATUS_IOC_ACCESS;
		cfg_000[dev_id].esp_desc = &(gemm_cfg_000[dev_id].esp);
		cfg_000[dev_id].hw_buf = acc_buf;
	}
};

static inline void reset_sync(token_t *acc_buf, int num_devices){
	int n;
   // printf("Inside reset sync %d\n", num_devices);
	for(n = 0; n< num_devices; n++){
		write_mem_wtfwd(&acc_buf[accel_prod_ready_offset[n]], (int64_t)1);//BM
		//printf("reset accel_prod_ready acc_buf[%d]: %d\n", accel_prod_ready_offset[n], *(acc_buf + accel_prod_ready_offset[n]));
		write_mem_wtfwd(&acc_buf[accel_cons_valid_offset[n]], (int64_t)0);
		//printf("reset accel_cons_valid acc_buf[%d]: %d\n", accel_cons_valid_offset[n], *(acc_buf + accel_cons_valid_offset[n]));
		write_mem_wtfwd(&acc_buf[accel_prod_valid_offset[n]], (int64_t)0);
		//printf("reset accel_prod_valid acc_buf[%d]: %d\n", accel_prod_valid_offset[n], *(acc_buf + accel_prod_valid_offset[n]));
		write_mem_wtfwd(&acc_buf[accel_cons_ready_offset[n]], (int64_t)1); 
		//printf("reset accel_cons_ready acc_buf[%d]: %d\n", accel_cons_ready_offset[n], *(acc_buf + accel_cons_ready_offset[n]));
	}
	asm volatile ("fence w, w");	

    // printf("end of reset sync\n");
}

#endif

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
		val.value_64 = read_mem_reqodata(res + i);
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
static void init_weights(token_t *sw_buf, token_t *sw_buf1,token_t *sw_buf2,unsigned in_len)
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
		write_mem_wtfwd(&sw_buf[i], val.value_64);
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
		write_mem_wtfwd(&sw_buf1[i], val.value_64);
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
		write_mem_wtfwd(&sw_buf2[i], val.value_64);
    	// printf("in2[%d] = %d, sw[%d] = %d\n", mat1size+i, in[mat1size+i], ninput*d2*d3 + offset2+ i, (int)sw_buf2[ninput*d2*d3 + offset2+ i]);
    }
}

// static inline void in_main(int ninputs, int d1, int d2, int d3, int transpose, int do_relu, int ld_offset2, token_t* acc_buf,native_t *sw_buf, native_t * out_arr)
// {
	// UpdateSync((void*) &acc_buf[cpu_prod_ready_offset], 1);

	// SpinSync((void*) &acc_buf[cpu_cons_ready_offset], 1); // wait for cons ready
	// //update weights at init
	// init_weights(&acc_buf[input_buffer_offset[0]+64], &acc_buf[input_buffer_offset[1]+64],&acc_buf[input_buffer_offset[1]+64],64*64); //hardcoded temporarily since we are doing fcnn over 1x64x64
		
	// int tinput = 0;
	// // printf("  Poll Cons Rdy...\n");
	// #if (COMP_MODE==MODE_PIPE)
	// //fill pipe
	// for(tinput = 0; tinput<2; tinput++){
	// 	start_counter();
	// 	SpinSync((void*) &acc_buf[cpu_cons_ready_offset], 1); // wait for cons ready
	// 	// printf("  Found Cons Rdy...\n");

	// 	//write input
	// 	init_buffer(&acc_buf[INPUT_OFFSET], sw_buf, 64);
		

	// 	// printf("  Provided config. Update Cons Rdy and last...\n");
	// 	asm volatile ("fence w, w");	//release semantics
	// 	UpdateSync((void*) &acc_buf[cpu_cons_ready_offset], 0);
	// 	UpdateSync((void*) &acc_buf[cpu_cons_valid_offset], 1);
	// 	t_cpu_write += end_counter();
	// }
	// #endif

	// hw_write_time = 0;
	// hw_read_time = 0;
	// // int out_iters = ((d1*d3)/mat_chk_out);
	// // int in_iters = (d2*d3/(loadable_chunk));
	// while(tinput<ninputs){
		
	// 	start_counter();
	// 	SpinSync((void*) &acc_buf[cpu_cons_ready_offset], 1); // wait for cons ready
	// 	// printf("  Found Cons Rdy...\n");

	// 	//write input
	// 	init_buffer(&acc_buf[INPUT_OFFSET], sw_buf, 64);
		

	// 	// printf("  Provided config. Update Cons Rdy and last...\n");
	// 	asm volatile ("fence w, w");	//release semantics
	// 	UpdateSync((void*) &acc_buf[cpu_cons_ready_offset], 0);
	// 	UpdateSync((void*) &acc_buf[cpu_cons_valid_offset], 1);
	// 	t_cpu_write += end_counter();
	// 	start_counter();
	// 	// Inform the accelerator - ready for next iteration.
	// 	UpdateSync((void*) &acc_buf[cpu_prod_ready_offset], 1);
	// 	// Wait for the accelerator to send output.
	// 	RevSpinSync((void*) &acc_buf[cpu_prod_valid_offset], 0);
		
	// 	// Reset flag for next iteration.
	// 	//printf("Prod Valid : iteration %d: %d\n", i, acc_buf[cpu_prod_valid_offset]);
	
	// 	t_gemm += end_counter();
	     
	// 	start_counter();
	// 	err += validate_buffer(&acc_buf[st_offset], &sw_buf[in_len], out_len);		
	// 	t_cpu_read += end_counter();	
	// 	UpdateSync((void*) &acc_buf[cpu_prod_valid_offset], 0);
	// 	UpdateSync((void*) &acc_buf[cpu_prod_ready_offset], 1);
			
	// 	tinput++;
	// }

	// #if (COMP_MODE==MODE_PIPE)
	// for(tinput = 0; tinput<2; tinput++){
	// 	RevSpinSync((void*) &acc_buf[cpu_prod_valid_offset], 0);
		
	// 	// Reset flag for next iteration.
	// 	//printf("Prod Valid : iteration %d: %d\n", i, acc_buf[cpu_prod_valid_offset]);
	
	// 	t_gemm += end_counter();
	     
	// 	start_counter();
	// 	err += validate_buffer(&acc_buf[st_offset], &sw_buf[in_len], out_len);		
	// 	t_cpu_read += end_counter();	
	// 	UpdateSync((void*) &acc_buf[cpu_prod_valid_offset], 0);
	// 	UpdateSync((void*) &acc_buf[cpu_prod_ready_offset], 1);
	// }
	// #endif
// }

#endif


static void init_parameters(int test, int32_t do_relu, int32_t transpose, int32_t ninputs,
			    int32_t d3, int32_t d2, int32_t d1,
			    unsigned *in_len, unsigned *in1_len, unsigned *out_len,
			    unsigned *in_size, unsigned *out_size, unsigned *size, native_t* sw_buf, int inp_offset, unsigned* st_offset)
{
    int32_t ld_offset1, ld_offset2;//, st_offset;
    unsigned in2_len;
    int i;

#ifndef FCN
    #ifdef ENABLE_SM
    *in1_len = round_up( d1 * d2, DMA_WORD_PER_BEAT(sizeof(token_t)));//ninputs *
    in2_len = round_up( d2 * d3, DMA_WORD_PER_BEAT(sizeof(token_t))); //ninputs *
    *in_len = *in1_len + in2_len;
    *out_len = round_up( d1 * d3, DMA_WORD_PER_BEAT(sizeof(token_t)));//ninputs *
    #else
    *in1_len = round_up(ninputs * d1 * d2, DMA_WORD_PER_BEAT(sizeof(token_t)));
    in2_len = round_up(ninputs * d2 * d3, DMA_WORD_PER_BEAT(sizeof(token_t))); 
    *in_len = *in1_len + in2_len;
    *out_len = round_up(ninputs * d1 * d3, DMA_WORD_PER_BEAT(sizeof(token_t)));
    #endif
    *in_size = *in_len * sizeof(token_t);
    *out_size = *out_len * sizeof(token_t);
    *size = *in_size + *out_size;
    #ifdef ENABLE_SM
    ld_offset1 = inp_offset; //0;
    ld_offset2 = ld_offset1 + *in1_len;
    *st_offset = 2*inp_offset + (*in_len);
    #else
    ld_offset1 = 0;
    ld_offset2 = *in1_len;
    *st_offset = *in_len;
    #endif
    gemm_cfg_000[0].do_relu = do_relu;
    gemm_cfg_000[0].transpose = transpose;
    gemm_cfg_000[0].ninputs = ninputs;
    gemm_cfg_000[0].d1 = d1;
    gemm_cfg_000[0].d2 = d2;
    gemm_cfg_000[0].d3 = d3;
    gemm_cfg_000[0].ld_offset1 = ld_offset1;
    gemm_cfg_000[0].ld_offset2 = ld_offset2;
    gemm_cfg_000[0].st_offset = *st_offset;
#else

    *in1_len = round_up( d1 * d2, DMA_WORD_PER_BEAT(sizeof(token_t)));//ninputs *
    in2_len = round_up( d2 * d3, DMA_WORD_PER_BEAT(sizeof(token_t))); //ninputs *
    *in_len = *in1_len + in2_len;
    *out_len = round_up( d1 * d3, DMA_WORD_PER_BEAT(sizeof(token_t)));//ninputs *
 
    *in_size = *in_len * sizeof(token_t);
    *out_size = *out_len * sizeof(token_t);
    *size = *in_size + *out_size;

     #ifdef ENABLE_SM
    ld_offset1 = inp_offset; //0;
    ld_offset2 = ld_offset1 + *in1_len;
    *st_offset = 2*inp_offset + (*in_len);
    #else

    ld_offset1 = 0;
    ld_offset2 = *in1_len;
    *st_offset = *in_len;
    #define INPUT_OFFSET 0
    #endif

	int32_t tile_size = *in_len;

    for(int dev_id = 0; dev_id < NUM_DEVICES; dev_id++){
		accel_prod_valid_offset[dev_id] = dev_id*(tile_size + INPUT_OFFSET) + CONS_VALID_OFFSET;
		accel_prod_ready_offset[dev_id] = dev_id*(tile_size + INPUT_OFFSET) + CONS_READY_OFFSET;
		accel_cons_ready_offset[dev_id] = dev_id*(tile_size + INPUT_OFFSET) + INPUT_OFFSET+ tile_size + CONS_READY_OFFSET;
		accel_cons_valid_offset[dev_id] = dev_id*(tile_size + INPUT_OFFSET) + INPUT_OFFSET+ tile_size + CONS_VALID_OFFSET;
		input_buffer_offset[dev_id]  	= dev_id*(tile_size + INPUT_OFFSET) + INPUT_OFFSET;
		output_buffer_offset[dev_id] 	= dev_id*(tile_size + INPUT_OFFSET) + INPUT_OFFSET + tile_size + INPUT_OFFSET;

        gemm_cfg_000[dev_id].prod_valid_offset = accel_prod_valid_offset[dev_id];//VALID_FLAG_OFFSET;
        gemm_cfg_000[dev_id].prod_ready_offset = accel_prod_ready_offset[dev_id];//READY_FLAG_OFFSET;
        gemm_cfg_000[dev_id].cons_valid_offset = accel_cons_valid_offset[dev_id] ; //PROD_VALID_OFFSET;//SYNC_VAR_SIZE + LEN + VALID_FLAG_OFFSET;
        gemm_cfg_000[dev_id].cons_ready_offset = accel_cons_ready_offset[dev_id]; //PROD_READY_OFFSET;//SYNC_VAR_SIZE + LEN + VALID_FLAG_OFFSET;
		gemm_cfg_000[dev_id].do_relu = do_relu;
		gemm_cfg_000[dev_id].transpose = transpose;
		gemm_cfg_000[dev_id].ninputs = ninputs;
		gemm_cfg_000[dev_id].d1 = d1;
		gemm_cfg_000[dev_id].d2 = d2;
		gemm_cfg_000[dev_id].d3 = d3;
        gemm_cfg_000[dev_id].ld_offset1 = input_buffer_offset[dev_id] ;
        gemm_cfg_000[dev_id].ld_offset2 = input_buffer_offset[dev_id]+ *in1_len;
        gemm_cfg_000[dev_id].st_offset = output_buffer_offset[dev_id] ;
	

    // print test info
    // printf("  Prepare hw %d parameters\n", dev_id);
    // printf("    .do_relu = %d\n", gemm_cfg_000[dev_id].do_relu);
    // printf("    .transpose = %d\n", gemm_cfg_000[dev_id].transpose);
    // printf("    .ninputs = %d\n", gemm_cfg_000[dev_id].ninputs);
    // printf("    .d3 = %d\n", gemm_cfg_000[dev_id].d3);
    // printf("    .d2 = %d\n", gemm_cfg_000[dev_id].d2);
    // printf("    .d1 = %d\n", gemm_cfg_000[dev_id].d1);
    // printf("    .st_offset = %d\n", gemm_cfg_000[dev_id].st_offset);
    // printf("    .ld_offset1 = %d\n", gemm_cfg_000[dev_id].ld_offset1);
    // printf("    .ld_offset2 = %d\n", gemm_cfg_000[dev_id].ld_offset2);
    // printf("    .prod_valid_offset = %d\n", gemm_cfg_000[dev_id].prod_valid_offset);
    // printf("    .prod_ready_offset = %d\n", gemm_cfg_000[dev_id].prod_ready_offset);
    // printf("    .cons_valid_offset = %d\n", gemm_cfg_000[dev_id].cons_valid_offset);
    // printf("    .cons_ready_offset = %d\n", gemm_cfg_000[dev_id].cons_ready_offset);
    }
	cpu_prod_ready_offset = &accel_cons_ready_offset[NUM_DEVICES-1];
	cpu_prod_valid_offset = &accel_cons_valid_offset[NUM_DEVICES-1];

	// printf("cpu_prod_ready_offset=%d\n",*cpu_prod_ready_offset);
	// printf("cpu_prod_valid_offset=%d\n",*cpu_prod_valid_offset);
	// printf("cpu_cons_ready_offset=%d\n",*cpu_cons_ready_offset);
	// printf("cpu_cons_valid_offset=%d\n",*cpu_cons_valid_offset);


	
#endif

    for (i = 0; i < *in_len; ++i) {
        sw_buf[i] = i % 17 - 8;
	//printf("Sw_buf[%d] = %x\n", i, (int) sw_buf[i]);
    }

    // print test info
    // printf("  Prepare test %d parameters\n", test);
    // printf("    .do_relu = %d\n", do_relu);
    // printf("    .transpose = %d\n", transpose);
    // printf("    .ninputs = %d\n", ninputs);
    // printf("    .d3 = %d\n", d3);
    // printf("    .d2 = %d\n", d2);
    // printf("    .d1 = %d\n", d1);
    // printf("    .st_offset = %d\n", *st_offset);
    // printf("    .ld_offset1 = %d\n", ld_offset1);
    // printf("    .ld_offset2 = %d\n", ld_offset2);
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
		write_mem_wtfwd(&in[i], val.value_64);
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

//     for (i = 0; i < in_len; i++) {
// 	native_t val = i % 17 - 8;
// #ifdef __FIXED
//         acc_buf[i] = float2fx(val, FX_IL);
// #else
//         acc_buf[i] = val;
// #endif
//      // sw_buf[i] = val;
//     }

    spandex_native_t gold_data;
    spandex_token_t in_data;
    void* src;  
    void* dst;

    src = (void*) sw_buf;
    dst = (void*) acc_buf;
    for (i = 0; i < in_len; i += 2, src += 8, dst += 8) {
	in_data.value_32_1 = i % 17 - 8;
	in_data.value_32_2 = (i+1) % 17 - 8;
//Removing cache pollution
#if 0
        gold_data.value_64 = read_mem_reqv(src);
#ifdef __FIXED
        in_data.value_32_1 = float2fx(sw_buf[i], FX_IL);//gold_data.value_32_1
        in_data.value_32_2 = float2fx(sw_buf[i+1], FX_IL);//gold_data.value_32_2
#else
        in_data.value_32_1 = (token_t) gold_data.value_32_1;
        in_data.value_32_2 = (token_t) gold_data.value_32_2;
#endif
#endif
        //printf("sw_buff[%d] = %x; sw_buff[%d] = %x;\n", i, (int) sw_buf[i], i + 1, (int) sw_buf[i + 1]);
        //printf("acc_buff[%d] = %x; acc_buff[%d] = %x;\n", i, (int) in_data.value_32_1, i + 1, (int) in_data.value_32_2);
        write_mem_wtfwd(dst, in_data.value_64);
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