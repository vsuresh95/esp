/* Copyright (c) 2011-2022 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include <fixed_point.h>

typedef int64_t token_t;

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}


#define SLD_ISCA_SYNTH 0x04c
#define DEV_NAME "sld,isca_synth_stratus"

/* <<--params-->> */
int32_t compute_ratio = 40;
int32_t size = 128;
int32_t speedup = 20 / 4;

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
#define NCHUNK(_sz) ((_sz % CHUNK_SIZE == 0) ?		\
			(_sz / CHUNK_SIZE) :		\
			(_sz / CHUNK_SIZE) + 1)

/* User defined registers */
/* <<--regs-->> */
#define ISCA_SYNTH_COMPUTE_RATIO_REG 0x44
#define ISCA_SYNTH_SIZE_REG 0x40

#define ITERATIONS 100

static uint64_t t_start = 0;
static uint64_t t_end = 0;

uint64_t t_sw;
uint64_t t_cpu_write;
uint64_t t_acc;
uint64_t t_cpu_read;

static inline void start_counter() {
	asm volatile (
		"li t0, 0;"
		"csrr t0, mcycle;"
		"mv %0, t0"
		: "=r" (t_start)
		:
		: "t0"
	);
}

static inline uint64_t end_counter() {
	asm volatile (
		"li t0, 0;"
		"csrr t0, mcycle;"
		"mv %0, t0"
		: "=r" (t_end)
		:
		: "t0"
	);

	return (t_end - t_start);
}


static int validate_buf(token_t *out, token_t *gold)
{
	int i;
	int j;
	unsigned errors = 0;
    int32_t local_size = size;

	start_counter();
	for (j = 0; j < local_size; j++)
		if (out[j] != 0)
			errors++;
    t_cpu_read += end_counter();

	return errors;
}


static void init_buf (token_t *in, token_t * gold)
{
	int i;
	int j;
    int32_t local_size = size;
    int32_t local_compute_ratio = compute_ratio;
    int32_t local_speedup = speedup;

	start_counter();
	for (j = 0; j < local_size; j++)
		in[j] = (token_t) j;
    t_cpu_write += end_counter();

	start_counter();
	for (j = 0; j < local_size; j++) {
        token_t elem = in[j];
		gold[j] = elem * elem + elem;
    }
    t_sw += end_counter();
}


int main(int argc, char * argv[])
{
	int i, j;
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

	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = size;
		out_words_adj = size;
	} else {
		in_words_adj = round_up(size, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(size, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj * (1);
	out_len = out_words_adj * (1);
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset  = in_len;
	mem_size = (out_offset * sizeof(token_t)) + out_size;

	// Search for the device
	printf("Scanning device tree... \n");

	ndev = probe(&espdevs, VENDOR_SLD, SLD_ISCA_SYNTH, DEV_NAME);
	if (ndev == 0) {
		printf("isca_synth not found\n");
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

        size = 128;

        for (j = 0; j < 8; j++)
		{
	        t_sw = 0;
	        t_cpu_write = 0;
	        t_acc = 0;
	        t_cpu_read = 0;

			/* TODO: Restore full test once ESP caches are integrated */
			coherence = ACC_COH_FULL;

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
		    iowrite32(dev, ISCA_SYNTH_COMPUTE_RATIO_REG, compute_ratio);
		    iowrite32(dev, ISCA_SYNTH_SIZE_REG, size);

            for (i = 0; i < ITERATIONS; i++)
            {
			    init_buf(mem, gold);

			    // Flush (customize coherence model here)
			    esp_flush(coherence);

			    // Start accelerators
	            start_counter();
			    iowrite32(dev, CMD_REG, CMD_MASK_START);

			    // Wait for completion
			    done = 0;
			    while (!done) {
			    	done = ioread32(dev, STATUS_REG);
			    	done &= STATUS_MASK_DONE;
			    }
			    iowrite32(dev, CMD_REG, 0x0);
                t_acc += end_counter();

			    /* Validation */
			    errors = validate_buf(&mem[out_offset], gold);
            }

	        printf("  size = %d\n", size);
	        printf("  SW = %lu\n", t_sw/ITERATIONS);
	        printf("  CPU write = %lu\n", t_cpu_write/ITERATIONS);
	        printf("  ACC = %lu\n", t_acc/ITERATIONS);
	        printf("  CPU read = %lu\n", t_cpu_read/ITERATIONS);
	        printf("  Speedup = %d\n", t_sw/(t_acc+t_cpu_write+t_cpu_read));

            size *= 2;
		}

        if (errors) printf("  ... FAIL\n");
        else printf("  ... PASS\n");

		aligned_free(ptable);
		aligned_free(mem);
		aligned_free(gold);
	}

	return 0;
}
