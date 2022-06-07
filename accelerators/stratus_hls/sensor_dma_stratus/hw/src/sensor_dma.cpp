// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "sensor_dma.hpp"
#include "sensor_dma_directives.hpp"

// Functions

#include "sensor_dma_functions.hpp"

// Processes

void sensor_dma::load_input()
{

    // Reset
    {
        HLS_PROTO("load-reset");

        rd_size_dbg.write(0);
        rd_sp_offset_dbg.write(0);
        src_offset_dbg.write(0);

        this->reset_load_input();

        wait();
    }

    // Config
    /* <<--params-->> */
    int64_t rd_size;
    int64_t rd_sp_offset;
    int64_t src_offset;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process

        // User-defined config code
        /* <<--local-params-->> */
    }

    // Load
    while(true)
    {
        this->compute_load_ready_handshake();

        switch (load_state_req)
        {
            case POLL_REQ:
            {
                dma_info_t dma_info(0, 1, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBvin;
                int64_t new_task = 0;

                //Wait for 1
                while (new_task != 1)
                {
                    HLS_UNROLL_LOOP(OFF);
                    this->dma_read_ctrl.put(dma_info);
                    dataBvin = this->dma_read_chnl.get();
                    wait();
                    new_task = dataBvin.range(DMA_WIDTH - 1, 0).to_int64();
                }
            }
            break;
            case CFG_REQ:
            {
                dma_info_t dma_info(0, NUM_CFG_REG, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBvin;

                this->dma_read_ctrl.put(dma_info);

                for (int i = 0; i < NUM_CFG_REG; i++)
                {
                    dataBvin = this->dma_read_chnl.get();
                    wait();
                    plm_cfg[i] = dataBvin.range(DMA_WIDTH - 1, 0).to_int64();
                }
            }
            break;
            case LOAD_DATA_REQ:
            {
                // Configuration unit - reading load op, size, src, dst
                {
                    rd_size = plm_cfg[RD_SIZE];
                    rd_sp_offset = plm_cfg[RD_SP_OFFSET];
                    src_offset = plm_cfg[SRC_OFFSET];
                    src_offset += 10;
                }

                {
                    rd_size_dbg.write(rd_size);
                    rd_sp_offset_dbg.write(rd_sp_offset);
                    src_offset_dbg.write(src_offset);
                }

                // Main load operation
                {
                    dma_info_t dma_info(src_offset, rd_size, DMA_SIZE);
                    sc_dt::sc_bv<DMA_WIDTH> dataBv;

                    this->dma_read_ctrl.put(dma_info);

                    for (int i = 0; i < rd_size; i++)
                    {
                        dataBv = this->dma_read_chnl.get();
                        wait();
                        plm_data[rd_sp_offset + i] = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
                    }
                }
            }
            break;
            default:
            break;
        }

        this->load_compute_done_handshake();
    }
}

void sensor_dma::store_output()
{
    // Reset
    {
        HLS_PROTO("store-reset");

        wr_size_dbg.write(0);
        wr_sp_offset_dbg.write(0);
        dst_offset_dbg.write(0);

        this->reset_store_output();

        wait();
    }

    // Config
    /* <<--params-->> */
    int64_t wr_size;
    int64_t wr_sp_offset;
    int64_t dst_offset;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process

        // User-defined config code
        /* <<--local-params-->> */
    }

    // Store
    while(true)
    {
        this->store_compute_ready_handshake();

        switch (store_state_req)
        {
            case UPDATE_REQ:
            {
                dma_info_t dma_info(0, 1, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBvout;
                dataBvout.range(DMA_WIDTH - 1, 0) = 0;

                this->dma_write_ctrl.put(dma_info);
                wait();
                this->dma_write_chnl.put(dataBvout);
                wait();
            }
            break;
            case STORE_DATA_REQ:
            {
                // Configuration unit - reading store op, size, src, dst
                {
                    wr_size = plm_cfg[WR_SIZE];
                    wr_sp_offset = plm_cfg[WR_SP_OFFSET];
                    dst_offset = plm_cfg[DST_OFFSET];
                    dst_offset += 10;
                }

                {
                    wr_size_dbg.write(wr_size);
                    wr_sp_offset_dbg.write(wr_sp_offset);
                    dst_offset_dbg.write(dst_offset);
                }

                {
                    dma_info_t dma_info(dst_offset, wr_size, DMA_SIZE);
                    sc_dt::sc_bv<DMA_WIDTH> dataBv;

                    this->dma_write_ctrl.put(dma_info);

                    for (int i = 0; i < wr_size; i++)
                    {
                        wait();
                        dataBv.range(DATA_WIDTH - 1, 0) = plm_data[wr_sp_offset + i];
                        this->dma_write_chnl.put(dataBv);
                    }
                }
            }
            break;
            default:
            break;
        }

        this->compute_store_done_handshake();
    }
}

void sensor_dma::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-reset");

        load_store_dbg.write(0);
        rd_op_dbg.write(0);
        wr_op_dbg.write(0);

        load_state_req = 0;
        store_state_req = 0;

        this->reset_compute_kernel();

        wait();
    } 

    int64_t load_store;
    int64_t rd_op;
    int64_t wr_op;

    // Synchronization unit - polling new message location
    while(true)
    {
        // Poll for new task
        {
            HLS_PROTO("poll-for-new-task");

            load_state_req = POLL_REQ;

            this->load_compute_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();
        }

        // Read config registers
        {
            HLS_PROTO("read-config-registers");

            load_state_req = CFG_REQ;

            this->load_compute_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();
        }

        // Schedule new task
        {
            load_store = plm_cfg[1];
            rd_op = plm_cfg[RD_OP];
            wr_op = plm_cfg[WR_OP];

            load_store_dbg.write(load_store);
            rd_op_dbg.write(rd_op);
            wr_op_dbg.write(wr_op);
        
            if (load_store == 1)
            {
                HLS_PROTO("schedule-new-task-1");

                store_state_req = STORE_DATA_REQ;

                this->compute_store_ready_handshake();
                wait();
                this->store_compute_done_handshake();
                wait();
            }
            else
            {
                HLS_PROTO("schedule-new-task-0");

                load_state_req = LOAD_DATA_REQ;

                this->load_compute_ready_handshake();
                wait();
                this->compute_load_done_handshake();
                wait();
            }
        }

        // Update lock
        {
            HLS_PROTO("schedule-new-task");

            store_state_req = UPDATE_REQ;

            this->compute_store_ready_handshake();
            wait();
            this->store_compute_done_handshake();
            wait();
        }

        // Decide next iteration
        {
            if (rd_op == 1 || wr_op == 1)
            {
                this->accelerator_done();
                this->process_done();
            }
        }
    }
}
