///////////////////////////////////////////////////////////////
// Acc invocation related variables
///////////////////////////////////////////////////////////////
int n;
int ndev;
int nfir;
struct esp_device *espdevs;
struct esp_device *dev;
struct esp_device *fir_dev;
unsigned **ptable = NULL;
token_t *mem;
float *gold;
const float ERROR_COUNT_TH = 0.001;

///////////////////////////////////////////////////////////////
// Acc start and stop functions
///////////////////////////////////////////////////////////////
int start_acc()
{
	ndev = probe(&espdevs, VENDOR_SLD, SLD_FFT2, DEV_NAME);
	if (ndev == 0) {
		printf("%s not found\n", DEV_NAME);
		return 0;
	}

	for (n = 0; n < ndev; n++) {

		printf("**************** %s.%d ****************\n", DEV_NAME, n);

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

		iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);

		// Pass common configuration parameters
		iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
		iowrite32(dev, COHERENCE_REG, coherence);

		iowrite32(dev, PT_ADDRESS_REG, (unsigned long long) ptable);
		iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
		iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

		// Use the following if input and output data are not allocated at the default offsets
		iowrite32(dev, SRC_OFFSET_REG, 2 * n * acc_size);
		iowrite32(dev, DST_OFFSET_REG, 2 * n * acc_size);

		// Pass accelerator-specific configuration parameters
		/* <<--regs-config-->> */
		iowrite32(dev, FFT2_LOGN_SAMPLES_REG, logn_samples);
		iowrite32(dev, FFT2_NUM_FFTS_REG, num_ffts);
		iowrite32(dev, FFT2_SCALE_FACTOR_REG, scale_factor);
		iowrite32(dev, FFT2_DO_SHIFT_REG, do_shift);
		iowrite32(dev, FFT2_DO_INVERSE_REG, n);

		// Flush (customize coherence model here)
		esp_flush(coherence);

		// Start accelerators
		iowrite32(dev, CMD_REG, CMD_MASK_START);
    }

	nfir = probe(&fir_dev, VENDOR_SLD, SLD_FIR, FIR_DEV_NAME);
	if (nfir == 0) {
		printf("%s not found\n", FIR_DEV_NAME);
		return 0;
	}

	for (n = 0; n < nfir; n++) {

		printf("**************** %s.%d ****************\n", FIR_DEV_NAME, n);

		dev = fir_dev;

		// Check DMA capabilities
		if (ioread32(dev, PT_NCHUNK_MAX_REG) == 0) {
			printf("  -> scatter-gather DMA is disabled. Abort.\n");
			return 0;
		}

		if (ioread32(dev, PT_NCHUNK_MAX_REG) < NCHUNK(mem_size)) {
			printf("  -> Not enough TLB entries available. Abort.\n");
			return 0;
		}

		iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);

		// Pass common configuration parameters
		iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
		iowrite32(dev, COHERENCE_REG, coherence);

		iowrite32(dev, PT_ADDRESS_REG, (unsigned long long) ptable);
		iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
		iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

		// Use the following if input and output data are not allocated at the default offsets
		iowrite32(dev, SRC_OFFSET_REG, acc_size);
		iowrite32(dev, DST_OFFSET_REG, acc_size);

		// Pass accelerator-specific configuration parameters
		/* <<--regs-config-->> */
		iowrite32(dev, FIR_LOGN_SAMPLES_REG, logn_samples);
		iowrite32(dev, FIR_NUM_FIRS_REG, num_ffts);

		// Flush (customize coherence model here)
		esp_flush(coherence);

		// Start accelerators
		iowrite32(dev, CMD_REG, CMD_MASK_START);
    }
}

void terminate_acc()
{
    unsigned done;
    unsigned spin_ct;

	for (n = 0; n < ndev; n++) {

		dev = &espdevs[n];

	    // Wait for completion
	    done = 0;
	    spin_ct = 0;
	    while (!done) {
	        done = ioread32(dev, STATUS_REG);
	        done &= STATUS_MASK_DONE;
	        spin_ct++;
	    }
	    iowrite32(dev, CMD_REG, 0x0);
    }

	for (n = 0; n < nfir; n++) {

		dev = fir_dev;

	    // Wait for completion
	    done = 0;
	    spin_ct = 0;
	    while (!done) {
	        done = ioread32(dev, STATUS_REG);
	        done &= STATUS_MASK_DONE;
	        spin_ct++;
	    }
	    iowrite32(dev, CMD_REG, 0x0);
    }
}