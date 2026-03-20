#ifndef CONV_BASELINE_H
#define CONV_BASELINE_H

int conv2d_baseline()
{
	int i;
	int n;
	int ndev;
	struct esp_device *espdevs;
	struct esp_device *dev;
	unsigned done;
	unsigned **ptable;
	token_t *mem;
	unsigned errors = 0;
	unsigned coherence;

    int32_t output_h, output_w, output_pool_h, output_pool_w;
		
	if (is_padded) {
		output_h = (feature_map_height - 1) / stride + 1;
		output_w = (feature_map_width - 1) / stride + 1;
	} else {
		output_h = (feature_map_height - filter_height) / stride + 1;
		output_w = (feature_map_width - filter_width) / stride + 1;
	}
	output_pool_h = pool_type ? output_h / 2 : output_h;
	output_pool_w = pool_type ? output_w / 2 : output_w;	

	// Input data and golden output (aligned to DMA_WIDTH makes your life easier)
	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
	    in_words_adj = n_channels * feature_map_height * feature_map_width;
	    weights_words_adj = n_filters * n_channels * filter_height * filter_width;
	    bias_words_adj = n_filters;
	    out_words_adj = n_filters * output_pool_h * output_pool_w;
	} else {
	    in_words_adj = n_channels * round_up(feature_map_height * feature_map_width,
				    DMA_WORD_PER_BEAT(sizeof(token_t)));
	    weights_words_adj = n_filters * n_channels * filter_height * filter_width;
	    bias_words_adj = n_filters;
	    out_words_adj = n_filters * round_up(output_pool_h * output_pool_w,
				     DMA_WORD_PER_BEAT(sizeof(token_t)));
	}

	in_len = in_words_adj * (1);
	weights_len = weights_words_adj * (1);
	bias_len = bias_words_adj * (1);
	out_len = out_words_adj * (1);
	in_size = in_len * sizeof(token_t);
	weights_size = weights_len * sizeof(token_t);
	bias_size = bias_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	weights_offset = in_len;
	bias_offset = in_len + weights_len;
	out_offset  = in_len + weights_len + bias_len;
	mem_size = in_size + weights_size + bias_size + out_size;


	// Search for the device
	printf("Scanning device tree... \n");

	ndev = probe(&espdevs, VENDOR_SLD, SLD_CONV2D, DEV_NAME);
	if (ndev == 0) {
		printf("conv2d not found\n");
		return 0;
	}

	dev = &espdevs[0];

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
	mem = aligned_malloc(mem_size);

	printf("  memory buffer base-address = %p\n", mem);

	// Allocate and populate page table
	ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (i = 0; i < NCHUNK(mem_size); i++)
		ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];
	printf("  ptable = %p\n", ptable);
	printf("  nchunk = %lu\n", NCHUNK(mem_size));

	/* TODO: Restore full test once ESP caches are integrated */
	coherence = ACC_COH_RECALL;

	printf("  Generate input...\n");

	init_buf(mem);

	// Pass common configuration parameters

	iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	iowrite32(dev, COHERENCE_REG, coherence);

	iowrite32(dev, PT_ADDRESS_REG_0, (unsigned long) ptable);
	iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

	// Use the following if input and output data are not allocated at the default offsets
	iowrite32(dev, SRC_OFFSET_REG, 0x0);
	iowrite32(dev, DST_OFFSET_REG, 0x0);

	// Pass accelerator-specific configuration parameters
	/* <<--regs-config-->> */
	iowrite32(dev, CONV2D_N_CHANNELS_REG, n_channels);
	iowrite32(dev, CONV2D_FEATURE_MAP_HEIGHT_REG, feature_map_height);
	iowrite32(dev, CONV2D_FEATURE_MAP_WIDTH_REG, feature_map_width);
	iowrite32(dev, CONV2D_N_FILTERS_REG, n_filters);
	iowrite32(dev, CONV2D_FILTER_HEIGHT_REG, filter_height);
	iowrite32(dev, CONV2D_FILTER_WIDTH_REG, filter_width);
	iowrite32(dev, CONV2D_IS_PADDED_REG, is_padded);
	iowrite32(dev, CONV2D_STRIDE_REG, stride);
	iowrite32(dev, CONV2D_DO_RELU_REG, do_relu);
	iowrite32(dev, CONV2D_POOL_TYPE_REG, pool_type);
	iowrite32(dev, CONV2D_BATCH_SIZE_REG, batch_size);
	iowrite32(dev, CONV2D_INPUT_OFFSET_REG, 0x0);
	iowrite32(dev, CONV2D_FILTERS_OFFSET_REG, in_len);
	iowrite32(dev, CONV2D_BIAS_OFFSET_REG, in_len + weights_len);
	iowrite32(dev, CONV2D_OUTPUT_OFFSET_REG, in_len + weights_len + bias_len);

	// Flush (customize coherence model here)
	esp_flush(coherence);

	// Start accelerators
	printf("  Start...\n");
	iowrite32(dev, CMD_REG, CMD_MASK_START);

	// Wait for completion
	done = 0;
	while (!done) {
		done = ioread32(dev, STATUS_REG);
		done &= STATUS_MASK_DONE;
	}
	iowrite32(dev, CMD_REG, 0x0);

	printf("  Done\n");
	printf("  validating...\n");

	/* Validation */
	errors = validate_buf(&mem[out_offset]);
	if (errors)
		printf("  ... FAIL: %d errors\n", errors);
	else
		printf("  ... PASS\n");

	aligned_free(ptable);
	aligned_free(mem);

	return 0;
}

#endif // CONV_BASELINE_H
