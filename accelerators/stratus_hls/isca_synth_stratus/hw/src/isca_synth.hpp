// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __ISCA_SYNTH_HPP__
#define __ISCA_SYNTH_HPP__

#include "isca_synth_conf_info.hpp"
#include "isca_synth_debug_info.hpp"

#include "esp_templates.hpp"

#include "isca_synth_directives.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/* <<--defines-->> */
#define DATA_WIDTH 64
#define DMA_SIZE SIZE_DWORD
#define PLM_OUT_WORD 4096
#define PLM_IN_WORD 4096

class isca_synth : public esp_accelerator_3P<DMA_WIDTH>
{
public:
    // Constructor
    SC_HAS_PROCESS(isca_synth);
    isca_synth(const sc_module_name& name)
    : esp_accelerator_3P<DMA_WIDTH>(name)
        , cfg("config")
    {
        // Signal binding
        cfg.bind_with(*this);

        // Map arrays to memories
        /* <<--plm-bind-->> */
        HLS_MAP_plm(plm_out_pong, PLM_OUT_NAME);
        HLS_MAP_plm(plm_out_ping, PLM_OUT_NAME);
        HLS_MAP_plm(plm_in_pong, PLM_IN_NAME);
        HLS_MAP_plm(plm_in_ping, PLM_IN_NAME);
    }

    // Processes

    // Load the input data
    void load_input();

    // Computation
    void compute_kernel();

    // Store the output data
    void store_output();

    // Configure isca_synth
    esp_config_proc cfg;

    // Functions

    // Private local memories
    sc_dt::sc_int<DATA_WIDTH> plm_in_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_in_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_out_ping[PLM_OUT_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_out_pong[PLM_OUT_WORD];

};


#endif /* __ISCA_SYNTH_HPP__ */
