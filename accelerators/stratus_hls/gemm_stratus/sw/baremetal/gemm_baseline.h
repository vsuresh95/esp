#ifndef GEMM_BASELINE_H
#define GEMM_BASELINE_H

void gemm_baseline() {
	printf("Starting gemm_baseline...\n");
	int i;
	int n;
	int ndev;
	struct esp_device *espdevs;
	struct esp_device *dev;
	unsigned done;
	unsigned spin_ct;
	unsigned **ptable0 = NULL;
	unsigned **ptable1 = NULL;
	unsigned **ptable2 = NULL;
	int *mem0;
	int *mem1;
	int *mem2;
	float *gold0;
	float *gold1;
	float *gold2;
	unsigned errors0 = 0;
	unsigned errors1 = 0;
	unsigned errors2 = 0;
	unsigned coherence;

	printf("dim_m %u dim_n %u dim_k %u\n", dim_m, dim_n, dim_k);
    unsigned mat_a_len = dim_m * dim_k;
    unsigned mat_b_len = dim_n * dim_k;
    unsigned mat_bias_len = dim_n;
    unsigned mat_c_len = dim_m * dim_n;
    // Data offsets
    unsigned mat_a_offset = 0;
    unsigned mat_b_offset = mat_a_offset + mat_a_len;
    unsigned mat_bias_offset = mat_b_offset + mat_b_len;
    unsigned mat_c_offset = mat_bias_offset + mat_bias_len;

    unsigned mem_size = 4 * (mat_c_offset + mat_c_len) * sizeof(float);

	// Search for the device
	ndev = probe(&espdevs, VENDOR_SLD, SLD_GEMM, DEV_NAME);
	if (ndev == 0) {
		printf("%s not found\n", DEV_NAME);
		return;
	}

	printf("**************** %s.0 ****************\n", DEV_NAME);

	dev = &espdevs[0];

	// Check DMA capabilities
	if (ioread32(dev, PT_NCHUNK_MAX_REG) == 0) {
		printf("  -> scatter-gather DMA is disabled. Abort.\n");
		return;
	}

	if (ioread32(dev, PT_NCHUNK_MAX_REG) < NCHUNK(mem_size)) {
		printf("  -> Not enough TLB entries available. Abort.\n");
		return;
	}

	// Allocate memory
	gold0 = aligned_malloc((mat_c_offset + mat_c_len) * sizeof(unsigned));
	gold1 = aligned_malloc((mat_c_offset + mat_c_len) * sizeof(unsigned));
	gold2 = aligned_malloc((mat_c_offset + mat_c_len) * sizeof(unsigned));
	mem0 = aligned_malloc(mem_size);
	mem1 = aligned_malloc(mem_size);
	mem2 = aligned_malloc(mem_size);

	// Allocate and populate page table
	ptable0 = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (i = 0; i < NCHUNK(mem_size); i++)
		ptable0[i] = (unsigned *) &mem0[i * (CHUNK_SIZE / sizeof(unsigned))];

	ptable1 = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (i = 0; i < NCHUNK(mem_size); i++)
		ptable1[i] = (unsigned *) &mem1[i * (CHUNK_SIZE / sizeof(unsigned))];

	ptable2 = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (i = 0; i < NCHUNK(mem_size); i++)
		ptable2[i] = (unsigned *) &mem2[i * (CHUNK_SIZE / sizeof(unsigned))];

	coherence = ACC_COH_RECALL;

	// Flush (customize coherence model here)
	esp_flush(coherence);

	///////////////////////////////////////////////////////
	/// Start first task
	///////////////////////////////////////////////////////
	init_buffer(&mem0[mat_a_offset], &mem0[mat_b_offset], &mem0[mat_bias_offset],
				&gold0[mat_a_offset], &gold0[mat_b_offset], &gold0[mat_bias_offset], &gold0[mat_c_offset]);

	iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	iowrite32(dev, COHERENCE_REG, coherence);
	iowrite32(dev, PT_ADDRESS_REG_0, (unsigned long long) ptable0);
	iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);
	iowrite32(dev, SRC_OFFSET_REG, 0x0);
	iowrite32(dev, DST_OFFSET_REG, 0x0);
	iowrite32(dev, GEMM_NINPUTS_REG, 1);
	iowrite32(dev, GEMM_D1_REG, dim_m);
	iowrite32(dev, GEMM_D2_REG, dim_k);
	iowrite32(dev, GEMM_D3_REG, dim_n);
	iowrite32(dev, GEMM_LD_OFFSET1_REG, mat_a_offset);
	iowrite32(dev, GEMM_LD_OFFSET2_REG, mat_b_offset);
	iowrite32(dev, GEMM_BIAS_OFFSET_REG, mat_bias_offset);
	iowrite32(dev, GEMM_ST_OFFSET_REG, mat_c_offset);
	iowrite32(dev, GEMM_DO_RELU_REG, 0);
	iowrite32(dev, GEMM_DO_BIAS_REG, 1);
	iowrite32(dev, GEMM_TRANSPOSE_REG, 0);

	// Start accelerator
	iowrite32(dev, CMD_REG, CMD_MASK_START);
	
	printf("First context started\n");

	done = 0;
	while (!done) {
		done = ioread32(dev, STATUS_REG);
		done &= STATUS_MASK_DONE;
	}
	iowrite32(dev, CMD_REG, 0x0);

	printf("First context task done\n");

	errors0 += validate_buffer(&mem0[mat_c_offset], &gold0[mat_c_offset]);

	///////////////////////////////////////////////////////
	/// Start second task
	///////////////////////////////////////////////////////
	init_buffer(&mem1[mat_a_offset], &mem1[mat_b_offset], &mem1[mat_bias_offset],
				&gold1[mat_a_offset], &gold1[mat_b_offset], &gold1[mat_bias_offset], &gold1[mat_c_offset]);

	iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	iowrite32(dev, COHERENCE_REG, coherence);
	iowrite32(dev, PT_ADDRESS_REG_0, (unsigned long long) ptable1);
	iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);
	iowrite32(dev, SRC_OFFSET_REG, 0x0);
	iowrite32(dev, DST_OFFSET_REG, 0x0);
	iowrite32(dev, GEMM_NINPUTS_REG, 1);
	iowrite32(dev, GEMM_D1_REG, dim_m);
	iowrite32(dev, GEMM_D2_REG, dim_k);
	iowrite32(dev, GEMM_D3_REG, dim_n);
	iowrite32(dev, GEMM_LD_OFFSET1_REG, mat_a_offset);
	iowrite32(dev, GEMM_LD_OFFSET2_REG, mat_b_offset);
	iowrite32(dev, GEMM_BIAS_OFFSET_REG, mat_bias_offset);
	iowrite32(dev, GEMM_ST_OFFSET_REG, mat_c_offset);
	iowrite32(dev, GEMM_DO_RELU_REG, 0);
	iowrite32(dev, GEMM_DO_BIAS_REG, do_bias);
	iowrite32(dev, GEMM_TRANSPOSE_REG, 0);

	// Start accelerator
	iowrite32(dev, CMD_REG, CMD_MASK_START);
	
	printf("Second context started\n");

	done = 0;
	while (!done) {
		done = ioread32(dev, STATUS_REG);
		done &= STATUS_MASK_DONE;
	}
	iowrite32(dev, CMD_REG, 0x0);

	printf("Second context task done\n");

	errors1 += validate_buffer(&mem1[mat_c_offset], &gold1[mat_c_offset]);

	///////////////////////////////////////////////////////
	/// Start third task
	///////////////////////////////////////////////////////
	init_buffer(&mem2[mat_a_offset], &mem2[mat_b_offset], &mem2[mat_bias_offset],
				&gold2[mat_a_offset], &gold2[mat_b_offset], &gold2[mat_bias_offset], &gold2[mat_c_offset]);

	iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	iowrite32(dev, COHERENCE_REG, coherence);
	iowrite32(dev, PT_ADDRESS_REG_0, (unsigned long long) ptable2);
	iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);
	iowrite32(dev, SRC_OFFSET_REG, 0x0);
	iowrite32(dev, DST_OFFSET_REG, 0x0);
	iowrite32(dev, GEMM_NINPUTS_REG, 1);
	iowrite32(dev, GEMM_D1_REG, dim_m);
	iowrite32(dev, GEMM_D2_REG, dim_k);
	iowrite32(dev, GEMM_D3_REG, dim_n);
	iowrite32(dev, GEMM_LD_OFFSET1_REG, mat_a_offset);
	iowrite32(dev, GEMM_LD_OFFSET2_REG, mat_b_offset);
	iowrite32(dev, GEMM_BIAS_OFFSET_REG, mat_bias_offset);
	iowrite32(dev, GEMM_ST_OFFSET_REG, mat_c_offset);
	iowrite32(dev, GEMM_DO_RELU_REG, 0);
	iowrite32(dev, GEMM_DO_BIAS_REG, 1);
	iowrite32(dev, GEMM_TRANSPOSE_REG, 0);

	// Start accelerator
	iowrite32(dev, CMD_REG, CMD_MASK_START);
	
	printf("Third context started\n");

	done = 0;
	while (!done) {
		done = ioread32(dev, STATUS_REG);
		done &= STATUS_MASK_DONE;
	}
	iowrite32(dev, CMD_REG, 0x0);

	printf("Third context task done\n");

	errors2 += validate_buffer(&mem2[mat_c_offset], &gold2[mat_c_offset]);

	aligned_free(ptable0);
	aligned_free(ptable1);
	aligned_free(ptable2);
	aligned_free(mem0);
	aligned_free(mem1);
	aligned_free(mem2);
	aligned_free(gold0);
	aligned_free(gold1);
	aligned_free(gold2);

	printf("  Errors = %d %d %d\n", errors0, errors1, errors2);
}

#endif // GEMM_BASELINE_H
