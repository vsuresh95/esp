// Copyright (c) 2011-2019 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __SYSTEM_HPP__
#define __SYSTEM_HPP__

#include "fir_conf_info.hpp"
#include "fir_debug_info.hpp"
#include "fir.hpp"
#include "fir_directives.hpp"
#include "fir_test.hpp"

#include "esp_templates.hpp"

#define SYNC_VAR_SIZE 2

#define MAX_BUFFERS_FULL 4
const size_t MEM_SIZE = MAX_BUFFERS_FULL * (2 * 262144) / (DMA_WIDTH/8);

#include "core/systems/esp_system.hpp"

#ifdef CADENCE
#include "fir_wrap.h"
#endif

class system_t : public esp_system<DMA_WIDTH, MEM_SIZE>
{
public:

    // ACC instance
#ifdef CADENCE
    fir_wrapper *acc;
#else
    fir *acc;
#endif

    // Constructor
    SC_HAS_PROCESS(system_t);
    system_t(sc_module_name name)
        : esp_system<DMA_WIDTH, MEM_SIZE>(name)
    {
        // ACC
#ifdef CADENCE
        acc = new fir_wrapper("fir_wrapper");
#else
        acc = new fir("fir_wrapper");
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
        acc->acc_fence(acc_fence);
        acc->debug(debug);

        /* <<--params-default-->> */
        // use_input_files = 1
        //logn_samples = 6;
        //num_firs = 46;

        use_input_files = 0;
        logn_samples = 10;
        num_firs = 1;

        //use_input_files = 1;
        //logn_samples = 6;
        //num_firs = 1;
        num_samples = (1 << logn_samples);
        do_inverse = 0;
        do_shift = 0;
        scale_factor = 1;
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
    int32_t logn_samples;
    int32_t num_samples;
    int32_t num_firs;
    int32_t do_inverse;
    int32_t do_shift;
    int32_t scale_factor;

    uint32_t in_words_adj;
    uint32_t out_words_adj;
    uint32_t in_size;
    uint32_t out_size;
    float *in;
    float *out;
    float *gold;

    uint32_t total_size;
    uint32_t use_input_files;
    // Other Functions
};

#endif // __SYSTEM_HPP__
