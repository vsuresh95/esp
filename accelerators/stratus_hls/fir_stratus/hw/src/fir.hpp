// Copyright (c) 2011-2019 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __FIR_HPP__
#define __FIR_HPP__

#include "fpdata.hpp"
#include "fir_conf_info.hpp"
#include "fir_debug_info.hpp"

#include "esp_templates.hpp"

#include "fir_directives.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/* <<--defines-->> */
#define MAX_LOGN_SAMPLES 14
#define MAX_NUM_SAMPLES  (1 << MAX_LOGN_SAMPLES)
#define DATA_WIDTH FX_WIDTH

#if (FX_WIDTH == 64)
#define DMA_SIZE SIZE_DWORD
#elif (FX_WIDTH == 32)
#define DMA_SIZE SIZE_WORD
#endif // FX_WIDTH

#define PLM_IN_WORD  (MAX_NUM_SAMPLES << 1)
#define PLM_OUT_WORD (MAX_NUM_SAMPLES << 1)

#define SYNC_VAR_SIZE 2

#define POLL_PREV_REQ 0
#define POLL_NEXT_REQ 1
#define LOAD_DATA_REQ 2
#define UPDATE_PREV_REQ 0
#define UPDATE_NEXT_REQ 1
#define STORE_DATA_REQ 2
#define STORE_FENCE 3
#define ACC_DONE 4

class fir : public esp_accelerator_3P<DMA_WIDTH>
{
public:
    // Handshake between store and load for auto-refills
    handshake_t store_to_load;

    // Compute -> Load
    handshake_t load_ready;

    // Compute -> Store
    handshake_t store_ready;

    // Load -> Compute
    handshake_t load_done;

    // Store -> Compute
    handshake_t store_done;

    // Compute -> Post_proc
    handshake_t post_proc_ready;

    // Compute -> Pre_proc
    handshake_t pre_proc_ready;

    // Post_proc -> Compute
    handshake_t post_proc_done;

    // Pre_proc -> Compute
    handshake_t pre_proc_done;

    // Constructor
    SC_HAS_PROCESS(fir);
    fir(const sc_module_name& name)
    : esp_accelerator_3P<DMA_WIDTH>(name)
        , cfg("config")
        , store_to_load("store-to-load")
        , load_ready("load_ready")
        , store_ready("store_ready")
        , load_done("load_done")
        , store_done("store_done")
        , post_proc_ready("post_proc_ready")
        , pre_proc_ready("pre_proc_ready")
        , post_proc_done("post_proc_done")
        , pre_proc_done("pre_proc_done")
    {
        // Signal binding
        cfg.bind_with(*this);

        // Clock and signal binding for additional handshake
        store_to_load.bind_with(*this);

        HLS_PRESERVE_SIGNAL(load_state_req_dbg, true);
        HLS_PRESERVE_SIGNAL(store_state_req_dbg, true);
        HLS_PRESERVE_SIGNAL(compute_state_req_dbg, true);

        // Map arrays to memories
        /* <<--plm-bind-->> */
        HLS_MAP_plm(A0, PLM_IN_NAME);
        HLS_MAP_plm(F0, PLM_FLT_NAME);
        HLS_MAP_plm(F0, PLM_TW_NAME);
        
        load_ready.bind_with(*this);
        store_ready.bind_with(*this);
        load_done.bind_with(*this);
        store_done.bind_with(*this);
        
        post_proc_ready.bind_with(*this);
        pre_proc_ready.bind_with(*this);
        post_proc_done.bind_with(*this);
        pre_proc_done.bind_with(*this);
    }

    sc_signal< sc_int<32> > load_state_req_dbg;
    sc_signal< sc_int<32> > store_state_req_dbg;
    sc_signal< sc_int<32> > compute_state_req_dbg;

    sc_int<32> load_state_req;
    sc_int<32> store_state_req;

    // Processes

    inline void reset_load_to_store()
    {
        this->store_to_load.req.reset_req();
    }

    inline void reset_store_to_load()
    {
        this->store_to_load.ack.reset_ack();
    }

    inline void load_to_store_handshake()
    {
        HLS_DEFINE_PROTOCOL("load-to-store-handshake");
        this->store_to_load.req.req();
    }

    inline void store_to_load_handshake()
    {
        HLS_DEFINE_PROTOCOL("store-to-load-handshake");
        this->store_to_load.ack.ack();
    }

    // Load the input data
    void load_input();

    // Computation
    void compute_kernel();

    // FFT Post-processing
    void fft_post_proc();

    // IFFT Pre-processing
    void ifft_pre_proc();

    // Store the output data
    void store_output();

    // Configure fir
    esp_config_proc cfg;

    // Private local memories
    sc_dt::sc_int<DATA_WIDTH> A0[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> F0[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> T0[PLM_IN_WORD];

    // Handshakes
    inline void compute_load_ready_handshake();
    inline void load_compute_ready_handshake();
    inline void compute_store_ready_handshake();
    inline void store_compute_ready_handshake();
    inline void compute_load_done_handshake();
    inline void load_compute_done_handshake();
    inline void compute_store_done_handshake();
    inline void store_compute_done_handshake();

    inline void compute_post_proc_ready_handshake();
    inline void post_proc_compute_ready_handshake();
    inline void compute_pre_proc_ready_handshake();
    inline void pre_proc_compute_ready_handshake();
    inline void compute_post_proc_done_handshake();
    inline void post_proc_compute_done_handshake();
    inline void compute_pre_proc_done_handshake();
    inline void pre_proc_compute_done_handshake();
};


#endif /* __FIR_HPP__ */
