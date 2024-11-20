// Copyright (c) 2011-2019 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef ENABLE_PP

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

        load_state_req_dbg.write(0);

        load_ready.ack.reset_ack();
        load_done.req.reset_req();

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t dim_m;
    int32_t dim_n;
    int32_t dim_k;
    int32_t prod_valid_offset;
    int32_t prod_ready_offset;
    int32_t cons_valid_offset;
    int32_t cons_ready_offset;
    int32_t input_1_offset;
    int32_t input_2_offset;
    int32_t output_offset;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        dim_m = config.dim_m;
        dim_n = config.dim_n;
        dim_k = config.dim_k;

        // Configured shared memory offsets for sync flags
        prod_valid_offset = config.prod_valid_offset;
        prod_ready_offset = config.prod_ready_offset;
        cons_valid_offset = config.cons_valid_offset;
        cons_ready_offset = config.cons_ready_offset;
        input_1_offset = config.input_1_offset;
        input_2_offset = config.input_2_offset;
    }

    // Load
    while(true)
    {
        HLS_PROTO("load-dma");

        wait();

        this->load_compute_ready_handshake();

        load_state_req_dbg.write(load_state_req);

        switch (load_state_req)
        {
#ifdef ENABLE_SM
            case POLL_PROD_VALID_REQ:
            {
                dma_info_t dma_info(prod_valid_offset / DMA_WORD_PER_BEAT, 2 * TEST_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                int32_t valid_task = 0;

                wait();

                // Wait for producer to send new data
                while (valid_task != 1)
                {
                    HLS_UNROLL_LOOP(OFF);
                    this->dma_read_ctrl.put(dma_info);
                    dataBv = this->dma_read_chnl.get();
                    wait();
                    valid_task = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
                    dataBv = this->dma_read_chnl.get();
                    wait();
                    last_task = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
                }
            }
            break;
            case POLL_CONS_READY_REQ:
            {
                dma_info_t dma_info(cons_ready_offset / DMA_WORD_PER_BEAT, TEST_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                int32_t ready_for_task = 0;

                wait();

                // Wait for consumer to accept new data
                while (ready_for_task != 1)
                {
                    HLS_UNROLL_LOOP(OFF);
                    this->dma_read_ctrl.put(dma_info);
                    dataBv = this->dma_read_chnl.get();
                    wait();
                    ready_for_task = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
                }
            }
            break;
#endif
            case LOAD_DATA_REQ:
            // Load input 1 data
            {
                dma_info_t dma_info(input_1_offset / DMA_WORD_PER_BEAT, dim_m * dim_k / DMA_WORD_PER_BEAT, DMA_SIZE);
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
                dma_info_t dma_info(input_2_offset / DMA_WORD_PER_BEAT, dim_n * dim_k / DMA_WORD_PER_BEAT, DMA_SIZE);
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
            break;
            default:
            break;
        }

        wait();

        this->load_compute_done_handshake();
    }
} // Function : load_input

void gemm::store_output()
{
    // Reset
    {
        HLS_PROTO("store-reset");

        this->reset_store_output();

        store_state_req_dbg.write(0);

        store_ready.ack.reset_ack();
        store_done.req.reset_req();

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t dim_m;
    int32_t dim_n;
    int32_t num_samples;
    int32_t prod_valid_offset;
    int32_t prod_ready_offset;
    int32_t cons_valid_offset;
    int32_t cons_ready_offset;
    int32_t output_offset;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process

        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        dim_m = config.dim_m;
        dim_n = config.dim_n;

        // Configured shared memory offsets for sync flags
        prod_valid_offset = config.prod_valid_offset;
        prod_ready_offset = config.prod_ready_offset;
        cons_valid_offset = config.cons_valid_offset;
        cons_ready_offset = config.cons_ready_offset;
        output_offset = config.output_offset;
    }

    // Store
    while(true)
    {
        HLS_PROTO("store-dma");

        wait();

        this->store_compute_ready_handshake();

        store_state_req_dbg.write(store_state_req);

        switch (store_state_req)
        {
#ifdef ENABLE_SM
            case UPDATE_PROD_READY_REQ:
            {
                dma_info_t dma_info(prod_ready_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                dataBv.range(DMA_WIDTH - 1, 0) = 1;

                this->dma_write_ctrl.put(dma_info);
                wait();
                this->dma_write_chnl.put(dataBv);
                wait();

                // Wait till the write is accepted at the cache (and previous fences)
                while (!(this->dma_write_chnl.ready)) wait();
                wait();
            }
            break;
            case UPDATE_PROD_VALID_REQ:
            {
                dma_info_t dma_info(prod_valid_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                dataBv.range(DMA_WIDTH - 1, 0) = 0;

                this->dma_write_ctrl.put(dma_info);
                wait();
                this->dma_write_chnl.put(dataBv);
                wait();

                // Wait till the write is accepted at the cache (and previous fences)
                while (!(this->dma_write_chnl.ready)) wait();
                wait();
            }
            break;
            case UPDATE_CONS_VALID_REQ:
            {
                dma_info_t dma_info(cons_valid_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                dataBv.range(DMA_WIDTH - 1, 0) = 1;

                this->dma_write_ctrl.put(dma_info);
                wait();
                this->dma_write_chnl.put(dataBv);
                wait();

                // Wait till the write is accepted at the cache (and previous fences)
                while (!(this->dma_write_chnl.ready)) wait();
                wait();
            }
            break;
            case UPDATE_CONS_READY_REQ:
            {
                dma_info_t dma_info(cons_ready_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                dataBv.range(DMA_WIDTH - 1, 0) = 0;

                this->dma_write_ctrl.put(dma_info);
                wait();
                this->dma_write_chnl.put(dataBv);
                wait();

                // Wait till the write is accepted at the cache (and previous fences)
                while (!(this->dma_write_chnl.ready)) wait();
                wait();
            }
            break;
#endif
            case STORE_DATA_REQ:
            {
                dma_info_t dma_info(output_offset / DMA_WORD_PER_BEAT, dim_m * dim_n / DMA_WORD_PER_BEAT, DMA_SIZE);
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
            break;
            case STORE_FENCE:
            {
                // Block till L2 to be ready to receive a fence, then send
                this->acc_fence.put(0x2);
                wait();
            }
            break;
            case ACC_DONE:
            {
                // Ensure the previous fence was accepted, then acc_done
                while (!(this->acc_fence.ready)) wait();
                wait();
                this->accelerator_done();
                wait();
            }
            break;
            default:
            break;
        }

        wait();

        this->store_compute_done_handshake();
    }
} // Function : store_output

void gemm::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-reset");

        this->reset_compute_kernel();

        compute_state_req_dbg.write(0);

        load_ready.req.reset_req();
        load_done.ack.reset_ack();
        store_ready.req.reset_req();
        store_done.ack.reset_ack();

        load_state_req = 0;
        store_state_req = 0;

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
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        dim_n = config.dim_n;
        dim_m = config.dim_m;
        dim_k = config.dim_k;
    }

    while(true)
    {
#ifdef ENABLE_SM
        // Poll producer's valid for new task
        {
            HLS_PROTO("poll-prod-valid");

            load_state_req = POLL_PROD_VALID_REQ;

            compute_state_req_dbg.write(POLL_PROD_VALID_REQ);

            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();
        }

        // Reset producer's valid
        {
            HLS_PROTO("update-prod-valid");

            store_state_req = UPDATE_PROD_VALID_REQ;

            compute_state_req_dbg.write(UPDATE_PROD_VALID_REQ);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_state_req_dbg.write(STORE_FENCE);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();
        }
#endif
        // Load input data
        {
            HLS_PROTO("load-input-data");

            load_state_req = LOAD_DATA_REQ;

            compute_state_req_dbg.write(LOAD_DATA_REQ);

            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();
        }
#ifdef ENABLE_SM
        // update producer's ready to accept new data
        {
            HLS_PROTO("update-prod-ready");

            store_state_req = UPDATE_PROD_READY_REQ;

            compute_state_req_dbg.write(UPDATE_PROD_READY_REQ);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_state_req_dbg.write(STORE_FENCE);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();

            compute_state_req_dbg.write(COMPUTE);
        }
#endif

        // Compute
        {
            // Number of tiles -- to perform computation over registers
            uint32_t num_blocks_m = (dim_m == BLOCK_SIZE) ? 1 : dim_m/BLOCK_SIZE;
            uint32_t num_blocks_n = (dim_n == BLOCK_SIZE) ? 1 : dim_n/BLOCK_SIZE;
            uint32_t num_blocks_k = (dim_k == BLOCK_SIZE) ? 1 : dim_k/BLOCK_SIZE;

            // Iterate over register block across M dimension
            for (uint32_t m_block = 0; m_block < num_blocks_m; m_block++)
            {
                uint32_t in_1_offset = (m_block * BLOCK_SIZE) * dim_k;

                // Iterate over register block across N dimension
                for (uint32_t n_block = 0; n_block < num_blocks_n; n_block++)
                {
                    uint32_t in_2_offset = (n_block * BLOCK_SIZE) * dim_k;

                    uint32_t out_offset = (m_block * BLOCK_SIZE) * dim_n;

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

                        in_1_offset += k_block * BLOCK_SIZE;
                        in_2_offset += k_block * BLOCK_SIZE;

                        // Perform block-level multiply - M dimension
                        for (uint32_t row_m = 0; row_m < BLOCK_SIZE; row_m++)
                        {
                            in_1_offset += row_m * dim_k;
                            out_offset += row_m * dim_n;

                            // read Mth block across K dimension of matrix 1 from PLM into a register array
                            for (int elem_k = 0; elem_k < BLOCK_SIZE; elem_k++)
                            {
                                HLS_UNROLL_LOOP(ON, "read_plm_m");
                                HLS_BREAK_ARRAY_DEPENDENCY(plm_in_1);

                                uint32_t in_1_index = in_1_offset + elem_k;
                                regs_1[elem_k] = plm_in_1[in_1_index];
                            }

                            // Perform block-level multiply - N dimension
                            for (uint32_t row_n = 0; row_n < BLOCK_SIZE; row_n++)
                            {
                                in_2_offset += row_n * dim_k;

                                // read Nth block across K dimension of matrix 2 from PLM into a register array
                                for (int elem_k = 0; elem_k < BLOCK_SIZE; elem_k++)
                                {
                                    HLS_UNROLL_LOOP(ON, "read_plm_n");
                                    HLS_BREAK_ARRAY_DEPENDENCY(plm_in_2);

                                    uint32_t in_2_index = in_2_offset + elem_k;
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
                                    uint32_t out_index = out_offset + row_n;
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
                                    uint32_t out_index = out_offset + row_n;
                                    plm_out[out_index] = regs_acc;
                                }
                            }                                  
                        }
                    }
                }
            }
        }

#ifdef ENABLE_SM
        // Poll consumer's ready to know if we can send new data
        {
            HLS_PROTO("poll-for-cons-ready");

            load_state_req = POLL_CONS_READY_REQ;

            compute_state_req_dbg.write(POLL_CONS_READY_REQ);

            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();
        }

        // Reset consumer's ready
        {
            HLS_PROTO("update-cons-ready");

            store_state_req = UPDATE_CONS_READY_REQ;

            compute_state_req_dbg.write(UPDATE_CONS_READY_REQ);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_state_req_dbg.write(STORE_FENCE);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();
        }
#endif
        // Store output data
        {
            HLS_PROTO("store-output-data");

            store_state_req = STORE_DATA_REQ;

            this->compute_store_ready_handshake();

            compute_state_req_dbg.write(STORE_DATA_REQ);

            wait();
            this->compute_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_state_req_dbg.write(STORE_FENCE);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();
        }
#ifdef ENABLE_SM
        // update consumer's ready for new data available
        {
            HLS_PROTO("update-cons-valid");

            store_state_req = UPDATE_CONS_VALID_REQ;

            compute_state_req_dbg.write(UPDATE_CONS_VALID_REQ);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_state_req_dbg.write(STORE_FENCE);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();
        }
#endif
        // End operation
        {
            HLS_PROTO("end-acc");

#ifdef ENABLE_SM
            if (last_task == 1)
            {
#endif
                store_state_req = ACC_DONE;

                compute_state_req_dbg.write(ACC_DONE);

                this->compute_store_ready_handshake();
                wait();
                this->compute_store_done_handshake();
                wait();
                this->process_done();
#ifdef ENABLE_SM
            }
#endif
        }
    } // while (true)
} // Function : compute_kernel

#else // ENABLE_PP

#include "gemm_pipelined.cpp"

#endif // ENABLE_PP
