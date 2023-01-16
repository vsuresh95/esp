// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "audio_dma.hpp"
#include "audio_dma_directives.hpp"

// Functions

#include "audio_dma_functions.hpp"

// Processes
void audio_dma::load_input()
{
    // Reset
    {
        HLS_PROTO("load-reset");

        this->reset_load_input();

        load_state_req_dbg.write(0);

        load_ready.ack.reset_ack();
        load_done.req.reset_req();

        for (int i = 0; i < NUM_CFG_REG; i++)
        {
            cfg_registers[i] = 0;
        }

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t start_offset;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        start_offset = config.start_offset;
    }

    int32_t rd_size;
    int32_t rd_sp_offset;
    int32_t mem_src_offset;

    // Load
    while(true)
    {
        HLS_PROTO("load-dma");

        wait();

        this->load_compute_ready_handshake();

        load_state_req_dbg.write(load_state_req);

        switch (load_state_req)
        {
            case POLL_PROD_VALID_REQ:
            {
                int32_t start_sync_offset = start_offset + VALID_FLAG_OFFSET;
                dma_info_t dma_info(start_sync_offset / DMA_WORD_PER_BEAT, READY_FLAG_OFFSET / DMA_WORD_PER_BEAT, DMA_SIZE);
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
                    load_store_op = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
                }
            }
            break;
            case POLL_CONS_READY_REQ:
            {
                int32_t end_sync_offset = cfg_registers[CONS_READY_OFFSET];
                dma_info_t dma_info(end_sync_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
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
            case CFG_REQ:
            {
                int32_t start_sync_offset = start_offset + SYNC_VAR_SIZE;
                dma_info_t dma_info(start_sync_offset / DMA_WORD_PER_BEAT, NUM_CFG_REG / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                this->dma_read_ctrl.put(dma_info);

                for (int i = 0; i < NUM_CFG_REG; i += DMA_WORD_PER_BEAT)
                {
                    dataBv = this->dma_read_chnl.get();
                    wait();
                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        cfg_registers[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                    }
                }
            }
            break;
            case LOAD_DATA_REQ:
            {
                // Configuration unit - reading load op, size, src, dst
                {
                    rd_size = cfg_registers[RD_SIZE];
                    rd_sp_offset = cfg_registers[RD_SP_OFFSET];
                    mem_src_offset = cfg_registers[MEM_SRC_OFFSET];
                }

                dma_info_t dma_info(mem_src_offset / DMA_WORD_PER_BEAT, rd_size / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                this->dma_read_ctrl.put(dma_info);

                for (int i = 0; i < rd_size; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(plm_data);

                    dataBv = this->dma_read_chnl.get();
                    wait();
                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        plm_data[rd_sp_offset + i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
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

void audio_dma::store_output()
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
    int32_t start_offset;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        start_offset = config.start_offset;
    }

    int32_t wr_size;
    int32_t wr_sp_offset;
    int32_t mem_dst_offset;

    // Store
    while(true)
    {
        HLS_PROTO("store-dma");

        wait();

        this->store_compute_ready_handshake();

        store_state_req_dbg.write(store_state_req);

        switch (store_state_req)
        {
            case UPDATE_PROD_READY_REQ:
            {
                int32_t start_sync_offset = start_offset + READY_FLAG_OFFSET;
                dma_info_t dma_info(start_sync_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
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
                int32_t start_sync_offset = start_offset + VALID_FLAG_OFFSET;
                dma_info_t dma_info(start_sync_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
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
                int32_t end_sync_offset = cfg_registers[CONS_VALID_OFFSET];
                dma_info_t dma_info(end_sync_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
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
                int32_t end_sync_offset = cfg_registers[CONS_READY_OFFSET];
                dma_info_t dma_info(end_sync_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
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
                // Configuration unit - reading store op, size, src, dst
                {
                    wr_size = cfg_registers[WR_SIZE];
                    wr_sp_offset = cfg_registers[WR_SP_OFFSET];
                    mem_dst_offset = cfg_registers[MEM_DST_OFFSET];
                }
                
                dma_info_t dma_info(mem_dst_offset / DMA_WORD_PER_BEAT, wr_size / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                this->dma_write_ctrl.put(dma_info);

                for (int i = 0; i < wr_size; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(plm_data);

                    wait();

                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = plm_data[wr_sp_offset + i + k];
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

void audio_dma::compute_kernel()
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

    while(true)
    {
        // Poll producer's valid for new task
        {
            HLS_PROTO("poll-prod-valid");

            load_state_req = POLL_PROD_VALID_REQ;

            compute_state_req_dbg.write(1);

            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();
        }

        // Reset producer's valid
        {
            HLS_PROTO("update-prod-valid");

            store_state_req = UPDATE_PROD_VALID_REQ;

            compute_state_req_dbg.write(2);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_state_req_dbg.write(3);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();
        }

        // Read config registers
        {
            HLS_PROTO("read-config-registers");

            load_state_req = CFG_REQ;

            compute_state_req_dbg.write(4);

            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();
        }

        {
            HLS_PROTO("check-if-load-store");

            // Load input data
            if (load_store_op == LOAD_OP)
            {
                {
                    HLS_PROTO("load-input-data");

                    load_state_req = LOAD_DATA_REQ;

                    compute_state_req_dbg.write(5);

                    this->compute_load_ready_handshake();
                    wait();
                    this->compute_load_done_handshake();
                    wait();
                }

                // update producer's ready to accept new data
                {
                    HLS_PROTO("update-prod-ready");

                    store_state_req = UPDATE_PROD_READY_REQ;

                    compute_state_req_dbg.write(6);

                    this->compute_store_ready_handshake();
                    wait();
                    this->compute_store_done_handshake();
                    wait();

                    // Wait for all writes to be done and then issue fence
                    store_state_req = STORE_FENCE;

                    compute_state_req_dbg.write(7);

                    this->compute_store_ready_handshake();
                    wait();
                    this->compute_store_done_handshake();
                    wait();

                    compute_state_req_dbg.write(8);
                }
            } 
            else if (load_store_op == STORE_OP)
            {
                // Poll consumer's ready to know if we can send new data
                {
                    HLS_PROTO("poll-for-cons-ready");

                    load_state_req = POLL_CONS_READY_REQ;

                    compute_state_req_dbg.write(9);

                    this->compute_load_ready_handshake();
                    wait();
                    this->compute_load_done_handshake();
                    wait();
                }

                // Reset consumer's ready
                {
                    HLS_PROTO("update-cons-ready");

                    store_state_req = UPDATE_CONS_READY_REQ;

                    compute_state_req_dbg.write(10);

                    this->compute_store_ready_handshake();
                    wait();
                    this->compute_store_done_handshake();
                    wait();

                    // Wait for all writes to be done and then issue fence
                    store_state_req = STORE_FENCE;

                    compute_state_req_dbg.write(11);

                    this->compute_store_ready_handshake();
                    wait();
                    this->compute_store_done_handshake();
                    wait();
                }

                // Store output data
                {
                    HLS_PROTO("store-output-data");

                    store_state_req = STORE_DATA_REQ;

                    this->compute_store_ready_handshake();

                    compute_state_req_dbg.write(12);

                    wait();
                    this->compute_store_done_handshake();
                    wait();

                    // Wait for all writes to be done and then issue fence
                    store_state_req = STORE_FENCE;

                    compute_state_req_dbg.write(13);

                    this->compute_store_ready_handshake();
                    wait();
                    this->compute_store_done_handshake();
                    wait();
                }

                // update consumer's ready for new data available
                {
                    HLS_PROTO("update-cons-valid");

                    store_state_req = UPDATE_CONS_VALID_REQ;

                    compute_state_req_dbg.write(14);

                    this->compute_store_ready_handshake();
                    wait();
                    this->compute_store_done_handshake();
                    wait();

                    // Wait for all writes to be done and then issue fence
                    store_state_req = STORE_FENCE;

                    compute_state_req_dbg.write(15);

                    this->compute_store_ready_handshake();
                    wait();
                    this->compute_store_done_handshake();
                    wait();
                }

                // update producer's ready to accept new data
                {
                    HLS_PROTO("update-prod-ready");

                    store_state_req = UPDATE_PROD_READY_REQ;

                    compute_state_req_dbg.write(16);

                    this->compute_store_ready_handshake();
                    wait();
                    this->compute_store_done_handshake();
                    wait();

                    // Wait for all writes to be done and then issue fence
                    store_state_req = STORE_FENCE;

                    compute_state_req_dbg.write(17);

                    this->compute_store_ready_handshake();
                    wait();
                    this->compute_store_done_handshake();
                    wait();
                }
            }
            else // END_OP
            {
                HLS_PROTO("end-acc");

                if (load_store_op == END_OP)
                {
                    store_state_req = ACC_DONE;

                    compute_state_req_dbg.write(15);

                    this->compute_store_ready_handshake();
                    wait();
                    this->compute_store_done_handshake();
                    wait();
                    this->process_done();
                }
            }
        }
    } // while (true)
} // Function : compute_kernel
