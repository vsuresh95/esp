// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __GEMM_5_2D_BLOCK_HPP__
#define __GEMM_5_2D_BLOCK_HPP__

#include "gemm_5_2d_block_conf_info.hpp"
#include "gemm_5_2d_block_debug_info.hpp"

#include "esp_templates.hpp"

#include "gemm_5_2d_block_directives.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/* <<--defines-->> */
#define DATA_WIDTH 32
#define DMA_SIZE SIZE_WORD
#define PLM_OUT_WORD 4096
#define PLM_IN_WORD 8192
#define BLOCK_SIZE 64
#define NUM_GEMM 4
#define ACC_REGION (BLOCK_SIZE/NUM_GEMM)

class gemm_5_2d_block : public esp_accelerator_3P<DMA_WIDTH>
{
public:
    // Constructor
    SC_HAS_PROCESS(gemm_5_2d_block);
    gemm_5_2d_block(const sc_module_name& name)
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

    // Configure gemm_5_2d_block
    esp_config_proc cfg;

    // Functions

    // Private local memories
    sc_dt::sc_int<DATA_WIDTH> plm_in_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_in_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_out_ping[PLM_OUT_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_out_pong[PLM_OUT_WORD];

};


#endif /* __GEMM_5_2D_BLOCK_HPP__ */
