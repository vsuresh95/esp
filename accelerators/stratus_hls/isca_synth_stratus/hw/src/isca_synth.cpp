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
        load_ready.ack.reset_ack();
        load_done.req.reset_req();

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

        // Configure DMA transaction
        uint32_t offset = 0;
        uint32_t len = size;

        dma_info_t dma_info(offset, len, DMA_SIZE);

        this->dma_read_ctrl.put(dma_info);

        for (uint16_t i = 0; i < len; i++)
        {
            sc_dt::sc_bv<DMA_WIDTH> dataBv;

            dataBv = this->dma_read_chnl.get();
            wait();

            plm_in_ping[i] = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
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
        store_ready.ack.reset_ack();
        store_done.req.reset_req();

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

        this->store_compute_handshake();

        // Configure DMA transaction
        uint32_t offset = size;
        uint32_t len = size;

        dma_info_t dma_info(offset, len, DMA_SIZE);

        this->dma_write_ctrl.put(dma_info);

        for (uint16_t i = 0; i < len; i++)
        {
            sc_dt::sc_bv<DMA_WIDTH> dataBv;

            wait();

            dataBv.range(DATA_WIDTH - 1, 0) = plm_in_ping[i];

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
        load_ready.req.reset_req();
        load_done.ack.reset_ack();
        store_ready.req.reset_req();
        store_done.ack.reset_ack();

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
    {
        wait();
        uint32_t len = size;

        this->compute_load_handshake();

        for (uint16_t i = 0; i < len; i+=TILE_SIZE)
        {
            uint64_t regs_elem[TILE_SIZE];
            HLS_FLATTEN_ARRAY(regs_elem);

            for (uint16_t j = 0; j < TILE_SIZE; j++)
            {
                HLS_UNROLL_LOOP(ON, "read-elem");
                HLS_BREAK_ARRAY_DEPENDENCY(plm_in_ping);
                regs_elem[j] = plm_in_ping[i+j];
            }

            for (uint16_t j = 0; j < TILE_SIZE; j++)
            {
                HLS_UNROLL_LOOP(ON, "self-mac");

                uint64_t elem = regs_elem[j];
                regs_elem[j] = elem * elem;
                regs_elem[j] += elem;
            }

            for (uint16_t j = 0; j < TILE_SIZE; j++)
            {
                HLS_UNROLL_LOOP(ON, "write-elem");
                HLS_BREAK_ARRAY_DEPENDENCY(plm_in_ping);
                plm_in_ping[i+j] = regs_elem[j];
            }
        }

        this->compute_store_handshake();
    }

    // Conclude
    {
        this->process_done();
    }
}
