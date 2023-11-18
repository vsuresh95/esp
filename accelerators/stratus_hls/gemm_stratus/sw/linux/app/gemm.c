// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "libesp.h"
#include "cfg.h"
#include "gemm.h"

// #define ITERATIONS 1000

// #include "fcn_input.h"

const float ERR_TH = 0.05;

extern void sw_run(int32_t do_relu, int32_t transpose, int32_t ninputs,
		   int32_t d3, int32_t d2, int32_t d1,
		   native_t *in1, native_t *in2, native_t *out);
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

static void validate_buffer(token_t *acc_buf, native_t *sw_buf, unsigned len)
{
    int i;
    native_t val;
    unsigned errors = 0;

    printf("\nPrint output\n");

    for (i = 0; i < len; i++) {

#ifdef __FIXED
	val = fx2float(acc_buf[i], FX_IL);
#else
	val = acc_buf[i];
#endif
    if ((fabs(sw_buf[i] - val) / fabs(sw_buf[i])) > ERR_TH){
	// if (sw_buf[i] != val) {
	    errors++;
	    if (errors <= MAX_PRINTED_ERRORS)
		printf("index %d : output %d : expected %d <-- ERROR\n", i, (int) val, (int) sw_buf[i]);
	}
    }

    if (!errors)
	printf("\n  ** Test PASSED! **\n");
    else
	printf("\n  ** Test FAILED! (%d errors) **\n", errors);
}


/* User-defined code */
static void init_buffer(token_t *acc_buf, native_t *sw_buf, unsigned in_len)
{
    int i;

    printf("  Initialize inputs\n");

    for (i = 0; i < in_len; i++) {
	native_t val = i % 17 - 8;
    #if(COMP_MODE==MODE_REG_INV)
        #ifdef __FIXED
                acc_buf[i] = float2fx(val, FX_IL);
        #else
                acc_buf[i] = val;
        #endif
    #endif
	sw_buf[i] = val;
    }
}


/* User-defined code */
static void init_parameters(int test, int32_t do_relu, int32_t transpose, int32_t ninputs,
			    int32_t d3, int32_t d2, int32_t d1,
			    unsigned *in_len, unsigned *in1_len, unsigned *out_len,
			    unsigned *in_size, unsigned *out_size, unsigned *size)
{
    int32_t ld_offset1, ld_offset2, st_offset;
    unsigned in2_len;
    
    *in1_len = ninputs * round_up(d1 * d2, DMA_WORD_PER_BEAT(sizeof(token_t)));
    in2_len = round_up(ninputs * d2 * d3, DMA_WORD_PER_BEAT(sizeof(token_t)));
    *in_len = *in1_len + in2_len;
    *out_len = round_up(ninputs * d1 * d3, DMA_WORD_PER_BEAT(sizeof(token_t)));
    *in_size = *in_len * sizeof(token_t);
    *out_size = *out_len * sizeof(token_t);
    *size = *in_size + *out_size;

    ld_offset1 = 0;
    ld_offset2 = *in1_len;
    st_offset = *in_len;

    #if(COMP_MODE==MODE_REG_INV)
        gemm_cfg_000[0].do_relu = do_relu;
        gemm_cfg_000[0].transpose = transpose;
        gemm_cfg_000[0].ninputs = ninputs;
        gemm_cfg_000[0].d1 = d1;
        gemm_cfg_000[0].d2 = d2;
        gemm_cfg_000[0].d3 = d3;
        gemm_cfg_000[0].ld_offset1 = ld_offset1;
        gemm_cfg_000[0].ld_offset2 = ld_offset2;
        gemm_cfg_000[0].st_offset = st_offset;

        // print test info
        printf("  Prepare test %d parameters\n", test);
        printf("    .do_relu = %d\n", do_relu);
        printf("    .transpose = %d\n", transpose);
        printf("    .ninputs = %d\n", ninputs);
        printf("    .d3 = %d\n", d3);
        printf("    .d2 = %d\n", d2);
        printf("    .d1 = %d\n", d1);
        printf("    .st_offset = %d\n", st_offset);
        printf("    .ld_offset1 = %d\n", ld_offset1);
        printf("    .ld_offset2 = %d\n", ld_offset2);
    #endif
}


int main(int argc, char **argv)
{
    int test, n_tests, start_test = 1;

    unsigned in_len;
    unsigned in1_len;
    unsigned out_len;
    unsigned in_size;
    unsigned out_size;
    unsigned size;

    // token_t *acc_buf;
    // native_t *sw_buf;

    // int32_t do_relu  [MAX_TESTS] = {   0,  0,  0,    0,   0,  0,   0,   0,   0,    0,
	// 			       0,  0,  0,    0,   0,  0,   0,   0,   0,    0,
	// 			       0,  0,  0,    0,   0,  0,   0,   0,   0,    0};

    // int32_t transpose[MAX_TESTS] = {   1,  1,  0,    1,   1,  0,   1,   1,   0,    1,
	// 			       1,  1,  0,    0,   1,  1,   1,   1,   1,    1,
	// 			       0,  0,  0,    0,   1,  0,   0,   1,   1,    1};

    // int32_t ninputs  [MAX_TESTS] = {   2, 32,  4,    1,   8,  1,   1, 128,   1,    1,
	// 			       1,  2,  1,    1,   1,  1,   4,   8,   2,    2,
	// 			       2,  2,  2,    1, 128,  1,   4,   2,   2,    2};

    // int32_t d3       [MAX_TESTS] = {   8,  8,  8,   32,  32, 32, 128, 128, 128,    1,
	// 			       1, 20,  2,    2,  64, 64,  11,  18,  18,   21,
	// 			      11, 18, 18,   21, 128,  8,   8,   8,   8,   21};

    // int32_t d2       [MAX_TESTS] = {   8,  8,  8,   32,  32, 32, 128, 128, 128, 2048,
	// 			    2048, 16, 64, 2048,   1,  2,  246,  25,  14,   14,
	// 			      26, 25, 14,   14, 128,  8,   8,   8,   8,   14};

    // int32_t d1       [MAX_TESTS] = {   8,  8,  8,   32,  32, 32, 128, 128, 128,    1,
	// 			       8,  1, 10,    1,  64, 64,  21,  22,  31,   22,
	// 			       21,22, 31,   22, 128,  8,   8,   8,   8,   11};

    int32_t do_relu = DO_RELU;// [MAX_TESTS] = {   0,  0,  0,    0,   0,  0,   0,   0,   0,    0,
				    //    0,  0,  0,    0,   0,  0,   0,   0,   0,    0,
				    //    0,  0,  0,    0,   0,  0,   0,   0,   0,    0};

    int32_t transpose = 1; //[MAX_TESTS] = {   0,  1,  0,    1,   1,  0,   1,   1,   0,    1,
				    //    1,  1,  0,    0,   1,  1,   1,   1,   1,    1,
				    //    0,  0,  0,    0,   1,  0,   0,   1,   1,    1};

    int32_t ninputs = 1000;// [MAX_TESTS] = {   1, 32,  4,    1,   8,  1,   1, 128,   1,    1,
				    //    1,  2,  1,    1,   1,  1,   4,   8,   2,    2,
				    //    2,  2,  2,    1, 128,  1,   4,   2,   2,    2};

    int32_t d3  = D3;//     [MAX_TESTS] = {   256,  8,  8,   32,  32, 32, 128, 128, 128,    1,
				    //    1, 20,  2,    2,  64, 64,  11,  18,  18,   21,
				    //   11, 18, 18,   21, 128,  8,   8,   8,   8,   21};

    int32_t d2  = D2;//     [MAX_TESTS] = {   256,  8,  8,   32,  32, 32, 128, 128, 128, 2048,
				    // 2048, 16, 64, 2048,   1,  2,  246,  25,  14,   14,
				    //   26, 25, 14,   14, 128,  8,   8,   8,   8,   14};

    int32_t d1  = D1;//     [MAX_TESTS] = {   1,  8,  8,   32,  32, 32, 128, 128, 128,    1,
				    //    8,  1, 10,    1,  64, 64,  21,  22,  31,   22,
				    //    21,22, 31,   22, 128,  8,   8,   8,   8,   11};
                       

    // printf("\n====== %s ======\n\n", cfg_000[0].devname);

    // command line arguments
    // if (argc < 3) {
	// n_tests = 1;
    // } else if (argc == 3) {
	// n_tests = strtol(argv[1], NULL, 10);
	// if (n_tests > MAX_TESTS) {
	//     printf("Wrong input arguments!");
	//     return 1;
	// }
	// start_test = strtol(argv[2], NULL, 10);
	// if (start_test > MAX_TESTS) {
	//     printf("Wrong input arguments!");
	//     return 1;
	// }

    // } else {
	// printf("Wrong input arguments!");
	// return 1;
    // }
    // printf("  Executing %d tests\n", n_tests);



    printf("\n====== %s ======\n\n", cfg_000[0].devname);

    // allocations
    printf("  Allocations\n");
    acc_buf = (token_t *) esp_alloc(MAX_SIZE);
    // cfg_000[0].hw_buf = acc_buf;

    // sw_buf = malloc(MAX_SIZE);
    sw_buf = esp_alloc(MAX_SIZE);
    test = 1;
    // for (test = start_test - 1; test < n_tests + start_test - 1; ++test) 
    {

	printf("\n\n-------------------\n");
	printf("TEST #%d\n", test + 1);

	// calculate test parameters
	init_parameters(test,
			do_relu, transpose, ninputs, d3, d2, d1,
			&in_len, &in1_len, &out_len, &in_size, &out_size, &size);



	calculate_tiles(ninputs, d1,d2,d3,transpose,&size_mat1, &size_mat2, &size_mat_out, &mat_chk_in, &mat_rem_in1,
	&mat_rem_in2, &mat_chk_out, &mat_rem_out, &load_cfg, &loadable_rows, &loadable_chunk, &index_d1_incr);

	int tiled_out_size = round_up(mat_chk_out, DMA_WORD_PER_BEAT(sizeof(token_t)));
	int tiled_in_len = SYNC_VAR_SIZE+round_up((2*loadable_chunk), DMA_WORD_PER_BEAT(sizeof(token_t)));
	int out_offset  = tiled_in_len + SYNC_VAR_SIZE;

    out_arr = esp_alloc(ninputs*size_mat_out);

    set_offsets(1, tiled_in_len); //todo
    //Initialize device configurations
    update_gemm_cfg(1);


	// initialize input data
	init_buffer(acc_buf, sw_buf, in_len);

	// hardware execution
	printf("  Start accelerator execution\n");
    hw_comp_start = get_counter();
    #if(COMP_MODE==MODE_REG_INV)
	esp_run(cfg_000, NACC);
    #else
    in_main( ninputs,  d1,  d2,  d3,  transpose,  do_relu, in1_len, acc_buf, sw_buf, out_arr);
    #endif
    hw_comp_end  =get_counter();
	printf("  Completed accelerator execution\n");

	// software execution
	printf("  Start software execution\n");
    sw_comp_start = get_counter();
			for(int iter = 0; iter < ITERATIONS; iter++)
	// sw_run(do_relu[test], transpose[test], ninputs[test], d3[test], d2[test], d1[test],
    sw_run(do_relu, transpose, ninputs, d3, d2, d1,
	       sw_buf, &sw_buf[in1_len], &sw_buf[in_len]);
    sw_comp_end = get_counter();		
	printf("  Completed software execution\n");

    printf("SW Comp Time: %lu\nHW Comp Time:%lu\n", (sw_comp_end-sw_comp_start)/ITERATIONS, (hw_comp_end-hw_comp_start));
	// validation
	// errors = print_input(buf, gold);
	validate_buffer(&acc_buf[in_len], &sw_buf[in_len], round_up(ninputs * d1 * d3, DMA_WORD_PER_BEAT(sizeof(token_t))));
    }

    // free
    esp_free(acc_buf);
    esp_free(sw_buf);

    printf("\n====== %s ======\n\n", cfg_000[0].devname);

    return 0;
}
