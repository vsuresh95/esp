// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include "libesp.h"
#include "cfg.h"

// #define ENABLE_SM
// #define SPX

#ifdef SPX
	#define COH_MODE 0
#else
	#define IS_ESP 1
	#define COH_MODE 1
#endif

#define ITERATIONS 100

#include "coh_func.h"
#include "sm.h"
#include "sw_func.h"

static uint64_t t_start = 0;
static uint64_t t_end = 0;

uint64_t t_sw_gemm;
uint64_t t_cpu_write;
uint64_t t_gemm;
uint64_t t_cpu_read;

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
        gold_data.value_64 = read_mem_reqv(src);
        out_data.value_64 = read_mem_reqodata(dst);

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
    }

    return errors;
}


/* User-defined code */
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
        gold_data.value_64 = read_mem_reqv(src);
#ifdef __FIXED
        in_data.value_32_1 = float2fx(gold_data.value_32_1, FX_IL);
        in_data.value_32_2 = float2fx(gold_data.value_32_2, FX_IL);
#else
        in_data.value_32_1 = (token_t) gold_data.value_32_1;
        in_data.value_32_2 = (token_t) gold_data.value_32_2;
#endif
        // printf("sw_buff[%d] = %d; sw_buff[%d] = %d;\n", i, (int) sw_buf[i], i + 1, (int) sw_buf[i + 1]);
        write_mem_wtfwd(dst, in_data.value_64);
    }
}

int main(int argc, char **argv)
{
    int i;
    unsigned err = 0;

    unsigned in_len;
    unsigned in1_len;
    unsigned out_len;
    unsigned in_size;
    unsigned out_size;
    unsigned size;

    token_t *acc_buf;
    native_t *sw_buf;

    t_sw_gemm = 0;
    t_cpu_write = 0;
    t_gemm = 0;
    t_cpu_read = 0;

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

    gemm_cfg_000[0].esp.coherence = coherence;
	gemm_cfg_000[0].spandex_conf = spandex_config.spandex_reg;
	// gemm_cfg_000[0].input_offset = SYNC_VAR_SIZE;
	// gemm_cfg_000[0].output_offset = SYNC_VAR_SIZE + LEN + SYNC_VAR_SIZE;
	// gemm_cfg_000[0].prod_valid_offset = VALID_FLAG_OFFSET;
	// gemm_cfg_000[0].prod_ready_offset = READY_FLAG_OFFSET;
	// gemm_cfg_000[0].cons_valid_offset = SYNC_VAR_SIZE + LEN + VALID_FLAG_OFFSET;
	// gemm_cfg_000[0].cons_ready_offset = SYNC_VAR_SIZE + LEN + READY_FLAG_OFFSET;

    printf("\n====== %s ======\n\n", cfg_000[0].devname);
	printf("	Coherence = %s\n", CohPrintHeader);
	printf("	ITERATIONS = %u\n", ITERATIONS);

    // allocations
    printf("  Allocations\n");
    acc_buf = (token_t *) esp_alloc(D * D * sizeof(token_t) * 3);   // Remember to change the size when adding the synchronization variables
    cfg_000[0].hw_buf = acc_buf;

    sw_buf = (native_t*) esp_alloc(D * D * sizeof(native_t) * 3);

    init_parameters(0, 0, T, NINPUTS, D, D, D, &in_len, &in1_len, &out_len, &in_size, &out_size, &size, sw_buf);

    for (i = 0; i < ITERATIONS; ++i) {
        start_counter();
        sw_run(0, T, NINPUTS, D, D, D, sw_buf, &sw_buf[in1_len], &sw_buf[in_len]);
        t_sw_gemm += end_counter();
        // printf("Iteration %d\n", i);
    }

#ifdef ENABLE_SM
    gemm_cfg_000[0].esp.start_stop = 1;
	esp_run(cfg_000, NACC);

	// Reset all sync variables to default values.
	UpdateSync((void*) &acc_buf[VALID_FLAG_OFFSET], 0);
	UpdateSync((void*) &acc_buf[READY_FLAG_OFFSET], 1);
	UpdateSync((void*) &acc_buf[END_FLAG_OFFSET], 0);
	UpdateSync((void*) &acc_buf[SYNC_VAR_SIZE + LEN + VALID_FLAG_OFFSET], 0);
	UpdateSync((void*) &acc_buf[SYNC_VAR_SIZE + LEN + READY_FLAG_OFFSET], 1);
	UpdateSync((void*) &acc_buf[SYNC_VAR_SIZE + LEN + END_FLAG_OFFSET], 0);

	for (i = 0; i < ITERATIONS; ++i) {
		// printf("SM Enabled\n");
		start_counter();
		// Wait for the accelerator to be ready
		SpinSync((void*) &acc_buf[READY_FLAG_OFFSET], 1);
		// Reset flag for the next iteration
		UpdateSync((void*) &acc_buf[READY_FLAG_OFFSET], 0);
		// When the accelerator is ready, we write the input data to it
		init_buffer(&acc_buf[SYNC_VAR_SIZE], sw_buf, in_len);
		

		if (i == ITERATIONS - 1) {
			UpdateSync((void*) &acc_buf[END_FLAG_OFFSET], 1);
			UpdateSync((void*) &acc_buf[SYNC_VAR_SIZE + LEN + END_FLAG_OFFSET], 1);
		}
		// Inform the accelerator to start.
		UpdateSync((void*) &acc_buf[VALID_FLAG_OFFSET], 1);
		t_cpu_write += end_counter();
		// printf("Buf Initialized\n");

		start_counter();
		// Wait for the accelerator to send output.
		SpinSync((void*) &acc_buf[SYNC_VAR_SIZE + LEN + VALID_FLAG_OFFSET], 1);
		// Reset flag for next iteration.
		UpdateSync((void*) &acc_buf[SYNC_VAR_SIZE + LEN + VALID_FLAG_OFFSET], 0);
		t_gemm += end_counter();
		// printf("Acc done\n");
		
		start_counter();
		err += validate_buffer(&acc_buf[in_len], &sw_buf[in_len], out_len);
		// Inform the accelerator - ready for next iteration.
		UpdateSync((void*) &acc_buf[SYNC_VAR_SIZE + LEN + READY_FLAG_OFFSET], 1);
		t_cpu_read += end_counter();
		// printf("Output read\n");
	}
#else
    for (i = 0; i < ITERATIONS; ++i) {
        start_counter();
        init_buffer(acc_buf, sw_buf, in_len);
        t_cpu_write += end_counter();

        start_counter();
        esp_run(cfg_000, NACC);
        t_gemm += end_counter();

        start_counter();
        err += validate_buffer(&acc_buf[in_len], &sw_buf[in_len], out_len);
        t_cpu_read += end_counter();
    }
#endif

    // free
    esp_free(acc_buf);
    esp_free(sw_buf);

    printf("    SW Time: %lu\n", t_sw_gemm);
    printf("    CPU Write Time: %lu\n", t_cpu_write);
    printf("    GEMM Time: %lu\n", t_gemm);
    printf("    CPU Read Time: %lu\n", t_cpu_read);
    printf("    Errors = %u\n", err);

    printf("\n====== %s ======\n\n", cfg_000[0].devname);

    return err;
}
