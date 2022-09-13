///////////////////////////////////////////////////////////////
// Acc invocation related variables
///////////////////////////////////////////////////////////////
int n;
int ndev;
struct esp_device *espdevs;
struct esp_device *fft_dev;
struct esp_device *ifft_dev;
struct esp_device *fir_dev;
unsigned **ptable = NULL;
token_t *mem;
float *gold;
token_t *fxp_filters;
const float ERROR_COUNT_TH = 0.001;

///////////////////////////////////////////////////////////////
// Accelerator helper functions
///////////////////////////////////////////////////////////////
int probe_acc()
{
	ndev = probe(&espdevs, VENDOR_SLD, SLD_FFT2, DEV_NAME);
	if (ndev == 0) {
		printf("%s not found\n", DEV_NAME);
		return 0;
	}

	fft_dev = &espdevs[0];
	ifft_dev = &espdevs[1];
}

int start_fft(struct esp_device *dev, int inverse)
{
	// printf("**************** %s.%d ****************\n", DEV_NAME, inverse);

	// Check DMA capabilities
	if (ioread32(dev, PT_NCHUNK_MAX_REG) == 0) {
		printf("  -> scatter-gather DMA is disabled. Abort.\n");
		return 0;
	}

	if (ioread32(dev, PT_NCHUNK_MAX_REG) < NCHUNK(mem_size)) {
		printf("  -> Not enough TLB entries available. Abort.\n");
		return 0;
	}

	#if (COH_MODE == 3 && !defined(ESP))
	spandex_config.w_cid = (inverse+3)%4;
	#endif

	iowrite32(dev, SPANDEX_REG, spandex_config.spandex_reg);

	// Pass common configuration parameters
	iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	iowrite32(dev, COHERENCE_REG, coherence);

	iowrite32(dev, PT_ADDRESS_REG, (unsigned long long) ptable);
	iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

	// Use the following if input and output data are not allocated at the default offsets
	iowrite32(dev, SRC_OFFSET_REG, 2 * inverse * acc_size);
	iowrite32(dev, DST_OFFSET_REG, 2 * inverse * acc_size);

	// Pass accelerator-specific configuration parameters
	/* <<--regs-config-->> */
	iowrite32(dev, FFT2_LOGN_SAMPLES_REG, logn_samples);
	iowrite32(dev, FFT2_NUM_FFTS_REG, num_ffts);
	iowrite32(dev, FFT2_SCALE_FACTOR_REG, scale_factor);
	iowrite32(dev, FFT2_DO_SHIFT_REG, do_shift);
	iowrite32(dev, FFT2_DO_INVERSE_REG, inverse);

	// Flush (customize coherence model here)
	esp_flush(coherence);

	// Start accelerators
	iowrite32(dev, CMD_REG, CMD_MASK_START);
}

void terminate_fft(struct esp_device *dev)
{
    unsigned done;
    unsigned spin_ct;

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

int start_fir()
{
	// printf("**************** %s ****************\n", FIR_DEV_NAME);

	// Check DMA capabilities
	if (ioread32(fir_dev, PT_NCHUNK_MAX_REG) == 0) {
		printf("  -> scatter-gather DMA is disabled. Abort.\n");
		return 0;
	}

	if (ioread32(fir_dev, PT_NCHUNK_MAX_REG) < NCHUNK(mem_size)) {
		printf("  -> Not enough TLB entries available. Abort.\n");
		return 0;
	}

	#if (COH_MODE == 3 && !defined(ESP))
	spandex_config.w_cid = 2;
	#endif

	iowrite32(fir_dev, SPANDEX_REG, spandex_config.spandex_reg);

	// Pass common configuration parameters
	iowrite32(fir_dev, SELECT_REG, ioread32(fir_dev, DEVID_REG));
	iowrite32(fir_dev, COHERENCE_REG, coherence);

	iowrite32(fir_dev, PT_ADDRESS_REG, (unsigned long long) ptable);
	iowrite32(fir_dev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(fir_dev, PT_SHIFT_REG, CHUNK_SHIFT);

	// Use the following if input and output data are not allocated at the default offsets
	iowrite32(fir_dev, SRC_OFFSET_REG, acc_size);
	iowrite32(fir_dev, DST_OFFSET_REG, acc_size);

	// Pass accelerator-specific configuration parameters
	/* <<--regs-config-->> */
	iowrite32(fir_dev, FIR_LOGN_SAMPLES_REG, logn_samples);
	iowrite32(fir_dev, FIR_NUM_FIRS_REG, num_ffts);

	// Flush (customize coherence model here)
	esp_flush(coherence);

	// Start accelerators
	iowrite32(fir_dev, CMD_REG, CMD_MASK_START);
}

void terminate_fir()
{
    unsigned done;
    unsigned spin_ct;

	// Wait for completion
	done = 0;
	spin_ct = 0;
	while (!done) {
	    done = ioread32(fir_dev, STATUS_REG);
	    done &= STATUS_MASK_DONE;
	    spin_ct++;
	}

	iowrite32(fir_dev, CMD_REG, 0x0);
}

///////////////////////////////////////////////////////////////
// Acc start and stop functions
///////////////////////////////////////////////////////////////
void start_acc()
{
	probe_acc();

	start_fft(fft_dev, 0);
	start_fft(ifft_dev, 1);
	start_fir();
}

void terminate_acc()
{
	terminate_fft(fft_dev);
	terminate_fft(ifft_dev);
	terminate_fir();
}