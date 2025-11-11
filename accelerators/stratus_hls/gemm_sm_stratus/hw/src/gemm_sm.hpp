// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __GEMM_SM_HPP__
#define __GEMM_SM_HPP__

#include "gemm_sm_directives.hpp"
#include "gemm_sm_info.hpp"
#include "gemm_sm_debug_info.hpp"
#include "esp_templates.hpp"
#include "fpdata.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)

/* <<--defines-->> */
#define DATA_WIDTH 32
#define DMA_SIZE SIZE_WORD
#define BLOCK_SIZE 16
#define TILE_SIZE 4096
#define PLM_IN_WORD TILE_SIZE
#define PLM_OUT_WORD TILE_SIZE
#define GEMM_QUEUE_DEPTH 2

class gemm_sm : public esp_accelerator_avu<DMA_WIDTH, SM_INFO_SIZE, GEMM_QUEUE_DEPTH, N_CONTEXTS_BITS>
{
public:
    // Constructor
    SC_HAS_PROCESS(gemm_sm);
    gemm_sm(const sc_module_name& name)
    : esp_accelerator_avu<DMA_WIDTH, SM_INFO_SIZE, GEMM_QUEUE_DEPTH, N_CONTEXTS_BITS>(name)
    {
        // Map arrays to memories
        /* <<--plm-bind-->> */
        HLS_MAP_plm(plm_in_ping, PLM_IN_NAME);
        HLS_MAP_plm(plm_in_pong, PLM_IN_NAME);
        HLS_MAP_plm(plm_wgt_ping, PLM_IN_NAME);
        HLS_MAP_plm(plm_wgt_pong, PLM_IN_NAME);
        HLS_MAP_plm(plm_out_ping, PLM_OUT_NAME);
        HLS_MAP_plm(plm_out_pong, PLM_OUT_NAME);
    }

    // Processes

    // Load the input data
    void load_input();

    // Computation
    void compute_kernel();

    // Store the output data
    void store_output();

    // Private local memories
    sc_dt::sc_int<DATA_WIDTH> plm_in_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_in_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_out_ping[PLM_OUT_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_out_pong[PLM_OUT_WORD];
};

#endif /* __GEMM_SM_HPP__ */
