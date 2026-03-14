// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "add.hpp"
#include "add_directives.hpp"

// Functions

#include "add_functions.hpp"

// Processes

void add::load_input()
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
    int32_t total_len;
    int32_t input1_offset;
    int32_t input2_offset;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        total_len = config.total_len;
        input1_offset = config.input1_offset;
        input2_offset = config.input2_offset;
    }

    // Load
    {
        HLS_PROTO("load-dma");
        wait();

        bool ping = true;
        uint32_t offset1 = round_up(input1_offset, DMA_WORD_PER_BEAT) * 1;
        uint32_t offset2 = round_up(input2_offset, DMA_WORD_PER_BEAT) * 1;

        uint32_t length = round_up(total_len, DMA_WORD_PER_BEAT);
        // Chunking
        for (int rem = length; rem > 0; rem -= PLM_IN_WORD)
        {
            wait();
            uint32_t len = rem > PLM_IN_WORD ? PLM_IN_WORD : rem;
            // Configure DMA transaction for Input 1
            {
                dma_info_t dma_info(offset1 / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
                offset1 += len;

                this->dma_read_ctrl.put(dma_info);

                for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(plm_in1_ping);
                    HLS_BREAK_DEP(plm_in1_pong);

                    sc_dt::sc_bv<DMA_WIDTH> dataBv;

                    dataBv = this->dma_read_chnl.get();
                    wait();

                    // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        if (ping)
                            plm_in1_ping[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                        else
                            plm_in1_pong[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                    }
                }
            }
            wait();
            // Configure DMA transaction for Input 2
            {
                dma_info_t dma_info(offset2 / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
                offset2 += len;

                this->dma_read_ctrl.put(dma_info);

                for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(plm_in2_ping);
                    HLS_BREAK_DEP(plm_in2_pong);

                    sc_dt::sc_bv<DMA_WIDTH> dataBv;

                    dataBv = this->dma_read_chnl.get();
                    wait();

                    // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        if (ping)
                            plm_in2_ping[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                        else
                            plm_in2_pong[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                    }
                }
            }
            this->load_compute_handshake();
            ping = !ping;
        }
    }

    // Conclude
    {
        this->process_done();
    }
}



void add::store_output()
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
    int32_t total_len;
    int32_t output_offset;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        total_len = config.total_len;
        output_offset = config.output_offset;
    }

    // Store
    {
        HLS_PROTO("store-dma");
        wait();

        bool ping = true;
        uint32_t offset = round_up(output_offset, DMA_WORD_PER_BEAT) * 1;

        wait();
        uint32_t length = round_up(total_len, DMA_WORD_PER_BEAT);
        // Chunking
        for (int rem = length; rem > 0; rem -= PLM_OUT_WORD)
        {
            this->store_compute_handshake();

            // Configure DMA transaction
            uint32_t len = rem > PLM_OUT_WORD ? PLM_OUT_WORD : rem;
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
                    if (ping)
                        dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_out_ping[i + k];
                    else
                        dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_out_pong[i + k];
                }
                this->dma_write_chnl.put(dataBv);
            }
            ping = !ping;
        }
    }

    // Conclude
    {
        this->accelerator_done();
        this->process_done();
    }
}


void add::compute_kernel()
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
    int32_t total_len;
    int32_t do_relu;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        total_len = config.total_len;
        do_relu = config.do_relu;
    }


    // Compute
    bool ping = true;
    {
        uint32_t length = total_len;

        for (int rem = length; rem > 0; rem -= PLM_IN_WORD)
        {
            uint32_t len = rem > PLM_IN_WORD ? PLM_IN_WORD : rem;

            this->compute_load_handshake();

            // Computing phase implementation
            for (int i = 0; i < len; i++) {
                FPDATA out;
                if (ping) {
                    out = int2fp<FPDATA, WORD_SIZE>(plm_in1_ping[i]) + int2fp<FPDATA, WORD_SIZE>(plm_in2_ping[i]);
                    out = (do_relu == 1) ? (out > FPDATA(0) ? out : FPDATA(0)) : out;
                    plm_out_ping[i] = fp2int<FPDATA, WORD_SIZE>(out);
                } else {
                    out = int2fp<FPDATA, WORD_SIZE>(plm_in1_pong[i]) + int2fp<FPDATA, WORD_SIZE>(plm_in2_pong[i]);
                    out = (do_relu == 1) ? (out > FPDATA(0) ? out : FPDATA(0)) : out;
                    plm_out_pong[i] = fp2int<FPDATA, WORD_SIZE>(out);
                }
            }

            this->compute_store_handshake();
            ping = !ping;
        }

        // Conclude
        {
            this->process_done();
        }
    }
}
