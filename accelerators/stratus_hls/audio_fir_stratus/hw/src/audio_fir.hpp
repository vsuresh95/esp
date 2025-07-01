// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __AUDIO_FIR_HPP__
#define __AUDIO_FIR_HPP__

#include "fpdata.hpp"
#include "audio_fir_directives.hpp"
#include "audio_fir_conf_info.hpp"
#include "audio_fir_debug_info.hpp"
#include "esp_templates.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/* <<--defines-->> */
#define DATA_WIDTH 32
#define DMA_SIZE SIZE_WORD

#define PLM_IN_WORD 2048
#define PLM_FLT_WORD 2050
#define PLM_TWD_WORD 1024

class audio_fir : public esp_accelerator_avu<DMA_WIDTH, N_INPUTS, N_OUTPUTS, N_CONTEXTS_BITS>
{
public:
    // Compute -> Load
    handshake_t load_ready;

    // Compute -> Store
    handshake_t store_ready;

    // Constructor
    SC_HAS_PROCESS(audio_fir);
    audio_fir(const sc_module_name& name)
    : esp_accelerator_avu<DMA_WIDTH, N_INPUTS, N_OUTPUTS, N_CONTEXTS_BITS>(name)
        , load_ready("load_ready")
        , store_ready("store_ready")
    {
        // Map arrays to memories
        /* <<--plm-bind-->> */
        HLS_MAP_plm(A0, PLM_IN_NAME);
        HLS_MAP_plm(F0, PLM_FLT_NAME);
        HLS_MAP_plm(T0, PLM_TW_NAME);
    }

    // Processes

    // Load the input data
    void load_input();

    // Computation
    void compute_kernel();

    // Store the output data
    void store_output();

    // Private local memories
    sc_dt::sc_int<DATA_WIDTH> A0[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> F0[PLM_FLT_WORD];
    sc_dt::sc_int<DATA_WIDTH> T0[PLM_TWD_WORD];
};


#endif /* __AUDIO_FIR_HPP__ */
