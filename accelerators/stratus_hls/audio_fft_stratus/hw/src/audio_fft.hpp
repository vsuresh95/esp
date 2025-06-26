// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __AUDIO_FFT_HPP__
#define __AUDIO_FFT_HPP__

#include "fpdata.hpp"
#include "audio_fft_directives.hpp"
#include "audio_fft_conf_info.hpp"
#include "audio_fft_debug_info.hpp"
#include "esp_templates.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/* <<--defines-->> */
#define DATA_WIDTH 32
#define DMA_SIZE SIZE_WORD

#define PLM_IN_WORD 2048
 
class audio_fft : public esp_accelerator_avu<DMA_WIDTH, N_INPUTS, N_OUTPUTS, N_CONTEXTS_BITS>
{
public:
    // Compute -> Load
    handshake_t load_ready;

    // Compute -> Store
    handshake_t store_ready;

    // Constructor
    SC_HAS_PROCESS(audio_fft);
    audio_fft(const sc_module_name& name)
    : esp_accelerator_avu<DMA_WIDTH, N_INPUTS, N_OUTPUTS, N_CONTEXTS_BITS>(name)
        , load_ready("load_ready")
        , store_ready("store_ready")
    {
        // Map arrays to memories
        /* <<--plm-bind-->> */
        HLS_MAP_plm(A0, PLM_IN_NAME);
    }

    // Processes

    // Load the input data
    void load_input();

    // Computation
    void compute_kernel();

    // Store the output data
    void store_output();

    // Functions
    void fft2_do_shift(unsigned int offset, unsigned int num_samples, unsigned int logn_samples);
    void fft2_bit_reverse(unsigned int offset, unsigned int n, unsigned int bits);

    // Private local memories
    sc_dt::sc_int<DATA_WIDTH> A0[PLM_IN_WORD];
};


#endif /* __AUDIO_FFT_HPP__ */
