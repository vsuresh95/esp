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

        this->reset_load_input();

        wait();
    }

    // Config
    /* <<--params-->> */
    int64_t rd_op;
    int64_t rd_size;
    int64_t rd_sp_offset;
    int64_t src_offset;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
    }

    // Load
    while(true)
    {
        wait();

        this->compute_load_ready_handshake();

        // Configuration unit - reading load op, size, src, dst
        {
            rd_op = plm_cfg[RD_OP];
            rd_size = plm_cfg[RD_SIZE];
            rd_sp_offset = plm_cfg[RD_SP_OFFSET];
            src_offset = plm_cfg[SRC_OFFSET];
            src_offset += 10;
        }

        {
            rd_op_dbg.write(rd_op);
            rd_size_dbg.write(rd_size);
            rd_sp_offset_dbg.write(rd_sp_offset);
            src_offset_dbg.write(src_offset);
        }

        // Configure DMA transaction
        dma_info_t dma_info(src_offset, rd_size, DMA_SIZE);

        this->dma_read_ctrl.put(dma_info);

        for (int i = 0; i < rd_size; i++)
        {
            sc_dt::sc_bv<DMA_WIDTH> dataBv;

            dataBv = this->dma_read_chnl.get();
            wait();
            plm_data[rd_sp_offset + i] = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
        }

        // Synchronization unit - update
        {
            dma_info_t dma_info(0, 1, DMA_SIZE);
            sc_dt::sc_bv<DMA_WIDTH> dataBvout;
            dataBvout.range(DMA_WIDTH - 1, 0) = 0;

            this->dma_write_ctrl.put(dma_info);
            wait();
            this->dma_write_chnl.put(dataBvout);
            wait();
        }

        if (rd_op == 1)
        {
            this->accelerator_done();
            this->process_done();
        }

        wait();

        this->load_compute_done_handshake();
    }
}

void sensor_dma::store_output()
{
    // Reset
    {
        HLS_PROTO("store-reset");

        this->reset_store_output();

        wait();
    }

    // Config
    /* <<--params-->> */
    int64_t wr_op;
    int64_t wr_size;
    int64_t wr_sp_offset;
    int64_t dst_offset;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
    }

    // Store
    while(true)
    {
        wait();

        this->store_compute_ready_handshake();

        // Configuration unit - reading store op, size, src, dst
        {
            wr_op = plm_cfg[WR_OP];
            wr_size = plm_cfg[WR_SIZE];
            wr_sp_offset = plm_cfg[WR_SP_OFFSET];
            dst_offset = plm_cfg[DST_OFFSET];
            dst_offset += 10;
        }

        {
            wr_op_dbg.write(wr_op);
            wr_size_dbg.write(wr_size);
            wr_sp_offset_dbg.write(wr_sp_offset);
            dst_offset_dbg.write(dst_offset);
        }

        // Configure DMA transaction
        dma_info_t dma_info(dst_offset, wr_size, DMA_SIZE);

        this->dma_write_ctrl.put(dma_info);

        for (int i = 0; i < wr_size; i++)
        {
            sc_dt::sc_bv<DMA_WIDTH> dataBv;

            wait();
            dataBv.range(DATA_WIDTH - 1, 0) = plm_data[wr_sp_offset + i];
            this->dma_write_chnl.put(dataBv);
        }

        // Synchronization unit - update
        {
            dma_info_t dma_info(0, 1, DMA_SIZE);
            sc_dt::sc_bv<DMA_WIDTH> dataBvout;
            dataBvout.range(DMA_WIDTH - 1, 0) = 0;

            this->dma_write_ctrl.put(dma_info);
            wait();
            this->dma_write_chnl.put(dataBvout);
            wait();
        }

        if (wr_op == 1)
        {
            this->accelerator_done();
            this->process_done();
        }

        wait();

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
        rd_size_dbg.write(0);
        rd_sp_offset_dbg.write(0);
        src_offset_dbg.write(0);
        wr_op_dbg.write(0);
        wr_size_dbg.write(0);
        wr_sp_offset_dbg.write(0);
        dst_offset_dbg.write(0);

        this->reset_compute_kernel();

        wait();
    } 

    int64_t new_task = 0;
    int64_t load_store;

    // Synchronization unit - polling new message location
    while(true)
    {
        {
            dma_info_t dma_info(0, 1, DMA_SIZE);
            sc_dt::sc_bv<DMA_WIDTH> dataBvin;

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

        {
            dma_info_t dma_info(1, 1, DMA_SIZE);
            sc_dt::sc_bv<DMA_WIDTH> dataBvin;

            this->dma_read_ctrl.put(dma_info);
            dataBvin = this->dma_read_chnl.get();
            wait();
            load_store = dataBvin.range(DMA_WIDTH - 1, 0).to_int64();
        }

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

        {
            load_store = plm_cfg[1];
            load_store_dbg.write(load_store);
        
            if (load_store == 1)
            {
                this->compute_store_ready_handshake();
                wait();
                this->store_compute_done_handshake();
                wait();
            }
            else
            {
                this->load_compute_ready_handshake();
                wait();
                this->compute_load_done_handshake();
                wait();
            }
        }     
    }
}
