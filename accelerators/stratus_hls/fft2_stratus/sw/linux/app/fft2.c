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

const float ERR_TH = 0.05;

#define NUM_ITERATIONS 100
#define N_TIME_MARKERS 10

unsigned long long StartTime;
unsigned long long EndTime;
unsigned long long TotalTime[N_TIME_MARKERS];

void StartCounter() {
	asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (StartTime)
		:
		: "t0"
	);
}

void EndCounter(unsigned Index) {
	asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (EndTime)
		:
		: "t0"
	);

    TotalTime[Index] += EndTime - StartTime;
}

/* User-defined code */
static int validate_buffer(token_t *out, float *gold)
{
	int j;
	unsigned errors = 0;
	const unsigned num_samples = 1<<logn_samples;

	StartCounter();
	for (j = 0; j < 2 * num_ffts * num_samples; j++) {
		native_t val = fx2float(out[j], FX_IL);
		if (val == 1234.1234) j = 0;
	}
	EndCounter(3);

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
	StartCounter();
	for (j = 0; j < 2 * num_ffts * num_samples; j++) {
		in[j] = float2fx((native_t) gold[j], FX_IL);
	}
	EndCounter(1);

	// Compute golden output
	StartCounter();
	fft2_comp(gold, num_ffts, (1<<logn_samples), logn_samples, do_inverse, do_shift);
	EndCounter(0);
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
	size = (out_offset * sizeof(token_t)) + out_size;
}


int main(int argc, char **argv)
{
	for (unsigned i = 0; i < N_TIME_MARKERS; i++)
    	TotalTime[i] = 0;
		
	int errors;

	float *gold;
	token_t *buf;

	const float ERROR_COUNT_TH = 0.001;
	const unsigned num_samples = (1 << logn_samples);

	init_parameters();

	buf = (token_t *) esp_alloc(size);
	cfg_000[0].hw_buf = buf;
	gold = malloc(out_len * sizeof(float));

	printf("\n====== %s ======\n\n", cfg_000[0].devname);
	/* <<--print-params-->> */
	printf("  .logn_samples = %d\n", logn_samples);
	printf("   num_samples  = %d\n", (1 << logn_samples));
	printf("  .num_ffts = %d\n", num_ffts);
	printf("  .do_inverse = %d\n", do_inverse);
	printf("  .do_shift = %d\n", do_shift);
	printf("  .scale_factor = %d\n", scale_factor);
	printf("\n  ** START **\n");

	for (unsigned i = 0; i < NUM_ITERATIONS; i++) {
		init_buffer(buf, gold);

		StartCounter();
		esp_run(cfg_000, NACC);
		EndCounter(2);

		errors = validate_buffer(&buf[out_offset], gold);
	}

	printf("SW op = %llu\n", TotalTime[0]/NUM_ITERATIONS);
	printf("init_buffer = %llu\n", TotalTime[1]/NUM_ITERATIONS);
	printf("accelerator op = %llu\n", TotalTime[2]/NUM_ITERATIONS);
	printf("validate_buffer = %llu\n", TotalTime[3]/NUM_ITERATIONS);

	free(gold);
	esp_free(buf);

        if ((float)(errors / (float)(2.0 * (float)num_ffts * (float)num_samples)) > ERROR_COUNT_TH)
		printf("  + TEST FAIL: exceeding error count threshold\n");
        else
		printf("  + TEST PASS: not exceeding error count threshold\n");

	printf("\n====== %s ======\n\n", cfg_000[0].devname);

	return errors;
}
