// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __SYSTEM_HPP__
#define __SYSTEM_HPP__

#include "gemm_1_sparse_conf_info.hpp"
#include "gemm_1_sparse_debug_info.hpp"
#include "gemm_1_sparse.hpp"
#include "gemm_1_sparse_directives.hpp"

#include "esp_templates.hpp"

const size_t MEM_SIZE = 798720 / (DMA_WIDTH/8);

#include "core/systems/esp_system.hpp"

#ifdef CADENCE
#include "gemm_1_sparse_wrap.h"
#endif

class system_t : public esp_system<DMA_WIDTH, MEM_SIZE>
{
public:

    // ACC instance
#ifdef CADENCE
    gemm_1_sparse_wrapper *acc;
#else
    gemm_1_sparse *acc;
#endif

    // Constructor
    SC_HAS_PROCESS(system_t);
    system_t(sc_module_name name)
        : esp_system<DMA_WIDTH, MEM_SIZE>(name)
    {
        // ACC
#ifdef CADENCE
        acc = new gemm_1_sparse_wrapper("gemm_1_sparse_wrapper");
#else
        acc = new gemm_1_sparse("gemm_1_sparse_wrapper");
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
        h_order_size = 8;
        gemm_m = 64;
        gemm_n = 64;
        gemm_k = 64;
        gemm_batch = 1;
        var_size = 8;
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
    int32_t h_order_size;
    int32_t gemm_m;
    int32_t gemm_n;
    int32_t gemm_k;
    int32_t gemm_batch;
    int32_t var_size;

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
