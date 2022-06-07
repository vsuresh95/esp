// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __SENSOR_DMA_HPP__
#define __SENSOR_DMA_HPP__

#include "sensor_dma_conf_info.hpp"
#include "sensor_dma_debug_info.hpp"

#include "esp_templates.hpp"

#include "sensor_dma_directives.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/* <<--defines-->> */
#define DATA_WIDTH 64
#define DMA_SIZE SIZE_DWORD
#define PLM_DATA_WORD 12 * 1024
#define PLM_CFG_WORD 10

#define NUM_CFG_REG PLM_CFG_WORD

#define NEW_TASK 0
#define LOAD_STORE 1
#define RD_OP 2
#define RD_SIZE 3
#define RD_SP_OFFSET 4
#define SRC_OFFSET 5
#define WR_OP 6
#define WR_SIZE 7
#define WR_SP_OFFSET 8
#define DST_OFFSET 9

#define POLL_REQ 0
#define CFG_REQ 1
#define LOAD_DATA_REQ 2
#define UPDATE_REQ 0
#define STORE_DATA_REQ 1

class sensor_dma : public esp_accelerator_3P<DMA_WIDTH>
{
public:
    // Constructor
    SC_HAS_PROCESS(sensor_dma);
    sensor_dma(const sc_module_name& name)
    : esp_accelerator_3P<DMA_WIDTH>(name)
        , cfg("config")
    {
        // Signal binding
        cfg.bind_with(*this);

        HLS_PRESERVE_SIGNAL(load_store_dbg, true);
        HLS_PRESERVE_SIGNAL(rd_op_dbg, true);
        HLS_PRESERVE_SIGNAL(rd_size_dbg, true);
        HLS_PRESERVE_SIGNAL(rd_sp_offset_dbg, true);
        HLS_PRESERVE_SIGNAL(src_offset_dbg, true);
        HLS_PRESERVE_SIGNAL(wr_op_dbg, true);
        HLS_PRESERVE_SIGNAL(wr_size_dbg, true);
        HLS_PRESERVE_SIGNAL(wr_sp_offset_dbg, true);
        HLS_PRESERVE_SIGNAL(dst_offset_dbg, true);

        // Map arrays to memories
        /* <<--plm-bind-->> */
        HLS_MAP_plm(plm_data, PLM_DATA_NAME);
        HLS_MAP_plm(plm_cfg, PLM_CFG_NAME);
    }

    sc_signal< sc_int<64> > load_store_dbg;
    sc_signal< sc_int<64> > rd_op_dbg;
    sc_signal< sc_int<64> > rd_size_dbg;
    sc_signal< sc_int<64> > rd_sp_offset_dbg;
    sc_signal< sc_int<64> > src_offset_dbg;
    sc_signal< sc_int<64> > wr_op_dbg;
    sc_signal< sc_int<64> > wr_size_dbg;
    sc_signal< sc_int<64> > wr_sp_offset_dbg;
    sc_signal< sc_int<64> > dst_offset_dbg;

    sc_int<64> load_state_req;
    sc_int<64> store_state_req;

    // Processes

    // Load the input data
    void load_input();

    // Computation
    void compute_kernel();

    // Store the output data
    void store_output();

    // Configure sensor_dma
    esp_config_proc cfg;

    // Functions

    // Private local memories
    sc_dt::sc_int<DATA_WIDTH> plm_data[PLM_DATA_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_cfg[PLM_CFG_WORD];

};


#endif /* __SENSOR_DMA_HPP__ */
