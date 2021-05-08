// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "gemm_0_block.hpp"
#include "gemm_0_block_directives.hpp"

// Functions

#include "gemm_0_block_functions.hpp"

// Processes

void gemm_0_block::load_input()
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
    int32_t gemm_batch;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        gemm_m = config.gemm_m;
        gemm_n = config.gemm_n;
        gemm_k = config.gemm_k;
        gemm_batch = config.gemm_batch;
    }

    // Load
    {
        HLS_PROTO("load-dma");
        wait();

        bool ping_m = true;
        uint32_t offset_m = 0;
        uint32_t length_m = gemm_m * gemm_k;

        for (int32_t rem_m = length_m; rem_m > 0; rem_m -= PLM_IN_WORD/2)
        {
            wait();
            uint32_t len_m = rem_m > PLM_IN_WORD/2 ? PLM_IN_WORD/2 : rem_m;

            // fetch data along dimension m
            dma_info_t dma_info(offset_m / DMA_WORD_PER_BEAT, len_m / DMA_WORD_PER_BEAT, DMA_SIZE);
            offset_m += len_m;
            this->dma_read_ctrl.put(dma_info);

            for (uint32_t i = 0; i < len_m; i += DMA_WORD_PER_BEAT)
            {
                HLS_BREAK_DEP(plm_in_ping);
                HLS_BREAK_DEP(plm_in_pong);

                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                dataBv = this->dma_read_chnl.get();
                wait();

                // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
                for (uint32_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                {
                    HLS_UNROLL_SIMPLE;
                    if (ping_m)
                        plm_in_ping[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                    else
                        plm_in_pong[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                    // ESP_REPORT_INFO("[INPUT M] rem_m = %d i = %d k = %d ping_m = %d data = %d",
                    // rem_m, i, k, ping_m, dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64());
                }
            }

            bool ping_n = true;
            uint32_t offset_n = gemm_m * gemm_k;
            uint32_t length_n = gemm_n * gemm_k;

            for (int32_t rem_n = length_n; rem_n > 0; rem_n -= PLM_IN_WORD/2)
            {
                wait();
                uint32_t len_n = rem_n > PLM_IN_WORD/2 ? PLM_IN_WORD/2 : rem_n;

                // fetch data along dimension n
                dma_info_t dma_info(offset_n / DMA_WORD_PER_BEAT, len_n / DMA_WORD_PER_BEAT, DMA_SIZE);
                offset_n += len_n;
                this->dma_read_ctrl.put(dma_info);

                for (uint32_t i = 0; i < len_n; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(plm_in_ping);
                    HLS_BREAK_DEP(plm_in_pong);

                    sc_dt::sc_bv<DMA_WIDTH> dataBv;

                    dataBv = this->dma_read_chnl.get();
                    wait();

                    // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
                    for (uint32_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        if (ping_n)
                            plm_in_ping[PLM_IN_WORD/2 + i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                        else
                            plm_in_pong[PLM_IN_WORD/2 + i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                        // ESP_REPORT_INFO("[INPUT N] rem_m = %d rem_n = %d i = %d k = %d ping_m = %d ping_n = %d data = %d",
                        // rem_m, rem_n, i, k, ping_m, ping_n, dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64());
                    }
                }
                this->load_compute_handshake();
                ping_n = !ping_n;
            }
            ping_m = !ping_m;
        }
    }

    // Conclude
    {
        this->process_done();
    }
}



void gemm_0_block::store_output()
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
    int32_t gemm_batch;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        gemm_m = config.gemm_m;
        gemm_n = config.gemm_n;
        gemm_k = config.gemm_k;
        gemm_batch = config.gemm_batch;
    }

    // Store
    {
        HLS_PROTO("store-dma");
        wait();

        uint32_t offset = (gemm_m * gemm_k) + (gemm_n * gemm_k);

        uint32_t length = gemm_m * gemm_n;

        bool ping = true;

        for (int32_t rem = length; rem > 0; rem -= PLM_OUT_WORD)
        {
            this->store_compute_handshake();

            uint32_t len = rem > PLM_OUT_WORD ? PLM_OUT_WORD : rem;

            dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
            offset += len;

            this->dma_write_ctrl.put(dma_info);

            for (uint32_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
            {
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                // Read from PLM
                wait();
                for (uint32_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                {
                    HLS_UNROLL_SIMPLE;
                    if (ping)
                        dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_out_ping[i + k];
                    else
                        dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_out_pong[i + k];
                    // ESP_REPORT_INFO("[OUTPUT] rem = %d i = %d k = %d ping = %d data = %d",
                    // rem, i, k, ping, dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64());
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


void gemm_0_block::compute_kernel()
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
    int32_t gemm_batch;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        gemm_m = config.gemm_m;
        gemm_n = config.gemm_n;
        gemm_k = config.gemm_k;
        gemm_batch = config.gemm_batch;
    }

    // Compute
    bool ping_m = true;
    bool ping_n = true;
    {
        uint32_t length_m = gemm_m * gemm_k;
        uint32_t length_n = gemm_n * gemm_k;

        uint32_t acc = 0;

        uint32_t out_pos = 0;

        uint32_t row_num = 0;

        for (int32_t rem_m = length_m; rem_m > 0; rem_m -= PLM_IN_WORD/2)
        {   
            uint32_t col_num = 0;

            for (int32_t rem_n = length_n; rem_n > 0; rem_n -= PLM_IN_WORD/2)
            {
                this->compute_load_handshake();
                
                // ESP_REPORT_INFO("[COMPUTE] Starting block row/batch = %d col = %d", row_num, col_num);

                // Computing phase implementation
                for (uint32_t m = 0; m < (PLM_IN_WORD/2)/gemm_n; m++)
                {
                    for (uint32_t n = 0; n < (PLM_IN_WORD/2)/gemm_k; n++)
                    {
                        for (uint32_t k = 0; k < gemm_k; k++)
                        {
                            if (ping_m & ping_n) {
                                acc += plm_in_ping[m*gemm_k + k] * plm_in_ping[(PLM_IN_WORD/2) + n*gemm_k + k];
                                // ESP_REPORT_INFO("[COMPUTE] ping_m = %d ping_n = %d m = %d n = %d k = %d acc = %d", ping_m, ping_n, m, n, k, acc);
                            } else if (ping_m & (!ping_n)) {
                                acc += plm_in_ping[m*gemm_k + k] * plm_in_pong[(PLM_IN_WORD/2) + n*gemm_k + k];
                                // ESP_REPORT_INFO("[COMPUTE] ping_m = %d ping_n = %d m = %d n = %d k = %d acc = %d", ping_m, ping_n, m, n, k, acc);
                            } else if ((!ping_m) & ping_n) {
                                acc += plm_in_pong[m*gemm_k + k] * plm_in_ping[(PLM_IN_WORD/2) + n*gemm_k + k];
                                // ESP_REPORT_INFO("[COMPUTE] ping_m = %d ping_n = %d m = %d n = %d k = %d acc = %d", ping_m, ping_n, m, n, k, acc);
                            } else {
                                acc += plm_in_pong[m*gemm_k + k] * plm_in_pong[(PLM_IN_WORD/2) + n*gemm_k + k];
                                // ESP_REPORT_INFO("[COMPUTE] ping_m = %d ping_n = %d m = %d n = %d k = %d acc = %d", ping_m, ping_n, m, n, k, acc);
                            }
                        }
                        if (ping_m)
                            plm_out_ping[out_pos + m*gemm_n + n] = acc;
                        else
                            plm_out_pong[out_pos + m*gemm_n + n] = acc;
                        // ESP_REPORT_INFO("[COMPUTE] output = %d acc = %d", out_pos + m*gemm_n + n , acc);
                        acc = 0;
                    }
                }
                out_pos += (PLM_IN_WORD/2)/gemm_k;
                ping_n = !ping_n;
                col_num++;
            }
            out_pos = 0;
            ping_m = !ping_m;
            row_num++;

            this->compute_store_handshake();
        }

        // Conclude
        {
            this->process_done();
        }
    }
}
