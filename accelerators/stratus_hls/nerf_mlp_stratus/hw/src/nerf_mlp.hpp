// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __NERF_MLP_HPP__
#define __NERF_MLP_HPP__

#include "nerf_mlp_conf_info.hpp"
#include "nerf_mlp_debug_info.hpp"

#include "esp_templates.hpp"

#include "nerf_mlp_directives.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/* <<--defines-->> */
#define DATA_WIDTH 64
#define DMA_SIZE SIZE_DWORD

#define LAYER_N_DIMS 256
#define LAYER_0_INPUTS 60
#define LAYER_0_OUTPUTS 256
#define LAYER_4_INPUTS 316
#define LAYER_4_OUTPUTS 256
#define LAYER_8_INPUTS 280
#define LAYER_8_OUTPUTS 256
#define LAYER_9_INPUTS 256
#define LAYER_9_OUTPUTS 128
#define LAYER_10_INPUTS 128
#define LAYER_10_OUTPUTS 3

#define MAC_TILE_SIZE 4

class nerf_mlp : public esp_accelerator_3P<DMA_WIDTH>
{
public:
    // Constructor
    SC_HAS_PROCESS(nerf_mlp);
    nerf_mlp(const sc_module_name& name)
    : esp_accelerator_3P<DMA_WIDTH>(name)
        , cfg("config")
    {
        // Signal binding
        cfg.bind_with(*this);

        // Map arrays to memories
        /* <<--plm-bind-->> */
        HLS_MAP_plm(plm_wgt_0, "plm_wgt_0_name");
        HLS_MAP_plm(plm_wgt_1, "plm_wgt_1_name");
        HLS_MAP_plm(plm_wgt_2, "plm_wgt_2_name");
        HLS_MAP_plm(plm_wgt_3, "plm_wgt_3_name");
        HLS_MAP_plm(plm_wgt_4, "plm_wgt_4_name");
        HLS_MAP_plm(plm_wgt_5, "plm_wgt_5_name");
        HLS_MAP_plm(plm_wgt_6, "plm_wgt_6_name");
        HLS_MAP_plm(plm_wgt_7, "plm_wgt_7_name");
        HLS_MAP_plm(plm_wgt_8, "plm_wgt_8_name");
        HLS_MAP_plm(plm_wgt_9, "plm_wgt_9_name");
        HLS_MAP_plm(plm_wgt_10, "plm_wgt_10_name");

        HLS_MAP_plm(plm_bias_0, "plm_bias_0_name");
        HLS_MAP_plm(plm_bias_1, "plm_bias_1_name");
        HLS_MAP_plm(plm_bias_2, "plm_bias_2_name");
        HLS_MAP_plm(plm_bias_3, "plm_bias_3_name");
        HLS_MAP_plm(plm_bias_4, "plm_bias_4_name");
        HLS_MAP_plm(plm_bias_5, "plm_bias_5_name");
        HLS_MAP_plm(plm_bias_6, "plm_bias_6_name");
        HLS_MAP_plm(plm_bias_7, "plm_bias_7_name");
        HLS_MAP_plm(plm_bias_8, "plm_bias_8_name");
        HLS_MAP_plm(plm_bias_9, "plm_bias_9_name");
        HLS_MAP_plm(plm_bias_10, "plm_bias_10_name");

        HLS_MAP_plm(plm_pos, "plm_pos_name");
        HLS_MAP_plm(plm_dir, "plm_dir_name");
    }

    // Processes

    // Load the input data
    void load_input();
    void load_input_dma(uint32_t len, uint32_t offset, sc_dt::sc_int<DATA_WIDTH> *plm_input);

    // Computation
    void compute_kernel();
    void compute_kernel_tile(uint16_t input_dim, uint16_t output_dim, sc_dt::sc_int<DATA_WIDTH> *plm_wgt, sc_dt::sc_int<DATA_WIDTH> *plm_bias, int64_t *regs_input, int64_t *regs_output, int64_t *regs_mul, int64_t *regs_wgt);

    // Store the output data
    void store_output();

    // Configure nerf_mlp
    esp_config_proc cfg;

    // Functions

    // Private local memories
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_0[15360];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_1[65536];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_2[65536];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_3[65536];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_4[80896];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_5[65536];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_6[65536];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_7[65536];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_8[71680];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_9[32768];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_10[372];

    sc_dt::sc_int<DATA_WIDTH> plm_bias_0[256];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_1[256];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_2[256];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_3[256];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_4[256];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_5[256];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_6[256];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_7[256];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_8[256];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_9[256];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_10[256];

    sc_dt::sc_int<DATA_WIDTH> plm_pos[60];
    sc_dt::sc_int<DATA_WIDTH> plm_dir[24];
    
    int64_t regs_ping[316];
    int64_t regs_wgt[MAC_TILE_SIZE];
    int64_t regs_bias[256];
    int64_t regs_mul[MAC_TILE_SIZE];
    int64_t regs_pong[316];
    int64_t regs_out[3];
};


#endif /* __NERF_MLP_HPP__ */
