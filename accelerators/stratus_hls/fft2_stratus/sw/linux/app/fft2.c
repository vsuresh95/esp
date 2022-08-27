#include "cfg.h"
#include "utils/fft2_utils.h"

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned size;
static unsigned acc_offset;
static unsigned acc_size;

const float ERR_TH = 0.05;

/* User-defined code */
static int validate_buffer(token_t *out, float *gold)
{
	int j;
	unsigned errors = 0;
	const unsigned num_samples = 1<<logn_samples;

	for (j = 0; j < 2 * num_ffts * num_samples; j++) {
		native_t val = fx2float(out[j+SYNC_VAR_SIZE], FX_IL);

		if ((fabs(gold[j] - val) / fabs(gold[j])) > ERR_TH) {
			printf(" GOLD[%u] = %f vs %f = out[%u]\n", j, gold[j], val, j);
			errors++;
		}
	}
	printf("  + Relative error > %.02f for %d values out of %d\n", ERR_TH, errors, 2 * num_ffts * num_samples);

	return errors;
}


/* User-defined code */
static void init_buffer(token_t *in, float *gold, token_t *in_filter, float *gold_filter, token_t *in_twiddle, float *gold_twiddle, float *gold_freqdata)
{
	int j;
	const float LO = -2.0;
	const float HI = 2.0;
	const unsigned num_samples = (1 << logn_samples);

	srand((unsigned int) time(NULL));

	for (j = 0; j < 2 * num_ffts * num_samples; j++) {
		float scaling_factor = (float) rand () / (float) RAND_MAX;
		gold[j] = LO + scaling_factor * (HI - LO);
	}

	for (j = 0; j < 2 * num_ffts * num_samples; j++) {
		float scaling_factor = (float) rand () / (float) RAND_MAX;
		gold_filter[j] = LO + scaling_factor * (HI - LO);
	}

	for (j = 0; j < 2 * num_ffts * num_samples; j++) {
		float scaling_factor = (float) rand () / (float) RAND_MAX;
		gold_twiddle[j] = LO + scaling_factor * (HI - LO);
	}

	// convert input to fixed point
	for (j = 0; j < 2 * num_ffts * num_samples; j++) {
		in[j+SYNC_VAR_SIZE] = float2fx((native_t) gold[j], FX_IL);
	}

	// convert input to fixed point
	for (j = 0; j < 2 * num_ffts * num_samples; j++) {
		in_filter[j] = float2fx((native_t) gold_filter[j], FX_IL);
	}

	// convert input to fixed point
	for (j = 0; j < 2 * num_ffts * num_samples; j++) {
		in_twiddle[j] = float2fx((native_t) gold_filter[j], FX_IL);
	}

	// Compute golden output
	fft2_comp(gold, num_ffts, (1<<logn_samples), logn_samples, 0 /* do_inverse */, do_shift);

    cpx_num fpnk, fpk, f1k, f2k, tw, tdc;
    cpx_num fk, fnkc, fek, fok, tmp;
    cpx_num cptemp;
    cpx_num *tmpbuf = (cpx_num *) gold;
    cpx_num *freqdata = (cpx_num *) gold_freqdata;
    cpx_num *super_twiddles = (cpx_num *) gold_twiddle;
    cpx_num *filter = (cpx_num *) gold_filter;

	for (j = 0; j < 2 * num_ffts * num_samples; j++) {
		printf("  1 GOLD[%u] = %f\n", j, gold[j]);
    }

    // Post-processing
    tdc.r = tmpbuf[0].r;
    tdc.i = tmpbuf[0].i;
    C_FIXDIV(tdc,2);
    freqdata[0].r = tdc.r + tdc.i;
    freqdata[num_ffts * num_samples].r = tdc.r - tdc.i;
    freqdata[num_ffts * num_samples].i = freqdata[0].i = 0;

    for ( j=1;j <= num_ffts * num_samples/2 ; ++j ) {
        fpk    = tmpbuf[j];
        fpnk.r =   tmpbuf[num_ffts * num_samples-j].r;
        fpnk.i = - tmpbuf[num_ffts * num_samples-j].i;
        C_FIXDIV(fpk,2);
        C_FIXDIV(fpnk,2);

        C_ADD( f1k, fpk , fpnk );
        C_SUB( f2k, fpk , fpnk );
        C_MUL( tw , f2k , super_twiddles[j-1]);

        freqdata[j].r = HALF_OF(f1k.r + tw.r);
        freqdata[j].i = HALF_OF(f1k.i + tw.i);
        freqdata[num_ffts * num_samples-j].r = HALF_OF(f1k.r - tw.r);
        freqdata[num_ffts * num_samples-j].i = HALF_OF(tw.i - f1k.i);
    }

	for (j = 0; j < 2 * (num_ffts * num_samples+1); j++) {
		printf("  2 GOLD[%u] = %f\n", j, gold_freqdata[j]);
    }

    // FIR
	for (j = 0; j < 2 * (num_ffts * num_samples+1); j++) {
        C_MUL( cptemp, freqdata[j], filter[j]);
        freqdata[j] = cptemp;
    }

	for (j = 0; j < 2 * (num_ffts * num_samples+1); j++) {
		printf("  3 GOLD[%u] = %f\n", j, gold_freqdata[j]);
    }

    // Pre-processing
    tmpbuf[0].r = freqdata[0].r + freqdata[num_ffts * num_samples].r;
    tmpbuf[0].i = freqdata[0].r - freqdata[num_ffts * num_samples].r;
    C_FIXDIV(tmpbuf[0],2);

    for (j = 1; j <= num_ffts * num_samples / 2; ++j) {
        fk = freqdata[j];
        fnkc.r = freqdata[num_ffts * num_samples - j].r;
        fnkc.i = -freqdata[num_ffts * num_samples - j].i;
        C_FIXDIV( fk , 2 );
        C_FIXDIV( fnkc , 2 );

        C_ADD (fek, fk, fnkc);
        C_SUB (tmp, fk, fnkc);
        C_MUL (fok, tmp, super_twiddles[j-1]);
        C_ADD (tmpbuf[j],     fek, fok);
        C_SUB (tmpbuf[num_ffts * num_samples - j], fek, fok);
        tmpbuf[num_ffts * num_samples - j].i *= -1;
    }

	for (j = 0; j < 2 * num_ffts * num_samples; j++) {
		printf("  4 GOLD[%u] = %f\n", j, gold[j]);
    }

	fft2_comp(gold, num_ffts, (1<<logn_samples), logn_samples, 1 /* do_inverse */, do_shift);
}


/* User-defined code */
static void init_parameters()
{
	const unsigned num_samples = (1 << logn_samples);

	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = 2 * num_ffts * num_samples;
		out_words_adj = 2 * num_ffts * num_samples;
	} else {
		in_words_adj = round_up(2 * num_ffts * num_samples, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(2 * num_ffts * num_samples, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj;
	out_len =  out_words_adj;
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset = 0;
	size = (out_offset * sizeof(token_t)) + out_size + (SYNC_VAR_SIZE * sizeof(token_t));

    acc_size = size;
    acc_offset = out_offset + out_len + SYNC_VAR_SIZE;
    size *= NUM_DEVICES+1+3;
}


int main(int argc, char **argv)
{
	int errors;

	float *gold;
	token_t *buf;

    const float ERROR_COUNT_TH = 0.001;
	const unsigned num_samples = (1 << logn_samples);

	init_parameters();

	buf = (token_t *) esp_alloc(size);
	cfg_000[0].hw_buf = buf;
	cfg_001[0].hw_buf = buf;
	cfg_002[0].hw_buf = buf;
	gold = malloc((4 * out_len + 2) * sizeof(float));

	printf("   buf = %p\n", buf);
	printf("   gold = %p\n", gold);

    fft2_cfg_001[0].src_offset = 2 * acc_size;
    fft2_cfg_001[0].dst_offset = 2 * acc_size;

    fir_cfg_000[0].src_offset = acc_size;
    fir_cfg_000[0].dst_offset = acc_size;

    volatile token_t* sm_sync = (volatile token_t*) buf;

	printf("\n====== %s ======\n\n", cfg_000[0].devname);
	/* <<--print-params-->> */
	printf("  .logn_samples = %d\n", logn_samples);
	printf("   num_samples  = %d\n", (1 << logn_samples));
	printf("  .num_ffts = %d\n", num_ffts);
	printf("  .do_inverse = %d\n", do_inverse);
	printf("  .do_shift = %d\n", do_shift);
	printf("  .scale_factor = %d\n", scale_factor);
	printf("\n  ** START **\n");
	init_buffer(buf, gold, (buf + acc_offset + 4 * out_len), (gold + out_len),
            (buf + acc_offset + 5 * out_len), (gold + 2 * out_len), (gold + 3 * out_len));

	sm_sync[0] = 0;
	sm_sync[acc_offset] = 0;
	sm_sync[2*acc_offset] = 0;
	sm_sync[3*acc_offset] = 0;

    // Invoke accelerators but do not check for end
	fft2_cfg_000[0].esp.start_stop = 1;
	fft2_cfg_001[0].esp.start_stop = 1;
	fir_cfg_000[0].esp.start_stop = 1;

	esp_run(cfg_000, NACC);
	esp_run(cfg_001, NACC);
	esp_run(cfg_002, NACC);

    // Start first accelerator
	sm_sync[0] = 1;

    // Wait for last accelerator
	while(sm_sync[NUM_DEVICES*acc_offset] != 1);

	printf("\n  ** DONE **\n");

	errors = validate_buffer(&buf[NUM_DEVICES*acc_offset], gold);

	free(gold);
	esp_free(buf);

    if ((float)(errors / (float)(2.0 * (float)num_ffts * (float)num_samples)) > ERROR_COUNT_TH)
	    printf("  + TEST FAIL: exceeding error count threshold\n");
    else
	    printf("  + TEST PASS: not exceeding error count threshold\n");

	printf("\n====== %s ======\n\n", cfg_000[0].devname);

	return errors;
}
