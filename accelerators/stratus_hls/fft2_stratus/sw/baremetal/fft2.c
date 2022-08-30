/* Copyright (c) 2011-2019 Columbia University, System Level Design Group */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>
#include "utils/fft2_utils.h"

#if (FFT2_FX_WIDTH == 64)
typedef long long token_t;
typedef double native_t;
#define fx2float fixed64_to_double
#define float2fx double_to_fixed64
#define FX_IL 42
#else // (FFT2_FX_WIDTH == 32)
typedef int token_t;
typedef float native_t;
#define fx2float fixed32_to_float
#define float2fx float_to_fixed32
#define FX_IL 14
#endif /* FFT2_FX_WIDTH */

const float ERR_TH = 0.05;

static unsigned DMA_WORD_PER_BEAT(unsigned _st)
{
        return (sizeof(void *) / _st);
}


token_t _sin(token_t x)
{
	int i = 1;
	token_t cur = x;
	token_t acc = 1;
	token_t fact= 1;
	token_t pow = x;
	while (fabs(acc) > .00000001 &&   i < 100){
		fact *= ((2*i)*(2*i+1));
		pow *= -1 * x*x;
		acc =  pow / fact;
		cur += acc;
		i++;
	}
	return cur;

}

token_t _cos(token_t x)
{
	int i = 1;
	token_t cur = 1;
	token_t acc = 1;
	token_t fact= 1;
	token_t pow = x;
	while (fabs(acc) > .00000001 &&   i < 100){
		fact *= ((2*i)*(2*i+1));
		pow *= -1 * x*x;
		acc =  pow / fact;
		cur += acc;
		i++;
	}
	return cur;

}

token_t _tan(token_t x) {
     return (_sin(x)/_cos(x));
}

typedef struct {
    float r;
    float i;
} cpx_num;

#   define S_MUL(a,b) ( (a)*(b) )
#define C_MUL(m,a,b) \
    do{ (m).r = (a).r*(b).r - (a).i*(b).i;\
        (m).i = (a).r*(b).i + (a).i*(b).r; }while(0)
#   define C_FIXDIV(c,div) \
    do{ (c).r /= (div);\
        (c).i /= (div); }while(0)
#   define C_MULBYSCALAR( c, s ) \
    do{ (c).r *= (s);\
        (c).i *= (s); }while(0)

#define  C_ADD( res, a,b)\
    do { \
        (res).r=(a).r+(b).r;  (res).i=(a).i+(b).i; \
    }while(0)
#define  C_SUB( res, a,b)\
    do { \
        (res).r=(a).r-(b).r;  (res).i=(a).i-(b).i; \
    }while(0)

#  define HALF_OF(x) ((x)*.5)

#define SLD_FFT2 0x057
#define DEV_NAME "sld,fft2_stratus"

#define SLD_FIR 0x050
#define FIR_DEV_NAME "sld,fir_stratus"

/* <<--params-->> */
const int32_t logn_samples = 6;
const int32_t num_samples = (1 << logn_samples);
const int32_t num_ffts = 1;
const int32_t do_inverse = 0;
const int32_t do_shift = 0;
const int32_t scale_factor = 1;
int32_t len;

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned mem_size;
static unsigned acc_offset;
static unsigned acc_size;

/* Size of the contiguous chunks for scatter/gather */
#define CHUNK_SHIFT 20
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK(_sz) ((_sz % CHUNK_SIZE == 0) ?		\
			(_sz / CHUNK_SIZE) :		\
			(_sz / CHUNK_SIZE) + 1)

/* User defined registers */
/* <<--regs-->> */
#define FFT2_LOGN_SAMPLES_REG 0x40
#define FFT2_NUM_FFTS_REG 0x44
#define FFT2_DO_INVERSE_REG 0x48
#define FFT2_DO_SHIFT_REG 0x4c
#define FFT2_SCALE_FACTOR_REG 0x50

#define FIR_LOGN_SAMPLES_REG 0x40
#define FIR_NUM_FIRS_REG 0x44
#define FIR_DO_INVERSE_REG 0x48
#define FIR_DO_SHIFT_REG 0x4c
#define FIR_SCALE_FACTOR_REG 0x50

#define SYNC_VAR_SIZE 2

#define NUM_DEVICES 3

static int validate_buf(token_t *out, float *gold)
{
	int j;
	unsigned errors = 0;

	for (j = 0; j < 2 * len; j++) {
		native_t val = fx2float(out[j+SYNC_VAR_SIZE], FX_IL);
		uint32_t ival = *((uint32_t*)&val);
		printf("%u G %08x O %08x\n", j, ((uint32_t*)gold)[j], ival);
		if ((fabs(gold[j] - val) / fabs(gold[j])) > ERR_TH) {
			errors++;
        }
	}

	//printf("  %u errors\n", errors);
	return errors;
}


static void init_buf(token_t *in, float *gold, token_t *in_filter, float *gold_filter, token_t *in_twiddle, float *gold_twiddle, float *gold_freqdata)
{
	int j;
	const float LO = -10.0;
	const float HI = 10.0;

	/* srand((unsigned int) time(NULL)); */

	for (j = 0; j < 2 * len; j++) {
		float scaling_factor = (float) rand () / (float) RAND_MAX;
		gold[j] = LO + scaling_factor * (HI - LO);
		uint32_t ig = ((uint32_t*)gold)[j];
		// printf("  IN[%u] = 0x%08x\n", j, ig);
	}

	for (j = 0; j < 2 * (len+1); j++) {
		float scaling_factor = (float) rand () / (float) RAND_MAX;
		gold_filter[j] = LO + scaling_factor * (HI - LO);
		uint32_t ig = ((uint32_t*)gold_filter)[j];
		// printf("  IN[%u] = 0x%08x\n", j, ig);
	}

	for (j = 0; j < 2 * len; j+=2) {
        token_t phase = -3.14159 * ((token_t) ((j+1) / len) + .5);
        gold_twiddle[j] = _cos(phase);
        gold_twiddle[j + 1] = _sin(phase);
		uint32_t ig = ((uint32_t*)gold_twiddle)[j];
		// printf("  IN[%u] = 0x%08x\n", j, ig);
	}

	// convert input to fixed point
	for (j = 0; j < 2 * len; j++)
		in[j+SYNC_VAR_SIZE] = float2fx((native_t) gold[j], FX_IL);

	// convert input to fixed point
	for (j = 0; j < 2 * (len+1); j++)
		in_filter[j] = float2fx((native_t) gold_filter[j], FX_IL);

	// convert input to fixed point
	for (j = 0; j < 2 * len; j++)
		in_twiddle[j] = float2fx((native_t) gold_twiddle[j], FX_IL);

	// Compute golden output
	fft2_comp(gold, num_ffts, num_samples, logn_samples, 0 /* do_inverse */, do_shift);

    cpx_num fpnk, fpk, f1k, f2k, tw, tdc;
    cpx_num fk, fnkc, fek, fok, tmp;
    cpx_num cptemp;
    cpx_num inv_twd;
    cpx_num *tmpbuf = (cpx_num *) gold;
    cpx_num *freqdata = (cpx_num *) gold_freqdata;
    cpx_num *super_twiddles = (cpx_num *) gold_twiddle;
    cpx_num *filter = (cpx_num *) gold_filter;

	// for (j = 0; j < 2 * len; j++) {
	// 	printf("  1 GOLD[%u] = 0x%08x\n", j, ((uint32_t*)gold)[j]);
    // }

    // Post-processing
    tdc.r = tmpbuf[0].r;
    tdc.i = tmpbuf[0].i;
    C_FIXDIV(tdc,2);
    freqdata[0].r = tdc.r + tdc.i;
    freqdata[len].r = tdc.r - tdc.i;
    freqdata[len].i = freqdata[0].i = 0;

    for ( j=1;j <= len/2 ; ++j ) {
        fpk    = tmpbuf[j]; 
        fpnk.r =   tmpbuf[len-j].r;
        fpnk.i = - tmpbuf[len-j].i;
        C_FIXDIV(fpk,2);
        C_FIXDIV(fpnk,2);

        C_ADD( f1k, fpk , fpnk );
        C_SUB( f2k, fpk , fpnk );
        C_MUL( tw , f2k , super_twiddles[j-1]);

        freqdata[j].r = HALF_OF(f1k.r + tw.r);
        freqdata[j].i = HALF_OF(f1k.i + tw.i);
        freqdata[len-j].r = HALF_OF(f1k.r - tw.r);
        freqdata[len-j].i = HALF_OF(tw.i - f1k.i);
    }

	// for (j = 0; j < 2 * (len+1); j++) {
	// 	printf("  2 GOLD[%u] = 0x%08x\n", j, ((uint32_t*)gold_freqdata)[j]);
    // }

    // FIR
	for (j = 0; j < len + 1; j++) {
        C_MUL( cptemp, freqdata[j], filter[j]);
        freqdata[j] = cptemp;
    }

	// for (j = 0; j < 2 * (len+1); j++) {
	// 	printf("  3 GOLD[%u] = 0x%08x\n", j, ((uint32_t*)gold_freqdata)[j]);
    // }

    // Pre-processing
    tmpbuf[0].r = freqdata[0].r + freqdata[len].r;
    tmpbuf[0].i = freqdata[0].r - freqdata[len].r;
    C_FIXDIV(tmpbuf[0],2);

    for (j = 1; j <= len/2; ++j) {
        fk = freqdata[j];
        fnkc.r = freqdata[len-j].r;
        fnkc.i = -freqdata[len-j].i;
        C_FIXDIV( fk , 2 );
        C_FIXDIV( fnkc , 2 );
        inv_twd = super_twiddles[j-1];
        C_MULBYSCALAR(inv_twd,-1);

        C_ADD (fek, fk, fnkc);
        C_SUB (tmp, fk, fnkc);
        C_MUL (fok, tmp, inv_twd);
        C_ADD (tmpbuf[j],     fek, fok);
        C_SUB (tmpbuf[len-j], fek, fok);
        tmpbuf[len-j].i *= -1;
    }

	// for (j = 0; j < 2 * len; j++) {
	// 	printf("  4 GOLD[%u] = 0x%08x\n", j, ((uint32_t*)gold)[j]);
    // }

	fft2_comp(gold, num_ffts, num_samples, logn_samples, 1 /* do_inverse */, do_shift);
}

int main(int argc, char * argv[])
{
	int i;
	int n;
	int ndev;
	struct esp_device *espdevs;
	struct esp_device *dev;
	struct esp_device *fir_dev;
	unsigned done;
	unsigned spin_ct;
	unsigned **ptable = NULL;
	token_t *mem;
	float *gold;
	unsigned errors = 0;
	unsigned coherence;
        const float ERROR_COUNT_TH = 0.001;

	len = num_ffts * (1 << logn_samples);
	printf("logn %u nsmp %u nfft %u inv %u shft %u len %u\n", logn_samples, num_samples, num_ffts, do_inverse, do_shift, len);
	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = 2 * len;
		out_words_adj = 2 * len;
	} else {
		in_words_adj = round_up(2 * len, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(2 * len, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj;
	out_len = out_words_adj;
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset  = 0;
	mem_size = (out_offset * sizeof(token_t)) + out_size + (SYNC_VAR_SIZE * sizeof(token_t));

    acc_size = mem_size;
    acc_offset = out_offset + out_len + SYNC_VAR_SIZE;
    mem_size *= NUM_DEVICES+5;

	printf("ilen %u isize %u o_off %u olen %u osize %u msize %u\n", in_len, out_len, in_size, out_size, out_offset, mem_size);

	// Allocate memory
	gold = aligned_malloc((6 * out_len) * sizeof(float));
	mem = aligned_malloc(mem_size);
	printf("  memory buffer base-address = %p\n", mem);

    volatile token_t* sm_sync = (volatile token_t*) mem;

	// Allocate and populate page table
	ptable = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
	for (i = 0; i < NCHUNK(mem_size); i++)
		ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];

	printf("  ptable = %p\n", ptable);
	printf("  nchunk = %lu\n", NCHUNK(mem_size));

	coherence = ACC_COH_FULL;
	printf("  --------------------\n");
	printf("  Generate input...\n");
	init_buf(mem, gold,
            (mem + acc_offset + 4 * out_len) /* in_filter */, (gold + out_len) /* gold_filter */,
            (mem + acc_offset + 6 * out_len) /* in_twiddle */, (gold + 3 * out_len) /* gold_twiddle */,
            (gold + 4 * out_len) /* gold_freqdata */);

	printf("Scanning device tree... \n");

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
		printf("  Start...\n");
		iowrite32(dev, CMD_REG, CMD_MASK_START);
    }

	unsigned nfir = probe(&fir_dev, VENDOR_SLD, SLD_FIR, FIR_DEV_NAME);
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
		printf("  Start...\n");
		iowrite32(dev, CMD_REG, CMD_MASK_START);
    }

	sm_sync[0] = 1;
	while(sm_sync[NUM_DEVICES*acc_offset] != 1);

	for (n = 0; n < ndev; n++) {

		printf("**************** %s.%d ****************\n", DEV_NAME, n);

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

        printf("  Done : spin_count = %u\n", spin_ct);
    }

	for (n = 0; n < nfir; n++) {

		printf("**************** %s.%d ****************\n", FIR_DEV_NAME, n);

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

        printf("  Done : spin_count = %u\n", spin_ct);
    }

	printf("  validating...\n");

	/* Validation */
	errors = validate_buf(&mem[NUM_DEVICES*acc_offset], gold);
	if ((float)((float)errors / (2.0 * (float)len)) > ERROR_COUNT_TH)
		printf("  ... FAIL : %u errors out of %u\n", errors, 2*len);
	else
		printf("  ... PASS : %u errors out of %u\n", errors, 2*len);

	aligned_free(ptable);
	aligned_free(mem);
	aligned_free(gold);

    // while(1);

	return 0;
}
