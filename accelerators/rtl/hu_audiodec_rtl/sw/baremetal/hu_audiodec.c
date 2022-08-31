/* Copyright (c) 2011-2021 Columbia University, System Level Design Group */
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

#define SLD_HU_AUDIODEC 0x040
#define DEV_NAME "sld,hu_audiodec_rtl"

/* <<--params-->> */
const int32_t conf_0  =  0;   // 0: dummy
const int32_t conf_1  =  0;   // 1: dummy, channel is fixed a 16
const int32_t conf_2  =  8;   // audio block size
// FIXME need to set dma_read_index and dma_write_index properly
const int32_t conf_3  =  0;   // dma_read_index  
const int32_t conf_4  =  64;  // dma_write_index 128*32b/64b = 64 
const int32_t conf_5  =  0;   // 5-07: dummy
const int32_t conf_6  =  0;
const int32_t conf_7  =  0;   
const int32_t conf_8  =  39413;   // 08: cfg_cos_alpha;  
const int32_t conf_9  =  60968;   // 09: cfg_sin_alpha;
const int32_t conf_10 =  -46013;   // 10: cfg_cos_beta;
const int32_t conf_11 =  -56152;   // 11: cfg_sin_beta;
const int32_t conf_12 =  -35750;   // 12: cfg_cos_gamma;
const int32_t conf_13 =  -22125;   // 13: cfg_sin_gamma;
const int32_t conf_14 =  -39414;   // 14: cfg_cos_2_alpha;
const int32_t conf_15 =  57035;   // 15: cfg_sin_2_alpha;
const int32_t conf_16 =  -15211;   // 16: cfg_cos_2_beta;
const int32_t conf_17 =  10276;   // 17: cfg_sin_2_beta;
const int32_t conf_18 =  27688;   // 18: cfg_cos_2_gamma;
const int32_t conf_19 =  -48707;   // 19: cfg_sin_2_gamma;
const int32_t conf_20 =  -42691;   // 20: cfg_cos_3_alpha;
const int32_t conf_21 =  -11292;   // 21: cfg_sin_3_alpha;
const int32_t conf_22 =  48018;   // 22: cfg_cos_3_beta;
const int32_t conf_23 =  23121;   // 23: cfg_sin_3_beta;
const int32_t conf_24 =  -53190;   // 24: cfg_cos_3_gamma;
const int32_t conf_25 =  22878;   // 25: cfg_sin_3_gamma;
const int32_t conf_26 =  0;		// 26-31: dummy
const int32_t conf_27 =  0;
const int32_t conf_28 =  0;
const int32_t conf_29 =  0;
const int32_t conf_30 =  0;
const int32_t conf_31 =  0; 

const int32_t CFG_REGS_10_REG = 4*16;
const int32_t CFG_REGS_11_REG = 4*17;
const int32_t CFG_REGS_12_REG = 4*18;
const int32_t CFG_REGS_13_REG = 4*19;
const int32_t CFG_REGS_14_REG = 4*20;
const int32_t CFG_REGS_15_REG = 4*21;
const int32_t CFG_REGS_16_REG = 4*22;
const int32_t CFG_REGS_17_REG = 4*23;
const int32_t CFG_REGS_18_REG = 4*24;
const int32_t CFG_REGS_19_REG = 4*25;
const int32_t CFG_REGS_29_REG = 4*26;
const int32_t CFG_REGS_1_REG = 4*27;
const int32_t CFG_REGS_28_REG = 4*28;
const int32_t CFG_REGS_0_REG = 4*29;
const int32_t CFG_REGS_3_REG = 4*30;
const int32_t CFG_REGS_2_REG = 4*31;
const int32_t CFG_REGS_5_REG = 4*32;
const int32_t CFG_REGS_4_REG = 4*33;
const int32_t CFG_REGS_7_REG = 4*34;
const int32_t CFG_REGS_6_REG = 4*35;
const int32_t CFG_REGS_21_REG = 4*36;
const int32_t CFG_REGS_9_REG = 4*37;
const int32_t CFG_REGS_20_REG = 4*38;
const int32_t CFG_REGS_8_REG = 4*39;
const int32_t CFG_REGS_23_REG = 4*40;
const int32_t CFG_REGS_22_REG = 4*41;
const int32_t CFG_REGS_25_REG = 4*42;
const int32_t CFG_REGS_24_REG = 4*43;
const int32_t CFG_REGS_27_REG = 4*44;
const int32_t CFG_REGS_26_REG = 4*45;
const int32_t CFG_REGS_30_REG = 4*46;
const int32_t CFG_REGS_31_REG = 4*47;

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned mem_size;

/* Size of the contiguous chunks for scatter/gather */
#define CHUNK_SHIFT 20
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK(_sz) ((_sz % CHUNK_SIZE == 0) ?	\
		     (_sz / CHUNK_SIZE) :	\
		     (_sz / CHUNK_SIZE) + 1)

/* User defined registers */
/* <<--regs-->> */

static int validate_buf(token_t* out, token_t* gold)
{
    int i;
    unsigned errors = 0;

    for (i = 0; i < 128; i++) {
	if (gold[i] != out[i]) {
	    printf("%d : %d <-- error \n", gold[i], out[i]);
	    errors++;
	} else {
	    printf("%d : %d\n", gold[i], out[i]);
	}
    }

    return errors;
}


static void init_buf (token_t* in, token_t* gold)
{
    int i;
    int j;
  

    // audio_in[8][16] block size of 8 and 16 channels
    int32_t audio_in[128] = {
	0, 46956, 20486, -7767, 8080, -15972, 48876, -61539, 17530, 6186, 55830, 1599, -17846, -46806, -22604, -48556,
	0, 14640, -6849, 39524, -39538, 49604, -62424, 30140, 39773, 57357, 30140, -20913, 1893, 5786, 6343, -39217,
	0, 31444, 59546, -24019, -39073, 52264, 10492, -53281, 4469, -21024, -14012, -17243, -37146, 19890, 14424, -2648,
	0, 39511, 45462, 1795, -55876, 31378, 2549, -48556, 31496, -8206, 12537, -6489, -35705, -32139, -1967, 4338,
	0, -8546, 52, 18920, -23613, -62404, 4194, -41072, 14693, 16495, 11429, -10211, -25297, -55149, -14956, 51098,
	0, 42283, -45751, -5604, 44479, 5767, -45574, 21889, -8278, -39728, 2824, 58903, -17204, 1644, -25900, -16666,
	0, 35730, -60104, 14955, -60195, -9982, 45278, -25055, -52849, 63714, 144, -28004, 20289, 57271, -8363, -2471,
	0, -603, -17682, 63740, -17636, 62023, 17897, 29314, 17137, 43312, -2255, 23802, 5426, 9083, 20270, 2097
    };

    // audio_out[8][16] block size of 8, and 16 channels 
    // reference output
    int32_t audio_out[128] = {
	0, -34519, -47810, 12544, 67691, -41491, -52565, 5253, -15552, -39352, -23718, -20251, -95005, 14864, 33952, -12760, 
	0, 8373, -27227, 20559, -84367, -675, 25144, -11761, -56951, 10040, -12294, 116, -30892, 15657, 2782, -19321, 
	0, -43239, -54495, 19574, 2178, -34299, -5258, 6577, -53470, 20580, 44490, -2974, -11615, -3359, -12250, 24679, 
	0, -34170, -64338, 28282, -12197, -36253, -42777, 10233, -56617, -22730, 24930, -12662, -42978, 1135, 32743, 5401, 
	0, 11577, -2974, 9014, 23693, -559, -104591, 1797, 13873, -37102, 28600, -4373, -26764, 3208, 61310, -26930, 
	0, -12007, 1306, -17954, -7669, 20787, 43619, -39543, -3618, -5218, -23187, 6950, -32588, -16500, 6610, 35079, 
	0, 3242, 6013, -14474, 3569, 13586, -35038, 49086, 44199, 57690, 1437, 31054, 16639, 14147, -32750, -39608, 
	0, 28706, -19949, 26343, -33510, -23263, 63536, 24505, -30473, 4608, -14731, 9331, 26619, -11471, 13574, 13357
    };

    // pack int16_t into int32_t data, little endian
    for (i = 0; i < 128; i++) {
	in[i] = audio_in[i];
    }
    for (i = 0; i < 128; i++) {
	gold[i] = audio_out[i];
    }
}

int main(int argc, char * argv[])
{
    int i;
    int n;
    int ndev;
    struct esp_device *espdevs;
    struct esp_device *dev;
    unsigned done;
    unsigned **ptable;
    token_t *mem;
    token_t *gold;
    unsigned errors = 0;
    unsigned coherence;

    in_len = 128;
    out_len = 128;
	
    in_size = in_len * sizeof(token_t);
    out_size = out_len * sizeof(token_t);	
    out_offset = in_len;
    mem_size = in_size + out_size;
    // Search for the device
    printf("Scanning device tree... \n");

    ndev = probe(&espdevs, VENDOR_SLD, SLD_HU_AUDIODEC, DEV_NAME);
    if (ndev == 0) {
	printf("device not found\n");
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

	// Allocate memory
	gold = aligned_malloc(out_size);
	mem = aligned_malloc(mem_size);
	printf("  memory buffer base-address = %p\n", mem);

	// Alocate and populate page table
	ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (i = 0; i < NCHUNK(mem_size); i++)
	    ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];

	printf("  ptable = %p\n", ptable);
	printf("  nchunk = %lu\n", NCHUNK(mem_size));

	/* TODO: Restore full test once ESP caches are integrated */
	coherence = ACC_COH_RECALL;

	printf("  --------------------\n");
	printf("  Generate input...\n");
	init_buf(mem, gold);

	// Pass common configuration parameters

	iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	iowrite32(dev, COHERENCE_REG, coherence);

#ifndef __sparc
	iowrite32(dev, PT_ADDRESS_REG, (unsigned long long) ptable);
#else
	iowrite32(dev, PT_ADDRESS_REG, (unsigned) ptable);
#endif
	iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

	// Use the following if input and output data are not allocated at the default offsets
	iowrite32(dev, SRC_OFFSET_REG, 0x0);
	// TODO need to add output offset?
	iowrite32(dev, DST_OFFSET_REG, 0x0);

	// Pass accelerator-specific configuration parameters
	/* <<--regs-config-->> */
	iowrite32(dev, CFG_REGS_0_REG,  conf_0 );
	iowrite32(dev, CFG_REGS_1_REG,  conf_1 );
	iowrite32(dev, CFG_REGS_2_REG,  conf_2 );
	iowrite32(dev, CFG_REGS_3_REG,  conf_3 );
	iowrite32(dev, CFG_REGS_4_REG,  conf_4 );
	iowrite32(dev, CFG_REGS_5_REG,  conf_5 );
	iowrite32(dev, CFG_REGS_6_REG,  conf_6 );
	iowrite32(dev, CFG_REGS_7_REG,  conf_7 );
	iowrite32(dev, CFG_REGS_8_REG,  conf_8 );
	iowrite32(dev, CFG_REGS_9_REG,  conf_9 );
	iowrite32(dev, CFG_REGS_10_REG, conf_10);
	iowrite32(dev, CFG_REGS_11_REG, conf_11);
	iowrite32(dev, CFG_REGS_12_REG, conf_12);
	iowrite32(dev, CFG_REGS_13_REG, conf_13);
	iowrite32(dev, CFG_REGS_14_REG, conf_14);
	iowrite32(dev, CFG_REGS_15_REG, conf_15);
	iowrite32(dev, CFG_REGS_16_REG, conf_16);
	iowrite32(dev, CFG_REGS_17_REG, conf_17);
	iowrite32(dev, CFG_REGS_18_REG, conf_18);
	iowrite32(dev, CFG_REGS_19_REG, conf_19);
	iowrite32(dev, CFG_REGS_20_REG, conf_20);
	iowrite32(dev, CFG_REGS_21_REG, conf_21);
	iowrite32(dev, CFG_REGS_22_REG, conf_22);
	iowrite32(dev, CFG_REGS_23_REG, conf_23);
	iowrite32(dev, CFG_REGS_24_REG, conf_24);
	iowrite32(dev, CFG_REGS_25_REG, conf_25);
	iowrite32(dev, CFG_REGS_26_REG, conf_26);
	iowrite32(dev, CFG_REGS_27_REG, conf_27);
	iowrite32(dev, CFG_REGS_28_REG, conf_28);
	iowrite32(dev, CFG_REGS_29_REG, conf_29);
	iowrite32(dev, CFG_REGS_30_REG, conf_30);
	iowrite32(dev, CFG_REGS_31_REG, conf_31);


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
	errors = validate_buf(&mem[out_offset], gold);
	if (errors)
	    printf("  ... FAIL\n");
	else
	    printf("  ... PASS\n");
    }
    aligned_free(ptable);
    aligned_free(mem);
    aligned_free(gold);

    return 0;
}
