// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __ROTATE_ORDER_HPP__
#define __ROTATE_ORDER_HPP__

#include "fpdata.hpp"
#include "rotate_order_conf_info.hpp"
#include "rotate_order_debug_info.hpp"

#include "esp_templates.hpp"

#include "rotate_order_directives.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/* <<--defines-->> */
#define DATA_WIDTH 32
#define DMA_SIZE SIZE_WORD
#define PLM_IN_WORD 16384

#define SYNC_VAR_SIZE 10
#define UPDATE_VAR_SIZE 2
#define VALID_FLAG_OFFSET 0
#define READY_FLAG_OFFSET 4

#define POLL_PROD_VALID_REQ 0
#define POLL_CONS_READY_REQ 1
#define LOAD_DATA_REQ 2
#define UPDATE_PROD_VALID_REQ 0
#define UPDATE_PROD_READY_REQ 1
#define UPDATE_CONS_VALID_REQ 2
#define UPDATE_CONS_READY_REQ 3
#define STORE_DATA_REQ 4
#define STORE_FENCE 5
#define ACC_DONE 6

enum ch_rotate_order_1 { kY, kZ, kX };
enum ch_rotate_order_2 { kV, kT, kR, kS, kU };
enum ch_rotate_order_3 { kQ, kO, kM, kK, kL, kN, kP };

class rotate_order : public esp_accelerator_3P<DMA_WIDTH>
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
    
    // Compute -> Rotate Order 1
    handshake_t rotate_1_ready;

    // Rotate Order 1 -> Compute
    handshake_t rotate_1_done;

    // Compute -> Rotate Order 2
    handshake_t rotate_2_ready;

    // Rotate Order 2 -> Compute
    handshake_t rotate_2_done;

    // Compute -> Rotate Order 3
    handshake_t rotate_3_ready;

    // Rotate Order 3 -> Compute
    handshake_t rotate_3_done;

    // Constructor
    SC_HAS_PROCESS(rotate_order);
    rotate_order(const sc_module_name& name)
    : esp_accelerator_3P<DMA_WIDTH>(name)
        , cfg("config")
        , load_ready("load_ready")
        , store_ready("store_ready")
        , load_done("load_done")
        , store_done("store_done")
        , rotate_1_ready("rotate_1_ready")
        , rotate_1_done("rotate_1_done")
        , rotate_2_ready("rotate_2_ready")
        , rotate_2_done("rotate_2_done")
        , rotate_3_ready("rotate_3_ready")
        , rotate_3_done("rotate_3_done")
    {
        // Signal binding
        cfg.bind_with(*this);
        
        HLS_PRESERVE_SIGNAL(load_state_req_dbg, true);
        HLS_PRESERVE_SIGNAL(store_state_req_dbg, true);
        HLS_PRESERVE_SIGNAL(compute_state_req_dbg, true);

        // Map arrays to memories
        /* <<--plm-bind-->> */
        HLS_MAP_plm(plm_ppfChannels, PLM_IN_NAME);

        load_ready.bind_with(*this);
        store_ready.bind_with(*this);
        load_done.bind_with(*this);
        store_done.bind_with(*this);

        SC_CTHREAD(rotate_order_1, this->clk.pos());
        this->reset_signal_is(this->rst, false);
        SC_CTHREAD(rotate_order_2, this->clk.pos());
        this->reset_signal_is(this->rst, false);
        SC_CTHREAD(rotate_order_3, this->clk.pos());
        this->reset_signal_is(this->rst, false);
    }

    sc_signal< sc_int<32> > load_state_req_dbg;
    sc_signal< sc_int<32> > store_state_req_dbg;
    sc_signal< sc_int<32> > compute_state_req_dbg;

    sc_int<32> load_state_req;
    sc_int<32> store_state_req;

    sc_int<32> last_task;

    sc_int<8> rotate_channels_ready;
    sc_int<8> rotate_channels_done;

    FPDATA m_fCosAlpha;
    FPDATA m_fSinAlpha;
    FPDATA m_fCosBeta;
    FPDATA m_fSinBeta;
    FPDATA m_fCosGamma;
    FPDATA m_fSinGamma;
    FPDATA m_fCos2Alpha;
    FPDATA m_fSin2Alpha;
    FPDATA m_fCos2Beta;
    FPDATA m_fSin2Beta;
    FPDATA m_fCos2Gamma;
    FPDATA m_fSin2Gamma;
    FPDATA m_fCos3Alpha;
    FPDATA m_fSin3Alpha;
    FPDATA m_fCos3Beta;
    FPDATA m_fSin3Beta;
    FPDATA m_fCos3Gamma;
    FPDATA m_fSin3Gamma;

    // Processes

    // Load the input data
    void load_input();

    // Computation
    void compute_kernel();

    // Store the output data
    void store_output();

    // Rotate Order 1
    void rotate_order_1();

    // Rotate Order 2
    void rotate_order_2();

    // Rotate Order 3
    void rotate_order_3();

    // Configure rotate_order
    esp_config_proc cfg;

    // Functions

    // Private local memories
    sc_dt::sc_int<DATA_WIDTH> plm_ppfChannels[PLM_IN_WORD];

    // Handshakes
    inline void compute_load_ready_handshake();
    inline void load_compute_ready_handshake();
    inline void compute_store_ready_handshake();
    inline void store_compute_ready_handshake();
    inline void compute_load_done_handshake();
    inline void load_compute_done_handshake();
    inline void compute_store_done_handshake();
    inline void store_compute_done_handshake();

    // Rotate Handshakes
    inline void compute_rotate_1_ready_handshake();
    inline void rotate_1_compute_ready_handshake();
    inline void compute_rotate_1_done_handshake();
    inline void rotate_1_compute_done_handshake();

    inline void compute_rotate_2_ready_handshake();
    inline void rotate_2_compute_ready_handshake();
    inline void compute_rotate_2_done_handshake();
    inline void rotate_2_compute_done_handshake();

    inline void compute_rotate_3_ready_handshake();
    inline void rotate_3_compute_ready_handshake();
    inline void compute_rotate_3_done_handshake();
    inline void rotate_3_compute_done_handshake();    
};


#endif /* __ROTATE_ORDER_HPP__ */
