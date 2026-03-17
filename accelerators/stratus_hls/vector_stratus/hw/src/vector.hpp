// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __VECTOR_HPP__
#define __VECTOR_HPP__

#include "fpdata.hpp"
#include "vector_conf_info.hpp"
#include "vector_debug_info.hpp"

#include "esp_templates.hpp"

#include "vector_directives.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/* <<--defines-->> */
#define DATA_WIDTH 32
#define DMA_SIZE SIZE_WORD
#define PLM_OUT_WORD 2048
#define PLM_IN_WORD 2048

class vector : public esp_accelerator_3P<DMA_WIDTH>
{
public:
    // Constructor
    SC_HAS_PROCESS(vector);
    vector(const sc_module_name& name)
    : esp_accelerator_3P<DMA_WIDTH>(name)
    {
        // Map arrays to memories
        /* <<--plm-bind-->> */
        HLS_MAP_plm(plm_out_pong, PLM_OUT_NAME);
        HLS_MAP_plm(plm_out_ping, PLM_OUT_NAME);
        HLS_MAP_plm(plm_in1_pong, PLM_IN_NAME);
        HLS_MAP_plm(plm_in1_ping, PLM_IN_NAME);
        HLS_MAP_plm(plm_in2_pong, PLM_IN_NAME);
        HLS_MAP_plm(plm_in2_ping, PLM_IN_NAME);
    }

    // Processes

    // Load the input data
    void load_input();

    // Computation
    void compute_kernel();

    // Store the output data
    void store_output();

    // Functions

    // Private local memories
    sc_dt::sc_int<DATA_WIDTH> plm_in1_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_in1_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_in2_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_in2_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_out_ping[PLM_OUT_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_out_pong[PLM_OUT_WORD];
};


#endif /* __VECTOR_HPP__ */
