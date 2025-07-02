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
    int32_t input_1_payload_offset;
    int32_t input_2_payload_offset;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
    }

    // Load
    while(true)
    {
        HLS_PROTO("load-loop");

        wait();

        this->load_avu_ready_handshake();

        // Read config information for current context
        {
            HLS_PROTO("read-load-config");

            conf_info_t config = this->conf_info.read();        
            HLS_FLATTEN_ARRAY(config.dim_m);
            HLS_FLATTEN_ARRAY(config.dim_n);
            HLS_FLATTEN_ARRAY(config.dim_k);
            HLS_FLATTEN_ARRAY(config.input_queue_base[current_context_int]);

            // User-defined config code
            /* <<--local-params-->> */
            dim_m = config.dim_m[current_context_int];
            dim_n = config.dim_n[current_context_int];
            dim_k = config.dim_k[current_context_int];

            // Configured shared memory base addresses for input queues
            input_1_payload_offset = config.input_queue_base[current_context_int][0] + PAYLOAD_OFFSET;
            input_2_payload_offset = config.input_queue_base[current_context_int][1] + PAYLOAD_OFFSET;

            wait();
        }

        // Load input 1 data
        {
            HLS_PROTO("load-input-1");

            dma_info_t dma_info(input_1_payload_offset / DMA_WORD_PER_BEAT, dim_m * dim_k / DMA_WORD_PER_BEAT, DMA_SIZE);
            sc_dt::sc_bv<DMA_WIDTH> dataBv;

            wait();

            this->dma_read_ctrl.put(dma_info);

            for (int i = 0; i < dim_m * dim_k; i += DMA_WORD_PER_BEAT)
            {
                HLS_BREAK_DEP(plm_in_1);

                dataBv = this->dma_read_chnl.get();
                wait();
                
                for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                {
                    HLS_UNROLL_SIMPLE;
                    plm_in_1[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                }
            }
        }
        // Load input 2 data
        {
            HLS_PROTO("load-input-2");

            dma_info_t dma_info(input_2_payload_offset / DMA_WORD_PER_BEAT, dim_n * dim_k / DMA_WORD_PER_BEAT, DMA_SIZE);
            sc_dt::sc_bv<DMA_WIDTH> dataBv;

            wait();

            this->dma_read_ctrl.put(dma_info);

            for (int i = 0; i < dim_n * dim_k; i += DMA_WORD_PER_BEAT)
            {
                HLS_BREAK_DEP(plm_in_2);

                dataBv = this->dma_read_chnl.get();
                wait();

                for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                {
                    HLS_UNROLL_SIMPLE;
                    plm_in_2[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                }
            }
        }

        wait();

        this->load_avu_done_handshake();
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
    int32_t output_payload_offset;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process
    }

    // Store
    while(true)
    {
        HLS_PROTO("store-loop");

        wait();

        this->store_avu_ready_handshake();

        // Read config information for current context
        {
            HLS_PROTO("read-store-config");

            conf_info_t config = this->conf_info.read();        
            HLS_FLATTEN_ARRAY(config.dim_m);
            HLS_FLATTEN_ARRAY(config.dim_n);
            HLS_FLATTEN_ARRAY(config.output_queue_base);

            // User-defined config code
            /* <<--local-params-->> */
            dim_m = config.dim_m[current_context_int];
            dim_n = config.dim_n[current_context_int];

            // Configured shared memory base addresses for output queue
            output_payload_offset = config.output_queue_base[current_context_int][0] + PAYLOAD_OFFSET;

            wait();
        }

        {
            HLS_PROTO("store-data");

            dma_info_t dma_info(output_payload_offset / DMA_WORD_PER_BEAT, dim_m * dim_n / DMA_WORD_PER_BEAT, DMA_SIZE);
            sc_dt::sc_bv<DMA_WIDTH> dataBv;

            wait();

            this->dma_write_ctrl.put(dma_info);

            for (int i = 0; i < dim_m * dim_n; i += DMA_WORD_PER_BEAT)
            {
                HLS_BREAK_DEP(plm_out);

                wait();

                for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                {
                    HLS_UNROLL_SIMPLE;
                    dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_out[i + k];
                }

                this->dma_write_chnl.put(dataBv);
            }

            // Wait till the last write is accepted at the cache
            wait();
            while (!(this->dma_write_chnl.ready)) wait();
        }

        wait();

        this->store_avu_done_handshake();
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

            wait();
        }

        // Compute GeMM
        {
            // Number of tiles -- to perform computation over registers
            uint32_t num_blocks_m = (dim_m == BLOCK_SIZE) ? 1 : dim_m/BLOCK_SIZE;
            uint32_t num_blocks_n = (dim_n == BLOCK_SIZE) ? 1 : dim_n/BLOCK_SIZE;
            uint32_t num_blocks_k = (dim_k == BLOCK_SIZE) ? 1 : dim_k/BLOCK_SIZE;

            // Iterate over register block across M dimension
            for (uint32_t m_block = 0; m_block < num_blocks_m; m_block++)
            {
                uint32_t in_1_offset = (m_block * BLOCK_SIZE) * dim_k;
                uint32_t out_offset = (m_block * BLOCK_SIZE) * dim_n;

                // Iterate over register block across N dimension
                for (uint32_t n_block = 0; n_block < num_blocks_n; n_block++)
                {
                    uint32_t in_2_offset = (n_block * BLOCK_SIZE) * dim_k;
                    uint32_t out_block_offset = out_offset + n_block * BLOCK_SIZE;

                    // Iterate over register block across K dimension
                    for (uint32_t k_block = 0; k_block < num_blocks_k; k_block++)
                    {                  
                        uint32_t regs_1[BLOCK_SIZE];
                        uint32_t regs_2[BLOCK_SIZE];
                        uint32_t regs_mul[BLOCK_SIZE];
                        uint32_t regs_acc;                        
                        HLS_FLATTEN_ARRAY(regs_1);
                        HLS_FLATTEN_ARRAY(regs_2);
                        HLS_FLATTEN_ARRAY(regs_mul);

                        uint32_t in_1_block_offset = in_1_offset + k_block * BLOCK_SIZE;
                        uint32_t in_2_block_offset = in_2_offset + k_block * BLOCK_SIZE;

                        // Perform block-level multiply - M dimension
                        for (uint32_t row_m = 0; row_m < BLOCK_SIZE; row_m++)
                        {
                            uint32_t in_1_elem_offset = in_1_block_offset + row_m * dim_k;
                            uint32_t out_elem_offset = out_block_offset + row_m * dim_n;

                            // read Mth block across K dimension of matrix 1 from PLM into a register array
                            for (int elem_k = 0; elem_k < BLOCK_SIZE; elem_k++)
                            {
                                HLS_UNROLL_LOOP(ON, "read_plm_m");
                                HLS_BREAK_ARRAY_DEPENDENCY(plm_in_1);

                                uint32_t in_1_index = in_1_elem_offset + elem_k;
                                regs_1[elem_k] = plm_in_1[in_1_index];
                            }

                            // Perform block-level multiply - N dimension
                            for (uint32_t row_n = 0; row_n < BLOCK_SIZE; row_n++)
                            {
                                uint32_t in_2_elem_offset = in_2_block_offset + row_n * dim_k;

                                // read Nth block across K dimension of matrix 2 from PLM into a register array
                                for (int elem_k = 0; elem_k < BLOCK_SIZE; elem_k++)
                                {
                                    HLS_UNROLL_LOOP(ON, "read_plm_n");
                                    HLS_BREAK_ARRAY_DEPENDENCY(plm_in_2);

                                    uint32_t in_2_index = in_2_elem_offset + elem_k;
                                    regs_2[elem_k] = plm_in_2[in_2_index];
                                }

                                // multiply all elements stored in regs_1 and regs_2
                                for (uint32_t mul = 0; mul < BLOCK_SIZE; mul++)
                                {
                                    HLS_UNROLL_LOOP(ON, "multiply_k");
                                    regs_mul[mul] = regs_1[mul] * regs_2[mul];
                                }

                                // read the previous partial sum, or not
                                if (k_block == 0)
                                {
                                    regs_acc = 0;
                                }
                                else
                                {
                                    uint32_t out_index = out_elem_offset + row_n;
                                    regs_acc = plm_out[out_index];
                                }

                                // Accumulate all products
                                for (uint32_t mul = 0; mul < BLOCK_SIZE; mul++)
                                {
                                    HLS_UNROLL_LOOP(ON, "accumulate_k_0");
                                    regs_acc += regs_mul[mul];
                                }

                                // write the partial sum to PLM
                                {
                                    uint32_t out_index = out_elem_offset + row_n;
                                    plm_out[out_index] = regs_acc;
                                }
                            }                                  
                        }
                    }
                }
            }
        }

        {
            HLS_PROTO("compute-done");

            this->compute_avu_done_handshake();

            wait();
        }
    } // while (true)
} // Function : compute_kernel
