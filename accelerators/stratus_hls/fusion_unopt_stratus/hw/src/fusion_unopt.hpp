// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __FUSION_UNOPT_HPP__
#define __FUSION_UNOPT_HPP__

#include "fusion_unopt_conf_info.hpp"
#include "fusion_unopt_debug_info.hpp"

#include "esp_templates.hpp"

#include "fusion_unopt_directives.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/* <<--defines-->> */
#define DATA_WIDTH 32
#define DMA_SIZE SIZE_WORD
#define PLM_OUT_WORD 15360
#define PLM_IN_WORD 310820

#define VENO 10
#define IMGWIDTH 640
#define IMGHEIGHT 480
#define HTDIM 512
#define SDF_BLOCK_SIZE 8
#define SDF_BLOCK_SIZE3 512

class fusion_unopt : public esp_accelerator_3P<DMA_WIDTH>
{
public:
    // Constructor
    SC_HAS_PROCESS(fusion_unopt);
    fusion_unopt(const sc_module_name& name)
    : esp_accelerator_3P<DMA_WIDTH>(name)
        , cfg("config")
    {
        // Signal binding
        cfg.bind_with(*this);

        // Debugging signals
        HLS_PRESERVE_SIGNAL(load_state_req_dbg, true);
        HLS_PRESERVE_SIGNAL(store_state_req_dbg, true);
        HLS_PRESERVE_SIGNAL(compute_state_req_dbg, true);

        // Map arrays to memories
        /* <<--plm-bind-->> */
        HLS_MAP_plm(plm_out_pong, PLM_OUT_NAME);
        HLS_MAP_plm(plm_out_ping, PLM_OUT_NAME);
        HLS_MAP_plm(plm_in_pong, PLM_IN_NAME);
        HLS_MAP_plm(plm_in_ping, PLM_IN_NAME);
    }

    // Debugging signals
    sc_signal< sc_int<32> > load_state_req_dbg;
    sc_signal< sc_int<32> > store_state_req_dbg;
    sc_signal< sc_int<32> > compute_state_req_dbg;

    // Processes

    // Load the input data
    void load_input();

    // // Helper function for the computation of each voxel
    // sc_dt::sc_int<DATA_WIDTH> computeUpdatedVoxelDepthInfo(int32_t voxel[2], // voxel[0] = sdf, voxel[1] = w_depth
    //                             int32_t pt_model[4],
    //                             int32_t M_d[16],
    //                             int32_t projParams_d[4],
    //                             int32_t mu,
    //                             int32_t maxW,
    //                             // int32_t depth[IMGWIDTH * IMGHEIGHT],	// remember to change this value when changing the size of the image
    //                             int32_t depth[1],	// remember to change this value when changing the size of the image
    //                             int32_t imgSize[2],
	// 							sc_dt::sc_int<DATA_WIDTH> newValue[2]);

    // Computation
    void compute_kernel();

    // Store the output data
    void store_output();

    // Configure fusion_unopt
    esp_config_proc cfg;

    // Functions

    // Private local memories
    sc_dt::sc_int<DATA_WIDTH> plm_in_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_in_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_out_ping[PLM_OUT_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_out_pong[PLM_OUT_WORD];
    // float plm_in_ping[PLM_IN_WORD];
    // float plm_in_pong[PLM_IN_WORD];
    // float plm_out_ping[PLM_OUT_WORD];
    // float plm_out_pong[PLM_OUT_WORD];

};


#endif /* __FUSION_UNOPT_HPP__ */
