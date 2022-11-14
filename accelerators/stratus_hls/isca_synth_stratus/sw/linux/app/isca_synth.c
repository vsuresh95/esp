// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include "libesp.h"
#include "cfg.h"

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned mem_size;

static uint64_t t_start = 0;
static uint64_t t_end = 0;

uint64_t t_sw;
uint64_t t_cpu_write;
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
static int validate_buffer(token_t *out, token_t *gold)
{
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


/* User-defined code */
static void init_buffer(token_t *in, token_t * gold)
{
	int j;
    int32_t local_size = size;
    int32_t local_compute_ratio = compute_ratio;
    int32_t local_speedup = speedup;

	start_counter();
	for (j = 0; j < local_size; j++)
			in[j] = (token_t) j;
    t_cpu_write += end_counter();

	start_counter();
	for (j = 0; j < local_size * local_compute_ratio * local_speedup; j++)
		asm volatile ("nop");
    t_sw += end_counter();
}


/* User-defined code */
static void init_parameters()
{
	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = 16384;
		out_words_adj = 16384;
	} else {
		in_words_adj = round_up(16384, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(16384, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj * (1);
	out_len =  out_words_adj * (1);
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset = in_len;
	mem_size = (out_offset * sizeof(token_t)) + out_size;
}


int main(int argc, char **argv)
{
	int errors;
    int i, j, k;

	token_t *gold;
	token_t *buf;

	init_parameters();

	buf = (token_t *) esp_alloc(mem_size);
	cfg_000[0].hw_buf = buf;
    
	gold = malloc(out_size);

    for (k = 0; k < 5; k++)
	{
        compute_ratio = k*20 + 1;
        isca_synth_cfg_000[0].compute_ratio = compute_ratio;

        size = 128;
        isca_synth_cfg_000[0].size = size;

	    printf("-------- Compute factor %d --------\n", compute_ratio);
        for (j = 0; j < 8; j++)
	    {
	        t_sw = 0;
	        t_cpu_write = 0;
	        t_acc = 0;
	        t_cpu_read = 0;

            for (i = 0; i < ITERATIONS; i++)
            {
                init_buffer(buf, gold);
                
	            start_counter();
                esp_run(cfg_000, NACC);
                t_acc += end_counter();
                
                errors = validate_buffer(&buf[out_offset], gold);
            }

	        printf("Size = %dkB,\t SW = %ld,\t CPU write = %ld,\t ACC = %ld,\t CPU read = %ld,\t Speedup = %f\n", 
                    size*8/1000, t_sw/ITERATIONS, t_cpu_write/ITERATIONS, t_acc/ITERATIONS, t_cpu_read/ITERATIONS,
                    (float) t_sw/(t_acc+t_cpu_write+t_cpu_read));

            size *= 2;
            isca_synth_cfg_000[0].size = size;
        }
    }

	free(gold);
	esp_free(buf);

	if (!errors)
		printf("+ Test PASSED\n");
	else
		printf("+ Test FAILED\n");

	printf("\n====== %s ======\n\n", cfg_000[0].devname);

	return errors;
}
