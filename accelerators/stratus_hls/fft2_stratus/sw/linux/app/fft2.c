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
static void init_buffer(token_t *in, float *gold)
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

	// convert input to fixed point
	for (j = 0; j < 2 * num_ffts * num_samples; j++) {
		in[j+SYNC_VAR_SIZE] = float2fx((native_t) gold[j], FX_IL);
	}

	// Compute golden output
	fft2_comp(gold, num_ffts, (1<<logn_samples), logn_samples, 0 /* do_inverse */, do_shift);

	for (j = 0; j < 2 * num_ffts * num_samples; j++) {
		printf("  INT GOLD[%u] = %f\n", j, gold[j]);
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
    size *= NUM_DEVICES+1;
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
	gold = malloc(out_len * sizeof(float));

	printf("   buf = %p\n", buf);
	printf("   gold = %p\n", gold);

    fft2_cfg_001[0].src_offset = acc_size;
    fft2_cfg_001[0].dst_offset = acc_size;

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
	init_buffer(buf, gold);

	sm_sync[0] = 0;
	sm_sync[acc_offset] = 0;
	sm_sync[2*acc_offset] = 0;

    // Invoke accelerators but do not check for end
	fft2_cfg_000[0].esp.start_stop = 1;
	fft2_cfg_001[0].esp.start_stop = 1;

	esp_run(cfg_000, NACC);
	esp_run(cfg_001, NACC);

    // Start first accelerator
	sm_sync[0] = 1;

    // Wait for last accelerator
	while(sm_sync[2*acc_offset] != 1);

	printf("\n  ** DONE **\n");

	errors = validate_buffer(&buf[2*acc_offset], gold);

	free(gold);
	esp_free(buf);

    if ((float)(errors / (float)(2.0 * (float)num_ffts * (float)num_samples)) > ERROR_COUNT_TH)
	    printf("  + TEST FAIL: exceeding error count threshold\n");
    else
	    printf("  + TEST PASS: not exceeding error count threshold\n");

	printf("\n====== %s ======\n\n", cfg_000[0].devname);

	return errors;
}
