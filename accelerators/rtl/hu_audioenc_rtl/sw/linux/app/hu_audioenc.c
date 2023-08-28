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

unsigned m_nDelayBufferLength;
float m_fDelay;
int m_nDelay;
int m_nIn;
int m_nOutA;
int m_nOutB;
float m_fInteriorGain;
float m_fExteriorGain;

void audio_enc_sw(token_in_t *gold_in, float *gold_out)
{
	unsigned sample_length = BLOCK_SIZE;
	unsigned sample_channels = NUM_CHANNELS;
	float SAMPLE_DIV = (2 << 14) - 1;
    float fSrcSample = 0;

	for (unsigned i = 0; i < sample_length; i++) {
        fSrcSample = (float) gold_in[i];
        fSrcSample /= SAMPLE_DIV;
        fSrcSample *= cfg_src_coeff[0];

        gold_out[kW*sample_length+i] = fSrcSample * m_fInteriorGain * cfg_chan_coeff[kW];

        fSrcSample *= m_fExteriorGain;
        for(unsigned j = 1; j < sample_channels; j++)
        {
            gold_out[j*sample_length+i] = fSrcSample * cfg_chan_coeff[j];
        }
    }
}

/* User-defined code */
static int validate_buffer(token_out_t *out, float *gold_out)
{
	unsigned init_length = BLOCK_SIZE;
	unsigned init_channel = NUM_CHANNELS;
	unsigned errors = 0;

	// Copying input data to accelerator buffer - transposed
	for (unsigned i = 0; i < init_length; i++) {
		for (unsigned j = 0; j < init_channel; j++) {
			native_t val = fixed32_to_float(out[i*init_channel + j], FX_IL);

			if ((fabs(gold_out[j*init_length + i] - val) / fabs(gold_out[j*init_length + i])) > ERR_TH) {
				if (errors < 2) {
					printf(" GOLD_OUT[%u] = %f vs %f = out[%u]\n", j*init_length + i, gold_out[j*init_length + i], val, j*init_length + i);
				}
				errors++;
			}
		}
	}

	return errors;
}

/* User-defined code */
static void init_buffer(token_in_t *in, token_in_t *gold_in, float* gold_out)
{
	unsigned init_length = BLOCK_SIZE;
	unsigned init_channel = NUM_CHANNELS;
	unsigned init_sources = NUM_SRCS;
	const float LO_SHORT = -32768.0;
	const float HI_SHORT = 32768.0;
	const float LO = -2.0;
	const float HI = 2.0;

	srand((unsigned int) time(NULL));

	float scaling_factor;

	// Initializing random input data for test
	for (unsigned i = 0; i < init_length; i++) {
		scaling_factor = (float) (rand () % 1024) / (float) 1024;
		gold_in[i] = (token_in_t) (LO_SHORT + scaling_factor * (HI_SHORT - LO_SHORT));
	}

	scaling_factor = (float) (rand () % 1024) / (float) 1024;
	m_fInteriorGain = LO + scaling_factor * (HI - LO);
	scaling_factor = (float) (rand () % 1024) / (float) 1024;
	m_fExteriorGain = LO + scaling_factor * (HI - LO);

	for (unsigned i = 0; i < init_sources; i++) {
		scaling_factor = (float) (rand () % 1024) / (float) 1024;
		cfg_src_coeff[i] = LO + scaling_factor * (HI - LO);
	}

	for (unsigned i = 0; i < init_channel; i++) {
		scaling_factor = (float) (rand () % 1024) / (float) 1024;
		cfg_chan_coeff[i] = LO + scaling_factor * (HI - LO);
	}

	struct timespec th_start;
	struct timespec th_end;

	// Copying input data to accelerator buffer - transposed
	for (unsigned i = 0; i < init_length; i++) {
		in[i] = gold_in[i];
	}

	gettime(&th_start);
	audio_enc_sw(gold_in, gold_out);
	gettime(&th_end);

	printf("  > Software time: %llu ns\n", ts_subtract(&th_start, &th_end));
}


/* User-defined code */
static void init_parameters()
{
	if (DMA_WORD_PER_BEAT(sizeof(token_in_t)) == 0) {
		in_words_adj = BLOCK_SIZE;
	} else {
		in_words_adj = round_up(BLOCK_SIZE, DMA_WORD_PER_BEAT(sizeof(token_in_t)));
	}
	if (DMA_WORD_PER_BEAT(sizeof(token_out_t)) == 0) {
		out_words_adj = BLOCK_SIZE * NUM_CHANNELS;
	} else {
		out_words_adj = round_up(BLOCK_SIZE * NUM_CHANNELS, DMA_WORD_PER_BEAT(sizeof(token_out_t)));
	}

	in_len = in_words_adj * (1);
	out_len =  out_words_adj * (1);
	in_size = in_len * sizeof(token_in_t);
	out_size = out_len * sizeof(token_out_t);
	out_offset = in_len;
	size = (out_offset * sizeof(token_in_t)) + out_size;
	m_nDelayBufferLength = (unsigned)((float) 150 * 48000 / 344 + 0.5f);
}


int main(int argc, char **argv)
{
	int errors;

	token_in_t *gold_in;
	float *gold_out;
	token_out_t *buf;

	init_parameters();

	buf = (token_out_t *) esp_alloc(size);
	cfg_000[0].hw_buf = buf;
    
	gold_in = malloc(in_size);
	gold_out = malloc(out_size);
	
	init_buffer((token_in_t *) buf, gold_in, gold_out);

	printf("\n====== %s ======\n\n", cfg_000[0].devname);
	printf("\n  ** START **\n");

	hu_audioenc_cfg_000[0].cfg_regs_16 = float_to_fixed32(cfg_src_coeff[0], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_32 = float_to_fixed32(m_fInteriorGain * cfg_chan_coeff[0], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_33 = float_to_fixed32(m_fExteriorGain * cfg_chan_coeff[1], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_34 = float_to_fixed32(m_fExteriorGain * cfg_chan_coeff[2], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_35 = float_to_fixed32(m_fExteriorGain * cfg_chan_coeff[3], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_36 = float_to_fixed32(m_fExteriorGain * cfg_chan_coeff[4], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_37 = float_to_fixed32(m_fExteriorGain * cfg_chan_coeff[5], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_38 = float_to_fixed32(m_fExteriorGain * cfg_chan_coeff[6], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_39 = float_to_fixed32(m_fExteriorGain * cfg_chan_coeff[7], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_40 = float_to_fixed32(m_fExteriorGain * cfg_chan_coeff[8], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_41 = float_to_fixed32(m_fExteriorGain * cfg_chan_coeff[9], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_42 = float_to_fixed32(m_fExteriorGain * cfg_chan_coeff[10], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_43 = float_to_fixed32(m_fExteriorGain * cfg_chan_coeff[11], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_44 = float_to_fixed32(m_fExteriorGain * cfg_chan_coeff[12], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_45 = float_to_fixed32(m_fExteriorGain * cfg_chan_coeff[13], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_46 = float_to_fixed32(m_fExteriorGain * cfg_chan_coeff[14], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_47 = float_to_fixed32(m_fExteriorGain * cfg_chan_coeff[15], FX_IL);

	hu_audioenc_cfg_000[0].src_offset = 0;
	hu_audioenc_cfg_000[0].dst_offset = BLOCK_SIZE * sizeof(token_in_t);

	esp_run(cfg_000, NACC);

	printf("\n  ** DONE **\n");

	errors = validate_buffer(&buf[out_offset/2], gold_out);

	free(gold_in);
	free(gold_out);
	esp_free(buf);

	if (!errors)
		printf("+ Test PASSED\n");
	else
		printf("+ Test FAILED, ERRORS = %d\n", errors);

	printf("\n====== %s ======\n\n", cfg_000[0].devname);

	return errors;
}
