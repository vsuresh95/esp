// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __SYSTEM_HPP__
#define __SYSTEM_HPP__

#include "basic_256_gemm_conf_info.hpp"
#include "basic_256_gemm_debug_info.hpp"
#include "basic_256_gemm.hpp"
#include "basic_256_gemm_directives.hpp"

#include "esp_templates.hpp"

const size_t MEM_SIZE = 786432 / (DMA_WIDTH/8);

#include "core/systems/esp_system.hpp"

#ifdef CADENCE
#include "basic_256_gemm_wrap.h"
#endif

class system_t : public esp_system<DMA_WIDTH, MEM_SIZE>
{
public:

    // ACC instance
#ifdef CADENCE
    basic_256_gemm_wrapper *acc;
#else
    basic_256_gemm *acc;
#endif

    // Constructor
    SC_HAS_PROCESS(system_t);
    system_t(sc_module_name name)
        : esp_system<DMA_WIDTH, MEM_SIZE>(name)
    {
        // ACC
#ifdef CADENCE
        acc = new basic_256_gemm_wrapper("basic_256_gemm_wrapper");
#else
        acc = new basic_256_gemm("basic_256_gemm_wrapper");
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
        M = 32;
        N = 16;
        K = 16;
        
        #if (DMA_WORD_PER_BEAT == 0)
            uint32_t A_words = M * K;
            uint32_t B_words = K * N;
        #else
            uint32_t A_words = round_up(M * K, DMA_WORD_PER_BEAT);
            uint32_t B_words = round_up(K * N, DMA_WORD_PER_BEAT);
        #endif
        
        A_mat_offset = 0;
        B_mat_offset = A_words;
        C_mat_offset = A_words + B_words;
        
        ESP_REPORT_INFO("Constructor: Matrix offsets set - A=%d, B=%d, C=%d", 
                        A_mat_offset, B_mat_offset, C_mat_offset);
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
    int32_t C_mat_offset;
    int32_t B_mat_offset;
    int32_t A_mat_offset;
    int32_t K;
    int32_t M;
    int32_t N;

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