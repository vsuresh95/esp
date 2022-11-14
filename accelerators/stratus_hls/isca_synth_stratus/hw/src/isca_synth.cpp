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

        uint32_t offset = 0;

        wait();

        // Configure DMA transaction
        uint32_t len = size;

        for (uint16_t i = 0; i < len; i += BURST_SIZE)
        {
            HLS_UNROLL_LOOP(OFF);
            this->load_compute_ready_handshake();

            dma_info_t dma_info(offset, BURST_SIZE, DMA_SIZE);

            offset += BURST_SIZE;

            this->dma_read_ctrl.put(dma_info);

            for (uint16_t k = 0; k < BURST_SIZE; k++)
            {
                HLS_UNROLL_LOOP(OFF);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                dataBv = this->dma_read_chnl.get();
                wait();

                plm_in_ping[i + k] = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
            }

            this->load_compute_done_handshake();
        }
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

        // Configure DMA transaction
        uint32_t offset = size;
        uint32_t len = size;

        for (uint16_t i = 0; i < len; i += BURST_SIZE)
        {
            HLS_UNROLL_LOOP(OFF);
            this->store_compute_ready_handshake();

            dma_info_t dma_info(offset, BURST_SIZE, DMA_SIZE);

            offset += BURST_SIZE;

            this->dma_write_ctrl.put(dma_info);

            for (uint16_t k = 0; k < BURST_SIZE; k++)
            {
                HLS_UNROLL_LOOP(OFF);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                dataBv.range(DATA_WIDTH - 1, 0) = plm_in_ping[i + k];

                this->dma_write_chnl.put(dataBv);
            }

            this->store_compute_done_handshake();
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
        HLS_PROTO("compute-loop");

        wait();
        uint32_t len = size;

        for (uint16_t i = 0; i < len; i += BURST_SIZE)
        {
            HLS_UNROLL_LOOP(OFF);
            wait();
            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();

            for (uint32_t k = 0; k < compute_ratio; k++)
	        {
                HLS_UNROLL_LOOP(OFF);
                wait();
            }

            wait();
            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
        }
    } 

    // Conclude
    {
        this->process_done();
    }
}
