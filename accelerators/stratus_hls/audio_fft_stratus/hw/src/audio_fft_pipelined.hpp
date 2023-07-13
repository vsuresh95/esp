// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __AUDIO_FFT_PIPELINED_HPP__
#define __AUDIO_FFT_PIPELINED_HPP__

#include "fpdata.hpp"
#include "audio_fft_conf_info.hpp"
#include "audio_fft_debug_info.hpp"

#include "esp_templates.hpp"

#include "audio_fft_directives.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/* <<--defines-->> */
#define DATA_WIDTH 32
#define DMA_SIZE SIZE_WORD

#define PLM_IN_WORD 2048

#define UPDATE_VAR_SIZE 2
#define TEST_VAR_SIZE 2

#define TEST_PROD_VALID_REQ 0
#define TEST_CONS_READY_REQ 1
#define LOAD_DATA_REQ 2
#define UPDATE_PROD_VALID_REQ 0
#define UPDATE_PROD_READY_REQ 1
#define UPDATE_CONS_VALID_REQ 2
#define UPDATE_CONS_READY_REQ 3
#define STORE_DATA_REQ 4
#define STORE_FENCE 5
#define ACC_DONE 6
#define COMPUTE 7

#define INPUT_ASI 1
#define OUTPUT_ASI 2

class audio_fft : public esp_accelerator_3P<DMA_WIDTH>
{
public:
    // Input ASI -> Load
    handshake_t input_load_start;

    // Input ASI -> Store
    handshake_t input_store_start;

    // Output ASI -> Load
    handshake_t output_load_start;

    // Output ASI -> Store
    handshake_t output_store_start;

    // Load -> Input ASI
    handshake_t load_input_done;

    // Store -> Input ASI
    handshake_t store_input_done;

    // Load -> Output ASI
    handshake_t load_output_done;

    // Store -> Output ASI
    handshake_t store_output_done;

    // Input ASI -> Compute
    handshake_t input_to_compute;

    // Compute -> Output ASI 
    handshake_t compute_to_output;

    // Constructor
    SC_HAS_PROCESS(audio_fft);
    audio_fft(const sc_module_name& name)
    : esp_accelerator_3P<DMA_WIDTH>(name)
        , cfg("config")
        , input_load_start("input_load_start")
        , input_store_start("input_store_start")
        , output_load_start("output_load_start")
        , output_store_start("output_store_start")
        , load_input_done("load_input_done")
        , store_input_done("store_input_done")
        , load_output_done("load_output_done")
        , store_output_done("store_output_done")
        , input_to_compute("input_to_compute")
        , compute_to_output("compute_to_output")
    {
        SC_CTHREAD(input_asi_kernel, this->clk.pos());
        this->reset_signal_is(this->rst, false);
        SC_CTHREAD(output_asi_kernel, this->clk.pos());
        this->reset_signal_is(this->rst, false);

        // Signal binding
        cfg.bind_with(*this);

        HLS_PRESERVE_SIGNAL(load_state_req_dbg, true);
        HLS_PRESERVE_SIGNAL(store_state_req_dbg, true);
        HLS_PRESERVE_SIGNAL(input_state_req_dbg, true);
        HLS_PRESERVE_SIGNAL(output_state_req_dbg, true);
        HLS_PRESERVE_SIGNAL(compute_state_req_dbg, true);

        // Map arrays to memories
        /* <<--plm-bind-->> */
        HLS_MAP_plm(A0, PLM_IN_NAME);
        HLS_MAP_plm(A1, PLM_IN_NAME);
        
        input_load_start.bind_with(*this);
        input_store_start.bind_with(*this);
        output_load_start.bind_with(*this);
        output_store_start.bind_with(*this);
        load_input_done.bind_with(*this);
        store_input_done.bind_with(*this);
        load_output_done.bind_with(*this);
        store_output_done.bind_with(*this);
        
        input_to_compute.bind_with(*this);
        compute_to_output.bind_with(*this);
    }

    sc_signal< sc_int<32> > load_state_req_dbg;
    sc_signal< sc_int<32> > store_state_req_dbg;
    sc_signal< sc_int<32> > input_state_req_dbg;
    sc_signal< sc_int<32> > output_state_req_dbg;
    sc_signal< sc_int<32> > compute_state_req_dbg;

    sc_int<32> load_state_req;
    sc_int<32> load_state_req_module;
    sc_int<32> store_state_req;
    sc_int<32> store_state_req_module;

    sc_int<32> prod_valid;
    sc_int<32> last_task;
    sc_int<32> cons_ready;
    sc_int<32> end_acc;

    sc_int<32> input_load_req;
    sc_int<32> output_load_req;
    sc_signal<bool> input_load_req_valid;
    sc_signal<bool> output_load_req_valid;
    sc_int<32> input_store_req;
    sc_int<32> output_store_req;
    sc_signal<bool> input_store_req_valid;
    sc_signal<bool> output_store_req_valid;

    // Processes

    // Load the input data
    void load_input();

    // Computation
    void compute_kernel();

    // Store the output data
    void store_output();

    // ASI submodule for input
    void input_asi_kernel();

    // ASI submodule for output
    void output_asi_kernel();

    // Configure audio_fft
    esp_config_proc cfg;

    // Functions

    // Private local memories
    sc_dt::sc_int<DATA_WIDTH> A0[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> A1[PLM_IN_WORD];

    // Handshakes
    inline void input_load_start_handshake();
    inline void load_input_start_handshake();
    inline void input_store_start_handshake();
    inline void store_input_start_handshake();
    inline void output_load_start_handshake();
    inline void load_output_start_handshake();
    inline void output_store_start_handshake();
    inline void store_output_start_handshake();
    inline void load_input_done_handshake();
    inline void input_load_done_handshake();
    inline void store_input_done_handshake();
    inline void input_store_done_handshake();
    inline void load_output_done_handshake();
    inline void output_load_done_handshake();
    inline void store_output_done_handshake();
    inline void output_store_done_handshake();

    inline void input_compute_handshake();
    inline void compute_input_handshake();
    inline void compute_output_handshake();
    inline void output_compute_handshake();
};


#endif /* __AUDIO_FFT_PIPELINED_HPP__ */
