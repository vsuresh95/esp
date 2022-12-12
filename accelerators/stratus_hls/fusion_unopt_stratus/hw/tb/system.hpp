// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __SYSTEM_HPP__
#define __SYSTEM_HPP__

#include "fusion_unopt_conf_info.hpp"
#include "fusion_unopt_debug_info.hpp"
#include "fusion_unopt.hpp"
#include "fusion_unopt_directives.hpp"

#include "esp_templates.hpp"

const size_t MEM_SIZE = 1304720 / (DMA_WIDTH/8);

#include "core/systems/esp_system.hpp"

#ifdef CADENCE
#include "fusion_unopt_wrap.h"
#endif

class system_t : public esp_system<DMA_WIDTH, MEM_SIZE>
{
public:

    // ACC instance
#ifdef CADENCE
    fusion_unopt_wrapper *acc;
#else
    fusion_unopt *acc;
#endif

    // Constructor
    SC_HAS_PROCESS(system_t);
    system_t(sc_module_name name)
        : esp_system<DMA_WIDTH, MEM_SIZE>(name)
    {
        // ACC
#ifdef CADENCE
        acc = new fusion_unopt_wrapper("fusion_unopt_wrapper");
#else
        acc = new fusion_unopt("fusion_unopt_wrapper");
#endif
        // Binding ACC
        acc->clk(clk);
        acc->rst(acc_rst);
        acc->dma_read_ctrl(dma_read_ctrl);
        acc->dma_write_ctrl(dma_write_ctrl);
        acc->dma_read_chnl(dma_read_chnl);
        acc->dma_write_chnl(dma_write_chnl);
        acc->conf_info(conf_info);
        acc->conf_done(conf_done);
        acc->acc_done(acc_done);
        acc->debug(debug);

        /* <<--params-default-->> */
        veno = 10;
        imgwidth = 640;
        htdim = 512;
        imgheight = 480;
        sdf_block_size = 8;
        sdf_block_size3 = 512;
    }

    // Processes

    // Configure accelerator
    void config_proc();

    // Helper function for the computation of each voxel
    int32_t computeUpdatedVoxelDepthInfo(int32_t voxel[2], // voxel[0] = sdf, voxel[1] = w_depth
                                int32_t pt_model[4],
                                int32_t M_d[16],
                                int32_t projParams_d[4],
                                int32_t mu,
                                int32_t maxW,
                                int32_t depth[640 * 480],	// remember to change this value when changing the size of the image
                                int32_t imgSize[2],
								int32_t newValue[2])

    // Load internal memory
    void load_memory();

    // Dump internal memory
    void dump_memory();

    // Validate accelerator results
    int validate();

    // Accelerator-specific data
    /* <<--params-->> */
    int32_t veno;
    int32_t imgwidth;
    int32_t htdim;
    int32_t imgheight;
    int32_t sdf_block_size;
    int32_t sdf_block_size3;

    uint32_t in_words_adj;
    uint32_t out_words_adj;
    uint32_t in_size;
    uint32_t out_size;
    int32_t *in;
    int32_t *out;
    int32_t *gold;

    // Other Functions
};

#endif // __SYSTEM_HPP__
