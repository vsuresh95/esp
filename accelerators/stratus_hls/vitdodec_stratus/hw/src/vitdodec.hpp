// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __VITDODEC_HPP__
#define __VITDODEC_HPP__

#include "vitdodec_conf_info.hpp"
#include "vitdodec_debug_info.hpp"

#include "esp_templates.hpp"

#include "vitdodec_directives.hpp"

#define ENABLE_SM

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/* <<--defines-->> */
#define DATA_WIDTH 8
#define DMA_SIZE SIZE_BYTE
#define PLM_OUT_WORD 18592
#define PLM_IN_WORD 24856

#define TRACEBACK_MAX   11

class vitdodec : public esp_accelerator_3P<DMA_WIDTH>
{
public:
    void asi_controller();
    // Constructor
    SC_HAS_PROCESS(vitdodec);
    vitdodec(const sc_module_name& name)
    : esp_accelerator_3P<DMA_WIDTH>(name)
        , cfg("config")
        , load_done("load_done")
        , store_done("store_done")
        , compute_done("compute_done")
        , load_next_tile("load_next_tile")
    {
        // Signal binding
        cfg.bind_with(*this);

	    load_done.bind_with<DMA_WIDTH>(*this);
	    store_done.bind_with<DMA_WIDTH>(*this);
	    compute_done.bind_with<DMA_WIDTH>(*this);
	    load_next_tile.bind_with<DMA_WIDTH>(*this);


        SC_CTHREAD(asi_controller, this->clk.pos());
        this->reset_signal_is(this->rst, false);

        HLS_PRESERVE_SIGNAL(compute_state_dbg);
        HLS_PRESERVE_SIGNAL(asi_state_dbg);
        HLS_PRESERVE_SIGNAL(load_iter_dbg);
        HLS_PRESERVE_SIGNAL(store_iter_dbg);
        HLS_PRESERVE_SIGNAL(load_state_dbg);
        HLS_PRESERVE_SIGNAL(store_state_dbg);
        HLS_PRESERVE_SIGNAL(load_unit_sp_write_dbg);
        HLS_PRESERVE_SIGNAL(store_unit_sp_read_dbg);
        // Map arrays to memories
        /* <<--plm-bind-->> */
        //HLS_MAP_plm(plm_out_pong, PLM_OUT_NAME);
        HLS_MAP_plm(plm_out_ping, PLM_OUT_NAME);
        //HLS_MAP_plm(plm_in_pong, PLM_IN_NAME);
        HLS_MAP_plm(plm_in_ping, PLM_IN_NAME);
    }

    // Processes

    // Load the input data
    void load_input();

    // Computation
    void compute_kernel();

    // Store the output data
    void store_output();

    // Configure vitdodec
    esp_config_proc cfg;

    // Functions
    
    inline void compute_done_req();
    inline void compute_done_ack();

    inline void load_done_req();
    inline void load_done_ack();
    inline void store_done_req();
    inline void store_done_ack();
    inline void load_next_tile_req(); 
    inline void load_next_tile_ack();

    // Private local memories
    sc_dt::sc_int<DATA_WIDTH> plm_in_ping[PLM_IN_WORD];
    //sc_dt::sc_int<DATA_WIDTH> plm_in_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> plm_out_ping[PLM_OUT_WORD];
    //sc_dt::sc_int<DATA_WIDTH> plm_out_pong[PLM_OUT_WORD];

    //handshakes

    handshake_t compute_done;
    handshake_t load_done;
    handshake_t store_done;
    handshake_t load_next_tile;

    //signals
    sc_int<32> load_state;
    sc_int<32> store_state;
    sc_int<32> last_task;

    sc_signal< sc_int<64> > asi_state_dbg;
    sc_signal< sc_int<64> > compute_state_dbg;
    sc_signal< sc_int<64> > load_iter_dbg;
    sc_signal< sc_int<64> > store_iter_dbg;
    sc_signal< sc_int<64> > load_state_dbg;
    sc_signal< sc_int<64> > store_state_dbg;
    sc_signal< sc_int<64> > load_unit_sp_write_dbg;
    sc_signal< sc_int<64> > store_unit_sp_read_dbg;

};


#endif /* __VITDODEC_HPP__ */
