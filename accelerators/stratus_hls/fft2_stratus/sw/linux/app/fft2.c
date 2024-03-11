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

#define ITERATIONS 1000

static uint64_t t_start = 0;
static uint64_t t_end = 0;

uint64_t t_cpu_write;
uint64_t t_sw;
uint64_t t_acc;
uint64_t t_cpu_read;

static inline void start_counter() {
	asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (t_start)
		:
		: "t0"
	);
}

static inline uint64_t end_counter() {
	asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (t_end)
		:
		: "t0"
	);

	return (t_end - t_start);
}

/* User-defined code */
static int validate_buffer(token_t *out, float *gold)
{
	int j;
	unsigned errors = 0;
	const unsigned num_samples = 1<<logn_samples;

	for (j = 0; j < 2 * num_ffts * num_samples; j++) {
		native_t val = fx2float(out[j], FX_IL);

		if ((fabs(gold[j] - val) / fabs(gold[j])) > ERR_TH) {
			if (errors < 2) {
				// printf(" GOLD[%u] = %f vs %f = out[%u]\n", j, gold[j], val, j);
			}
			errors++;
		}
	}
	// printf("  + Relative error > %.02f for %d values out of %d\n", ERR_TH, errors, 2 * num_ffts * num_samples);

	return errors;
}


/* User-defined code */
static void init_buffer(token_t *in, float *gold)
{
	int j;

	// convert input to fixed point
	for (j = 0; j < 2 * num_ffts * num_samples; j++) {
		in[j] = float2fx((native_t) gold[j], FX_IL);
	}
}

	// Compute golden output
	start_counter();
	fft2_comp(gold, num_ffts, (1<<logn_samples), logn_samples, do_inverse, do_shift);
	t_sw += end_counter();
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
	int j;
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

	t_cpu_write = 0;
	t_sw = 0;
	t_acc = 0;
	t_cpu_read = 0;

	for (int i = 0; i < ITERATIONS; i++) {
		init_buffer(buf, gold);

		start_counter();
		esp_run(cfg_000, NACC);
		t_acc += end_counter();

		errors = validate_buffer(&buf[out_offset], gold);
	}

	for (int i = 0; i < ITERATIONS; i++) {
		start_counter();
		init_buffer(buf, gold);
		t_cpu_write += end_counter();

		start_counter();
		esp_run(cfg_000, NACC);
		t_acc += end_counter();

		start_counter();
		errors = validate_buffer(&buf[out_offset], gold);
		t_cpu_read += end_counter();
	}

	free(gold);
	esp_free(buf);

        if ((float)(errors / (float)(2.0 * (float)num_ffts * (float)num_samples)) > ERROR_COUNT_TH)
		printf("  + TEST FAIL: exceeding error count threshold\n");
        else
		printf("  + TEST PASS: not exceeding error count threshold\n");

	printf("\n====== %s ======\n\n", cfg_000[0].devname);

	printf("  CPU write = %lu\n", t_cpu_write/ITERATIONS);
	printf("  Software = %lu\n", t_sw/ITERATIONS);
	printf("  Accelerator = %lu\n", t_acc/ITERATIONS);
	printf("  CPU read = %lu\n", t_cpu_read/ITERATIONS);

	return errors;
}
