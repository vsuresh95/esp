// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __AUDIO_FFT_HPP__
#define __AUDIO_FFT_HPP__

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

#define TEST_INPUT_IS_FULL 0
#define POLL_OUTPUT_IS_EMPTY 1
#define LOAD_DATA_REQ 2
#define UPDATE_INPUT_IS_EMPTY 0
#define UPDATE_OUTPUT_IS_FULL 1
#define STORE_DATA_REQ 2
#define STORE_FENCE 3
#define ACC_DONE 4

#define VALID_OFFSET 0
#define PAYLOAD_OFFSET 2

#define BACKOFF_INIT 8
#define BACKOFF_LIMIT 128
 
class audio_fft : public esp_accelerator_3P<DMA_WIDTH>
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

    // Constructor
    SC_HAS_PROCESS(audio_fft);
    audio_fft(const sc_module_name& name)
    : esp_accelerator_3P<DMA_WIDTH>(name)
        , cfg("config")
        , load_ready("load_ready")
        , store_ready("store_ready")
        , load_done("load_done")
        , store_done("store_done")
    {
        SC_CTHREAD(cycle_counter, this->clk.pos());
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

        // Map arrays to memories
        /* <<--plm-bind-->> */
        HLS_MAP_plm(A0, PLM_IN_NAME);
        
        load_ready.bind_with(*this);
        store_ready.bind_with(*this);
        load_done.bind_with(*this);
        store_done.bind_with(*this);
    }

    sc_signal< sc_int<32> > load_state_req_dbg;
    sc_signal< sc_int<32> > store_state_req_dbg;
    sc_signal< sc_int<32> > compute_state_req_dbg;
    sc_signal< sc_uint<32> > accel_cycles_dbg;
    sc_signal< sc_uint<MAX_CONTEXTS_BITS> > current_context_int_dbg;
    sc_signal< sc_int<1> > switch_context_dbg;
    sc_signal< sc_uint<32> > cycles_elapsed_dbg;
    sc_signal< sc_uint<32> > start_cycles_dbg;
    sc_signal< sc_uint<32> > backoff_count_dbg;

    sc_int<32> load_state_req;
    sc_int<32> store_state_req;
    sc_int<32> input_is_full;
    sc_uint<MAX_CONTEXTS_BITS> current_context_int;
    sc_uint<32> accel_cycles;
    sc_uint<32> cycles_elapsed;
    sc_uint<32> start_cycles;
    
    // Output signal for current context
    sc_out< sc_uint<MAX_CONTEXTS_BITS> > current_context;

    // Processes

    // Load the input data
    void load_input();

    // Computation
    void compute_kernel();

    // Store the output data
    void store_output();

    // Configure audio_fft
    esp_config_proc cfg;

    // Functions
    void fft2_do_shift(unsigned int offset, unsigned int num_samples, unsigned int logn_samples);
    void fft2_bit_reverse(unsigned int offset, unsigned int n, unsigned int bits);

    // Private local memories
    sc_dt::sc_int<DATA_WIDTH> A0[PLM_IN_WORD];

    // Handshakes
    inline void compute_load_ready_handshake();
    inline void load_compute_ready_handshake();
    inline void compute_store_ready_handshake();
    inline void store_compute_ready_handshake();
    inline void compute_load_done_handshake();
    inline void load_compute_done_handshake();
    inline void compute_store_done_handshake();
    inline void store_compute_done_handshake();

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


#endif /* __AUDIO_FFT_HPP__ */
