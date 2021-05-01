// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "mac.hpp"
#include "mac_directives.hpp"

// Functions

#include "mac_functions.hpp"

// Processes

void mac::load_input()
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
    int32_t mac_n;
    int32_t mac_vec;
    int32_t mac_len;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        mac_n = config.mac_n;
        mac_vec = config.mac_vec;
        mac_len = config.mac_len;
    }

    // Load
    {
        HLS_PROTO("load-dma");
        wait();

        bool ping = true;
        uint32_t offset = 0;

        // Batching
        for (uint16_t b = 0; b < mac_n; b++)
        {
            wait();
#if (DMA_WORD_PER_BEAT == 0)
            uint32_t length = mac_len * mac_vec;
#else
            uint32_t length = round_up(mac_len * mac_vec, DMA_WORD_PER_BEAT);
#endif
            // Chunking
            for (int rem = length; rem > 0; rem -= PLM_IN_WORD)
            {
                wait();
                // Configure DMA transaction
                uint32_t len = rem > PLM_IN_WORD ? PLM_IN_WORD : rem;
#if (DMA_WORD_PER_BEAT == 0)
                // data word is wider than NoC links
                dma_info_t dma_info(offset * DMA_BEAT_PER_WORD, len * DMA_BEAT_PER_WORD, DMA_SIZE);
#else
                dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
#endif
                offset += len;

                this->dma_read_ctrl.put(dma_info);

#if (DMA_WORD_PER_BEAT == 0)
                // data word is wider than NoC links
                for (uint16_t i = 0; i < len; i++)
                {
                    sc_dt::sc_bv<DATA_WIDTH> dataBv;

                    for (uint16_t k = 0; k < DMA_BEAT_PER_WORD; k++)
                    {
                        dataBv.range((k+1) * DMA_WIDTH - 1, k * DMA_WIDTH) = this->dma_read_chnl.get();
                        wait();
                    }

                    // Write to PLM
                    if (ping)
                        plm_in_ping[i] = dataBv.to_int64();
                    else
                        plm_in_pong[i] = dataBv.to_int64();
                }
#else
                for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(plm_in_ping);
                    HLS_BREAK_DEP(plm_in_pong);

                    sc_dt::sc_bv<DMA_WIDTH> dataBv;

                    dataBv = this->dma_read_chnl.get();
                    wait();

                    // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        if (ping)
                            plm_in_ping[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                        else
                            plm_in_pong[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                    }
                }
#endif
                this->load_compute_handshake();
                ping = !ping;
            }
        }
    }

    // Conclude
    {
        this->process_done();
    }
}



void mac::store_output()
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
    int32_t mac_n;
    int32_t mac_vec;
    int32_t mac_len;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        mac_n = config.mac_n;
        mac_vec = config.mac_vec;
        mac_len = config.mac_len;
    }

    // Store
    {
        HLS_PROTO("store-dma");
        wait();

        bool ping = true;
#if (DMA_WORD_PER_BEAT == 0)
        uint32_t store_offset = (mac_len * mac_vec) * mac_n;
#else
        uint32_t store_offset = round_up(mac_len * mac_vec, DMA_WORD_PER_BEAT) * mac_n;
#endif
        uint32_t offset = store_offset;

        wait();
        // Batching
        for (uint16_t b = 0; b < mac_n; b++)
        {
            wait();
#if (DMA_WORD_PER_BEAT == 0)
            uint32_t length = mac_vec;
#else
            uint32_t length = round_up(mac_vec, DMA_WORD_PER_BEAT);
#endif
            // Chunking
            for (int rem = length; rem > 0; rem -= PLM_OUT_WORD)
            {

                this->store_compute_handshake();

                // Configure DMA transaction
                uint32_t len = rem > PLM_OUT_WORD ? PLM_OUT_WORD : rem;
#if (DMA_WORD_PER_BEAT == 0)
                // data word is wider than NoC links
                dma_info_t dma_info(offset * DMA_BEAT_PER_WORD, len * DMA_BEAT_PER_WORD, DMA_SIZE);
#else
                dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
#endif
                offset += len;

                this->dma_write_ctrl.put(dma_info);

#if (DMA_WORD_PER_BEAT == 0)
                // data word is wider than NoC links
                for (uint16_t i = 0; i < len; i++)
                {
                    // Read from PLM
                    sc_dt::sc_int<DATA_WIDTH> data;
                    wait();
                    if (ping)
                        data = plm_out_ping[i];
                    else
                        data = plm_out_pong[i];
                    sc_dt::sc_bv<DATA_WIDTH> dataBv(data);

                    uint16_t k = 0;
                    for (k = 0; k < DMA_BEAT_PER_WORD - 1; k++)
                    {
                        this->dma_write_chnl.put(dataBv.range((k+1) * DMA_WIDTH - 1, k * DMA_WIDTH));
                        wait();
                    }
                    // Last beat on the bus does not require wait(), which is
                    // placed before accessing the PLM
                    this->dma_write_chnl.put(dataBv.range((k+1) * DMA_WIDTH - 1, k * DMA_WIDTH));
                }
#else
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
#endif
                ping = !ping;
            }
        }
    }

    // Conclude
    {
        this->accelerator_done();
        this->process_done();
    }
}


void mac::compute_kernel()
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
    int32_t mac_n;
    int32_t mac_vec;
    int32_t mac_len;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        mac_n = config.mac_n;
        mac_vec = config.mac_vec;
        mac_len = config.mac_len;
    }


    // Compute
    bool ping = true;
    bool out_ping = true;
    {
        for (uint16_t b = 0; b < mac_n; b++)
        {
            uint32_t in_length = mac_len * mac_vec;
            uint32_t out_length = mac_vec;
    
            uint32_t vector_index = 0;
            uint32_t vector_number = 0;
            int32_t acc = 0;
    
            for (int in_rem = in_length; in_rem > 0; in_rem -= PLM_IN_WORD)
            {
    
                uint32_t in_len  = in_rem  > PLM_IN_WORD  ? PLM_IN_WORD  : in_rem;
    
                this->compute_load_handshake();
    
                // Computing phase implementation
                for (int i = 0; i < in_len; i += 2) {
    
                    // Multiply and accumulate
                    if (ping)
                        acc += plm_in_ping[i] * plm_in_ping[i + 1];
                    else
                        acc += plm_in_pong[i] * plm_in_pong[i + 1];
    
                    vector_index += 2;
    
                    // Write accumulated result
                    if (vector_index == mac_len) {
                        if (out_ping)
                            plm_out_ping[vector_number] = acc;
                        else
                            plm_out_pong[vector_number] = acc;
    
                        acc = 0;
                        vector_index = 0;
                        vector_number++;
                    }
                }
    
                ping = !ping;
            }
    
            this->compute_store_handshake();
            out_ping = !out_ping;
        }
        // Conclude
        {
            this->process_done();
        }
    }
}
