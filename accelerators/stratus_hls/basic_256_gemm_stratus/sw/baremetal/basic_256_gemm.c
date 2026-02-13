/* Copyright (c) 2011-2023 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include <fixed_point.h>

typedef int32_t token_t;

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
	return (sizeof(void *) / _st);
}

#define SLD_BASIC_256_GEMM 0x04a
#define DEV_NAME "sld,basic_256_gemm_stratus"
#define ITERATIONS 10

/* Matrix dimensions - CHANGE THESE TO TEST DIFFERENT SIZES */
const int32_t M = 256;
const int32_t K = 512;
const int32_t N = 256;

/* Size of the contiguous chunks for scatter/gather */
#define CHUNK_SHIFT 20
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK(_sz) ((_sz % CHUNK_SIZE == 0) ? (_sz / CHUNK_SIZE) : (_sz / CHUNK_SIZE) + 1)

/* User defined registers */
#define BASIC_256_GEMM_C_MAT_OFFSET_REG 0x54
#define BASIC_256_GEMM_B_MAT_OFFSET_REG 0x50
#define BASIC_256_GEMM_A_MAT_OFFSET_REG 0x4c
#define BASIC_256_GEMM_K_REG 0x48
#define BASIC_256_GEMM_M_REG 0x44
#define BASIC_256_GEMM_N_REG 0x40

static inline uint64_t get_counter() {
    uint64_t t;
    asm volatile (
        "li t0, 0;"
        "csrr t0, mcycle;"
        "mv %0, t0"
        : "=r" (t)
        :
        : "t0"
    );
    return t;
}


int main(int argc, char * argv[])
{
	int i, j, k;
	struct esp_device *dev;
	unsigned done;
	unsigned **ptable;
	token_t *mem;
	token_t *gold;
	unsigned errors = 0;

	printf("=== GEMM Accelerator Test ===\n");
	printf("Matrix size: %dx%d × %dx%d = %dx%d\n\n", M, K, K, N, M, N);

	// Calculate sizes
	unsigned A_words = (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) ? 
		M * K : round_up(M * K, DMA_WORD_PER_BEAT(sizeof(token_t)));
	unsigned B_words = (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) ? 
		K * N : round_up(K * N, DMA_WORD_PER_BEAT(sizeof(token_t)));
	unsigned C_words = (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) ? 
		M * N : round_up(M * N, DMA_WORD_PER_BEAT(sizeof(token_t)));

	unsigned A_offset = 0;
	unsigned B_offset = A_words;
	unsigned C_offset = A_words + B_words;
	unsigned mem_size = (A_words + B_words + C_words) * sizeof(token_t);

	// Find device
	//printf("Searching for accelerator...\n");
	struct esp_device *espdevs;
	int ndev = probe(&espdevs, VENDOR_SLD, SLD_BASIC_256_GEMM, DEV_NAME);
	if (ndev == 0) {
		printf("ERROR: Accelerator not found!\n");
		return 1;
	}
	dev = &espdevs[0];
	//printf("Found accelerator!\n\n");

	// Check DMA capabilities
	if (ioread32(dev, PT_NCHUNK_MAX_REG) == 0) {
		printf("ERROR: Scatter-gather DMA disabled\n");
		return 1;
	}
	if (ioread32(dev, PT_NCHUNK_MAX_REG) < NCHUNK(mem_size)) {
		printf("ERROR: Not enough TLB entries\n");
		return 1;
	}

	// Allocate memory
	gold = aligned_malloc(C_words * sizeof(token_t));
	mem = aligned_malloc(mem_size);
	ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (i = 0; i < NCHUNK(mem_size); i++)
		ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];

	token_t *A = &mem[A_offset];
	token_t *B = &mem[B_offset];
	token_t *C = &mem[C_offset];

	uint64_t cpu_start = get_counter();

	// Initialize matrices with simple values (1, 2, 3, ...)
	printf("Initializing matrices...\n");
	for (i = 0; i < M; i++)
		for (j = 0; j < K; j++)
			A[i * K + j] = i * K + j + 1;

	for (i = 0; i < K; i++)
		for (j = 0; j < N; j++)
			B[i * N + j] = i * N + j + 1;

	for (i = 0; i < M; i++)
		for (j = 0; j < N; j++)
			C[i * N + j] = 0;
	
	uint64_t cpu_compute = get_counter();

	// Compute expected result
	//printf("Computing expected result...\n");
	//for (i = 0; i < M; i++) {
	//	for (j = 0; j < N; j++) {
	//		int32_t sum = 0;
	//		for (k = 0; k < K; k++)
	//			sum += A[i * K + k] * B[k * N + j];
	//		gold[i * N + j] = sum;
	//	}
	//}

	uint64_t cpu_end = get_counter();

	// Print input matrices (if small enough)
	//if (M <= 8 && K <= 8 && N <= 8) {
	//	printf("\nMatrix A:\n");
	//	for (i = 0; i < M; i++) {
	//		for (j = 0; j < K; j++)
	//			printf("%4d ", A[i * K + j]);
	//		printf("\n");
	//	}
//
	//	printf("\nMatrix B:\n");
	//	for (i = 0; i < K; i++) {
	//		for (j = 0; j < N; j++)
	//			printf("%4d ", B[i * N + j]);
	//		printf("\n");
	//	}
//
	//	printf("\nExpected C:\n");
	//	for (i = 0; i < M; i++) {
	//		for (j = 0; j < N; j++)
	//			printf("%4d ", gold[i * N + j]);
	//		printf("\n");
	//	}
	//	printf("\n");
	//}
	uint64_t acc_runtime = 0;
	for(int i=0; i<ITERATIONS; i++){
		uint64_t acc_start = get_counter();

		// Configure accelerator
		//printf("Configuring accelerator...\n");
		iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
		iowrite32(dev, COHERENCE_REG, ACC_COH_NONE);
		
	#ifndef __sparc
		iowrite32(dev, PT_ADDRESS_REG, (unsigned long long) ptable);
	#else
		iowrite32(dev, PT_ADDRESS_REG, (unsigned) ptable);
	#endif
		iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
		iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);
		iowrite32(dev, SRC_OFFSET_REG, 0x0);
		iowrite32(dev, DST_OFFSET_REG, 0x0);

		iowrite32(dev, BASIC_256_GEMM_M_REG, M);
		iowrite32(dev, BASIC_256_GEMM_N_REG, N);
		iowrite32(dev, BASIC_256_GEMM_K_REG, K);
		iowrite32(dev, BASIC_256_GEMM_A_MAT_OFFSET_REG, A_offset);
		iowrite32(dev, BASIC_256_GEMM_B_MAT_OFFSET_REG, B_offset);
		iowrite32(dev, BASIC_256_GEMM_C_MAT_OFFSET_REG, C_offset);

		esp_flush(ACC_COH_NONE);

		uint64_t acc_run = get_counter();

		// Run accelerator
		//printf("Running accelerator...\n");
		iowrite32(dev, CMD_REG, CMD_MASK_START);

		done = 0;
		while (!done) {
			done = ioread32(dev, STATUS_REG);
			done &= STATUS_MASK_DONE;
		}
		iowrite32(dev, CMD_REG, 0x0);

		uint64_t acc_end = get_counter();

		//printf("Accelerator finished!\n\n");

		// Print actual output (if small enough)
		//if (M <= 8 && N <= 8) {
		//	printf("Actual C from accelerator:\n");
		//	for (i = 0; i < M; i++) {
		//		for (j = 0; j < N; j++)
		//			printf("%4d ", C[i * N + j]);
		//		printf("\n");
		//	}
		//	printf("\n");
		//}

		// Validate
		//printf("Validating results...\n");
		//errors = 0;
		//for (i = 0; i < M; i++) {
		//	for (j = 0; j < N; j++) {
		//		if (C[i * N + j] != gold[i * N + j]) {
		//			printf("  ERROR at C[%d][%d]: expected %d, got %d\n", 
		//			       i, j, gold[i * N + j], C[i * N + j]);
		//			errors++;
		//			if (errors >= 10) {
		//				printf("  ... (stopping after 10 errors)\n");
		//				goto done_validation;
		//			}
		//		}
		//	}
		//}

	done_validation:
	//	if (errors == 0) {
	//		printf("*** PASS *** All %d elements correct!\n", M * N);
	//	} else {
	//		printf("*** FAIL *** %d errors out of %d elements\n", errors, M * N);
	//	}

		//uint64_t cpu_runtime = cpu_end - cpu_start;

		acc_runtime = acc_runtime + acc_end - acc_run;
	}
	acc_runtime = acc_runtime/ITERATIONS;

	printf("ACC rumtime: %d cycles\n", acc_runtime);



	// Cleanup
	aligned_free(ptable);
	aligned_free(mem);
	aligned_free(gold);

	return errors ? 1 : 0;
}


