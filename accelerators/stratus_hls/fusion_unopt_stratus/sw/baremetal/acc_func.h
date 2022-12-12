///////////////////////////////////////////////////////////////
// Acc invocation related variables
///////////////////////////////////////////////////////////////
int n;
int ndev;
struct esp_device *espdevs;
struct esp_device *dev;
unsigned **ptable = NULL;
token_t *mem;
token_t *gold;

///////////////////////////////////////////////////////////////
// Accelerator helper functions
///////////////////////////////////////////////////////////////

int probe_acc() {
    ndev = probe(&espdevs, VENDOR_SLD, SLD_FUSION_UNOPT, DEV_NAME);
	if (ndev == 0) {
		printf("fusion_unopt not found\n");
		return 0;
	}

    dev = &espdevs[0];
}

int start_fusion(struct esp_device *dev) {
    // printf("**************** %s.%d ****************\n", DEV_NAME, n);

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
    iowrite32(dev, SRC_OFFSET_REG, 0x0);
    iowrite32(dev, DST_OFFSET_REG, 0x0);

    // Pass accelerator-specific configuration parameters
    /* <<--regs-config-->> */
    iowrite32(dev, FUSION_UNOPT_VENO_REG, veno);
    iowrite32(dev, FUSION_UNOPT_IMGWIDTH_REG, imgwidth);
    iowrite32(dev, FUSION_UNOPT_HTDIM_REG, htdim);
    iowrite32(dev, FUSION_UNOPT_IMGHEIGHT_REG, imgheight);
    iowrite32(dev, FUSION_UNOPT_SDF_BLOCK_SIZE_REG, sdf_block_size);
    iowrite32(dev, FUSION_UNOPT_SDF_BLOCK_SIZE3_REG, sdf_block_size3);

    // Flush (customize coherence model here)
    esp_flush(coherence);

    // Start accelerators
    iowrite32(dev, CMD_REG, CMD_MASK_START);
}

void terminate_fusion(struct esp_device *dev) {
    unsigned done;
    // unsigned spin_ct;

    // Wait for completion
    done = 0;
    // spin_ct = 0;
    while (!done) {
        done = ioread32(dev, STATUS_REG);
        done &= STATUS_MASK_DONE;
        // spin_ct++;
        // printf("%u\n", done);
    }
    
    iowrite32(dev, CMD_REG, 0x0);
}

