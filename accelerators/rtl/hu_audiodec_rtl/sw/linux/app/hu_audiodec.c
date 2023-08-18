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

const float ERR_TH = 0.05;

void rotate_order1_sw(float *gold)
{
	float temp_sample[NUM_CHANNELS];
	unsigned sample_length = BLOCK_SIZE;

    for(unsigned i = 0; i < sample_length; i++)
    {
		// Alpha rotation
        temp_sample[kY] = -gold[kX*sample_length+i] * m_fSinAlpha + gold[kY*sample_length+i] * m_fCosAlpha;
        temp_sample[kZ] = gold[kZ*sample_length+i];
        temp_sample[kX] = gold[kX*sample_length+i] * m_fCosAlpha + gold[kY*sample_length+i] * m_fSinAlpha;

        // Beta rotation
        gold[kY*sample_length+i] = temp_sample[kY];
        gold[kZ*sample_length+i] = temp_sample[kZ] * m_fCosBeta +  temp_sample[kX] * m_fSinBeta;
        gold[kX*sample_length+i] = temp_sample[kX] * m_fCosBeta - temp_sample[kZ] * m_fSinBeta;

        // Gamma rotation
        temp_sample[kY] = -gold[kX*sample_length+i] * m_fSinGamma + gold[kY*sample_length+i] * m_fCosGamma;
        temp_sample[kZ] = gold[kZ*sample_length+i];
        temp_sample[kX] = gold[kX*sample_length+i] * m_fCosGamma + gold[kY*sample_length+i] * m_fSinGamma;

        gold[kX*sample_length+i] = temp_sample[kX];
        gold[kY*sample_length+i] = temp_sample[kY];
        gold[kZ*sample_length+i] = temp_sample[kZ];
    }
}

void rotate_order2_sw(float *gold)
{
	float temp_sample[NUM_CHANNELS];
	unsigned sample_length = BLOCK_SIZE;

    float fSqrt3 = sqrt(3.f);

    for(unsigned i = 0; i < sample_length; i++)
    {
        // Alpha rotation
        temp_sample[kV] = - gold[kU*sample_length+i] * m_fSin2Alpha + gold[kV*sample_length+i] * m_fCos2Alpha;
        temp_sample[kT] = - gold[kS*sample_length+i] * m_fSinAlpha + gold[kT*sample_length+i] * m_fCosAlpha;
        temp_sample[kR] = gold[kR*sample_length+i];
        temp_sample[kS] = gold[kS*sample_length+i] * m_fCosAlpha + gold[kT*sample_length+i] * m_fSinAlpha;
        temp_sample[kU] = gold[kU*sample_length+i] * m_fCos2Alpha + gold[kV*sample_length+i] * m_fSin2Alpha;

        // Beta rotation
        gold[kV*sample_length+i] = -m_fSinBeta * temp_sample[kT] + m_fCosBeta * temp_sample[kV];
        gold[kT*sample_length+i] = -m_fCosBeta * temp_sample[kT] + m_fSinBeta * temp_sample[kV];
        gold[kR*sample_length+i] = (0.75f * m_fCos2Beta + 0.25f) * temp_sample[kR]
                            + (0.5 * fSqrt3 * pow(m_fSinBeta,2.0) ) * temp_sample[kU]
                            + (fSqrt3 * m_fSinBeta * m_fCosBeta) * temp_sample[kS];
        gold[kS*sample_length+i] = m_fCos2Beta * temp_sample[kS]
                            - fSqrt3 * m_fCosBeta * m_fSinBeta * temp_sample[kR]
                            + m_fCosBeta * m_fSinBeta * temp_sample[kU];
        gold[kU*sample_length+i] = (0.25f * m_fCos2Beta + 0.75f) * temp_sample[kU]
                            - m_fCosBeta * m_fSinBeta * temp_sample[kS]
                            +0.5 * fSqrt3 * pow(m_fSinBeta,2.0) * temp_sample[kR];

        // Gamma rotation
        temp_sample[kV] = - gold[kU*sample_length+i] * m_fSin2Gamma + gold[kV*sample_length+i] * m_fCos2Gamma;
        temp_sample[kT] = - gold[kS*sample_length+i] * m_fSinGamma + gold[kT*sample_length+i] * m_fCosGamma;

        temp_sample[kR] = gold[kR*sample_length+i];
        temp_sample[kS] = gold[kS*sample_length+i] * m_fCosGamma + gold[kT*sample_length+i] * m_fSinGamma;
        temp_sample[kU] = gold[kU*sample_length+i] * m_fCos2Gamma + gold[kV*sample_length+i] * m_fSin2Gamma;

        gold[kR*sample_length+i] = temp_sample[kR];
        gold[kS*sample_length+i] = temp_sample[kS];
        gold[kT*sample_length+i] = temp_sample[kT];
        gold[kU*sample_length+i] = temp_sample[kU];
        gold[kV*sample_length+i] = temp_sample[kV];
    }
}

void rotate_order3_sw(float *gold)
{
	float temp_sample[NUM_CHANNELS];
	unsigned sample_length = BLOCK_SIZE;

	/* (should move these somewhere that does recompute each time) */
	float fSqrt3_2 = sqrt(3.f/2.f);
	float fSqrt15 = sqrt(15.f);
	float fSqrt5_2 = sqrt(5.f/2.f);

    for(unsigned i = 0; i < sample_length; i++)
    {
        // Alpha rotation
        temp_sample[kQ] = - gold[kP*sample_length+i] * m_fSin3Alpha + gold[kQ*sample_length+i] * m_fCos3Alpha;
        temp_sample[kO] = - gold[kN*sample_length+i] * m_fSin2Alpha + gold[kO*sample_length+i] * m_fCos2Alpha;
        temp_sample[kM] = - gold[kL*sample_length+i] * m_fSinAlpha + gold[kM*sample_length+i] * m_fCosAlpha;
        temp_sample[kK] = gold[kK*sample_length+i];
        temp_sample[kL] = gold[kL*sample_length+i] * m_fCosAlpha + gold[kM*sample_length+i] * m_fSinAlpha;
        temp_sample[kN] = gold[kN*sample_length+i] * m_fCos2Alpha + gold[kO*sample_length+i] * m_fSin2Alpha;
        temp_sample[kP] = gold[kP*sample_length+i] * m_fCos3Alpha + gold[kQ*sample_length+i] * m_fSin3Alpha;

        // Beta rotation
        gold[kQ*sample_length+i] = 0.125f * temp_sample[kQ] * (5.f + 3.f*m_fCos2Beta)
                    - fSqrt3_2 * temp_sample[kO] *m_fCosBeta * m_fSinBeta
                    + 0.25f * fSqrt15 * temp_sample[kM] * pow(m_fSinBeta,2.0f);
        gold[kO*sample_length+i] = temp_sample[kO] * m_fCos2Beta
                    - fSqrt5_2 * temp_sample[kM] * m_fCosBeta * m_fSinBeta
                    + fSqrt3_2 * temp_sample[kQ] * m_fCosBeta * m_fSinBeta;
        gold[kM*sample_length+i] = 0.125f * temp_sample[kM] * (3.f + 5.f*m_fCos2Beta)
                    - fSqrt5_2 * temp_sample[kO] *m_fCosBeta * m_fSinBeta
                    + 0.25f * fSqrt15 * temp_sample[kQ] * pow(m_fSinBeta,2.0f);
        gold[kK*sample_length+i] = 0.25f * temp_sample[kK] * m_fCosBeta * (-1.f + 15.f*m_fCos2Beta)
                    + 0.5f * fSqrt15 * temp_sample[kN] * m_fCosBeta * pow(m_fSinBeta,2.f)
                    + 0.5f * fSqrt5_2 * temp_sample[kP] * pow(m_fSinBeta,3.f)
                    + 0.125f * fSqrt3_2 * temp_sample[kL] * (m_fSinBeta + 5.f * m_fSin3Beta);
        gold[kL*sample_length+i] = 0.0625f * temp_sample[kL] * (m_fCosBeta + 15.f * m_fCos3Beta)
                    + 0.25f * fSqrt5_2 * temp_sample[kN] * (1.f + 3.f * m_fCos2Beta) * m_fSinBeta
                    + 0.25f * fSqrt15 * temp_sample[kP] * m_fCosBeta * pow(m_fSinBeta,2.f)
                    - 0.125 * fSqrt3_2 * temp_sample[kK] * (m_fSinBeta + 5.f * m_fSin3Beta);
        gold[kN*sample_length+i] = 0.125f * temp_sample[kN] * (5.f * m_fCosBeta + 3.f * m_fCos3Beta)
                    + 0.25f * fSqrt3_2 * temp_sample[kP] * (3.f + m_fCos2Beta) * m_fSinBeta
                    + 0.5f * fSqrt15 * temp_sample[kK] * m_fCosBeta * pow(m_fSinBeta,2.f)
                    + 0.125 * fSqrt5_2 * temp_sample[kL] * (m_fSinBeta - 3.f * m_fSin3Beta);
        gold[kP*sample_length+i] = 0.0625f * temp_sample[kP] * (15.f * m_fCosBeta + m_fCos3Beta)
                    - 0.25f * fSqrt3_2 * temp_sample[kN] * (3.f + m_fCos2Beta) * m_fSinBeta
                    + 0.25f * fSqrt15 * temp_sample[kL] * m_fCosBeta * pow(m_fSinBeta,2.f)
                    - 0.5 * fSqrt5_2 * temp_sample[kK] * pow(m_fSinBeta,3.f);

        // Gamma rotation
        temp_sample[kQ] = - gold[kP*sample_length+i] * m_fSin3Gamma + gold[kQ*sample_length+i] * m_fCos3Gamma;
        temp_sample[kO] = - gold[kN*sample_length+i] * m_fSin2Gamma + gold[kO*sample_length+i] * m_fCos2Gamma;
        temp_sample[kM] = - gold[kL*sample_length+i] * m_fSinGamma + gold[kM*sample_length+i] * m_fCosGamma;
        temp_sample[kK] = gold[kK*sample_length+i];
        temp_sample[kL] = gold[kL*sample_length+i] * m_fCosGamma + gold[kM*sample_length+i] * m_fSinGamma;
        temp_sample[kN] = gold[kN*sample_length+i] * m_fCos2Gamma + gold[kO*sample_length+i] * m_fSin2Gamma;
        temp_sample[kP] = gold[kP*sample_length+i] * m_fCos3Gamma + gold[kQ*sample_length+i] * m_fSin3Gamma;

        gold[kQ*sample_length+i] = temp_sample[kQ];
        gold[kO*sample_length+i] = temp_sample[kO];
        gold[kM*sample_length+i] = temp_sample[kM];
        gold[kK*sample_length+i] = temp_sample[kK];
        gold[kL*sample_length+i] = temp_sample[kL];
        gold[kN*sample_length+i] = temp_sample[kN];
        gold[kP*sample_length+i] = temp_sample[kP];
    }
}

/* User-defined code */
static int validate_buffer(token_t *out, float *gold)
{
	unsigned init_length = BLOCK_SIZE;
	unsigned init_channel = NUM_CHANNELS;
	unsigned errors = 0;

	// Copying input data to accelerator buffer - transposed
	for (unsigned i = 0; i < init_length; i++) {
		for (unsigned j = 0; j < init_channel; j++) {
			if (i != 0) {
				native_t val = fixed32_to_float(out[i*init_channel + j], FX_IL);

				if ((fabs(gold[j*init_length + i] - val) / fabs(gold[j*init_length + i])) > ERR_TH) {
					if (errors < 2) {
						printf(" GOLD[%u] = %f vs %f = out[%u]\n", j*init_length + i, gold[j*init_length + i], val, j*init_length + i);
					}
					errors++;
				}
			}
		}
	}

	return errors;
}

/* User-defined code */
static void init_buffer(token_t *in, float *gold)
{
	unsigned init_length = BLOCK_SIZE;
	unsigned init_channel = NUM_CHANNELS;
	const float LO = -2.0;
	const float HI = 2.0;

	srand((unsigned int) time(NULL));

	float scaling_factor;

	// Initializing random input data for test
	for (unsigned i = 0; i < init_channel; i++) {
		for (unsigned j = 0; j < init_length; j++) {
			scaling_factor = (float) rand () / (float) RAND_MAX;
			gold[i*init_length + j] = LO + scaling_factor * (HI - LO);
		}
	}

	scaling_factor = (float) rand () / (float) RAND_MAX;
	m_fCosAlpha = LO + scaling_factor * (HI - LO);
	scaling_factor = (float) rand () / (float) RAND_MAX;
	m_fSinAlpha = LO + scaling_factor * (HI - LO);
	scaling_factor = (float) rand () / (float) RAND_MAX;
	m_fCosBeta = LO + scaling_factor * (HI - LO);
	scaling_factor = (float) rand () / (float) RAND_MAX;
	m_fSinBeta = LO + scaling_factor * (HI - LO);
	scaling_factor = (float) rand () / (float) RAND_MAX;
	m_fCosGamma = LO + scaling_factor * (HI - LO);
	scaling_factor = (float) rand () / (float) RAND_MAX;
	m_fSinGamma = LO + scaling_factor * (HI - LO);
	scaling_factor = (float) rand () / (float) RAND_MAX;
	m_fCos2Alpha = LO + scaling_factor * (HI - LO);
	scaling_factor = (float) rand () / (float) RAND_MAX;
	m_fSin2Alpha = LO + scaling_factor * (HI - LO);
	scaling_factor = (float) rand () / (float) RAND_MAX;
	m_fCos2Beta = LO + scaling_factor * (HI - LO);
	scaling_factor = (float) rand () / (float) RAND_MAX;
	m_fSin2Beta = LO + scaling_factor * (HI - LO);
	scaling_factor = (float) rand () / (float) RAND_MAX;
	m_fCos2Gamma = LO + scaling_factor * (HI - LO);
	scaling_factor = (float) rand () / (float) RAND_MAX;
	m_fSin2Gamma = LO + scaling_factor * (HI - LO);
	scaling_factor = (float) rand () / (float) RAND_MAX;
	m_fCos3Alpha = LO + scaling_factor * (HI - LO);
	scaling_factor = (float) rand () / (float) RAND_MAX;
	m_fSin3Alpha = LO + scaling_factor * (HI - LO);
	scaling_factor = (float) rand () / (float) RAND_MAX;
	m_fCos3Beta = LO + scaling_factor * (HI - LO);
	scaling_factor = (float) rand () / (float) RAND_MAX;
	m_fSin3Beta = LO + scaling_factor * (HI - LO);
	scaling_factor = (float) rand () / (float) RAND_MAX;
	m_fCos3Gamma = LO + scaling_factor * (HI - LO);
	scaling_factor = (float) rand () / (float) RAND_MAX;
	m_fSin3Gamma = LO + scaling_factor * (HI - LO);

	// Copying input data to accelerator buffer - transposed
	for (unsigned i = 0; i < init_length; i++) {
		for (unsigned j = 0; j < init_channel; j++) {
			if (i != 0) {
				in[i*init_channel + j] = float_to_fixed32(gold[j*init_length + i], FX_IL);
			}
		}
	}

	struct timespec th_start;
	struct timespec th_end;

	gettime(&th_start);
	rotate_order1_sw(gold);
	rotate_order2_sw(gold);
	rotate_order3_sw(gold);
	gettime(&th_end);

	printf("  > Software time: %llu ns\n", ts_subtract(&th_start, &th_end));
}


/* User-defined code */
static void init_parameters()
{
	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
		in_words_adj = BLOCK_SIZE * NUM_CHANNELS;
		out_words_adj = BLOCK_SIZE * NUM_CHANNELS;
	} else {
		in_words_adj = round_up(BLOCK_SIZE * NUM_CHANNELS, DMA_WORD_PER_BEAT(sizeof(token_t)));
		out_words_adj = round_up(BLOCK_SIZE * NUM_CHANNELS, DMA_WORD_PER_BEAT(sizeof(token_t)));
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

	float *gold;
	token_t *buf;

	init_parameters();

	buf = (token_t *) esp_alloc(size);
	cfg_000[0].hw_buf = buf;
    
	gold = malloc(out_size);

	init_buffer(buf, gold);

	printf("\n====== %s ======\n\n", cfg_000[0].devname);
	printf("\n  ** START **\n");

	hu_audiodec_cfg_000[0].src_offset = 0;
	hu_audiodec_cfg_000[0].dst_offset = (NUM_CHANNELS * BLOCK_SIZE) * sizeof(token_t);
	hu_audiodec_cfg_000[0].cfg_regs_2 = BLOCK_SIZE;
	hu_audiodec_cfg_000[0].cfg_regs_8   = float_to_fixed32(m_fCosAlpha, FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_9   = float_to_fixed32(m_fSinAlpha, FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_10  = float_to_fixed32(m_fCosBeta, FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_11  = float_to_fixed32(m_fSinBeta, FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_12  = float_to_fixed32(m_fCosGamma, FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_13  = float_to_fixed32(m_fSinGamma, FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_14  = float_to_fixed32(m_fCos2Alpha, FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_15  = float_to_fixed32(m_fSin2Alpha, FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_16  = float_to_fixed32(m_fCos2Beta, FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_17  = float_to_fixed32(m_fSin2Beta, FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_18  = float_to_fixed32(m_fCos2Gamma, FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_19  = float_to_fixed32(m_fSin2Gamma, FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_20  = float_to_fixed32(m_fCos3Alpha, FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_21  = float_to_fixed32(m_fSin3Alpha, FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_22  = float_to_fixed32(m_fCos3Beta, FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_23  = float_to_fixed32(m_fSin3Beta, FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_24  = float_to_fixed32(m_fCos3Gamma, FX_IL);
	hu_audiodec_cfg_000[0].cfg_regs_25  = float_to_fixed32(m_fSin3Gamma, FX_IL);

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
