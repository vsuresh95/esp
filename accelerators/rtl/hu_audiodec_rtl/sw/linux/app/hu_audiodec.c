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
static unsigned size;

/* User-defined code */
static int validate_buffer(token_t *out, token_t *gold)
{
	int i;
	int j;
	unsigned errors = 0;

	for (i = 0; i < 1; i++)
		for (j = 0; j < cfg_regs_10; j++)
			if (gold[i * out_words_adj + j] != out[i * out_words_adj + j])
				errors++;

	return errors;
}


/* User-defined code */
static void init_buffer(token_t *in, token_t * gold)
{
	int i;
	int j;

	for (i = 0; i < 1; i++)
		for (j = 0; j < cfg_regs_10; j++)
			in[i * in_words_adj + j] = (token_t) j;

	for (i = 0; i < 1; i++)
		for (j = 0; j < cfg_regs_10; j++)
			gold[i * out_words_adj + j] = (token_t) j;
}


/* User-defined code */
static void init_parameters()
{
	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = cfg_regs_10;
		out_words_adj = cfg_regs_10;
	} else {
		in_words_adj = round_up(cfg_regs_10, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(cfg_regs_10, DMA_WORD_PER_BEAT(sizeof(token_t)));
	}
	in_len = in_words_adj * (1);
	out_len =  out_words_adj * (1);
	in_size = in_len * sizeof(token_t);
	out_size = out_len * sizeof(token_t);
	out_offset = in_len;
	size = (out_offset * sizeof(token_t)) + out_size;
}


int main(int argc, char **argv)
{
	int errors;

	token_t *gold;
	token_t *buf;

	init_parameters();

	buf = (token_t *) esp_alloc(size);
	cfg_000[0].hw_buf = buf;
    
	gold = malloc(out_size);

	init_buffer(buf, gold);

	printf("\n====== %s ======\n\n", cfg_000[0].devname);
	/* <<--print-params-->> */
	printf("  .cfg_regs_31 = %d\n", cfg_regs_31);
	printf("  .cfg_regs_30 = %d\n", cfg_regs_30);
	printf("  .cfg_regs_26 = %d\n", cfg_regs_26);
	printf("  .cfg_regs_27 = %d\n", cfg_regs_27);
	printf("  .cfg_regs_24 = %d\n", cfg_regs_24);
	printf("  .cfg_regs_25 = %d\n", cfg_regs_25);
	printf("  .cfg_regs_22 = %d\n", cfg_regs_22);
	printf("  .cfg_regs_23 = %d\n", cfg_regs_23);
	printf("  .cfg_regs_8 = %d\n", cfg_regs_8);
	printf("  .cfg_regs_20 = %d\n", cfg_regs_20);
	printf("  .cfg_regs_9 = %d\n", cfg_regs_9);
	printf("  .cfg_regs_21 = %d\n", cfg_regs_21);
	printf("  .cfg_regs_6 = %d\n", cfg_regs_6);
	printf("  .cfg_regs_7 = %d\n", cfg_regs_7);
	printf("  .cfg_regs_4 = %d\n", cfg_regs_4);
	printf("  .cfg_regs_5 = %d\n", cfg_regs_5);
	printf("  .cfg_regs_2 = %d\n", cfg_regs_2);
	printf("  .cfg_regs_3 = %d\n", cfg_regs_3);
	printf("  .cfg_regs_0 = %d\n", cfg_regs_0);
	printf("  .cfg_regs_28 = %d\n", cfg_regs_28);
	printf("  .cfg_regs_1 = %d\n", cfg_regs_1);
	printf("  .cfg_regs_29 = %d\n", cfg_regs_29);
	printf("  .cfg_regs_19 = %d\n", cfg_regs_19);
	printf("  .cfg_regs_18 = %d\n", cfg_regs_18);
	printf("  .cfg_regs_17 = %d\n", cfg_regs_17);
	printf("  .cfg_regs_16 = %d\n", cfg_regs_16);
	printf("  .cfg_regs_15 = %d\n", cfg_regs_15);
	printf("  .cfg_regs_14 = %d\n", cfg_regs_14);
	printf("  .cfg_regs_13 = %d\n", cfg_regs_13);
	printf("  .cfg_regs_12 = %d\n", cfg_regs_12);
	printf("  .cfg_regs_11 = %d\n", cfg_regs_11);
	printf("  .cfg_regs_10 = %d\n", cfg_regs_10);
	printf("\n  ** START **\n");

	esp_run(cfg_000, NACC);

	printf("\n  ** DONE **\n");

	errors = validate_buffer(&buf[out_offset], gold);

	free(gold);
	esp_free(buf);

	if (!errors)
		printf("+ Test PASSED\n");
	else
		printf("+ Test FAILED\n");

	printf("\n====== %s ======\n\n", cfg_000[0].devname);

	return errors;
}
