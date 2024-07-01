// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __SORT_HPP__
#define __SORT_HPP__

#include "sort_conf_info.hpp"
#include "sort_debug_info.hpp"

#include "esp_templates.hpp"

#include "sort_directives.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/* <<--defines-->> */
#define DATA_WIDTH 32
#define DMA_SIZE SIZE_WORD

#define PLM_IN_WORD 2048

#define UPDATE_VAR_SIZE 2
#define TEST_VAR_SIZE 2

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
#define COMPUTE 7

class sort : public esp_accelerator_3P<DMA_WIDTH>
{
    public:

        // Computation <-> Computation

        // Ack and req channel
        handshake_t compute_2_ready;

        // Compute -> Load
        handshake_t load_ready;

        // Compute 2 -> Store
        handshake_t store_ready;

        // Load -> Compute
        handshake_t load_done;

        // Store -> Compute 2
        handshake_t store_done;

        // Constructor
        SC_HAS_PROCESS(sort);
        sort(const sc_module_name& name)
          : esp_accelerator_3P<DMA_WIDTH>(name)
          , cfg("config")
        , compute_2_ready("compute_2_ready")
        , load_ready("load_ready")
        , store_ready("store_ready")
        , load_done("load_done")
        , store_done("store_done")
        {
            // Signal binding
            cfg.bind_with(*this);

            HLS_PRESERVE_SIGNAL(load_state_req_dbg, true);
            HLS_PRESERVE_SIGNAL(store_state_req_dbg, true);
            HLS_PRESERVE_SIGNAL(compute_state_req_dbg, true);
            HLS_PRESERVE_SIGNAL(compute_2_state_req_dbg, true);

            // // for debugging
            HLS_PRESERVE_SIGNAL(compute_stage_dbg, true);
            HLS_PRESERVE_SIGNAL(compute_2_stage_dbg, true);

            SC_CTHREAD(compute_2_kernel, this->clk.pos());
            reset_signal_is(this->rst, false);
            // set_stack_size(0x400000);

            // Clock and reset binding for additional handshake
            compute_2_ready.bind_with(*this);

            load_ready.bind_with(*this);
            store_ready.bind_with(*this);
            load_done.bind_with(*this);
            store_done.bind_with(*this);

	    // Binding for memories
	    HLS_MAP_A0;
	    // HLS_MAP_A1;
	    HLS_MAP_B0;
	    // HLS_MAP_B1;
	    HLS_MAP_C0;
	    // HLS_MAP_C1;
        }

        sc_signal< sc_int<32> > load_state_req_dbg;
        sc_signal< sc_int<32> > store_state_req_dbg;
        sc_signal< sc_int<32> > compute_state_req_dbg;
        sc_signal< sc_int<32> > compute_2_state_req_dbg;

        // // for debugging
        sc_signal< sc_int<32> > compute_stage_dbg;
        sc_signal< sc_int<32> > compute_2_stage_dbg;

        sc_int<32> load_state_req;
        sc_int<32> store_state_req;

        sc_int<32> last_task;

        // Processes

        // Load the input data
        void load_input();

        // Realize the computation
        void compute_kernel();
        void compute_2_kernel();

        // Store the output data
        void store_output();

        // Configure sort
        esp_config_proc cfg;

        // Functions

        // Reset first compute_kernel channels
        inline void reset_compute_1_kernel();

        // Reset second compute_kernel channels
        inline void reset_compute_2_kernel();

        // Handshake callable by compute_kernel
        inline void compute_compute_2_handshake();

        // Handshake callable by store_output
        inline void compute_2_compute_handshake();

        // Private local memories
	uint32_t A0[LEN];
	// uint32_t A1[LEN];

	uint32_t B0[LEN];
	// uint32_t B1[LEN];
	uint32_t C0[LEN / NUM][NUM];
	// uint32_t C1[LEN / NUM][NUM];

    // Handshakes
    inline void compute_load_ready_handshake();
    inline void load_compute_ready_handshake();
    inline void compute_2_store_ready_handshake();
    inline void store_compute_2_ready_handshake();
    inline void compute_load_done_handshake();
    inline void load_compute_done_handshake();
    inline void compute_2_store_done_handshake();
    inline void store_compute_2_done_handshake();
};

inline void sort::reset_compute_1_kernel()
{
    this->input_ready.ack.reset_ack();
    this->compute_2_ready.req.reset_req();
}

inline void sort::reset_compute_2_kernel()
{
    this->compute_2_ready.ack.reset_ack();
    this->output_ready.req.reset_req();
}

inline void sort::compute_compute_2_handshake()
{
	HLS_DEFINE_PROTOCOL("compute-compute_2-handshake");

	this->compute_2_ready.req.req();
}

inline void sort::compute_2_compute_handshake()
{
	HLS_DEFINE_PROTOCOL("compute_2-compute-handshake");

	this->compute_2_ready.ack.ack();
}


#endif /* __SORT_HPP__ */
