// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "gemm_5_2d_block.hpp"
#include "gemm_5_2d_block_directives.hpp"

// Functions

#include "gemm_5_2d_block_functions.hpp"

// Processes

void gemm_5_2d_block::load_input()
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
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        gemm_m = config.gemm_m;
        gemm_n = config.gemm_n;
        gemm_k = config.gemm_k;
    }

    // Load
    {
        HLS_PROTO("load-dma");
        wait();

        bool ping = true;

        // Moving in M dimension for matrix 1, and moving to new row of output
        for (uint32_t num_m = 0; num_m < gemm_m/BLOCK_SIZE; num_m++)
        {
            wait();
            // Moving in N dimension for matrix 2, and moving to new column of output
            for (uint32_t num_n = 0; num_n < gemm_n/BLOCK_SIZE; num_n++)
            {
                wait();
                // Moving in K dimension for both matrices to fully compute output
                for (uint32_t num_k = 0; num_k < gemm_k/BLOCK_SIZE; num_k++)
                {
                    wait();
                    // read the two 64x64 blocks from matrix 1 & 2 - only offset's change depending on 
                    // position in outer loops
                    for (uint32_t mat_num = 0; mat_num < 2; mat_num++)
                    {
                        uint32_t offset;

                        wait();

                        // offset from start + vertical offset + horizontal offset
                        if (mat_num)
                            offset = (gemm_m * gemm_k) + (num_n * BLOCK_SIZE * gemm_k) + (num_k * BLOCK_SIZE);
                        else
                            offset = (num_m * BLOCK_SIZE * gemm_k) + (num_k * BLOCK_SIZE);
                    
                        // each new row of the block
                        for (uint32_t row_num = 0; row_num < BLOCK_SIZE; row_num++)
                        {
                            wait();

                            dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, BLOCK_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                            this->dma_read_ctrl.put(dma_info);
                            offset += gemm_k;

                            for (uint32_t i = 0; i < BLOCK_SIZE; i += DMA_WORD_PER_BEAT)
                            {
                                HLS_BREAK_DEP(plm_in_ping);
                                HLS_BREAK_DEP(plm_in_pong);

                                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                                dataBv = this->dma_read_chnl.get();
                                wait();

                                // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
                                for (uint32_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                                {
                                    uint32_t plm_index = (mat_num * (PLM_IN_WORD/2)) + (row_num * BLOCK_SIZE) + i + k;
                                    HLS_UNROLL_SIMPLE;
                                    if (ping)
                                        plm_in_ping[plm_index] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                                    else
                                        plm_in_pong[plm_index] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                                }
                            }
                        }
                    }
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



void gemm_5_2d_block::store_output()
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
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        gemm_m = config.gemm_m;
        gemm_n = config.gemm_n;
        gemm_k = config.gemm_k;
    }

    // Store
    {
        HLS_PROTO("store-dma");
        wait();

        bool ping = true;

        // Moving in M dimension to new row of output
        for (uint32_t num_m = 0; num_m < gemm_m/BLOCK_SIZE; num_m++)
        {
            wait();
            // Moving in N dimension to new column of output
            for (uint32_t num_n = 0; num_n < gemm_n/BLOCK_SIZE; num_n++)
            {
                this->store_compute_handshake();

                uint32_t offset = (gemm_m * gemm_k) + (gemm_n * gemm_k) + (num_m * BLOCK_SIZE * gemm_n) + (num_n * BLOCK_SIZE);

                // each new row of the block
                for (uint32_t row_num = 0; row_num < BLOCK_SIZE; row_num++)
                {
                    wait();

                    dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, BLOCK_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                    offset += gemm_n;

                    this->dma_write_ctrl.put(dma_info);

                    for (uint32_t i = 0; i < BLOCK_SIZE ; i += DMA_WORD_PER_BEAT)
                    {
                        sc_dt::sc_bv<DMA_WIDTH> dataBv;

                        // Read from PLM
                        wait();
                        for (uint32_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                        {
                            HLS_UNROLL_SIMPLE;
                            if (ping)
                                dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_out_ping[(row_num * BLOCK_SIZE) + i + k];
                            else
                                dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_out_pong[(row_num * BLOCK_SIZE) + i + k];
                        }
                        this->dma_write_chnl.put(dataBv);
                    }
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


void gemm_5_2d_block::compute_kernel()
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
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        gemm_m = config.gemm_m;
        gemm_n = config.gemm_n;
        gemm_k = config.gemm_k;
    }

    // Compute
    bool ping = true;
    bool ping_out = true;
    {
        // Moving in M dimension for matrix 1, and moving to new row of output
        for (uint32_t num_m = 0; num_m < gemm_m/BLOCK_SIZE; num_m++)
        {
            // Moving in N dimension for matrix 2, and moving to new column of output
            for (uint32_t num_n = 0; num_n < gemm_n/BLOCK_SIZE; num_n++)
            {
                // Moving in K dimension for both matrices to fully compute output
                for (uint32_t num_k = 0; num_k < gemm_k/BLOCK_SIZE; num_k++)
                {
                    this->compute_load_handshake();

                    uint32_t regs_m[64];
                    uint32_t regs_n[64];
                    uint32_t regs_mul[64];
                    HLS_FLATTEN_ARRAY(regs_m);
                    HLS_FLATTEN_ARRAY(regs_n);
                    HLS_FLATTEN_ARRAY(regs_mul);

                    // Computing phase implementation
                    for (uint32_t m = 0; m < BLOCK_SIZE; m++)
                    {
                        // read the entire row for matrix 1 from PLM into an array
                        for (int elem_m = 0; elem_m < BLOCK_SIZE; elem_m++)
                        {
                            HLS_UNROLL_LOOP(COMPLETE, 16, "read_plm_m");
                            HLS_BREAK_ARRAY_DEPENDENCY(plm_in_ping);
                            HLS_BREAK_ARRAY_DEPENDENCY(plm_in_pong);
                            HLS_BREAK_ARRAY_DEPENDENCY(regs_m);

                            uint32_t m_index = m*BLOCK_SIZE + elem_m;

                            if (ping)
                                regs_m[elem_m] = plm_in_ping[m_index];
                            else
                                regs_m[elem_m] = plm_in_pong[m_index];
                        }

                        for (uint32_t n = 0; n < BLOCK_SIZE; n++)
                        {
                            HLS_PIPELINE_LOOP(HARD_STALL, 5, "pipe_mac");

                            ////////////////////////////////////////////////////////////////
                            // START READ_PLM for N
                            ////////////////////////////////////////////////////////////////
                            
                            // read the entire row for matrix 1 from PLM into an array - 0-15
                            for (int elem_n = 0; elem_n < (BLOCK_SIZE/4); elem_n++)
                            {
                                HLS_UNROLL_LOOP(ON, "read_plm_n_0");
                                HLS_BREAK_ARRAY_DEPENDENCY(plm_in_ping);
                                HLS_BREAK_ARRAY_DEPENDENCY(plm_in_pong);
                                HLS_BREAK_ARRAY_DEPENDENCY(regs_n);

                                uint32_t n_index = (PLM_IN_WORD/2) + n*BLOCK_SIZE + elem_n;

                                if (ping)
                                    regs_n[elem_n] = plm_in_ping[n_index];
                                else
                                    regs_n[elem_n] = plm_in_pong[n_index];
                            }

                            // read the entire row for matrix 1 from PLM into an array - 16-31
                            for (int elem_n = (BLOCK_SIZE/4); elem_n < 2*(BLOCK_SIZE/4); elem_n++)
                            {
                                HLS_UNROLL_LOOP(ON, "read_plm_n_1");
                                HLS_BREAK_ARRAY_DEPENDENCY(plm_in_ping);
                                HLS_BREAK_ARRAY_DEPENDENCY(plm_in_pong);
                                HLS_BREAK_ARRAY_DEPENDENCY(regs_n);

                                uint32_t n_index = (PLM_IN_WORD/2) + n*BLOCK_SIZE + elem_n;

                                if (ping)
                                    regs_n[elem_n] = plm_in_ping[n_index];
                                else
                                    regs_n[elem_n] = plm_in_pong[n_index];
                            }

                            // read the entire row for matrix 1 from PLM into an array - 32-47
                            for (int elem_n = 2*(BLOCK_SIZE/4); elem_n < 3*(BLOCK_SIZE/4); elem_n++)
                            {
                                HLS_UNROLL_LOOP(ON, "read_plm_n_2");
                                HLS_BREAK_ARRAY_DEPENDENCY(plm_in_ping);
                                HLS_BREAK_ARRAY_DEPENDENCY(plm_in_pong);
                                HLS_BREAK_ARRAY_DEPENDENCY(regs_n);

                                uint32_t n_index = (PLM_IN_WORD/2) + n*BLOCK_SIZE + elem_n;

                                if (ping)
                                    regs_n[elem_n] = plm_in_ping[n_index];
                                else
                                    regs_n[elem_n] = plm_in_pong[n_index];
                            }

                            // read the entire row for matrix 1 from PLM into an array - 48-63
                            for (int elem_n = 3*(BLOCK_SIZE/4); elem_n < BLOCK_SIZE; elem_n++)
                            {
                                HLS_UNROLL_LOOP(ON, "read_plm_n_3");
                                HLS_BREAK_ARRAY_DEPENDENCY(plm_in_ping);
                                HLS_BREAK_ARRAY_DEPENDENCY(plm_in_pong);
                                HLS_BREAK_ARRAY_DEPENDENCY(regs_n);

                                uint32_t n_index = (PLM_IN_WORD/2) + n*BLOCK_SIZE + elem_n;

                                if (ping)
                                    regs_n[elem_n] = plm_in_ping[n_index];
                                else
                                    regs_n[elem_n] = plm_in_pong[n_index];
                            }

                            ////////////////////////////////////////////////////////////////
                            // END READ_PLM for N
                            ////////////////////////////////////////////////////////////////
                            
                            // multiply all elements stored in regs_m and regs_n
                            for (uint32_t k = 0; k < BLOCK_SIZE; k++)
                            {
                                HLS_UNROLL_LOOP(ON, "multiply_k");
                                HLS_BREAK_ARRAY_DEPENDENCY(regs_m);
                                HLS_BREAK_ARRAY_DEPENDENCY(regs_n);
                                HLS_BREAK_ARRAY_DEPENDENCY(regs_mul);

                                regs_mul[k] = regs_m[k] * regs_n[k];
                            }

                            uint32_t acc = 0;

                            // accmulate all the multiplies
                            for (uint32_t k = 0; k < BLOCK_SIZE; k++)
                            {
                                HLS_UNROLL_LOOP(ON, "accumulate_k");
                                HLS_BREAK_ARRAY_DEPENDENCY(regs_mul);

                                acc += regs_mul[k];
                            }

                            // assign the accumulate to the plm_out
                            {
                                HLS_DEFINE_PROTOCOL("read_write_output");
                                HLS_BREAK_ARRAY_DEPENDENCY(plm_out_ping);
                                HLS_BREAK_ARRAY_DEPENDENCY(plm_out_pong);

                                uint32_t partial_output;
                                uint32_t out_index = m*BLOCK_SIZE + n;
                                if (num_k == 0)
                                    partial_output = 0;
                                else {
                                    if (ping_out)
                                        partial_output = plm_out_ping[out_index];
                                    else
                                        partial_output = plm_out_pong[out_index];
                                }

                                wait();

                                if (ping_out)
                                    plm_out_ping[out_index] = partial_output + acc;
                                else
                                    plm_out_pong[out_index] = partial_output + acc;
                            }
                        }
                    }
                    ping = !ping;
                }
                this->compute_store_handshake();
                ping_out = !ping_out;
            }
        }

        // Conclude
        {
            this->process_done();
        }
    }
}
