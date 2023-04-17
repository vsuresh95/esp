// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __AUDIO_FFI_HPP__
#define __AUDIO_FFI_HPP__

#include "fpdata.hpp"
#include "audio_ffi_conf_info.hpp"
#include "audio_ffi_debug_info.hpp"

#include "esp_templates.hpp"

#include "audio_ffi_directives.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/* <<--defines-->> */
#define DATA_WIDTH 32
#define DMA_SIZE SIZE_WORD

#define PLM_IN_WORD 2048
#define PLM_FLT_WORD 2050
#define PLM_TWD_WORD 1024

#define SYNC_VAR_SIZE 10
#define UPDATE_VAR_SIZE 2
#define VALID_FLAG_OFFSET 0
#define READY_FLAG_OFFSET 4
#define FLT_VALID_FLAG_OFFSET 6
#define FLT_READY_FLAG_OFFSET 8

#define TEST_PROD_VALID_REQ 0
#define TEST_FLT_PROD_VALID_REQ 1
#define TEST_CONS_READY_REQ 2
#define LOAD_DATA_REQ 3
#define UPDATE_PROD_VALID_REQ 0
#define UPDATE_FLT_PROD_VALID_REQ 1
#define UPDATE_PROD_READY_REQ 2
#define UPDATE_FLT_PROD_READY_REQ 3
#define UPDATE_CONS_VALID_REQ 4
#define UPDATE_CONS_READY_REQ 5
#define STORE_DATA_REQ 6
#define STORE_FENCE 7
#define ACC_DONE 8

class audio_ffi : public esp_accelerator_3P<DMA_WIDTH>
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

    // Input ASI -> FFT
    handshake_t input_to_fft;

    // FFT -> FIR
    handshake_t fft_to_fir;

    // FIR -> IFFT
    handshake_t fir_to_ifft;

    // IFFT -> Output ASI 
    handshake_t ifft_to_store;

    // Constructor
    SC_HAS_PROCESS(audio_ffi);
    audio_ffi(const sc_module_name& name)
    : esp_accelerator_3P<DMA_WIDTH>(name)
        , cfg("config")
        , load_ready("load_ready")
        , store_ready("store_ready")
        , load_done("load_done")
        , store_done("store_done")
        , input_to_fft("input_to_fft")
        , fft_to_fir("fft_to_fir")
        , fir_to_ifft("fir_to_ifft")
        , ifft_to_store("ifft_to_store")
    {
        SC_CTHREAD(input_asi_kernel, this->clk.pos());
        this->reset_signal_is(this->rst, false);
        SC_CTHREAD(output_asi_kernel, this->clk.pos());
        this->reset_signal_is(this->rst, false);
        SC_CTHREAD(fft_kernel, this->clk.pos());
        this->reset_signal_is(this->rst, false);
        SC_CTHREAD(fir_kernel, this->clk.pos());
        this->reset_signal_is(this->rst, false);
        SC_CTHREAD(ifft_kernel, this->clk.pos());
        this->reset_signal_is(this->rst, false);

        // Signal binding
        cfg.bind_with(*this);

        HLS_PRESERVE_SIGNAL(load_state_req_dbg, true);
        HLS_PRESERVE_SIGNAL(store_state_req_dbg, true);
        HLS_PRESERVE_SIGNAL(input_state_req_dbg, true);
        HLS_PRESERVE_SIGNAL(output_state_req_dbg, true);

        HLS_PRESERVE_SIGNAL(fft_state_dbg, true);
        HLS_PRESERVE_SIGNAL(fir_state_dbg, true);
        HLS_PRESERVE_SIGNAL(ifft_state_dbg, true);

        // Map arrays to memories
        /* <<--plm-bind-->> */
        HLS_MAP_plm(A0, PLM_IN_NAME);
        HLS_MAP_plm(B0, PLM_IN_NAME);
        HLS_MAP_plm(C0, PLM_IN_NAME);
        HLS_MAP_plm(D0, PLM_IN_NAME);
        HLS_MAP_plm(A1, PLM_IN_NAME);
        HLS_MAP_plm(B1, PLM_IN_NAME);
        HLS_MAP_plm(C1, PLM_IN_NAME);
        HLS_MAP_plm(D1, PLM_IN_NAME);

        HLS_MAP_plm(F0, PLM_FLT_NAME);
        HLS_MAP_plm(F1, PLM_FLT_NAME);
        HLS_MAP_plm(T0, PLM_TW_NAME);
        HLS_MAP_plm(T1, PLM_TW_NAME);
        
        load_ready.bind_with(*this);
        store_ready.bind_with(*this);
        load_done.bind_with(*this);
        store_done.bind_with(*this);
        
        input_to_fft.bind_with(*this);
        fft_to_fir.bind_with(*this);
        fir_to_ifft.bind_with(*this);
        ifft_to_store.bind_with(*this);
    }

    sc_signal< sc_int<32> > load_state_req_dbg;
    sc_signal< sc_int<32> > store_state_req_dbg;
    sc_signal< sc_int<32> > input_state_req_dbg;
    sc_signal< sc_int<32> > output_state_req_dbg;

    sc_signal< sc_int<32> > fft_state_dbg;
    sc_signal< sc_int<32> > fir_state_dbg;
    sc_signal< sc_int<32> > ifft_state_dbg;

    sc_int<32> load_state_req;
    sc_int<32> store_state_req;

    sc_int<32> prod_valid;
    sc_int<32> last_task;
    sc_int<32> cons_ready;

    sc_signal<bool> load_arbiter;
    sc_signal<bool> store_arbiter;

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

    // FFT kernel
    void fft_kernel();

    // FIR kernel
    void fir_kernel();

    // IFFT kernel
    void ifft_kernel();

    // Configure audio_ffi
    esp_config_proc cfg;

    // Functions
    void fft2_do_shift(unsigned int offset, unsigned int num_samples, unsigned int logn_samples);
    void fft2_bit_reverse(unsigned int offset, unsigned int n, unsigned int bits);

    // Private local memories
    sc_dt::sc_int<DATA_WIDTH> A0[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> A1[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> B0[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> B1[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> C0[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> C1[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> D0[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> D1[PLM_IN_WORD];

    sc_dt::sc_int<DATA_WIDTH> F0[PLM_FLT_WORD];
    sc_dt::sc_int<DATA_WIDTH> F1[PLM_FLT_WORD];
    sc_dt::sc_int<DATA_WIDTH> T0[PLM_TWD_WORD];
    sc_dt::sc_int<DATA_WIDTH> T1[PLM_TWD_WORD];

    // Handshakes
    inline void compute_load_ready_handshake();
    inline void load_compute_ready_handshake();
    inline void compute_store_ready_handshake();
    inline void store_compute_ready_handshake();
    inline void compute_load_done_handshake();
    inline void load_compute_done_handshake();
    inline void compute_store_done_handshake();
    inline void store_compute_done_handshake();

    inline void input_fft_handshake();
    inline void fft_input_handshake();
    inline void fft_fir_handshake();
    inline void fir_fft_handshake();
    inline void fir_ifft_handshake();
    inline void ifft_fir_handshake();
    inline void ifft_output_handshake();
    inline void output_ifft_handshake();
};


#endif /* __AUDIO_FFI_HPP__ */
