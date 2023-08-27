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

float *workaround_in, *delay_buffer;

void audio_enc_sw(float *gold_in, float *gold_out, float *delay_buffer)
{
	unsigned sample_length = BLOCK_SIZE;
	unsigned sample_channels = NUM_CHANNELS;
	float SAMPLE_DIV = (2 << 14) - 1;
	float amp = 1.0f;
    float fSrcSample = 0;

    for (unsigned i = 0; i < sample_length; i++) {
        gold_out[i] = amp * (gold_in[i] / SAMPLE_DIV);

		//Store
		delay_buffer[m_nIn] = gold_out[i];

		//Read
		fSrcSample = delay_buffer[m_nOutA] * (1.f - m_fDelay)
					+ delay_buffer[m_nOutB] * m_fDelay;
					
        gold_out[kW*sample_length+i] = fSrcSample * cfg_src_coeff[kW] * cfg_chan_coeff[kW];

        for(unsigned j = 1; j < sample_channels; j++)
        {
            gold_out[j*sample_length+i] = fSrcSample * cfg_src_coeff[j] * cfg_chan_coeff[j];
        }

        m_nIn = (m_nIn + 1) % m_nDelayBufferLength;
        m_nOutA = (m_nOutA + 1) % m_nDelayBufferLength;
        m_nOutB = (m_nOutB + 1) % m_nDelayBufferLength;
    }
}

void audio_sw_workaround(float *gold_in, float *workaround_in, float *delay_buffer)
{
	float SAMPLE_DIV = (2 << 14) - 1;
	float amp = 1.0f;

    for (unsigned i = 0; i < BLOCK_SIZE; i++) {
		//Store
		delay_buffer[m_nIn] = gold_in[i];

		//Read
		workaround_in[i] = delay_buffer[m_nOutA] * (1.f - m_fDelay)
					+ delay_buffer[m_nOutB] * m_fDelay;

        m_nIn = (m_nIn + 1) % m_nDelayBufferLength;
        m_nOutA = (m_nOutA + 1) % m_nDelayBufferLength;
        m_nOutB = (m_nOutB + 1) % m_nDelayBufferLength;

        workaround_in[i] = amp * (workaround_in[i] * SAMPLE_DIV);
    }
}

/* User-defined code */
static int validate_buffer(token_t *out, float *gold_out)
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
static void init_buffer(token_t *in, float *gold_in, float* gold_out)
{
	unsigned init_length = BLOCK_SIZE;
	unsigned init_channel = NUM_CHANNELS;
	const float LO = -2.0;
	const float HI = 2.0;

	srand((unsigned int) time(NULL));

	float scaling_factor;

	// Initializing random input data for test
	for (unsigned i = 0; i < init_length; i++) {
		scaling_factor = (float) rand () / (float) RAND_MAX;
		gold_in[i] = LO + scaling_factor * (HI - LO);
	}

	for (unsigned i = 0; i < init_channel; i++) {
		scaling_factor = (float) rand () / (float) RAND_MAX;
		cfg_src_coeff[i] = LO + scaling_factor * (HI - LO);
		scaling_factor = (float) rand () / (float) RAND_MAX;
		cfg_chan_coeff[i] = LO + scaling_factor * (HI - LO);
	}

	// SW workaround parameters
    m_fDelay = (rand () / RAND_MAX) / 344 * 48000 + 0.5f;
    m_nDelay = (int) m_fDelay;
    m_fDelay -= m_nDelay;
    m_nIn = 0;
    m_nOutA = (m_nIn - m_nDelay + m_nDelayBufferLength) % m_nDelayBufferLength;
    m_nOutB = (m_nOutA + 1) % m_nDelayBufferLength;

	for (unsigned i = 0; i < m_nDelayBufferLength; i++) {
		delay_buffer[i] = 0;
	}

	struct timespec th_start;
	struct timespec th_end;

	gettime(&th_start);
	audio_sw_workaround(gold_in, workaround_in, delay_buffer);
	gettime(&th_end);

	printf("  > Software workaround time: %llu ns\n", ts_subtract(&th_start, &th_end));

	// Copying input data to accelerator buffer - transposed
	for (unsigned j = 0; j < init_channel; j++) {
		for (unsigned i = 0; i < init_length; i++) {
			in[j*init_length+i] = float_to_fixed32(workaround_in[i], FX_IL);
		}
	}
	
    m_fDelay = (rand () / RAND_MAX) / 344 * 48000 + 0.5f;
    m_nDelay = (int) m_fDelay;
    m_fDelay -= m_nDelay;
    m_nIn = 0;
    m_nOutA = (m_nIn - m_nDelay + m_nDelayBufferLength) % m_nDelayBufferLength;
    m_nOutB = (m_nOutA + 1) % m_nDelayBufferLength;

	for (unsigned i = 0; i < m_nDelayBufferLength; i++) {
		delay_buffer[i] = 0;
	}

	gettime(&th_start);
	audio_enc_sw(gold_in, gold_out, delay_buffer);
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
	m_nDelayBufferLength = (unsigned)((float) 150 * 48000 / 344 + 0.5f);
}


int main(int argc, char **argv)
{
	int errors;

	float *gold_in;
	float *gold_out;
	token_t *buf;

	init_parameters();

	buf = (token_t *) esp_alloc(size);
	cfg_000[0].hw_buf = buf;
    
	gold_in = malloc(in_size);
	gold_out = malloc(out_size);

	workaround_in = malloc(in_size);
	delay_buffer = malloc(m_nDelayBufferLength * sizeof(native_t));
	
	init_buffer(buf, gold_in, gold_out);

	free(gold_in);

	printf("\n====== %s ======\n\n", cfg_000[0].devname);
	printf("\n  ** START **\n");

	hu_audioenc_cfg_000[0].cfg_regs_16 = float_to_fixed32(cfg_src_coeff[0], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_17 = float_to_fixed32(cfg_src_coeff[1], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_18 = float_to_fixed32(cfg_src_coeff[2], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_19 = float_to_fixed32(cfg_src_coeff[3], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_20 = float_to_fixed32(cfg_src_coeff[4], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_21 = float_to_fixed32(cfg_src_coeff[5], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_22 = float_to_fixed32(cfg_src_coeff[6], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_23 = float_to_fixed32(cfg_src_coeff[7], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_24 = float_to_fixed32(cfg_src_coeff[8], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_25 = float_to_fixed32(cfg_src_coeff[9], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_26 = float_to_fixed32(cfg_src_coeff[10], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_27 = float_to_fixed32(cfg_src_coeff[11], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_28 = float_to_fixed32(cfg_src_coeff[12], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_29 = float_to_fixed32(cfg_src_coeff[13], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_30 = float_to_fixed32(cfg_src_coeff[14], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_31 = float_to_fixed32(cfg_src_coeff[15], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_32 = float_to_fixed32(cfg_chan_coeff[0], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_33 = float_to_fixed32(cfg_chan_coeff[1], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_34 = float_to_fixed32(cfg_chan_coeff[2], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_35 = float_to_fixed32(cfg_chan_coeff[3], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_36 = float_to_fixed32(cfg_chan_coeff[4], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_37 = float_to_fixed32(cfg_chan_coeff[5], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_38 = float_to_fixed32(cfg_chan_coeff[6], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_39 = float_to_fixed32(cfg_chan_coeff[7], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_40 = float_to_fixed32(cfg_chan_coeff[8], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_41 = float_to_fixed32(cfg_chan_coeff[9], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_42 = float_to_fixed32(cfg_chan_coeff[10], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_43 = float_to_fixed32(cfg_chan_coeff[11], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_44 = float_to_fixed32(cfg_chan_coeff[12], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_45 = float_to_fixed32(cfg_chan_coeff[13], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_46 = float_to_fixed32(cfg_chan_coeff[14], FX_IL);
	hu_audioenc_cfg_000[0].cfg_regs_47 = float_to_fixed32(cfg_chan_coeff[15], FX_IL);

	hu_audioenc_cfg_000[0].src_offset = 0;
	hu_audioenc_cfg_000[0].dst_offset = (NUM_CHANNELS * BLOCK_SIZE) * sizeof(token_t);

	esp_run(cfg_000, NACC);

	printf("\n  ** DONE **\n");

	errors = validate_buffer(&buf[out_offset], gold_out);

	free(gold_out);
	esp_free(buf);

	free(workaround_in);
	free(delay_buffer);

	if (!errors)
		printf("+ Test PASSED\n");
	else
		printf("+ Test FAILED\n");

	printf("\n====== %s ======\n\n", cfg_000[0].devname);

	return errors;
}
