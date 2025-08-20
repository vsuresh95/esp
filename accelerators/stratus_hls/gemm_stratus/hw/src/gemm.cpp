// Copyright (c) 2011-2019 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "gemm.hpp"
#include "gemm_directives.hpp"

// Functions

#include "gemm_functions.hpp"

// Processes

void gemm::load_input()
{
    // Reset
    {
        HLS_PROTO("load-reset");
        this->reset_load_input();
        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t dim_m;
    int32_t dim_n;
    int32_t dim_k;
    int32_t input_payload_offset;
    int32_t weight_payload_base;
    int32_t weight_payload_offset;
    bool in_pingpong;
    bool pingpong;
    bool kill_task;
    int32_t tile_size_m, tile_size_n, tile_size_k;
    {
        HLS_PROTO("load-config");
        cfg.wait_for_config(); // config process
    }

    // Load
    while(true)
    {
        HLS_PROTO("load-loop");

        this->load_avu_ready_handshake();
        wait();

        // Read config information for current context
        {
            HLS_PROTO("read-load-config");
            conf_info_t config = this->conf_info.read();

            HLS_FLATTEN_ARRAY(config.dim_m);
            HLS_FLATTEN_ARRAY(config.dim_n);
            HLS_FLATTEN_ARRAY(config.dim_k);
            HLS_FLATTEN_ARRAY(config.input_base);
            HLS_FLATTEN_ARRAY(config.weight_base);

            // User-defined config code
            /* <<--local-params-->> */
            dim_m = config.dim_m[current_context_int];
            dim_n = config.dim_n[current_context_int];
            dim_k = config.dim_k[current_context_int];
            weight_payload_base = config.weight_base[current_context_int];
            input_payload_offset = config.input_base[current_context_int] + PAYLOAD_OFFSET;
            in_pingpong = true;
            pingpong = true;
            kill_task = false;
            wait();
        }

        {
            HLS_PROTO("load-input");

            // Tile sizes for larger matrices
            tile_size_k = dim_k;
            tile_size_m = TILE_SIZE / tile_size_k;
            tile_size_n = (tile_size_m / DMA_WORD_PER_BEAT) * DMA_WORD_PER_BEAT;
            wait();

            // Load input matrix
            for (int tile_m = 0; tile_m < dim_m && !kill_task; tile_m += tile_size_m)
            {
                // Accomodate dim_m not being multiple of tile_size_m
                unsigned actual_tile_m = (tile_m + tile_size_m < dim_m) ? tile_size_m : dim_m - tile_m;
                acquire_load_dma_accel();

                // Accomododate offset or size not being a multiple of DMA_WORD_PER_BEAT
                int32_t aligned_input_offset = input_payload_offset / DMA_WORD_PER_BEAT;
                int32_t input_alignment_skew = input_payload_offset - (aligned_input_offset * DMA_WORD_PER_BEAT);
                int32_t aligned_input_words = (input_alignment_skew + (actual_tile_m * tile_size_k) + DMA_WORD_PER_BEAT - 1) / DMA_WORD_PER_BEAT;

                // Create a DMA request
                dma_info_t dma_info_1(aligned_input_offset, aligned_input_words, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                uint32_t plm_in_index = 0;

                wait();
                this->dma_read_ctrl.put(dma_info_1);

                // Number of invalid words at the beginning of the burst
                int32_t begin_input_invalid = input_alignment_skew;
                // Number of invalid words at the end of the burst
                int32_t end_input_invalid = begin_input_invalid + (actual_tile_m * tile_size_k);

                for (int i = 0; i < aligned_input_words * DMA_WORD_PER_BEAT; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_ARRAY_DEPENDENCY(plm_in_ping);
                    HLS_BREAK_ARRAY_DEPENDENCY(plm_in_pong);

                    dataBv = this->dma_read_chnl.get();
                    wait();

                    for (uint16_t j = 0; j < DMA_WORD_PER_BEAT; j++)
                    {
                        HLS_UNROLL_SIMPLE;
                        if (i + j >= begin_input_invalid && i + j < end_input_invalid) {
                            if (in_pingpong) plm_in_ping[plm_in_index++] = dataBv.range((j+1) * DATA_WIDTH - 1, j * DATA_WIDTH).to_uint64();
                            else plm_in_pong[plm_in_index++] = dataBv.range((j+1) * DATA_WIDTH - 1, j * DATA_WIDTH).to_uint64();
                        }
                    }
                }

                release_load_dma_accel();
                input_payload_offset += actual_tile_m * tile_size_k;

                // Load weight matrix
                for (int tile_n = 0; tile_n < dim_n && !kill_task; tile_n += tile_size_n)
                {
                    // Accomodate dim_n not being multiple of tile_size_n
                    // Ensure non-tiled loads are not tiled.
                    unsigned actual_tile_n, actual_tile_k;
                    if (tile_n == 0) {
                        actual_tile_n = (tile_size_n < dim_n) ? tile_size_n : dim_n * dim_k;
                        actual_tile_k = (tile_size_n < dim_n) ? tile_size_k : 1;
                    } else {
                        actual_tile_n = (tile_n + tile_size_n < dim_n) ? tile_size_n : dim_n - tile_n;
                        actual_tile_k = tile_size_k;
                    }

                    acquire_load_dma_accel();
                    weight_payload_offset = weight_payload_base + tile_n;

                    for (int k = 0; k < actual_tile_k; k++)
                    {
                        // Accomododate offset or size not being a multiple of DMA_WORD_PER_BEAT
                        int32_t aligned_wgt_offset = weight_payload_offset / DMA_WORD_PER_BEAT;
                        int32_t wgt_alignment_skew = weight_payload_offset - (aligned_wgt_offset * DMA_WORD_PER_BEAT);
                        int32_t aligned_wgt_words = (wgt_alignment_skew + actual_tile_n + DMA_WORD_PER_BEAT - 1) / DMA_WORD_PER_BEAT;

                        // Create a DMA request
                        dma_info_t dma_info_2(aligned_wgt_offset, aligned_wgt_words, DMA_SIZE);
                        uint32_t plm_wgt_index = k * actual_tile_n;

                        wait();
                        this->dma_read_ctrl.put(dma_info_2);

                        // Number of invalid words at the beginning of the burst
                        int32_t begin_wgt_invalid = wgt_alignment_skew;
                        // Number of invalid words at the end of the burst
                        int32_t end_wgt_invalid = begin_wgt_invalid + actual_tile_n;

                        for (int i = 0; i < aligned_wgt_words * DMA_WORD_PER_BEAT; i += DMA_WORD_PER_BEAT)
                        {
                            HLS_BREAK_ARRAY_DEPENDENCY(plm_wgt_ping);
                            HLS_BREAK_ARRAY_DEPENDENCY(plm_wgt_pong);

                            dataBv = this->dma_read_chnl.get();
                            wait();

                            for (uint16_t j = 0; j < DMA_WORD_PER_BEAT; j++)
                            {
                                HLS_UNROLL_SIMPLE;
                                if (i + j >= begin_wgt_invalid && i + j < end_wgt_invalid) {
                                    if (pingpong) plm_wgt_ping[plm_wgt_index++] = dataBv.range((j+1) * DATA_WIDTH - 1, j * DATA_WIDTH).to_int64();
                                    else plm_wgt_pong[plm_wgt_index++] = dataBv.range((j+1) * DATA_WIDTH - 1, j * DATA_WIDTH).to_int64();
                                }
                            }
                        }

                        weight_payload_offset += dim_n;
                    }

                    release_load_dma_accel();
                    this->load_compute_handshake();
                    wait();

                    // Check if a context switch was triggered
                    while (output_poll_complete == POLL_PENDING) wait();
                    if (output_poll_complete == EXEC_KILL) kill_task = true;

                    if (!kill_task) pingpong = !pingpong;
                }
                if (!kill_task) in_pingpong = !in_pingpong;
                wait();
            }
        }
        this->load_avu_done_handshake();
        wait();
    }
} // Function : load_input

void gemm::store_output()
{
    // Reset
    {
        HLS_PROTO("store-reset");
        this->reset_store_output();
        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t dim_m;
    int32_t dim_n;
    int32_t dim_k;
    int32_t output_payload_base;
    int32_t output_payload_offset;
    bool pingpong;
    int32_t tile_size_m, tile_size_n, tile_size_k;
    {
        HLS_PROTO("store-config");
        cfg.wait_for_config(); // config process
    }

    // Store
    while(true)
    {
        HLS_PROTO("store-loop");

        this->store_avu_ready_handshake();
        wait();

        // Read config information for current context
        {
            HLS_PROTO("read-store-config");
            conf_info_t config = this->conf_info.read();

            HLS_FLATTEN_ARRAY(config.dim_m);
            HLS_FLATTEN_ARRAY(config.dim_n);
            HLS_FLATTEN_ARRAY(config.dim_k);
            HLS_FLATTEN_ARRAY(config.output_base);

            // User-defined config code
            /* <<--local-params-->> */
            dim_m = config.dim_m[current_context_int];
            dim_n = config.dim_n[current_context_int];
            dim_k = config.dim_k[current_context_int];
            output_payload_base = config.output_base[current_context_int] + PAYLOAD_OFFSET;
            pingpong = true;
            wait();
        }
        {
            HLS_PROTO("store-data");

            // Tile sizes for larger matrices
            tile_size_k = dim_k;
            tile_size_m = TILE_SIZE / tile_size_k;
            tile_size_n = (tile_size_m / DMA_WORD_PER_BEAT) * DMA_WORD_PER_BEAT;
            wait();

            for (int tile_m = 0; tile_m < dim_m; tile_m += tile_size_m)
            {
                unsigned actual_tile_m = (tile_m + tile_size_m < dim_m) ? tile_size_m : dim_m - tile_m;

                for (int tile_n = 0; tile_n < dim_n; tile_n += tile_size_n)
                {
                    // Accomodate dim_n not being multiple of tile_size_n
                    // Ensure non-tiled stores are not tiled.
                    unsigned actual_tile_m_2, actual_tile_n;
                    if (tile_n == 0) {
                        actual_tile_n = (tile_size_n < dim_n) ? tile_size_n : dim_m * dim_n;
                        actual_tile_m_2 = (tile_size_n < dim_n) ? actual_tile_m : 1;
                    } else {
                        actual_tile_n = (tile_n + tile_size_n < dim_n) ? tile_size_n : dim_n - tile_n;
                        actual_tile_m_2 = actual_tile_m;
                    }

                    this->store_compute_handshake();
                    acquire_store_dma_accel();

                    output_payload_offset = output_payload_base + (tile_m * dim_n) + tile_n;

                    for (int m = 0; m < actual_tile_m_2; m++)
                    {
                        // Accomododate offset or size not being a multiple of DMA_WORD_PER_BEAT
                        int32_t aligned_output_offset = output_payload_offset / DMA_WORD_PER_BEAT;
                        int32_t output_alignment_skew = output_payload_offset - (aligned_output_offset * DMA_WORD_PER_BEAT);
                        int32_t aligned_output_words = (output_alignment_skew + actual_tile_n + DMA_WORD_PER_BEAT - 1) / DMA_WORD_PER_BEAT;

                        dma_info_t dma_info(aligned_output_offset, aligned_output_words, DMA_SIZE);
                        sc_dt::sc_bv<DMA_WIDTH> dataBv;
                        uint32_t plm_output_index = m * actual_tile_n;

                        wait();
                        this->dma_write_ctrl.put(dma_info);

                        for (int i = 0; i < aligned_output_words * DMA_WORD_PER_BEAT; i += DMA_WORD_PER_BEAT)
                        {
                            HLS_BREAK_ARRAY_DEPENDENCY(plm_out_ping);
                            HLS_BREAK_ARRAY_DEPENDENCY(plm_out_pong);
                            wait();

                            for (uint16_t j = 0; j < DMA_WORD_PER_BEAT; j++)
                            {
                                HLS_UNROLL_SIMPLE;
                                if (pingpong) dataBv.range((j+1) * DATA_WIDTH - 1, j * DATA_WIDTH) = plm_out_ping[plm_output_index++];
                                else dataBv.range((j+1) * DATA_WIDTH - 1, j * DATA_WIDTH) = plm_out_pong[plm_output_index++];
                            }

                            this->dma_write_chnl.put(dataBv);
                        }
                        // Wait till the last write is accepted at the cache
                        wait();
                        while (!(this->dma_write_chnl.ready)) wait();

                        output_payload_offset += dim_n;
                    }
                    release_store_dma_accel();

                    pingpong = !pingpong;
                    wait();
                }
                wait();
            }
        }
        this->store_avu_done_handshake();
        wait();
    }
} // Function : store_output

void gemm::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-reset");
        this->reset_compute_kernel();
        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t dim_n;
    int32_t dim_m;
    int32_t dim_k;
    bool in_pingpong;
    bool pingpong;
    bool kill_task;
    int32_t tile_size_m, tile_size_n, tile_size_k;
    {
        HLS_PROTO("compute-config");
        cfg.wait_for_config(); // config process
        wait();
    }

    while(true)
    {
        // Read config information for current context
        {
            HLS_PROTO("read-compute-config");
            this->compute_avu_ready_handshake();
            wait();
            
            conf_info_t config = this->conf_info.read();

            HLS_FLATTEN_ARRAY(config.dim_m);
            HLS_FLATTEN_ARRAY(config.dim_n);
            HLS_FLATTEN_ARRAY(config.dim_k);

            // User-defined config code
            /* <<--local-params-->> */
            dim_m = config.dim_m[current_context_int];
            dim_n = config.dim_n[current_context_int];
            dim_k = config.dim_k[current_context_int];
            in_pingpong = true;
            pingpong = true;
            kill_task = false;
            wait();
        }
        // Compute GeMM
        {
            // Tile sizes for larger matrices
            tile_size_k = dim_k;
            tile_size_m = TILE_SIZE / tile_size_k;
            tile_size_n = (tile_size_m / DMA_WORD_PER_BEAT) * DMA_WORD_PER_BEAT;

            for (int tile_m = 0; tile_m < dim_m && !kill_task; tile_m += tile_size_m)
            {
                unsigned actual_tile_m = (tile_m + tile_size_m < dim_m) ? tile_size_m : dim_m - tile_m;

                for (int tile_n = 0; tile_n < dim_n && !kill_task; tile_n += tile_size_n)
                {
                    unsigned actual_tile_n = (tile_n + tile_size_n < dim_n) ? tile_size_n : dim_n - tile_n;

                    this->compute_load_handshake();

                    // Iterate over register block across M dimension
                    for (unsigned block_m = 0; block_m < actual_tile_m; block_m += BLOCK_SIZE)
                    {
                        // Iterate over register block across N dimension
                        for (unsigned block_n = 0; block_n < actual_tile_n; block_n += BLOCK_SIZE)
                        {
                            // Iterate over register block across K dimension
                            for (unsigned block_k = 0; block_k < dim_k; block_k += BLOCK_SIZE)
                            {
                                FPDATA regs_m[BLOCK_SIZE];
                                FPDATA regs_n[BLOCK_SIZE];
                                FPDATA regs_mul[BLOCK_SIZE];
                                FPDATA regs_valid[BLOCK_SIZE];
                                FPDATA regs_acc;
                                HLS_FLATTEN_ARRAY(regs_m);
                                HLS_FLATTEN_ARRAY(regs_n);
                                HLS_FLATTEN_ARRAY(regs_mul);
                                HLS_FLATTEN_ARRAY(regs_valid);

                                // If the remainder of is not a multiple of block_size, we will zero out
                                // the read elements with a valid vector
                                for (unsigned elem_k = 0; elem_k < BLOCK_SIZE; elem_k++)
                                {
                                    regs_valid[elem_k] = (block_k + elem_k < dim_k) ? 1 : 0;
                                }

                                // Perform block-level multiply - M dimension
                                for (unsigned row_m = 0; row_m < BLOCK_SIZE; row_m++)
                                {
                                    HLS_BREAK_ARRAY_DEPENDENCY(plm_in_ping);
                                    HLS_BREAK_ARRAY_DEPENDENCY(plm_in_pong);

                                    unsigned elem_m = block_m + row_m;
                                    unsigned idx_mk = (elem_m * dim_k) + block_k;

                                    // If the remainder is not a multiple of block_size, break out of the loop
                                    if (elem_m >= actual_tile_m) continue;

                                    // read Mth block across K dimension of matrix 1 from PLM into a register array
                                    for (unsigned elem_k = 0; elem_k < BLOCK_SIZE; elem_k++)
                                    {
                                        HLS_UNROLL_LOOP(ON, "read_plm_m");
                                        if (in_pingpong) regs_m[elem_k] = regs_valid[elem_k] * INT2FP(plm_in_ping[idx_mk + elem_k]);
                                        else regs_m[elem_k] = regs_valid[elem_k] * INT2FP(plm_in_pong[idx_mk + elem_k]);
                                    }

                                    // Perform block-level multiply - N dimension
                                    for (unsigned row_n = 0; row_n < BLOCK_SIZE; row_n++)
                                    {
                                        HLS_BREAK_ARRAY_DEPENDENCY(plm_wgt_ping);
                                        HLS_BREAK_ARRAY_DEPENDENCY(plm_wgt_pong);
                                        HLS_BREAK_ARRAY_DEPENDENCY(plm_out_ping);
                                        HLS_BREAK_ARRAY_DEPENDENCY(plm_out_pong);

                                        unsigned elem_n = block_n + row_n;                                                                                
                                        unsigned idx_kn = (block_k * actual_tile_n) + elem_n;
                                        unsigned idx_mn = (elem_m * actual_tile_n) + elem_n;

                                        // If the remainder is not a multiple of block_size, break out of the loop
                                        if (elem_n >= actual_tile_n) continue;

                                        // read Nth block across K dimension of matrix 2 from PLM into a register array
                                        for (unsigned elem_k = 0; elem_k < BLOCK_SIZE; elem_k++)
                                        {
                                            HLS_UNROLL_LOOP(ON, "read_plm_n");
                                            if (pingpong) regs_n[elem_k] = regs_valid[elem_k] * INT2FP(plm_wgt_ping[idx_kn + (elem_k * actual_tile_n)]);
                                            else regs_n[elem_k] = regs_valid[elem_k] * INT2FP(plm_wgt_pong[idx_kn + (elem_k * actual_tile_n)]);
                                        }

                                        // multiply all elements stored in regs_1 and regs_2
                                        for (unsigned elem_k = 0; elem_k < BLOCK_SIZE; elem_k++)
                                        {
                                            HLS_UNROLL_LOOP(ON, "multiply_k");
                                            regs_mul[elem_k] = regs_m[elem_k] * regs_n[elem_k];                                            
                                        }

                                        // read the previous partial sum, or not
                                        if (block_k == 0) {
                                            regs_acc = 0;
                                        } else {
                                            if (pingpong) regs_acc = INT2FP(plm_out_ping[idx_mn]);
                                            else regs_acc = INT2FP(plm_out_pong[idx_mn]);
                                        }

                                        // Accumulate all products
                                        for (unsigned elem_k = 0; elem_k < BLOCK_SIZE; elem_k++)
                                        {
                                            HLS_UNROLL_LOOP(ON, "accumulate_k_0");
                                            regs_acc += regs_mul[elem_k];
                                        }

                                        // write the partial sum to PLM
                                        {
                                            if (pingpong) plm_out_ping[idx_mn] = FP2INT(regs_acc);
                                            else plm_out_pong[idx_mn] = FP2INT(regs_acc);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    // Check if a context switch was triggered
                    {
                        HLS_PROTO("compute-check-switch");
                        while (output_poll_complete == POLL_PENDING) wait();
                        if (output_poll_complete == EXEC_KILL) kill_task = true;
                    }

                    if (!kill_task) {
                        this->compute_store_handshake();
                        pingpong = !pingpong;
                    }
                }
                if (!kill_task) in_pingpong = !in_pingpong;
            }
        }
        {
            HLS_PROTO("compute-done");
            this->compute_avu_done_handshake();
            wait();
        }
    } // while (true)
} // Function : compute_kernel
