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
#define DATA_WIDTH 32
#define DMA_SIZE SIZE_DWORD

#define LAYER_N_DIMS 256
#define LAYER_0_INPUTS 63
#define LAYER_0_OUTPUTS 256
#define LAYER_5_INPUTS 319
#define LAYER_5_OUTPUTS 256
#define LAYER_9_INPUTS 283
#define LAYER_9_OUTPUTS 128
#define LAYER_10_INPUTS 128
#define LAYER_10_OUTPUTS 3
#define LAYER_11_INPUTS 256
#define LAYER_11_OUTPUTS 1
#define LAYER_5_INPUTS_ROUND 320
#define LAYER_0_INPUTS_ROUND 64
#define LAYER_9_INPUTS_ROUND 288
#define LAYER_10_OUTPUTS_ROUND 16
#define LAYER_11_OUTPUTS_ROUND 16

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
        HLS_MAP_plm(plm_wgt_0, PLM_WGT_0_NAME);
        HLS_MAP_plm(plm_wgt_1, PLM_WGT_1_NAME);
        HLS_MAP_plm(plm_wgt_2, PLM_WGT_2_NAME);
        HLS_MAP_plm(plm_wgt_3, PLM_WGT_3_NAME);
        HLS_MAP_plm(plm_wgt_4, PLM_WGT_4_NAME);
        HLS_MAP_plm(plm_wgt_5, PLM_WGT_5_NAME);
        HLS_MAP_plm(plm_wgt_6, PLM_WGT_6_NAME);
        HLS_MAP_plm(plm_wgt_7, PLM_WGT_7_NAME);
        HLS_MAP_plm(plm_wgt_8, PLM_WGT_8_NAME);
        HLS_MAP_plm(plm_wgt_9, PLM_WGT_9_NAME);
        HLS_MAP_plm(plm_wgt_10, PLM_WGT_10_NAME);
        HLS_MAP_plm(plm_wgt_11, PLM_WGT_11_NAME);

        HLS_MAP_plm(plm_bias_0, PLM_BIAS_0_NAME);
        HLS_MAP_plm(plm_bias_1, PLM_BIAS_1_NAME);
        HLS_MAP_plm(plm_bias_2, PLM_BIAS_2_NAME);
        HLS_MAP_plm(plm_bias_3, PLM_BIAS_3_NAME);
        HLS_MAP_plm(plm_bias_4, PLM_BIAS_4_NAME);
        HLS_MAP_plm(plm_bias_5, PLM_BIAS_5_NAME);
        HLS_MAP_plm(plm_bias_6, PLM_BIAS_6_NAME);
        HLS_MAP_plm(plm_bias_7, PLM_BIAS_7_NAME);
        HLS_MAP_plm(plm_bias_8, PLM_BIAS_8_NAME);
        HLS_MAP_plm(plm_bias_9, PLM_BIAS_9_NAME);
        HLS_MAP_plm(plm_bias_10, PLM_BIAS_10_NAME);
        HLS_MAP_plm(plm_bias_11, PLM_BIAS_11_NAME);

        HLS_MAP_plm(plm_act_1, PLM_ACT_1_NAME);
        HLS_MAP_plm(plm_act_2, PLM_ACT_2_NAME);
        HLS_MAP_plm(plm_act_3, PLM_ACT_3_NAME);
        HLS_MAP_plm(plm_act_4, PLM_ACT_4_NAME);
        HLS_MAP_plm(plm_act_5, PLM_ACT_5_NAME);
        HLS_MAP_plm(plm_act_6, PLM_ACT_6_NAME);
        HLS_MAP_plm(plm_act_7, PLM_ACT_7_NAME);
        HLS_MAP_plm(plm_act_8, PLM_ACT_8_NAME);
        HLS_MAP_plm(plm_act_9, PLM_ACT_9_NAME);
        HLS_MAP_plm(plm_act_10, PLM_ACT_10_NAME);

        HLS_MAP_plm(plm_pos, PLM_POS_NAME);
        HLS_MAP_plm(plm_dir, PLM_DIR_NAME);

        HLS_MAP_plm(plm_out, PLM_OUT_NAME);

        PRESERVE_SIGNALS;
    }

    // Processes

    // Load the input data
    void load_input();
    void load_input_dma(uint32_t len, uint32_t offset, sc_dt::sc_int<DATA_WIDTH> *plm_input);

    // Computation
    void compute_kernel();

    // Store the output data
    void store_output();

    // Configure nerf_mlp
    esp_config_proc cfg;

    // Functions

    // Private local memories
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_0[LAYER_0_INPUTS*LAYER_0_OUTPUTS];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_1[LAYER_N_DIMS*LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_2[LAYER_N_DIMS*LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_3[LAYER_N_DIMS*LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_4[LAYER_N_DIMS*LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_5[LAYER_5_INPUTS*LAYER_5_OUTPUTS];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_6[LAYER_N_DIMS*LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_7[LAYER_N_DIMS*LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_8[LAYER_N_DIMS*LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_9[LAYER_9_INPUTS*LAYER_9_OUTPUTS];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_10[LAYER_10_INPUTS*LAYER_10_OUTPUTS];
    sc_dt::sc_int<DATA_WIDTH> plm_wgt_11[LAYER_11_INPUTS*LAYER_11_OUTPUTS];

    sc_dt::sc_int<DATA_WIDTH> plm_bias_0[LAYER_0_OUTPUTS];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_1[LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_2[LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_3[LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_4[LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_5[LAYER_5_OUTPUTS];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_6[LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_7[LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_8[LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_9[LAYER_9_OUTPUTS];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_10[LAYER_10_OUTPUTS_ROUND];
    sc_dt::sc_int<DATA_WIDTH> plm_bias_11[LAYER_11_OUTPUTS_ROUND];

    sc_dt::sc_int<DATA_WIDTH> plm_act_1[LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_act_2[LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_act_3[LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_act_4[LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_act_5[LAYER_5_INPUTS_ROUND];
    sc_dt::sc_int<DATA_WIDTH> plm_act_6[LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_act_7[LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_act_8[LAYER_N_DIMS];
    sc_dt::sc_int<DATA_WIDTH> plm_act_9[LAYER_9_INPUTS_ROUND];
    sc_dt::sc_int<DATA_WIDTH> plm_act_10[LAYER_10_INPUTS];

    sc_dt::sc_int<DATA_WIDTH> plm_pos[LAYER_0_INPUTS];
    sc_dt::sc_int<DATA_WIDTH> plm_dir[LAYER_9_INPUTS-LAYER_N_DIMS];
    
    sc_dt::sc_int<DATA_WIDTH> plm_out[LAYER_10_OUTPUTS+LAYER_11_OUTPUTS];

    sc_signal< sc_int<DATA_WIDTH> > cur_load_data_dbg;
    sc_signal< bool > cur_output_valid_dbg;
    sc_signal< sc_int<DATA_WIDTH> > cur_output_dbg;
    sc_signal< sc_int<DATA_WIDTH> > cur_wgt_dbg;
    sc_signal< bool > cur_input_valid_dbg;
    sc_signal< sc_int<DATA_WIDTH> > cur_input_dbg;
};


#endif /* __NERF_MLP_HPP__ */
