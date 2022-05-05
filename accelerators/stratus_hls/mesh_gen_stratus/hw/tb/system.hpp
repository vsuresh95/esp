// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __SYSTEM_HPP__
#define __SYSTEM_HPP__

#include "mesh_gen_conf_info.hpp"
#include "mesh_gen_debug_info.hpp"
#include "mesh_gen.hpp"
#include "mesh_gen_directives.hpp"

#include "esp_templates.hpp"

const size_t MEM_SIZE = 131072 / (DMA_WIDTH/8);

#include "core/systems/esp_system.hpp"

#ifdef CADENCE
#include "mesh_gen_wrap.h"
#endif

class system_t : public esp_system<DMA_WIDTH, MEM_SIZE>
{
public:

    // ACC instance
#ifdef CADENCE
    mesh_gen_wrapper *acc;
#else
    mesh_gen *acc;
#endif

    // Constructor
    SC_HAS_PROCESS(system_t);
    system_t(sc_module_name name)
        : esp_system<DMA_WIDTH, MEM_SIZE>(name)
    {
        // ACC
#ifdef CADENCE
        acc = new mesh_gen_wrapper("mesh_gen_wrapper");
#else
        acc = new mesh_gen("mesh_gen_wrapper");
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
        num_hash_table = 1;
    }

    // Processes

    // Configure accelerator
    void config_proc();

    // Load internal memory
    void load_memory();

    // Dump internal memory
    void dump_memory();

    // Validate accelerator results
    int validate();

    // Accelerator-specific data
    /* <<--params-->> */
    int32_t num_hash_table;

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
