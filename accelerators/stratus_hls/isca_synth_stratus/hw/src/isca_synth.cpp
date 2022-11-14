// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "isca_synth.hpp"
#include "isca_synth_directives.hpp"

// Functions

#include "isca_synth_functions.hpp"

// Processes

void isca_synth::load_input()
{
    // Reset
    {
        HLS_PROTO("load-reset");

        this->reset_load_input();

        // explicit PLM ports reset if any

        // User-defined reset code

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t compute_ratio;
    int32_t size;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        compute_ratio = config.compute_ratio;
        size = config.size;
    }

    // Load
    {
        HLS_PROTO("load-dma");
        wait();

        uint32_t offset = 0;

        wait();

        // Configure DMA transaction
        uint32_t len = round_up(size, DMA_WORD_PER_BEAT);

        dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);

        offset += len;

        this->dma_read_ctrl.put(dma_info);

        for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
        {
            HLS_BREAK_DEP(plm_in_ping);

            sc_dt::sc_bv<DMA_WIDTH> dataBv;

            dataBv = this->dma_read_chnl.get();
            wait();

            // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
            for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
            {
                HLS_UNROLL_SIMPLE;
                plm_in_ping[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
            }
        }

        this->load_compute_handshake();
    }

    // Conclude
    {
        this->process_done();
    }
}



void isca_synth::store_output()
{
    // Reset
    {
        HLS_PROTO("store-reset");

        this->reset_store_output();

        // explicit PLM ports reset if any

        // User-defined reset code

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t compute_ratio;
    int32_t size;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        compute_ratio = config.compute_ratio;
        size = config.size;
    }

    // Store
    {
        HLS_PROTO("store-dma");
        wait();

        uint32_t offset = round_up(size, DMA_WORD_PER_BEAT) * 1;
        uint32_t len = round_up(size, DMA_WORD_PER_BEAT);

        wait();

        this->store_compute_handshake();

        // Configure DMA transaction
        dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
        offset += len;

        this->dma_write_ctrl.put(dma_info);

        for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
        {
            sc_dt::sc_bv<DMA_WIDTH> dataBv;

            // Read from PLM
            wait();
            for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
            {
                HLS_UNROLL_SIMPLE;
                dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_out_ping[i + k];
            }

            this->dma_write_chnl.put(dataBv);
        }
    }

    // Conclude
    {
        this->accelerator_done();
        this->process_done();
    }
}


void isca_synth::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-reset");

        this->reset_compute_kernel();

        // explicit PLM ports reset if any

        // User-defined reset code

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t compute_ratio;
    int32_t size;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        compute_ratio = config.compute_ratio;
        size = config.size;
    }

    // Compute
    for (uint32_t i = 0; i < compute_ratio; i++)
	{
        HLS_UNROLL_LOOP(OFF)
        {
            HLS_DEFINE_PROTOCOL("compute-delay");
            wait();
	    }
    }

    // Conclude
    {
        this->process_done();
    }
}
