// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __AUDIO_FIR_HPP__
#define __AUDIO_FIR_HPP__

#include "fpdata.hpp"
#include "audio_fir_conf_info.hpp"
#include "audio_fir_debug_info.hpp"

#include "esp_templates.hpp"

#include "audio_fir_directives.hpp"
#include "avu_mon_info.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/* <<--defines-->> */
#define DATA_WIDTH 32
#define DMA_SIZE SIZE_WORD

#define PLM_IN_WORD 2048
#define PLM_FLT_WORD 2050
#define PLM_TWD_WORD 1024

#define UPDATE_VAR_SIZE 2
#define TEST_VAR_SIZE 2

#define TEST_INPUT_IS_FULL 0
#define POLL_FILTER_IS_FULL 1
#define TEST_OUTPUT_IS_EMPTY 2
#define LOAD_DATA_REQ 3
#define LOAD_FILTERS_REQ 4
#define COMPUTE 5
#define UPDATE_INPUT_IS_EMPTY 6
#define UPDATE_FILTER_IS_EMPTY 7
#define UPDATE_OUTPUT_IS_FULL 8
#define STORE_DATA_REQ 9
#define STORE_FENCE 10
#define ACC_DONE 11

#define VALID_OFFSET 0
#define PAYLOAD_OFFSET 2

#define BACKOFF_INIT 8
#define BACKOFF_LIMIT_INPUT 128
#define BACKOFF_LIMIT_OUTPUT 512

class audio_fir : public esp_accelerator_3P<DMA_WIDTH>
{
public:
    // Compute -> Load
    handshake_t load_ready;

    // Compute -> Store
    handshake_t store_ready;

    // Load -> Compute
    handshake_t load_done;

    // Store -> Compute
    handshake_t store_done;

    // Compute -> Util monitor (Start)
    handshake_t compute_ready;

    // Compute -> Util monitor (End)
    handshake_t compute_done;

    // Constructor
    SC_HAS_PROCESS(audio_fir);
    audio_fir(const sc_module_name& name)
    : esp_accelerator_3P<DMA_WIDTH>(name)
        , cfg("config")
        , load_ready("load_ready")
        , store_ready("store_ready")
        , load_done("load_done")
        , store_done("store_done")
        , compute_ready("compute_ready")
        , compute_done("compute_done")
        , mon_chnl("mon_chnl")
    {
        SC_CTHREAD(cycle_counter, this->clk.pos());
        this->reset_signal_is(this->rst, false);
        
        SC_CTHREAD(util_monitor, this->clk.pos());
        this->reset_signal_is(this->rst, false);

        // Signal binding
        cfg.bind_with(*this);

        HLS_PRESERVE_SIGNAL(load_state_req_dbg, true);
        HLS_PRESERVE_SIGNAL(store_state_req_dbg, true);
        HLS_PRESERVE_SIGNAL(compute_state_req_dbg, true);
        HLS_PRESERVE_SIGNAL(accel_cycles_dbg, true);
        HLS_PRESERVE_SIGNAL(current_context_int_dbg, true);
        HLS_PRESERVE_SIGNAL(switch_context_dbg, true);
        HLS_PRESERVE_SIGNAL(cycles_elapsed_dbg, true);
        HLS_PRESERVE_SIGNAL(start_cycles_dbg, true);
        HLS_PRESERVE_SIGNAL(backoff_count_dbg, true);
        HLS_PRESERVE_SIGNAL(active_cycles_dbg, true);

        // Map arrays to memories
        /* <<--plm-bind-->> */
        HLS_MAP_plm(A0, PLM_IN_NAME);
        HLS_MAP_plm(F0, PLM_FLT_NAME);
        HLS_MAP_plm(T0, PLM_TW_NAME);
        
        load_ready.bind_with(*this);
        store_ready.bind_with(*this);
        load_done.bind_with(*this);
        store_done.bind_with(*this);
        compute_ready.bind_with(*this);
        compute_done.bind_with(*this);
    }

    sc_signal< sc_int<32> > load_state_req_dbg;
    sc_signal< sc_int<32> > store_state_req_dbg;
    sc_signal< sc_int<32> > compute_state_req_dbg;
    sc_signal< sc_uint<32> > accel_cycles_dbg;
    sc_signal< sc_uint<MAX_CONTEXTS_BITS> > current_context_int_dbg;
    sc_signal< sc_int<1> > switch_context_dbg;
    sc_signal< sc_uint<64> > cycles_elapsed_dbg;
    sc_signal< sc_uint<64> > start_cycles_dbg;
    sc_signal< sc_uint<32> > backoff_count_dbg;
    sc_signal< sc_uint<64> > active_cycles_dbg;

    sc_int<32> load_state_req;
    sc_int<32> store_state_req;
    sc_int<32> input_is_full;
    sc_int<32> output_is_empty;
    sc_uint<MAX_CONTEXTS_BITS> current_context_int;
    sc_uint<64> accel_cycles;
    sc_uint<64> start_cycles;
    sc_int<1> discarded_compute;
    
    // Output signal for current context
    sc_out< sc_uint<MAX_CONTEXTS_BITS> > current_context;
    
    // Output channel for monitor metrics
    b_put_initiator<avu_mon_info_t> mon_chnl;

    // Processes

    // Load the input data
    void load_input();

    // Computation
    void compute_kernel();

    // Store the output data
    void store_output();

    // Utilization Monitor
    void util_monitor();

    // Configure audio_fir
    esp_config_proc cfg;

    // Private local memories
    sc_dt::sc_int<DATA_WIDTH> A0[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> F0[PLM_FLT_WORD];
    sc_dt::sc_int<DATA_WIDTH> T0[PLM_TWD_WORD];

    // Handshakes
    inline void compute_load_ready_handshake();
    inline void load_compute_ready_handshake();
    inline void compute_store_ready_handshake();
    inline void store_compute_ready_handshake();
    inline void compute_load_done_handshake();
    inline void load_compute_done_handshake();
    inline void compute_store_done_handshake();
    inline void store_compute_done_handshake();
    inline void compute_util_ready_handshake();
    inline void util_compute_ready_handshake();
    inline void compute_util_done_handshake();
    inline void util_compute_done_handshake();

    void cycle_counter()
    {
        {
            HLS_DEFINE_PROTOCOL("reset-counter");
            accel_cycles = 0;
            wait();
        }        
        
        while(true)
        {
            {
                HLS_DEFINE_PROTOCOL("counter-loop");
                accel_cycles++;
                accel_cycles_dbg.write(accel_cycles);
                wait();
            }
        }
    }
};


#endif /* __AUDIO_FIR_HPP__ */
