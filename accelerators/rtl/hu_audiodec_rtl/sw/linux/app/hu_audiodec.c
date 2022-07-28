// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0
#include "libesp.h"
#include "cfg.h"
#include "data.h"

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
		for (j = 0; j < 16384; j++)
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
		for (j = 0; j < 16384; j++)
			in[i * in_words_adj + j] = new_audio_in[i * in_words_adj + j];

	for (i = 0; i < 1; i++)
		for (j = 0; j < 16384; j++)
			gold[i * out_words_adj + j] = new_audio_out[i * in_words_adj + j];
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
	size = (out_offset * sizeof(token_t)) + out_size;
}

/* Pure software computation for the audio blocks */
void rotateOrder_sw() {
	int i;

	float output[16384];	// for the new test
	float valueX, valueY, valueZ, valueR, valueS, valueT, valueU, valueV, valueK, valueL, valueM, valueN, valueO, valueP, valueQ;
	float vX, vY, vZ, vR, vS, vT, vU, vV, vK, vL, vM, vN, vO, vP, vQ;

	float sinBetaSq = conf_11_f * conf_11_f;
	float sinBetaCb = conf_11_f * conf_11_f * conf_11_f;
	float fSqrt3 = 1.7320508;
	float fSqrt3_2 = 1.2247448;
	float fSqrt15 = 3.8729833;
	float fSqrt5_2 = 1.5811388;
	float fxp025 = 0.25;
	float fxp050 = 0.5;
	float fxp075 = 0.75;
	float fxp00625 = 0.0625;
	float fxp0125 = 0.125;

	for (i = 0; i < 16384; i += 16) {	// for the new test
		// Rotate Order 1
		valueY = -audio_in[i + 3] * conf_9_f + audio_in[i + 1] * conf_8_f;
		valueZ = audio_in[i + 2];
		valueX = audio_in[i + 3] * conf_8_f + audio_in[i + 1] * conf_9_f;

		vY = valueY;
		vZ = valueX * conf_11_f + valueZ * conf_10_f;
		vX = valueX * conf_10_f + valueZ * conf_11_f;

		output[i + 1] = -vX * conf_13_f + vY * conf_12_f;
		output[i + 2] = vZ;
        output[i + 3] = vX * conf_12_f + vY * conf_13_f;

		// Rotate Order 2
		valueV = -audio_in[i + 8] * conf_15_f + audio_in[i + 4] * conf_14_f;
        valueT = -audio_in[i + 7] * conf_9_f + audio_in[i + 5] * conf_8_f;
        valueR = audio_in[i + 6];
        valueS = audio_in[i + 7] * conf_8_f + audio_in[i + 5] * conf_9_f;
        valueU = audio_in[i + 8] * conf_14_f + audio_in[i + 4] * conf_15_f;

        vV = -conf_11_f * valueT + conf_10_f * valueV;
    	vT = -conf_10_f * valueT + conf_11_f * valueV;
		vR = (fxp075 * conf_16_f + fxp025) * valueR + (fxp050 * fSqrt3 * sinBetaSq) * valueU + (fSqrt3 * conf_11_f * conf_10_f) * valueS;
		vS = conf_16_f * valueS - fSqrt3 * conf_10_f * conf_11_f * valueR + conf_10_f * conf_11_f * valueU;
		vU = (fxp025 * conf_16_f + fxp075) * valueU - conf_10_f * conf_11_f * valueS + fxp050 * fSqrt3 * sinBetaSq * valueR;

		output[i + 4] = -vU * conf_19_f + vV * conf_18_f;
		output[i + 5] = -vS * conf_13_f + vT * conf_12_f;
		output[i + 6] = vR;
		output[i + 7] = vS * conf_12_f + vT * conf_13_f;
		output[i + 8] = vU * conf_18_f + vV * conf_19_f;

		// Rotate Order 3
		valueQ = -audio_in[i + 15] * conf_21_f + audio_in[i + 9] * conf_20_f;
		valueO = -audio_in[i + 14] * conf_15_f + audio_in[i + 10] * conf_14_f;
		valueM = -audio_in[i + 13] * conf_9_f + audio_in[i + 11] * conf_8_f;
		valueK = audio_in[i + 12];
		valueL = audio_in[i + 13] * conf_8_f + audio_in[i + 11] * conf_9_f;
		valueN = audio_in[i + 14] * conf_14_f + audio_in[i + 10] * conf_15_f;
		valueP = audio_in[i + 15] * conf_20_f + audio_in[i + 9] * conf_21_f;

		vQ = fxp0125 * valueQ * (5 + 3 * conf_16_f) - fSqrt3_2 * valueO * conf_10_f * conf_11_f + fxp025 * fSqrt15 * valueM * sinBetaSq;
		vO = valueO * conf_16_f - fSqrt5_2 * valueM * conf_10_f * conf_11_f + fSqrt3_2 * valueQ * conf_10_f * conf_11_f;
		vM = fxp0125 * valueM * (3 + 5 * conf_16_f) - fSqrt5_2 * valueO * conf_10_f * conf_11_f + fxp025 * fSqrt15 * valueQ * sinBetaSq;
		vK = fxp025 * valueK * conf_10_f * (-1 + 15 * conf_16_f) + fxp050 * fSqrt15 * valueN * conf_10_f * sinBetaSq + fxp050 * fSqrt5_2 * valueP * sinBetaCb + fxp0125 * fSqrt3_2 * valueL * (conf_11_f + 5 * conf_23_f);
		vL = fxp00625 * valueL * (conf_10_f + 15 * conf_22_f) + fxp025 * fSqrt5_2 * valueN * (1 + 3 * conf_16_f) * conf_11_f + fxp025 * fSqrt15 * valueP * conf_10_f * sinBetaSq - fxp0125 * fSqrt3_2 * valueK * (conf_11_f + 5 * conf_23_f);
		vN = fxp0125 * valueN * (5 * conf_10_f + 3 * conf_22_f) + fxp025 * fSqrt3_2 * valueP * (3 + conf_16_f) * conf_11_f + fxp050 * fSqrt15 * valueK * conf_10_f * sinBetaSq + fxp0125 * fSqrt5_2 * valueL * (conf_11_f - 3 * conf_23_f);
		vP = fxp00625 * valueP * (15 * conf_10_f + conf_22_f) - fxp025 * fSqrt3_2 * valueN * (3 + conf_16_f) * conf_11_f + fxp025 * fSqrt15 * valueL * conf_10_f * sinBetaSq - fxp050 * fSqrt5_2 * valueK * sinBetaCb;

		output[i + 9] = -vP * conf_25_f + vQ * conf_24_f;
		output[i + 10] = -vN * conf_19_f + vO * conf_18_f;
		output[i + 11] = -vL * conf_13_f + vM * conf_12_f;
		output[i + 12] = vK;
		output[i + 13] = vL * conf_12_f + vM * conf_13_f;
		output[i + 14] = vN * conf_18_f + vO * conf_19_f;
		output[i + 15] = vP * conf_24_f + vQ * conf_25_f;
	}

	for (i = 0; i < 16384; i += 16) {	// for the new test
		output[i] = audio_in[i];
	}

	// Print out the baseline for error percentage checking
	// for the new test
	printf("baseline = [\n");
	for (i = 0; i < 16384; ++i) {
		printf("%f, ", output[i]);
		if ((i + i) % 16 == 0) {
			printf("\n");
		}
	}
	printf("]\n");

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

	rotateOrder_sw();

	return errors;
}
