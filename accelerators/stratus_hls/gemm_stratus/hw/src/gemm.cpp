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
    int32_t weight_payload_offset;
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
            HLS_FLATTEN_ARRAY(config.input_base);
            HLS_FLATTEN_ARRAY(config.weight_base);

            // User-defined config code
            /* <<--local-params-->> */
            dim_m = config.dim_m[current_context_int];
            dim_n = config.dim_n[current_context_int];
            dim_k = config.dim_k[current_context_int];
            weight_payload_offset = config.weight_base[current_context_int];

            // Configured shared memory base addresses for input queues
            input_payload_offset = config.input_base[current_context_int][0] + PAYLOAD_OFFSET;

            wait();
        }

        // Load input 1 data
        {
            HLS_PROTO("load-input-1");

            dma_info_t dma_info(input_payload_offset / DMA_WORD_PER_BEAT, dim_m * dim_k / DMA_WORD_PER_BEAT, DMA_SIZE);
            sc_dt::sc_bv<DMA_WIDTH> dataBv;

            wait();

            this->dma_read_ctrl.put(dma_info);

            for (int i = 0; i < dim_m * dim_k; i += DMA_WORD_PER_BEAT)
            {
                HLS_BREAK_DEP(plm_in);

                dataBv = this->dma_read_chnl.get();
                wait();
                
                for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                {
                    HLS_UNROLL_SIMPLE;
                    plm_in[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_uint64();
                }
            }
        }
        // Load input 2 data
        {
            HLS_PROTO("load-input-2");

            dma_info_t dma_info(weight_payload_offset / DMA_WORD_PER_BEAT, dim_n * dim_k / DMA_WORD_PER_BEAT, DMA_SIZE);
            sc_dt::sc_bv<DMA_WIDTH> dataBv;

            wait();

            this->dma_read_ctrl.put(dma_info);

            for (int i = 0; i < dim_n * dim_k; i += DMA_WORD_PER_BEAT)
            {
                HLS_BREAK_DEP(plm_wgt);

                dataBv = this->dma_read_chnl.get();
                wait();

                for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                {
                    HLS_UNROLL_SIMPLE;
                    plm_wgt[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
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
            HLS_FLATTEN_ARRAY(config.output_base);

            // User-defined config code
            /* <<--local-params-->> */
            dim_m = config.dim_m[current_context_int];
            dim_n = config.dim_n[current_context_int];

            // Configured shared memory base addresses for output queue
            output_payload_offset = config.output_base[current_context_int][0] + PAYLOAD_OFFSET;

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
            // Iterate over register block across M dimension
            for (unsigned block_m = 0; block_m < dim_m; block_m += BLOCK_SIZE)
            {
                // Iterate over register block across N dimension
                for (unsigned block_n = 0; block_n < dim_n; block_n += BLOCK_SIZE)
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
                            unsigned elem_m = block_m + row_m;
                            unsigned idx_mk = (elem_m * dim_k) + block_k;

                            // If the remainder is not a multiple of block_size, break out of the loop
                            if (elem_m >= dim_m) continue;

                            // read Mth block across K dimension of matrix 1 from PLM into a register array
                            for (unsigned elem_k = 0; elem_k < BLOCK_SIZE; elem_k++)
                            {
                                HLS_UNROLL_LOOP(ON, "read_plm_m");
                                HLS_BREAK_ARRAY_DEPENDENCY(plm_in);
                                regs_m[elem_k] = regs_valid[elem_k] * INT2FP(plm_in[idx_mk + elem_k]);
                            }

                            // Perform block-level multiply - N dimension
                            for (unsigned row_n = 0; row_n < BLOCK_SIZE; row_n++)
                            {
                                unsigned elem_n = block_n + row_n;
                                unsigned idx_kn = (block_k * dim_n) + elem_n;
                                unsigned idx_mn = (elem_m * dim_n) + elem_n;

                                // If the remainder is not a multiple of block_size, break out of the loop
                                if (elem_n >= dim_n) continue;
                                
                                // read Nth block across K dimension of matrix 2 from PLM into a register array
                                for (unsigned elem_k = 0; elem_k < BLOCK_SIZE; elem_k++)
                                {
                                    HLS_UNROLL_LOOP(ON, "read_plm_n");
                                    HLS_BREAK_ARRAY_DEPENDENCY(plm_wgt);
                                    regs_n[elem_k] = regs_valid[elem_k] * INT2FP(plm_wgt[idx_kn + (elem_k * dim_n)]);
                                }

                                // multiply all elements stored in regs_1 and regs_2
                                for (unsigned elem_k = 0; elem_k < BLOCK_SIZE; elem_k++)
                                {
                                    HLS_UNROLL_LOOP(ON, "multiply_k");
                                    regs_mul[elem_k] = regs_m[elem_k] * regs_n[elem_k];
                                }

                                // read the previous partial sum, or not
                                if (block_k == 0)
                                {
                                    regs_acc = 0;
                                }
                                else
                                {
                                    regs_acc = INT2FP(plm_out[idx_mn]);
                                }

                                // Accumulate all products
                                for (unsigned elem_k = 0; elem_k < BLOCK_SIZE; elem_k++)
                                {
                                    HLS_UNROLL_LOOP(ON, "accumulate_k_0");
                                    regs_acc += regs_mul[elem_k];
                                }

                                // write the partial sum to PLM
                                {
                                    plm_out[idx_mn] = FP2INT(regs_acc);

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
