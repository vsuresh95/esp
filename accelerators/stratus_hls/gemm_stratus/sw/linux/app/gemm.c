// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include "libesp.h"
#include "cfg.h"

// #if (COMP_MODE != MODE_REG_INV)
// #define ENABLE_SM
// #endif
//#define SPX

//#ifdef SPX
//	#define COH_MODE 2
//#else
//	#define IS_ESP 1
//	#define COH_MODE 0
//#endif

#define ITERATIONS 1000

// #include "coh_func.h"
// #include "sm.h"
// #include "sw_func.h"
#include "gemm.h"

static uint64_t t_start = 0;
static uint64_t t_end = 0;

uint64_t t_sw_gemm;
uint64_t t_cpu_write;
uint64_t t_gemm;
uint64_t t_cpu_read;
uint64_t t_total;

// static inline
void start_counter() {
    asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
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
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (t_end)
		:
		: "t0"
	);

	return (t_end - t_start);
}

static unsigned validate_buffer(token_t *acc_buf, native_t *sw_buf, unsigned len)
{
    int i;
    unsigned errors = 0;

    // printf("\nPrint output\n");

//     for (i = 0; i < len; i++) {

// #ifdef __FIXED
// 	native_t val = fx2float(acc_buf[i], FX_IL);
// #else
// 	native_t val = acc_buf[i];
// #endif
// 	if (sw_buf[i] != val) {
// 	    errors++;
// 	    if (errors <= MAX_PRINTED_ERRORS)
// 		printf("index %d : output %d : expected %d <-- ERROR\n", i, (int) val, (int) sw_buf[i]);
// 	}
//     }

//     if (!errors)
// 	printf("\n  ** Test PASSED! **\n");
//     else
// 	printf("\n  ** Test FAILED! **\n");

    spandex_native_t gold_data;
    spandex_token_t out_data;
    void* src;  
	void* dst;

    src = (void*) sw_buf;
    dst = (void*) acc_buf;
    for (i = 0; i < len; i+= 2, src += 8, dst += 8) {
        out_data.value_64 = read_mem_reqodata(dst);
//Removing cache pollution
#if 0
        gold_data.value_64 = read_mem_reqv(src);
#ifdef __FIXED
        native_t val = fx2float(out_data.value_32_1, FX_IL);
        if (val != gold_data.value_32_1) {
            ++errors;
            // printf("sw_buf[%d] = %d; acc_buf[%d] = %d;\n", i, (int) out_data.value_32_1, i, (int) val);
        }
        val = fx2float(out_data.value_32_2, FX_IL);
        if (val != gold_data.value_32_2) {
            ++errors;
            // printf("sw_buf[%d] = %d; acc_buf[%d] = %d;\n", i + 1, (int) out_data.value_32_2, i + 1, (int) val);
        }
#else
        native_t val = out_data.value_32_1;
        if (val != gold_data.value_32_1) {
            ++errors;
            // printf("sw_buf[%d] = %d; acc_buf[%d] = %d;\n", i, (int) val, i, (int) out_data.value_32_1);
        }
        val = out_data.value_32_2;
        if (val != gold_data.value_32_2) {
            ++errors;
            // printf("sw_buf[%d] = %d; acc_buf[%d] = %d;\n", i + 1, (int) val, i + 1, (int) out_data.value_32_2);
        }
#endif
#endif
    }

    return errors;
}


/* User-defined code */


int main(int argc, char **argv)
{
    unsigned err = 0;
#ifndef FCN
    int i;

    unsigned in_len;
    unsigned in1_len;
    unsigned out_len;
    unsigned in_size;
    unsigned out_size;
    unsigned size;
    unsigned d1 = D;
    unsigned d2 = D;
    unsigned d3 = D;
	unsigned transpose = 0;
    if(argc>1){
	d1 = atoi(argv[1]);
	if(argc>2) d2 = atoi(argv[2]);
	else d2 = d1;
	if(argc>3) d3 = atoi(argv[3]);
	else d3 = d1;
	if(argc>4) transpose = atoi(argv[4]);
    }

    token_t *acc_buf;
    native_t *sw_buf;

    t_sw_gemm = 0;
    t_cpu_write = 0;
    t_gemm = 0;
    t_cpu_read = 0;


    gemm_cfg_000[0].esp.coherence = coherence;
	gemm_cfg_000[0].spandex_conf = spandex_config.spandex_reg;


	// printf("	Coherence = %s\n", CohPrintHeader);
	// printf("	ITERATIONS = %u\n", ITERATIONS);

    // allocations
    // printf("  Allocations\n");
    

    //acc_buf = (token_t *) esp_alloc(D * D * sizeof(token_t) * 3);   // Remember to change the size when adding the synchronization variables

    unsigned num_inp_output = d1*d2 + d2*d3 + d1*d3;
    //sw_buf = (native_t*) esp_alloc(D * D * sizeof(native_t) * 3);
    sw_buf = (native_t*) esp_alloc(num_inp_output * sizeof(native_t));

    // init_parameters(0, 0, T, NINPUTS, D, D, D, &in_len, &in1_len, &out_len, &in_size, &out_size, &size, sw_buf);
	unsigned st_offset;
#ifdef ENABLE_SM
    gemm_cfg_000[0].esp.start_stop = 1;
    init_parameters(0, 0, transpose, ITERATIONS, d1, d2, d3, &in_len, &in1_len, &out_len, &in_size, &out_size, &size, sw_buf, INPUT_OFFSET, &st_offset);
    acc_buf = (token_t *) esp_alloc( (2*INPUT_OFFSET + num_inp_output) * sizeof(token_t)) ;   // Remember to change the size when adding the synchronization variables
	// printf("acc buf size = %d\n\n",(int)(2*INPUT_OFFSET + num_inp_output));
    // printf("\n====== %s ======\n\n", cfg_000[0].devname);
	gemm_cfg_000[0].prod_valid_offset = CONS_VALID_OFFSET;//VALID_FLAG_OFFSET;
	gemm_cfg_000[0].prod_ready_offset = CONS_READY_OFFSET;//READY_FLAG_OFFSET;
	gemm_cfg_000[0].cons_valid_offset = INPUT_OFFSET + in_len + CONS_VALID_OFFSET; //PROD_VALID_OFFSET;//SYNC_VAR_SIZE + LEN + VALID_FLAG_OFFSET;
	gemm_cfg_000[0].cons_ready_offset = INPUT_OFFSET + in_len + CONS_READY_OFFSET; //PROD_READY_OFFSET;//SYNC_VAR_SIZE + LEN + VALID_FLAG_OFFSET;

	const unsigned cpu_cons_valid_offset =gemm_cfg_000[0].prod_valid_offset ;//CONS_VALID_OFFSET;//VALID_FLAG_OFFSET;
	const unsigned cpu_cons_ready_offset =gemm_cfg_000[0].prod_ready_offset ;//CONS_READY_OFFSET;//READY_FLAG_OFFSET;
	const unsigned cpu_prod_valid_offset =gemm_cfg_000[0].cons_valid_offset ;//INPUT_OFFSET + in_len + CONS_VALID_OFFSET; //PROD_VALID_OFFSET;//SYNC_VAR_SIZE + LEN + VALID_FLAG_OFFSET;
	const unsigned cpu_prod_ready_offset =gemm_cfg_000[0].cons_ready_offset ;//INPUT_OFFSET + in_len + CONS_READY_OFFSET; //PROD_READY_OFFSET;//SYNC_VAR_SIZE + LEN + VALID_FLAG_OFFSET;
    // printf("    .prod_valid_offset = 0x%x\n", gemm_cfg_000[0].prod_valid_offset);
    // printf("    .prod_ready_offset = 0x%x\n", gemm_cfg_000[0].prod_ready_offset);
    // printf("    .cons_valid_offset = 0x%x\n", gemm_cfg_000[0].cons_valid_offset);
    // printf("    .cons_ready_offset = 0x%x\n", gemm_cfg_000[0].cons_ready_offset);

for(int test = 0; test < 2*INPUT_OFFSET + num_inp_output; test++) acc_buf[test] = 0;
	cfg_000[0].hw_buf = acc_buf;
#else
    init_parameters(0, 0, 0, NINPUTS, d1, d2, d3, &in_len, &in1_len, &out_len, &in_size, &out_size, &size, sw_buf, INPUT_OFFSET, &st_offset);
    acc_buf = (token_t *) esp_alloc( (in_len + out_len) * sizeof(token_t)) ;   // Remember to change the size when adding the synchronization variables
    //printf("\n====== %s ======\n\n", cfg_001[0].devname);
	cfg_001[0].hw_buf = acc_buf;
#endif


#ifdef ENABLE_SM
	asm volatile ("fence w, rw");
	write_mem_wtfwd(&acc_buf[cpu_cons_valid_offset],0);//write consumer valid
	asm volatile ("fence w, rw");
	write_mem_wtfwd(&acc_buf[cpu_cons_ready_offset],1);//write consumer ready
	asm volatile ("fence w, rw");
	write_mem_wtfwd(&acc_buf[cpu_prod_valid_offset],0);//write producer valid
	asm volatile ("fence w, rw");
	write_mem_wtfwd(&acc_buf[cpu_prod_ready_offset],0);//write producer ready
	asm volatile ("fence w, rw");
	esp_run(cfg_000, NACC);

	for (i = 0; i < ITERATIONS; ++i) {
		start_counter();
		// //print_flags(1, acc_buf);
		{
		//printf("gemm_cfg_000[0].cons_valid_offset [%d] = %d\n", gemm_cfg_000[0].cons_valid_offset, acc_buf[gemm_cfg_000[0].cons_valid_offset]);
		//printf("gemm_cfg_000[0].prod_rdy_offset [%d] =  %d\n", gemm_cfg_000[0].prod_ready_offset, acc_buf[gemm_cfg_000[0].prod_ready_offset]);
		//printf("gemm_cfg_000[0].cons_rdy_offset [%d] =  %d\n", gemm_cfg_000[0].cons_ready_offset, acc_buf[gemm_cfg_000[0].cons_ready_offset]);
		//printf("gemm_cfg_000[0].prod_valid_offset [%d] =  %d\n", gemm_cfg_000[0].prod_valid_offset, acc_buf[gemm_cfg_000[0].prod_valid_offset]);
	}
		// UpdateSync((void*) &acc_buf[cpu_prod_ready_offset], 1);
		// Wait for the accelerator to be ready
		SpinSync((void*) &acc_buf[cpu_cons_ready_offset], 1);
		UpdateSync((void*) &acc_buf[cpu_cons_ready_offset], 0);
		// //print_flags(1, acc_buf);

		//printf("gemm_cfg_000[0].cons_valid_offset [%d] = %d\n", gemm_cfg_000[0].cons_valid_offset, acc_buf[gemm_cfg_000[0].cons_valid_offset]);
		//printf("gemm_cfg_000[0].prod_rdy_offset [%d] =  %d\n", gemm_cfg_000[0].prod_ready_offset, acc_buf[gemm_cfg_000[0].prod_ready_offset]);
		//printf("gemm_cfg_000[0].cons_rdy_offset [%d] =  %d\n", gemm_cfg_000[0].cons_ready_offset, acc_buf[gemm_cfg_000[0].cons_ready_offset]);
		//printf("gemm_cfg_000[0].prod_valid_offset [%d] =  %d\n", gemm_cfg_000[0].prod_valid_offset, acc_buf[gemm_cfg_000[0].prod_valid_offset]);
		// When the accelerator is ready, we write the input data to it
		init_buffer(&acc_buf[INPUT_OFFSET], sw_buf, in_len);
		if (i == ITERATIONS - 1) {
          		UpdateSync((void*) &acc_buf[cpu_cons_valid_offset], 2);
		}else{
		// Inform the accelerator to start.
			UpdateSync((void*) &acc_buf[cpu_cons_valid_offset], 1);
      		}
		t_cpu_write += end_counter();
		
		start_counter();
		// Inform the accelerator - ready for next iteration.
		UpdateSync((void*) &acc_buf[cpu_prod_ready_offset], 1);
		// //print_flags(1, acc_buf);

		//printf("gemm_cfg_000[0].cons_valid_offset [%d] = %d\n", gemm_cfg_000[0].cons_valid_offset, acc_buf[gemm_cfg_000[0].cons_valid_offset]);
		//printf("gemm_cfg_000[0].prod_rdy_offset [%d] =  %d\n", gemm_cfg_000[0].prod_ready_offset, acc_buf[gemm_cfg_000[0].prod_ready_offset]);
		//printf("gemm_cfg_000[0].cons_rdy_offset [%d] =  %d\n", gemm_cfg_000[0].cons_ready_offset, acc_buf[gemm_cfg_000[0].cons_ready_offset]);
		//printf("gemm_cfg_000[0].prod_valid_offset [%d] =  %d\n", gemm_cfg_000[0].prod_valid_offset, acc_buf[gemm_cfg_000[0].prod_valid_offset]);
		// Wait for the accelerator to send output.
		RevSpinSync((void*) &acc_buf[cpu_prod_valid_offset], 0);
		
		// Reset flag for next iteration.
		//printf("Prod Valid : iteration %d: %d\n", i, acc_buf[cpu_prod_valid_offset]);
		UpdateSync((void*) &acc_buf[cpu_prod_valid_offset], 0);
		// //print_flags(1, acc_buf);

		//printf("gemm_cfg_000[0].cons_valid_offset [%d] = %d\n", gemm_cfg_000[0].cons_valid_offset, acc_buf[gemm_cfg_000[0].cons_valid_offset]);
		//printf("gemm_cfg_000[0].prod_rdy_offset [%d] =  %d\n", gemm_cfg_000[0].prod_ready_offset, acc_buf[gemm_cfg_000[0].prod_ready_offset]);
		//printf("gemm_cfg_000[0].cons_rdy_offset [%d] =  %d\n", gemm_cfg_000[0].cons_ready_offset, acc_buf[gemm_cfg_000[0].cons_ready_offset]);
		//printf("gemm_cfg_000[0].prod_valid_offset [%d] =  %d\n", gemm_cfg_000[0].prod_valid_offset, acc_buf[gemm_cfg_000[0].prod_valid_offset]);
		t_gemm += end_counter();
	     
		start_counter();
		err += validate_buffer(&acc_buf[st_offset], &sw_buf[in_len], out_len);		
		t_cpu_read += end_counter();
	}

	printf("Result: GEMM ASI %dx%dx%d %s CPU_WRITE = %lu\n", d1,d2,d3,CohPrintHeader, t_cpu_write);
	printf("Result: GEMM ASI %dx%dx%d %s CPU_READ = %lu\n", d1,d2,d3,CohPrintHeader, t_cpu_read);
	printf("Result: GEMM ASI %dx%dx%d %s Acc = %lu\n", d1,d2,d3,CohPrintHeader, t_gemm);
	
		
#else
	t_total = 0;
	start_counter();
    for (i = 0; i < ITERATIONS; ++i) {
        //start_counter();
        init_buffer(acc_buf, sw_buf, in_len);
        //t_cpu_write += end_counter();

    	// gemm_cfg_000[0].esp.start_stop = 0;
        //start_counter();
        esp_run(cfg_001, NACC);
        //t_gemm += end_counter();

        //start_counter();
        err += validate_buffer(&acc_buf[in_len], &sw_buf[in_len], out_len);
        //t_cpu_read += end_counter();
    }

    for (i = 0; i < ITERATIONS; ++i) {
        start_counter();
        //sw_run(0, 0, NINPUTS, D, D, D, sw_buf, &sw_buf[in1_len], &sw_buf[in_len]);
        sw_run(0, 0, NINPUTS, d1, d2, d3, sw_buf, &sw_buf[in1_len], &sw_buf[in_len]);
        t_sw_gemm += end_counter();
        // printf("Iteration %d\n", i);
    }
	t_total+=end_counter();
    //printf("\n====== %s ======\n\n", cfg_001[0].devname);

	printf("Result: GEMM SW %dx%dx%d = %lu\n", d1,d2,d3,t_sw_gemm);
	printf("Result: GEMM Linux %dx%dx%d Total = %lu\n", d1,d2,d3,t_total);
#endif

    // printf("    SW Time: %lu\n", t_sw_gemm);
    // printf("    CPU Write Time: %lu\n", t_cpu_write);
    // printf("    GEMM Time: %lu\n", t_gemm);
    // printf("    CPU Read Time: %lu\n", t_cpu_read);
    // printf("    Errors = %u\n", err);
//FCN
#else
	int test, n_tests, start_test = 1;

    unsigned in_len;
    unsigned in1_len;
    unsigned out_len;
    unsigned in_size;
    unsigned out_size;
    unsigned size;
	unsigned st_offset;

	int32_t do_relu = 1;
    int32_t transpose = 0; 
    int32_t ninputs = 1; 
	// #ifndef ENABLE_SM
	// ninputs=1;
	// #endif
    int32_t d3  = 64;
    int32_t d2  = 64;
    int32_t d1  = 1;


    token_t *acc_buf;
    native_t *sw_buf;
    native_t *out_arr;
    // acc_buf = (token_t *) esp_alloc(MAX_SIZE);
	sw_buf = esp_alloc(MAX_SIZE);
    test = 1;


    #ifdef ENABLE_SM
	init_parameters(0, do_relu, transpose, ITERATIONS, d3, d2, d1,
			&in_len, &in1_len, &out_len, &in_size, &out_size, &size, sw_buf, INPUT_OFFSET, &st_offset);
	#else
	init_parameters(0, do_relu, transpose, ninputs, d3, d2, d1,
			&in_len, &in1_len, &out_len, &in_size, &out_size, &size, sw_buf, INPUT_OFFSET, &st_offset);
	#endif


    acc_buf = (token_t *) esp_alloc(output_buffer_offset[NUM_DEVICES-1]+out_size);
	//printf("Accel mem size: %d\n", output_buffer_offset[NUM_DEVICES-1]+out_size);

	//calculate_tiles(ninputs, d1,d2,d3,transpose,&in1_len, &in_len, &out_size, &mat_chk_in);

	// int tiled_out_size = 64;
	// int tiled_in_len = SYNC_VAR_SIZE+round_up((2*loadable_chunk), DMA_WORD_PER_BEAT(sizeof(token_t)));
	// int out_offset  = tiled_in_len + SYNC_VAR_SIZE;
	int size_mat_out = 64;
    out_arr = esp_alloc(ninputs*size_mat_out);

    //set_offsets(1, tiled_in_len); 
    //Initialize device configurations
    update_gemm_cfg(NUM_DEVICES, coherence, spandex_config, acc_buf);
	reset_sync(acc_buf, NUM_DEVICES);
	asm volatile ("fence w, w");
		start_counter();
	init_weights(&acc_buf[input_buffer_offset[0]+in1_len], &acc_buf[input_buffer_offset[1]+in1_len],
		&acc_buf[input_buffer_offset[1]+in1_len],d2*d3); //hardcoded temporarily since we are doing fcnn over 1x64x64
	t_cpu_write += end_counter();	
    // hw_comp_start = get_counter();
	int64_t hw_comp_end = 0, hw_init_end = 0;
	#ifdef ENABLE_SM
	start_counter();
	// esp_run(cfg_000, NACC);

	for(int dev = 0; dev< NUM_DEVICES; dev++){
		esp_run(cfg_000+dev, NACC);
			//printf("ran dev %d \n", dev);
	}

	hw_init_end += end_counter();
    // hw_comp_start = get_counter();
    //#ifdef ENABLE_SM
	// in_main( ITERATIONS,  d1,  d2,  d3,  transpose,  do_relu, in1_len, acc_buf, sw_buf, out_arr);

	//print_flags(3, acc_buf);
	// UpdateSync((void*) &acc_buf[*cpu_prod_ready_offset], 1);

	//SpinSync((void*) &acc_buf[*cpu_cons_ready_offset], 1); // wait for cons ready
	//update weights at init
	// 	start_counter();
	// init_weights(&acc_buf[input_buffer_offset[0]+64], &acc_buf[input_buffer_offset[1]+64],
	// 	&acc_buf[input_buffer_offset[1]+64],64*64); //hardcoded temporarily since we are doing fcnn over 1x64x64
	// t_cpu_write += end_counter();

	int tinput = 0;
	// printf("  Poll Cons Rdy...\n");
	//print_flags(3, acc_buf);
	#if (COMP_MODE==MODE_PIPE)
	//fill pipe
	start_counter();
	for(tinput = 0; tinput<2; tinput++){
		SpinSync((void*) &acc_buf[*cpu_cons_ready_offset], 1); // wait for cons ready
		//printf("  Found Cons Rdy...\n");
		//print_flags(3, acc_buf);

		//write input
		init_buffer(&acc_buf[INPUT_OFFSET], sw_buf, in1_len);
		

		//printf("  Provided prefill %d. Update Cons Rdy and last...\n", tinput);
		asm volatile ("fence w, w");	//release semantics
		UpdateSync((void*) &acc_buf[*cpu_cons_ready_offset], 0);
		//print_flags(3, acc_buf);
		UpdateSync((void*) &acc_buf[*cpu_cons_valid_offset], 1);
		//print_flags(3, acc_buf);
	}
	t_cpu_write += end_counter();
	#endif

	hw_write_time = 0;
	hw_read_time = 0;
	// int out_iters = ((d1*d3)/mat_chk_out);
	// int in_iters = (d2*d3/(loadable_chunk));
		start_counter();
	while(tinput<ITERATIONS){
		
		// start_counter();
		//print_flags(3, acc_buf);
		//printf("  Wait for Cons Rdy...%d\n",tinput);
		SpinSync((void*) &acc_buf[*cpu_cons_ready_offset], 1); // wait for cons ready
		//print_flags(3, acc_buf);

		//write input
		init_buffer(&acc_buf[INPUT_OFFSET], sw_buf, in1_len);

		//printf("  Provided input...\n");
		asm volatile ("fence w, w");	//release semantics
		UpdateSync((void*) &acc_buf[*cpu_cons_ready_offset], 0);
		//print_flags(3, acc_buf);
		//inform accelerator last tile
		if(tinput == ITERATIONS-1)
			UpdateSync((void*) &acc_buf[*cpu_cons_valid_offset], 2);
		else
			UpdateSync((void*) &acc_buf[*cpu_cons_valid_offset], 1);
		//print_flags(3, acc_buf);

		t_cpu_write += end_counter();
		// start_counter();
		// Inform the accelerator - ready for next iteration.
		// UpdateSync((void*) &acc_buf[*cpu_prod_ready_offset], 1);

		//printf("  Wait for prod valid... %d\n", *cpu_prod_valid_offset);

		// Wait for the accelerator to send output.
		RevSpinSync((void*) &acc_buf[*cpu_prod_valid_offset], 0);
		
		// Reset flag for next iteration.
		//printf("Prod Valid : iteration %d: %d\n", tinput, acc_buf[*cpu_prod_valid_offset]);
		//print_flags(3, acc_buf);
	
	     
		// start_counter();
		err += validate_buffer(&acc_buf[st_offset], &sw_buf[in_len], out_len);		
		UpdateSync((void*) &acc_buf[*cpu_prod_valid_offset], 0);
		UpdateSync((void*) &acc_buf[*cpu_prod_ready_offset], 1);
		// t_cpu_read += end_counter();	
		tinput++;
	}
		t_gemm += end_counter();

	#if (COMP_MODE==MODE_PIPE)
		start_counter();
	for(tinput = 0; tinput<2; tinput++){
		RevSpinSync((void*) &acc_buf[*cpu_prod_valid_offset], 0);
		
		// Reset flag for next iteration.
		//printf("Prod Valid : iteration %d: %d\n", i, acc_buf[cpu_prod_valid_offset]);
	
		//t_gemm += end_counter();
	     
		err += validate_buffer(&acc_buf[st_offset], &sw_buf[in_len], out_len);	
		UpdateSync((void*) &acc_buf[*cpu_prod_valid_offset], 0);
		UpdateSync((void*) &acc_buf[*cpu_prod_ready_offset], 1);	
	}
		t_cpu_read += end_counter();	
	#endif

    #if COMP_MODE==MODE_CHAIN
	printf("Result: FCNN Chaining %s = %lu\n", CohPrintHeader, (t_cpu_read+t_cpu_write+t_gemm)/(ITERATIONS));
   	#else
	printf("Result: FCNN Pipelining %s = %lu\n", CohPrintHeader, (t_cpu_read+t_cpu_write+t_gemm)/(ITERATIONS));
	#endif

    //#endif
	//hw_comp_end  +=end_counter();

	//#ifndef ENABLE_SM
	#else
	int64_t sw_write = 0;
    int64_t sw_read = 0;
start_counter();
	for(int iter = 0; iter < ITERATIONS; iter++){ 
		init_weights(&acc_buf[in1_len], &acc_buf[size+in1_len],
		&acc_buf[2*size+in1_len],d2*d3); //hardcoded temporarily since we are doing fcnn over 1x64x64
	
		init_buffer(acc_buf, sw_buf, in1_len);
        //t_cpu_write += end_counter();

		for(int dev = 0; dev< NUM_DEVICES; dev++){
        	esp_run(cfg_000+dev, NACC);
			//printf("ran dev %d for iter %d\n", dev, iter);
		}
        //t_gemm += end_counter();

        //start_counter();
        err += validate_buffer(&acc_buf[in_len], &sw_buf[in_len], out_len);
        //t_cpu_read += end_counter();
	}
	t_total+=end_counter();

    start_counter();
	for(int iter = 0; iter < ITERATIONS; iter++){
	// sw_run(do_relu[test], transpose[test], ninputs[test], d3[test], d2[test], d1[test],

        // read_mem_reqodata(sw_buf, in_len);
        sw_run(do_relu, transpose, ninputs, d3, d2, d1,
                sw_buf, &sw_buf[in1_len], &sw_buf[in_len]);

       // start_counter();
        for(int i = 0; i<size_mat_out; i+=2){
            spandex_native_t val;
            val.value_64 = read_mem_reqodata(&sw_buf[in_len+i]);
        }
    }
    sw_read += end_counter();
	//#endif
	//validate_buffer(&acc_buf[in_len], &sw_buf[in_len], round_up(ninputs * d1 * d3, DMA_WORD_PER_BEAT(sizeof(token_t))));
   
    printf("Result: FCNN SW = %lu\n", (sw_read)/(ITERATIONS));
    printf("Result: FCNN Linux = %lu\n", t_total/(ITERATIONS));
	#endif
	

    esp_free(out_arr);

#endif
    // free
    esp_free(acc_buf);
    esp_free(sw_buf);

    return err;
}
