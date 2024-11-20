// Copyright (c) 2011-2019 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "gemm_pipelined.hpp"
#include "gemm_directives.hpp"

// Functions

#include "gemm_pipelined_functions.hpp"

// Processes

void gemm::load_input()
{
    // Reset
    {
        HLS_PROTO("load-reset");

        this->reset_load_input();

        load_state_req_dbg.write(0);

        input_load_start.ack.reset_ack();
        output_load_start.ack.reset_ack();
        load_input_done.req.reset_req();
        load_output_done.req.reset_req();

        prod_valid = 0;
        cons_ready = 0;
        load_state_req = 0;
        load_state_req_module = 0;

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
    bool pingpong;
    int32_t task_arbiter;
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
        cons_ready_offset = config.cons_ready_offset;
        cons_valid_offset = config.cons_valid_offset;
        input_1_offset = config.input_1_offset;
        input_2_offset = config.input_2_offset;

        pingpong = false;
        task_arbiter = 0;
    }

    // Load
    while(true)
    {
        HLS_PROTO("load-dma");

        wait();

        // Wait for either input ASI or output ASI
        while (!(input_load_req_valid || output_load_req_valid)) wait();

        if (task_arbiter == 0 && input_load_req_valid) {
            this->load_input_start_handshake();
            load_state_req = input_load_req;
            load_state_req_module = INPUT_ASI;
            task_arbiter++;
        } else if (task_arbiter == 1 && output_load_req_valid) {
            this->load_output_start_handshake();
            load_state_req = output_load_req;
            load_state_req_module = OUTPUT_ASI;
            task_arbiter = 0;
        } else {
            if (task_arbiter == 1) task_arbiter = 0;
            else task_arbiter++;
            continue;
        }

        load_state_req_dbg.write(load_state_req);

        switch (load_state_req)
        {
            case TEST_PROD_VALID_REQ:
            {
                // Test for producer to send new data
                dma_info_t dma_info(prod_valid_offset / DMA_WORD_PER_BEAT, 2 * TEST_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();
                this->dma_read_ctrl.put(dma_info);

                dataBv = this->dma_read_chnl.get();
                wait();
                prod_valid = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
                dataBv = this->dma_read_chnl.get();
                wait();
                last_task = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
            }
            break;
            case TEST_CONS_READY_REQ:
            {
                // Test for consumer to accept new data
                dma_info_t dma_info(cons_ready_offset / DMA_WORD_PER_BEAT, TEST_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();
                this->dma_read_ctrl.put(dma_info);

                dataBv = this->dma_read_chnl.get();
                wait();
                cons_ready = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
            }
            break;
            case LOAD_DATA_REQ:
            // Load input 1 data
            {
                dma_info_t dma_info(input_1_offset / DMA_WORD_PER_BEAT, dim_m * dim_k / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                this->dma_read_ctrl.put(dma_info);

                for (int i = 0; i < dim_m * dim_k; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(plm_in_1_ping);
                    HLS_BREAK_DEP(plm_in_1_pong);

                    dataBv = this->dma_read_chnl.get();
                    wait();

                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        if (!pingpong)
                            plm_in_1_ping[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                        else
                            plm_in_1_pong[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
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
                    HLS_BREAK_DEP(plm_in_2_ping);
                    HLS_BREAK_DEP(plm_in_2_pong);

                    dataBv = this->dma_read_chnl.get();
                    wait();

                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        if (!pingpong)
                            plm_in_2_ping[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                        else
                            plm_in_2_pong[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                    }
                }
            }

            pingpong = !pingpong;
            break;
            default:
            break;
        }

        wait();

        if (load_state_req_module == INPUT_ASI) {
            this->load_input_done_handshake();
        } else {
            this->load_output_done_handshake();
        }

        load_state_req_module = 0;
    }
} // Function : load_input

void gemm::store_output()
{
    // Reset
    {
        HLS_PROTO("store-reset");

        this->reset_store_output();

        store_state_req_dbg.write(0);

        input_store_start.ack.reset_ack();
        output_store_start.ack.reset_ack();
        store_input_done.req.reset_req();
        store_output_done.req.reset_req();

        store_state_req = 0;
        store_state_req_module = 0;

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
    bool pingpong;
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

        pingpong = false;
    }

    // Store
    while(true)
    {
        HLS_PROTO("store-dma");

        wait();

        // Wait for either input ASI or output ASI
        while (!(input_store_req_valid || output_store_req_valid)) wait();

        if (input_store_req_valid) {
            this->store_input_start_handshake();
            store_state_req = input_store_req;
            store_state_req_module = INPUT_ASI;
        } else {
            this->store_output_start_handshake();
            store_state_req = output_store_req;
            store_state_req_module = OUTPUT_ASI;
        }

        store_state_req_dbg.write(store_state_req);

        switch (store_state_req)
        {
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
            case STORE_DATA_REQ:
            {
                dma_info_t dma_info(output_offset / DMA_WORD_PER_BEAT, dim_m * dim_n / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                this->dma_write_ctrl.put(dma_info);

                for (int i = 0; i < dim_m * dim_n; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(plm_out_ping);
                    HLS_BREAK_DEP(plm_out_pong);

                    wait();

                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        if (!pingpong)
                            dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_out_ping[i + k];
                        else
                            dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_out_pong[i + k];
                    }

                    this->dma_write_chnl.put(dataBv);
                }

                // Wait till the last write is accepted at the cache
                wait();
                while (!(this->dma_write_chnl.ready)) wait();
            }

            pingpong = !pingpong;
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

        if (store_state_req_module == INPUT_ASI) {
            this->store_input_done_handshake();
        } else {
            this->store_output_done_handshake();
        }

        store_state_req_module = 0;
    }
} // Function : store_output

void gemm::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-reset");

        this->reset_compute_kernel();

        compute_state_req_dbg.write(0);

        input_to_compute.ack.reset_ack();
        compute_to_output.req.reset_req();

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t dim_m;
    int32_t dim_n;
    int32_t dim_k;
    bool pingpong;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        dim_m = config.dim_m;
        dim_n = config.dim_n;
        dim_k = config.dim_k;
        pingpong = false;
    }

    while(true)
    {
        // Wait for load to be complete
        {
            HLS_PROTO("wait-for-load");

            this->compute_input_handshake();
            wait();

            compute_state_req_dbg.write(COMPUTE);
        }

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
                                HLS_BREAK_ARRAY_DEPENDENCY(plm_in_1_ping);
                                HLS_BREAK_ARRAY_DEPENDENCY(plm_in_1_pong);

                                uint32_t in_1_index = in_1_offset + elem_k;
                                if (!pingpong)
                                    regs_1[elem_k] = plm_in_1_ping[in_1_index];
                                else
                                    regs_1[elem_k] = plm_in_1_pong[in_1_index];
                            }

                            // Perform block-level multiply - N dimension
                            for (uint32_t row_n = 0; row_n < BLOCK_SIZE; row_n++)
                            {
                                in_2_offset += row_n * dim_k;

                                // read Nth block across K dimension of matrix 2 from PLM into a register array
                                for (int elem_k = 0; elem_k < BLOCK_SIZE; elem_k++)
                                {
                                    HLS_UNROLL_LOOP(ON, "read_plm_n");
                                    HLS_BREAK_ARRAY_DEPENDENCY(plm_in_2_ping);
                                    HLS_BREAK_ARRAY_DEPENDENCY(plm_in_2_pong);

                                    uint32_t in_2_index = in_2_offset + elem_k;
                                    if (!pingpong)
                                        regs_2[elem_k] = plm_in_2_ping[in_2_index];
                                    else
                                        regs_2[elem_k] = plm_in_2_pong[in_2_index];
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
                                    if (!pingpong)
                                        regs_acc = plm_out_ping[out_index];
                                    else
                                        regs_acc = plm_out_pong[out_index];
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
                                    if (!pingpong)
                                        plm_out_ping[out_index] = regs_acc;
                                    else
                                        plm_out_pong[out_index] = regs_acc;
                                }
                            }                                  
                        }
                    }
                }
            }
        }

        // Inform output to start
        {
            HLS_PROTO("inform-output-start");

            compute_state_req_dbg.write(0);

            this->compute_output_handshake();
            wait();
        }

        pingpong = !pingpong;
    } // while (true)
} // Function : compute_kernel


void gemm::input_asi_kernel()
{
    // Reset
    {
        HLS_PROTO("input-asi-reset");

        input_state_req_dbg.write(0);

        input_load_start.req.reset_req();
        input_store_start.req.reset_req();
        load_input_done.ack.reset_ack();
        store_input_done.ack.reset_ack();
        input_to_compute.req.reset_req();

        input_load_req = 0;
        input_store_req = 0;
        input_load_req_valid = false;
        input_store_req_valid = false;

        end_acc = 0;

        wait();
    }

    // Config
    /* <<--params-->> */
    {
        HLS_PROTO("input-asi-config");

        cfg.wait_for_config(); // config process

        // User-defined config code
        /* <<--local-params-->> */
    }

    while(true)
    {
        // Test producer's valid for new task
        {
            HLS_PROTO("test-prod-valid");

            input_load_req = TEST_PROD_VALID_REQ;
            input_load_req_valid = true;

            input_state_req_dbg.write(TEST_PROD_VALID_REQ);

            this->input_load_start_handshake();
            wait();
            input_load_req_valid = false;
            wait();
            this->input_load_done_handshake();
            wait();
        }

        {
            HLS_PROTO("prod-valid-check");

            if (prod_valid == 1)
            {
                prod_valid = 0;
                end_acc = last_task;

                // Reset producer's valid
                {
                    HLS_PROTO("update-prod-valid");

                    input_store_req = UPDATE_PROD_VALID_REQ;
                    input_store_req_valid = true;

                    input_state_req_dbg.write(UPDATE_PROD_VALID_REQ);

                    this->input_store_start_handshake();
                    wait();
                    input_store_req_valid = false;
                    wait();
                    this->input_store_done_handshake();
                    wait();

                    // Wait for all writes to be done and then issue fence
                    input_store_req = STORE_FENCE;
                    input_store_req_valid = true;

                    input_state_req_dbg.write(STORE_FENCE);

                    this->input_store_start_handshake();
                    wait();
                    input_store_req_valid = false;
                    wait();
                    this->input_store_done_handshake();
                    wait();
                }

                // Load input data
                {
                    HLS_PROTO("load-input-data");

                    input_load_req = LOAD_DATA_REQ;
                    input_load_req_valid = true;

                    input_state_req_dbg.write(LOAD_DATA_REQ);

                    this->input_load_start_handshake();
                    wait();
                    input_load_req_valid = false;
                    wait();
                    this->input_load_done_handshake();
                    wait();
                }

                // update producer's ready to accept new data
                {
                    HLS_PROTO("update-prod-ready");

                    input_store_req = UPDATE_PROD_READY_REQ;
                    input_store_req_valid = true;

                    input_state_req_dbg.write(UPDATE_PROD_READY_REQ);

                    this->input_store_start_handshake();
                    wait();
                    input_store_req_valid = false;
                    wait();
                    this->input_store_done_handshake();
                    wait();

                    // Wait for all writes to be done and then issue fence
                    input_store_req = STORE_FENCE;
                    input_store_req_valid = true;

                    input_state_req_dbg.write(STORE_FENCE);

                    this->input_store_start_handshake();
                    wait();
                    input_store_req_valid = false;
                    wait();
                    this->input_store_done_handshake();
                    wait();
                }

                // Inform compute to start
                {
                    HLS_PROTO("inform-compute-start");

                    this->input_compute_handshake();
                    wait();
                }
            }
        }
    } // while (true)
} // Function : input_asi_kernel 

void gemm::output_asi_kernel()
{
    // Reset
    {
        HLS_PROTO("output-asi-reset");

        output_state_req_dbg.write(0);

        output_load_start.req.reset_req();
        output_store_start.req.reset_req();
        load_output_done.ack.reset_ack();
        store_output_done.ack.reset_ack();
        compute_to_output.ack.reset_ack();

        output_load_req = 0;
        output_store_req = 0;
        output_load_req_valid = false;
        output_store_req_valid = false;

        wait();
    }

    // Config
    /* <<--params-->> */
    {
        HLS_PROTO("output-asi-config");

        cfg.wait_for_config(); // config process

        // User-defined config code
        /* <<--local-params-->> */
    }

    while(true)
    {
        // Wait for compute to be complete
        {
            HLS_PROTO("wait-for-compute");

            this->output_compute_handshake();
            wait();
        }

        // Poll consumer's ready to know if we can send new data
        {
            HLS_PROTO("poll-for-cons-ready");

            while (cons_ready == 0)
            {
                output_load_req = TEST_CONS_READY_REQ;
                output_load_req_valid = true;

                output_state_req_dbg.write(TEST_CONS_READY_REQ);

                this->output_load_start_handshake();
                wait();
                output_load_req_valid = false;
                wait();
                this->output_load_done_handshake();
                wait();
            }
        }

        {
            HLS_PROTO("cons-ready-check");

            cons_ready = 0;

            // Reset consumer's ready
            {
                HLS_PROTO("update-cons-ready");

                output_store_req = UPDATE_CONS_READY_REQ;
                output_store_req_valid = true;

                output_state_req_dbg.write(UPDATE_CONS_READY_REQ);

                this->output_store_start_handshake();
                wait();
                output_store_req_valid = false;
                wait();
                this->output_store_done_handshake();
                wait();

                // Wait for all writes to be done and then issue fence
                output_store_req = STORE_FENCE;
                output_store_req_valid = true;

                output_state_req_dbg.write(STORE_FENCE);

                this->output_store_start_handshake();
                wait();
                output_store_req_valid = false;
                wait();
                this->output_store_done_handshake();
                wait();
            }
            // Store output data
            {
                HLS_PROTO("store-output-data");

                output_store_req = STORE_DATA_REQ;
                output_store_req_valid = true;

                output_state_req_dbg.write(STORE_DATA_REQ);

                this->output_store_start_handshake();
                wait();
                output_store_req_valid = false;
                wait();
                this->output_store_done_handshake();
                wait();

                // Wait for all writes to be done and then issue fence
                output_store_req = STORE_FENCE;
                output_store_req_valid = true;

                output_state_req_dbg.write(STORE_FENCE);

                this->output_store_start_handshake();
                wait();
                output_store_req_valid = false;
                wait();
                this->output_store_done_handshake();
                wait();
            }
            // update consumer's ready for new data available
            {
                HLS_PROTO("update-cons-ready");

                output_store_req = UPDATE_CONS_VALID_REQ;
                output_store_req_valid = true;

                output_state_req_dbg.write(UPDATE_CONS_VALID_REQ);

                this->output_store_start_handshake();
                wait();
                output_store_req_valid = false;
                wait();
                this->output_store_done_handshake();
                wait();

                // Wait for all writes to be done and then issue fence
                output_store_req = STORE_FENCE;
                output_store_req_valid = true;

                output_state_req_dbg.write(STORE_FENCE);

                this->output_store_start_handshake();
                wait();
                output_store_req_valid = false;
                wait();
                this->output_store_done_handshake();
                wait();
            }

            // End operation
            {
                HLS_PROTO("end-acc");

                if (end_acc == 1)
                {
                    output_store_req = ACC_DONE;
                    output_store_req_valid = true;

                    output_state_req_dbg.write(ACC_DONE);

                    this->output_store_start_handshake();
                    wait();
                    output_store_req_valid = false;
                    wait();
                    this->output_store_done_handshake();
                    wait();
                    this->process_done();
                }
            }
        }
    } // while (true)
} // Function : output_asi_kernel 