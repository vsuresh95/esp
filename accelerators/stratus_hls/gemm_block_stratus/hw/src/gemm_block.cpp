// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "gemm_block.hpp"
#include "gemm_block_directives.hpp"

// Functions

#include "gemm_block_functions.hpp"

// Processes

void gemm_block::load_input()
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
    int32_t gemm_m;
    int32_t gemm_n;
    int32_t gemm_k;
    int32_t offset_c;
    int32_t gemm_batch;
    int32_t offset_b;
    int32_t offset_a;
    int32_t block_size;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        gemm_m = config.gemm_m;
        gemm_n = config.gemm_n;
        gemm_k = config.gemm_k;
        offset_c = config.offset_c;
        gemm_batch = config.gemm_batch;
        offset_b = config.offset_b;
        offset_a = config.offset_a;
        block_size = config.block_size;
    }

    // Load
    {
        HLS_PROTO("load-dma");
        wait();

        bool ping = true;
        uint32_t offset = 0;

        // Batching
        for (uint16_t b = 0; b < gemm_batch; b++)
        {
            wait();
#if (DMA_WORD_PER_BEAT == 0)
            uint32_t length = (gemm_m * gemm_k) + (gemm_n * gemm_k);
#else
            uint32_t length = round_up((gemm_m * gemm_k) + (gemm_n * gemm_k), DMA_WORD_PER_BEAT);
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



void gemm_block::store_output()
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
    int32_t gemm_m;
    int32_t gemm_n;
    int32_t gemm_k;
    int32_t offset_c;
    int32_t gemm_batch;
    int32_t offset_b;
    int32_t offset_a;
    int32_t block_size;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        gemm_m = config.gemm_m;
        gemm_n = config.gemm_n;
        gemm_k = config.gemm_k;
        offset_c = config.offset_c;
        gemm_batch = config.gemm_batch;
        offset_b = config.offset_b;
        offset_a = config.offset_a;
        block_size = config.block_size;
    }

    // Store
    {
        HLS_PROTO("store-dma");
        wait();

        bool ping = true;
#if (DMA_WORD_PER_BEAT == 0)
        uint32_t store_offset = ((gemm_m * gemm_k) + (gemm_n * gemm_k)) * gemm_batch;
#else
        uint32_t store_offset = round_up((gemm_m * gemm_k) + (gemm_n * gemm_k), DMA_WORD_PER_BEAT) * gemm_batch;
#endif
        uint32_t offset = store_offset;

        wait();
        // Batching
        for (uint16_t b = 0; b < gemm_batch; b++)
        {
            wait();
#if (DMA_WORD_PER_BEAT == 0)
            uint32_t length = gemm_m * gemm_n;
#else
            uint32_t length = round_up(gemm_m * gemm_n, DMA_WORD_PER_BEAT);
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


void gemm_block::compute_kernel()
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
    int32_t gemm_m;
    int32_t gemm_n;
    int32_t gemm_k;
    int32_t offset_c;
    int32_t gemm_batch;
    int32_t offset_b;
    int32_t offset_a;
    int32_t block_size;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        gemm_m = config.gemm_m;
        gemm_n = config.gemm_n;
        gemm_k = config.gemm_k;
        offset_c = config.offset_c;
        gemm_batch = config.gemm_batch;
        offset_b = config.offset_b;
        offset_a = config.offset_a;
        block_size = config.block_size;
    }


    // Compute
    bool ping = true;
    {
        for (uint16_t b = 0; b < gemm_batch; b++)
        {
            uint32_t in_length = (gemm_m * gemm_k) + (gemm_n * gemm_k);
            uint32_t out_length = gemm_m * gemm_n;
            int out_rem = out_length;

            for (int in_rem = in_length; in_rem > 0; in_rem -= PLM_IN_WORD)
            {

                uint32_t in_len  = in_rem  > PLM_IN_WORD  ? PLM_IN_WORD  : in_rem;
                uint32_t out_len = out_rem > PLM_OUT_WORD ? PLM_OUT_WORD : out_rem;
                uint32_t acc;

                this->compute_load_handshake();

                // Computing phase implementation
                for (int a = 0; a < (gemm_m/block_size)+1; a++)
                {
                    for (int b = 0; b < (gemm_n/block_size)+1; b++)
                    {
                        for (int c = 0; c < (gemm_k/block_size)+1; c++)
                        {
                            block_size_m = (a == gemm_m/block_size) ? gemm_m%block_size : block_size;
                            block_size_n = (b == gemm_n/block_size) ? gemm_n%block_size : block_size;
                            block_size_k = (c == gemm_k/block_size) ? gemm_k%block_size : block_size;

                            for (int m = 0; m < block_size_m; m++)
                            {
                                for (int n = 0; n < block_size_n; n++)
                                {
                                    acc = 0;

                                    for (int k = 0; k < block_size_k; k++)
                                    {
                                        // TODO: assumes even number of elements in the arrays for ping-pong
                                        if (ping)
                                            acc += plm_in_ping[m*gemm_k + k] * plm_in_ping[gemm_m*gemm_k + n*gemm_k + k];
                                        else
                                            acc += plm_in_pong[m*gemm_k + k] * plm_in_pong[gemm_m*gemm_k + n*gemm_k + k];
                                    }

                                    if (ping)
                                        plm_out_ping[m*gemm_n + n] = acc;
                                    else
                                        plm_out_pong[m*gemm_n + n] = acc;
                                }
                            }
                        }
                    }
                }

                out_rem -= PLM_OUT_WORD;
                this->compute_store_handshake();
                ping = !ping;
            }
        }

        // Conclude
        {
            this->process_done();
        }
    }
}
