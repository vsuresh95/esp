// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __TILED_APP_HPP__
#define __TILED_APP_HPP__

#include "tiled_app_conf_info.hpp"
#include "tiled_app_debug_info.hpp"

#include "esp_templates.hpp"

#include "tiled_app_directives.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/* <<--defines-->> */
//#define DATA_WIDTH 64
#define DMA_SIZE SIZE_DWORD
#define PLM_OUT_WORD 1024
#define PLM_IN_WORD 1024

//#define SYNTH_APP_CFA

#ifdef  ENABLE_SM
class tiled_app : public esp_accelerator_asi<DMA_WIDTH>
#else
class tiled_app : public esp_accelerator_3P<DMA_WIDTH>
#endif
{
public:

    // Constructor
    SC_HAS_PROCESS(tiled_app);
    tiled_app(const sc_module_name& name)
#ifdef  ENABLE_SM
    : esp_accelerator_asi<DMA_WIDTH>(name)
#else
    : esp_accelerator_3P<DMA_WIDTH>(name)
        , cfg("config")
#endif
        , load_done("load_done")
        , store_done("store_done")
        , compute_done("compute_done")
        , load_next_tile("load_next_tile")
        , output_load_start("output_load_start")
        , output_store_start("output_store_start")
        , load_output_done("load_output_done")
        , store_output_done("store_output_done")
    #ifndef SYNTH_APP_CFA
        , compute_0_done("compute_0_done")
        , compute_1_done("compute_1_done")
        , compute_2_done("compute_2_done")
        , compute_3_done("compute_3_done")
        , compute_4_done("compute_4_done")
        , compute_5_done("compute_5_done")
        , compute_6_done("compute_6_done")
        , compute_7_done("compute_7_done")
        , compute_8_done("compute_8_done")
        , compute_9_done("compute_9_done")
        , compute_10_done("compute_10_done")
        , compute_11_done("compute_11_done")
        , compute_12_done("compute_12_done")
        , compute_13_done("compute_13_done")
    #endif
    {
        // Signal binding
#ifndef  ENABLE_SM
        cfg.bind_with(*this);
#endif
	    load_done.bind_with<DMA_WIDTH>(*this);
	    store_done.bind_with<DMA_WIDTH>(*this);
	    compute_done.bind_with<DMA_WIDTH>(*this);
	    load_next_tile.bind_with<DMA_WIDTH>(*this);


        // SC_CTHREAD(input_asi_controller, this->clk.pos());
        // this->reset_signal_is(this->rst, false);
        // SC_CTHREAD(output_asi_controller, this->clk.pos());
        // this->reset_signal_is(this->rst, false);
        // SC_CTHREAD(compute_kernel_0, this->clk.pos());
        // this->reset_signal_is(this->rst, false);

    #ifndef SYNTH_APP_CFA
        SC_CTHREAD(compute_kernel_1, this->clk.pos());
       this->reset_signal_is(this->rst, false);
        SC_CTHREAD(compute_kernel_2, this->clk.pos());
       this->reset_signal_is(this->rst, false);
        SC_CTHREAD(compute_kernel_3, this->clk.pos());
       this->reset_signal_is(this->rst, false);
        SC_CTHREAD(compute_kernel_4, this->clk.pos());
       this->reset_signal_is(this->rst, false);
        SC_CTHREAD(compute_kernel_5, this->clk.pos());
       this->reset_signal_is(this->rst, false);
        SC_CTHREAD(compute_kernel_6, this->clk.pos());
       this->reset_signal_is(this->rst, false);
        SC_CTHREAD(compute_kernel_7, this->clk.pos());
       this->reset_signal_is(this->rst, false);
        SC_CTHREAD(compute_kernel_8, this->clk.pos());
       this->reset_signal_is(this->rst, false);
        SC_CTHREAD(compute_kernel_9, this->clk.pos());
       this->reset_signal_is(this->rst, false);
        SC_CTHREAD(compute_kernel_10, this->clk.pos());
       this->reset_signal_is(this->rst, false);
        SC_CTHREAD(compute_kernel_11, this->clk.pos());
       this->reset_signal_is(this->rst, false);
        SC_CTHREAD(compute_kernel_12, this->clk.pos());
       this->reset_signal_is(this->rst, false);
        SC_CTHREAD(compute_kernel_13, this->clk.pos());
       this->reset_signal_is(this->rst, false);
        SC_CTHREAD(compute_kernel_14, this->clk.pos());
       this->reset_signal_is(this->rst, false);
    #endif

        HLS_PRESERVE_SIGNAL(input_state_req_dbg);
        HLS_PRESERVE_SIGNAL(output_state_req_dbg);
        HLS_PRESERVE_SIGNAL(compute0_state_dbg);

    #ifndef SYNTH_APP_CFA
        HLS_PRESERVE_SIGNAL(compute1_state_dbg);
        HLS_PRESERVE_SIGNAL(compute2_state_dbg);
        HLS_PRESERVE_SIGNAL(compute3_state_dbg);
        HLS_PRESERVE_SIGNAL(compute4_state_dbg);
        HLS_PRESERVE_SIGNAL(compute5_state_dbg);
        HLS_PRESERVE_SIGNAL(compute6_state_dbg);
        HLS_PRESERVE_SIGNAL(compute7_state_dbg);
        HLS_PRESERVE_SIGNAL(compute8_state_dbg);
        HLS_PRESERVE_SIGNAL(compute9_state_dbg);
        HLS_PRESERVE_SIGNAL(compute10_state_dbg);
        HLS_PRESERVE_SIGNAL(compute11_state_dbg);
        HLS_PRESERVE_SIGNAL(compute12_state_dbg);
        HLS_PRESERVE_SIGNAL(compute13_state_dbg);
        HLS_PRESERVE_SIGNAL(compute14_state_dbg);
        HLS_MAP_plm(compute_0_ping, PLM_OUT_NAME);
        HLS_MAP_plm(compute_0_pong, PLM_OUT_NAME);
        HLS_MAP_plm(compute_1_ping, PLM_OUT_NAME);
        HLS_MAP_plm(compute_1_pong, PLM_OUT_NAME);
        HLS_MAP_plm(compute_2_ping, PLM_OUT_NAME);
        HLS_MAP_plm(compute_2_pong, PLM_OUT_NAME);
        HLS_MAP_plm(compute_3_ping, PLM_OUT_NAME);
        HLS_MAP_plm(compute_3_pong, PLM_OUT_NAME);
        HLS_MAP_plm(compute_4_ping, PLM_OUT_NAME);
        HLS_MAP_plm(compute_4_pong, PLM_OUT_NAME);
        HLS_MAP_plm(compute_5_ping, PLM_OUT_NAME);
        HLS_MAP_plm(compute_5_pong, PLM_OUT_NAME);
        HLS_MAP_plm(compute_6_ping, PLM_OUT_NAME);
        HLS_MAP_plm(compute_6_pong, PLM_OUT_NAME);
        HLS_MAP_plm(compute_7_ping, PLM_OUT_NAME);
        HLS_MAP_plm(compute_7_pong, PLM_OUT_NAME);
        HLS_MAP_plm(compute_8_ping, PLM_OUT_NAME);
        HLS_MAP_plm(compute_8_pong, PLM_OUT_NAME);
        HLS_MAP_plm(compute_9_ping, PLM_OUT_NAME);
        HLS_MAP_plm(compute_9_pong, PLM_OUT_NAME);
        HLS_MAP_plm(compute_10_ping, PLM_OUT_NAME);
        HLS_MAP_plm(compute_10_pong, PLM_OUT_NAME);
        HLS_MAP_plm(compute_11_ping, PLM_OUT_NAME);
        HLS_MAP_plm(compute_11_pong, PLM_OUT_NAME);
        HLS_MAP_plm(compute_12_ping, PLM_OUT_NAME);
        HLS_MAP_plm(compute_12_pong, PLM_OUT_NAME);
        HLS_MAP_plm(compute_13_ping, PLM_OUT_NAME);
        HLS_MAP_plm(compute_13_pong, PLM_OUT_NAME);
        HLS_MAP_plm(output_pong, PLM_OUT_NAME);
        HLS_MAP_plm(plm_in_pong, PLM_OUT_NAME);
    #endif

        HLS_PRESERVE_SIGNAL(asi_state_dbg);
        HLS_PRESERVE_SIGNAL(load_iter_dbg);
        HLS_PRESERVE_SIGNAL(store_iter_dbg);
        HLS_PRESERVE_SIGNAL(load_state_dbg);
        HLS_PRESERVE_SIGNAL(store_state_dbg);
        HLS_PRESERVE_SIGNAL(load_unit_sp_write_dbg);
        HLS_PRESERVE_SIGNAL(store_unit_sp_read_dbg);
        // Map arrays to memories
        /* <<--plm-bind-->> */
        // HLS_MAP_plm(plm_in_pong, PLM_OUT_NAME);
        HLS_MAP_plm(output_ping, PLM_OUT_NAME);
        HLS_MAP_plm(plm_in_ping, PLM_OUT_NAME);
        // HLS_MAP_plm(output_ping, PLM_OUT_NAME);
        // HLS_MAP_plm(output_pong, PLM_OUT_NAME);
    }

    // Processes

    sc_signal< sc_int<64> > asi_state_dbg;
    sc_signal< sc_int<64> > compute0_state_dbg;
    //#ifndef SYNTH_APP_CFA
    //sc_signal< sc_int<64> > compute1_state_dbg;
    //sc_signal< sc_int<64> > compute2_state_dbg;
    //#endif
    sc_signal< sc_int<64> > load_iter_dbg;
    sc_signal< sc_int<64> > store_iter_dbg;
    sc_signal< sc_int<64> > load_state_dbg;
    sc_signal< sc_int<64> > store_state_dbg;
    sc_signal< sc_int<64> > load_unit_sp_write_dbg;
    sc_signal< sc_int<64> > store_unit_sp_read_dbg;


    // Load the input data
    void load_input();

    // Computation
    void compute_kernel();

    //#ifndef SYNTH_APP_CFA
    //void compute_kernel_1();
    //void compute_kernel_2();
    //#endif

    // Store the output data
    void store_output();

     // ASI submodule for input
    void input_asi_controller();

    // ASI submodule for output
    void output_asi_controller();



    // Configure tiled_app
#ifndef  ENABLE_SM
    esp_config_proc cfg;
#endif


    sc_int<32> load_state;
    sc_int<32> store_state;

    sc_int<32> prod_valid;
    sc_int<32> flt_valid;
    sc_int<32> cons_ready;
    sc_int<32> end_acc;
    sc_int<32> last_task;

    // Custom handshakes
    // handshake_t load_store_cfg_done;


    // Input ASI -> Load
    handshake_t load_next_tile;  
    // Load -> Input ASI
    handshake_t load_done;
    // Output ASI -> Load
    handshake_t output_load_start;
    // Output ASI -> Store
    handshake_t output_store_start;
    // Store -> Input ASI
    handshake_t store_done;
    // Load -> Output ASI
    handshake_t load_output_done;
    // Store -> Output ASI
    handshake_t store_output_done;
    //#ifndef SYNTH_APP_CFA
    //// compute_kernel_0 -> compute_kernel_1 ASI 
    //handshake_t compute_0_done;
    //// compute_kernel_1 -> compute_kernel_2 ASI 
    //handshake_t compute_1_done;
    //// compute_kernel_2 -> Output ASI 
    //#endif
    handshake_t compute_done;


    // Functions
    
    inline void compute_done_req();
    inline void compute_done_ack();

    inline void load_done_req();
    inline void load_done_ack();
    inline void store_done_req();
    inline void store_done_ack();
    inline void load_next_tile_req(); 
    inline void load_next_tile_ack();

    //monolithic add
    inline void load_output_done_handshake();
    inline void output_load_done_handshake();
    inline void store_output_done_handshake();
    inline void output_store_done_handshake(); 
    inline void output_load_start_handshake();
    inline void load_output_start_handshake();
    inline void output_store_start_handshake();
    inline void store_output_start_handshake();
    //#ifndef SYNTH_APP_CFA
    //inline void compute_0_done_req();
    //inline void compute_0_done_ack();
    //inline void compute_1_done_req();
    //inline void compute_1_done_ack();
    //#endif

    sc_signal< sc_int<32> > load_state_req_dbg;
    sc_signal< sc_int<32> > store_state_req_dbg;
    sc_signal< sc_int<32> > input_state_req_dbg;
    sc_signal< sc_int<32> > output_state_req_dbg;

    sc_int<32> load_state_req_module;
    sc_int<32> store_state_req_module;



    sc_int<32> input_load_req;
    sc_int<32> output_load_req;
    sc_signal<bool> input_load_req_valid;
    sc_signal<bool> output_load_req_valid;
    sc_int<32> input_store_req;
    sc_int<32> output_store_req;
    sc_signal<bool> input_store_req_valid;
    sc_signal<bool> output_store_req_valid;


    // Private local memories
    sc_dt::sc_int<DATA_WIDTH> plm_in_ping[PLM_IN_WORD];
    // sc_dt::sc_int<DATA_WIDTH> plm_in_pong[PLM_IN_WORD];
    //#ifndef SYNTH_APP_CFA
    //sc_dt::sc_int<DATA_WIDTH> compute_0_ping[PLM_IN_WORD];
    //sc_dt::sc_int<DATA_WIDTH> compute_0_pong[PLM_IN_WORD];
    //sc_dt::sc_int<DATA_WIDTH> compute_1_ping[PLM_IN_WORD];
    //sc_dt::sc_int<DATA_WIDTH> compute_1_pong[PLM_IN_WORD];
    //#endif
    sc_dt::sc_int<DATA_WIDTH> output_ping[PLM_IN_WORD];
    // sc_dt::sc_int<DATA_WIDTH> output_pong[PLM_IN_WORD];

#ifndef SYNTH_APP_CFA
    sc_dt::sc_int<DATA_WIDTH> plm_in_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> output_pong[PLM_IN_WORD];
    sc_signal< sc_int<32> >compute1_state_dbg;
    sc_signal< sc_int<32> >compute2_state_dbg;
    sc_signal< sc_int<32> >compute3_state_dbg;
    sc_signal< sc_int<32> >compute4_state_dbg;
    sc_signal< sc_int<32> >compute5_state_dbg;
    sc_signal< sc_int<32> >compute6_state_dbg;
    sc_signal< sc_int<32> >compute7_state_dbg;
    sc_signal< sc_int<32> >compute8_state_dbg;
    sc_signal< sc_int<32> >compute9_state_dbg;
    sc_signal< sc_int<32> >compute10_state_dbg;
    sc_signal< sc_int<32> >compute11_state_dbg;
    sc_signal< sc_int<32> >compute12_state_dbg;
    sc_signal< sc_int<32> >compute13_state_dbg;
    sc_signal< sc_int<32> >compute14_state_dbg;
    void compute_kernel_1();
    void compute_kernel_2();
    void compute_kernel_3();
    void compute_kernel_4();
    void compute_kernel_5();
    void compute_kernel_6();
    void compute_kernel_7();
    void compute_kernel_8();
    void compute_kernel_9();
    void compute_kernel_10();
    void compute_kernel_11();
    void compute_kernel_12();
    void compute_kernel_13();
    void compute_kernel_14();
    // compute_kernel() -> compute_kernel_1() ASI
    handshake_t compute_0_done;
    // compute_kernel_1() -> compute_kernel_2() ASI
    handshake_t compute_1_done;
    // compute_kernel_2() -> compute_kernel_3() ASI
    handshake_t compute_2_done;
    // compute_kernel_3() -> compute_kernel_4() ASI
    handshake_t compute_3_done;
    // compute_kernel_4() -> compute_kernel_5() ASI
    handshake_t compute_4_done;
    // compute_kernel_5() -> compute_kernel_6() ASI
    handshake_t compute_5_done;
    // compute_kernel_6() -> compute_kernel_7() ASI
    handshake_t compute_6_done;
    // compute_kernel_7() -> compute_kernel_8() ASI
    handshake_t compute_7_done;
    // compute_kernel_8() -> compute_kernel_9() ASI
    handshake_t compute_8_done;
    // compute_kernel_9() -> compute_kernel_10() ASI
    handshake_t compute_9_done;
    // compute_kernel_10() -> compute_kernel_11() ASI
    handshake_t compute_10_done;
    // compute_kernel_11() -> compute_kernel_12() ASI
    handshake_t compute_11_done;
    // compute_kernel_12() -> compute_kernel_13() ASI
    handshake_t compute_12_done;
    // compute_kernel_13() -> compute_kernel_14() ASI
    handshake_t compute_13_done;
    inline void compute_0_done_req();
    inline void compute_0_done_ack();
    inline void compute_1_done_req();
    inline void compute_1_done_ack();
    inline void compute_2_done_req();
    inline void compute_2_done_ack();
    inline void compute_3_done_req();
    inline void compute_3_done_ack();
    inline void compute_4_done_req();
    inline void compute_4_done_ack();
    inline void compute_5_done_req();
    inline void compute_5_done_ack();
    inline void compute_6_done_req();
    inline void compute_6_done_ack();
    inline void compute_7_done_req();
    inline void compute_7_done_ack();
    inline void compute_8_done_req();
    inline void compute_8_done_ack();
    inline void compute_9_done_req();
    inline void compute_9_done_ack();
    inline void compute_10_done_req();
    inline void compute_10_done_ack();
    inline void compute_11_done_req();
    inline void compute_11_done_ack();
    inline void compute_12_done_req();
    inline void compute_12_done_ack();
    inline void compute_13_done_req();
    inline void compute_13_done_ack();
    sc_dt::sc_int<DATA_WIDTH> compute_0_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_0_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_1_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_1_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_2_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_2_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_3_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_3_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_4_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_4_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_5_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_5_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_6_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_6_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_7_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_7_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_8_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_8_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_9_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_9_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_10_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_10_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_11_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_11_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_12_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_12_pong[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_13_ping[PLM_IN_WORD];
    sc_dt::sc_int<DATA_WIDTH> compute_13_pong[PLM_IN_WORD];
#endif




};


#endif /* __TILED_APP_HPP__ */
