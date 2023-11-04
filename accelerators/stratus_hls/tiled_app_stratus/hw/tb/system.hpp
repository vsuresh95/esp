// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __SYSTEM_HPP__
#define __SYSTEM_HPP__

#include "tiled_app_conf_info.hpp"
#include "tiled_app_debug_info.hpp"
#include "tiled_app.hpp"
#include "tiled_app_directives.hpp"

#include "esp_templates.hpp"

const size_t MEM_SIZE = 196608 / (DMA_WIDTH/8);
#define TB_NUM_TILES 2
#define TB_TILE_SZ 1024
//#define PRINT_ALL 1

#include "core/systems/esp_system.hpp"

#ifdef CADENCE
#include "tiled_app_wrap.h"
#endif

class system_t : public esp_system<DMA_WIDTH, MEM_SIZE>
{
public:

    // ACC instance
#ifdef CADENCE
    tiled_app_wrapper *acc;
#else
    tiled_app *acc;
#endif

    // Constructor
    SC_HAS_PROCESS(system_t);
    system_t(sc_module_name name)
        : esp_system<DMA_WIDTH, MEM_SIZE>(name)
    {
        // ACC
#ifdef CADENCE
        acc = new tiled_app_wrapper("tiled_app_wrapper");
#else
        acc = new tiled_app("tiled_app_wrapper");
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
        num_tiles = TB_NUM_TILES;//12;
        tile_size = TB_TILE_SZ; //1024;
        rd_wr_enable = 0;
    }

    // Processes

    // Configure accelerator
    void config_proc();

    void init_sync_cfg();
    bool read_sync();
    bool write_sync();

    // Load internal memory
    void load_memory();

    // Dump internal memory
    void dump_memory();

    // Validate accelerator results
    int validate();

    // Accelerator-specific data
    /* <<--params-->> */
    int32_t num_tiles;
    int32_t tile_size;
    int32_t rd_wr_enable;

    uint32_t in_words_adj;
    uint32_t out_words_adj;
    uint32_t in_size;
    uint32_t out_size;
    int64_t *in;
    int64_t *out;
    int64_t *gold;

    // Other Functions
};

#endif // __SYSTEM_HPP__
