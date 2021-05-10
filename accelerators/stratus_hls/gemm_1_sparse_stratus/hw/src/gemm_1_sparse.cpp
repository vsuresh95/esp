// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "gemm_1_sparse.hpp"
#include "gemm_1_sparse_directives.hpp"

// Functions

#include "gemm_1_sparse_functions.hpp"

// Processes

void gemm_1_sparse::load_input()
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
    int32_t h_order_size;
    int32_t gemm_m;
    int32_t gemm_n;
    int32_t gemm_k;
    int32_t gemm_batch;
    int32_t var_size;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        h_order_size = config.h_order_size;
        gemm_m = config.gemm_m;
        gemm_n = config.gemm_n;
        gemm_k = config.gemm_k;
        gemm_batch = config.gemm_batch;
        var_size = config.var_size;
    }

    // Load
    {
        HLS_PROTO("load-dma");

        // Read variables and H order
        {
            wait();

            uint32_t offset = 0;

            uint32_t length = round_up(var_size + h_order_size, DMA_WORD_PER_BEAT);

            dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, length / DMA_WORD_PER_BEAT, DMA_SIZE);

            offset += length;

            this->dma_read_ctrl.put(dma_info);

            for (uint16_t i = 0; i < length; i += DMA_WORD_PER_BEAT)
            {
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                dataBv = this->dma_read_chnl.get();
                wait();

                // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
                for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                {
                    HLS_UNROLL_SIMPLE;
                    plm_in_res[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                }
            }
        }

        // load both input matrices block by block
        {
            wait();
                
            // fetch blocks for as many times as there are elements in variables
            for (int32_t rem_m = 0; rem_m < var_size; rem_m+=2)
            {
                bool ping = true;

                uint32_t offset_h = var_size + h_order_size + (gemm_m * gemm_k);

                wait();
                
                // fetch blocks for as many times as there are elements in h_order
                for (int32_t rem_n = 0; rem_n < h_order_size; rem_n+=2)
                {
                    uint32_t offset_cov = var_size + h_order_size + (plm_in_res[rem_m] /* var->id() */ * gemm_k) + plm_in_res[var_size + rem_n] /* meas_var->id() */;
                    uint32_t len_cov_m = plm_in_res[rem_m + 1]; /* var->size() */ 
                    uint32_t len_cov_n = plm_in_res[var_size + rem_n + 1]; /* meas_var->size() */

                    wait();
                
                    // fetch the current block of the first matrix
                    for (uint32_t row = 0; row < len_cov_m; row++)
                    {
                        dma_info_t dma_info(offset_cov / DMA_WORD_PER_BEAT, len_cov_n / DMA_WORD_PER_BEAT, DMA_SIZE);
                        this->dma_read_ctrl.put(dma_info);

                        offset_cov += gemm_k;

                        wait();
                
                        for (uint32_t i = 0; i < len_cov_n; i += DMA_WORD_PER_BEAT)
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
                                if (ping)
                                    plm_in_ping[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                                else
                                    plm_in_pong[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                                ESP_REPORT_INFO("[INPUT COV] rem_m = %d row = %d i = %d k = %d ping = %d data = %d",
                                rem_m, row, i, k, ping, dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64());
                            }
                        }
                    }
                
                    uint32_t len_h = plm_in_res[var_size + rem_n] /* meas_var->id() */;

                    wait();

                    // fetch the current block of the second matrix
                    for (uint32_t row = 0; row < gemm_n; row++)
                    {
                        ESP_REPORT_INFO("[INPUT H] Entered input H, len_h = %d\n", len_h);

                        dma_info_t dma_info(offset_h / DMA_WORD_PER_BEAT, len_h / DMA_WORD_PER_BEAT, DMA_SIZE);
                        this->dma_read_ctrl.put(dma_info);

                        offset_h += gemm_k;

                        wait();

                        for (uint32_t i = 0; i < len_h; i += DMA_WORD_PER_BEAT)
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
                                if (ping)
                                    plm_in_ping[(PLM_IN_WORD/2) + i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                                else
                                    plm_in_pong[(PLM_IN_WORD/2) + i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                                ESP_REPORT_INFO("[INPUT H] rem_m = %d row = %d i = %d k = %d ping = %d data = %d",
                                rem_m, row, i, k, ping, dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64());
                            }
                        }
                    }
                    offset_h += len_h;
                    
                    this->load_compute_handshake();
                    ping = !ping;
                }
            }
        }
    }

    // Conclude
    {
        this->process_done();
    }
}

void gemm_1_sparse::store_output()
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
    int32_t h_order_size;
    int32_t gemm_m;
    int32_t gemm_n;
    int32_t gemm_k;
    int32_t gemm_batch;
    int32_t var_size;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        h_order_size = config.h_order_size;
        gemm_m = config.gemm_m;
        gemm_n = config.gemm_n;
        gemm_k = config.gemm_k;
        gemm_batch = config.gemm_batch;
        var_size = config.var_size;
    }

    // Store
    {
        HLS_PROTO("store-dma");
        wait();

        // store blocks for as many times as there are elements in variables
        for (int32_t rem_m = 0; rem_m < var_size; rem_m+=2)
        {
            bool ping = true;

            uint32_t offset = var_size + h_order_size + (gemm_m * gemm_k) + (gemm_n * gemm_k);

            wait();

            // store blocks for as many times as there are elements in h_order
            for (int32_t rem_n = 0; rem_n < h_order_size; rem_n+=2)
            {
                this->store_compute_handshake();

                uint32_t len = plm_in_res[rem_m + 1] /* var->size() */  * gemm_n;

                dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
                offset += len;

                this->dma_write_ctrl.put(dma_info);

                wait();

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
                        ESP_REPORT_INFO("[OUTPUT] rem_m = %d row = %d i = %d k = %d ping = %d data = %d",
                        rem_m, rem_n, i, k, ping, dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64());
                    }
                    this->dma_write_chnl.put(dataBv);
                }
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


void gemm_1_sparse::compute_kernel()
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
    int32_t h_order_size;
    int32_t gemm_m;
    int32_t gemm_n;
    int32_t gemm_k;
    int32_t gemm_batch;
    int32_t var_size;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        h_order_size = config.h_order_size;
        gemm_m = config.gemm_m;
        gemm_n = config.gemm_n;
        gemm_k = config.gemm_k;
        gemm_batch = config.gemm_batch;
        var_size = config.var_size;
    }


    // Compute
    bool ping_out = true;
    uint32_t acc = 0;
    {
        // compute for as many times as there are elements in variables
        for (int32_t rem_m = 0; rem_m < var_size; rem_m+=2)
        {
            bool ping_in = true;

            // compute for as many times as there are elements in h_order
            for (int32_t rem_n = 0; rem_n < h_order_size; rem_n+=2)
            {
                this->compute_load_handshake();

                uint32_t len_cov_m = plm_in_res[rem_m + 1]; /* var->size() */ 
                uint32_t len_cov_n = plm_in_res[var_size + rem_n + 1]; /* meas_var->size() */

                // Computing phase implementation
                for (uint32_t m = 0; m < len_cov_m; m++)
                {
                    for (uint32_t n = 0; n < gemm_n; n++)
                    {
                        for (uint32_t k = 0; k < len_cov_n; k++)
                        {
                            if (ping_in) {
                                acc += plm_in_ping[m*len_cov_n + k] * plm_in_ping[(PLM_IN_WORD/2) + n*len_cov_n + k];
                                ESP_REPORT_INFO("[COMPUTE] ping_out = %d ping_in = %d m = %d n = %d k = %d acc = %d", ping_out, ping_in, m, n, k, acc);
                            } else {
                                acc += plm_in_pong[m*len_cov_n + k] * plm_in_pong[(PLM_IN_WORD/2) + n*len_cov_n + k];
                                ESP_REPORT_INFO("[COMPUTE] ping_out = %d ping_in = %d m = %d n = %d k = %d acc = %d", ping_out, ping_in, m, n, k, acc);
                            }
                        }
                        if (ping_out)
                            plm_out_ping[m*gemm_n + n] += acc;
                        else
                            plm_out_pong[m*gemm_n + n] += acc;
                        ESP_REPORT_INFO("[COMPUTE] output = %d acc = %d", m*gemm_n + n , acc);
                        acc = 0;
                    }
                }
                ping_in = !ping_in;
            }
            this->compute_store_handshake();               
            ping_out = !ping_out;
        }

        // Conclude
        {
            this->process_done();
        }
    }
}
