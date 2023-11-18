// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __GEMM_HPP__
#define __GEMM_HPP__

#include "gemm_data.hpp"
#include "fpdata.hpp"
#include "gemm_conf_info.hpp"
#include "gemm_debug_info.hpp"
#include "esp_templates.hpp"
#include "gemm_directives.hpp"
#include "common.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)

class gemm : public esp_accelerator_3P<DMA_WIDTH>
{
public:

    // Constructor
    SC_HAS_PROCESS(gemm);
    gemm(const sc_module_name& name)
	: esp_accelerator_3P<DMA_WIDTH>(name)
	, cfg("config")
        , load_done("load_done")
        , store_done("store_done")
        , compute_done("compute_done")
        , output_done("output_done")
        , load_compute_cfg_done("load_compute_cfg_done")
        , load_store_cfg_done("load_store_cfg_done")
        , load_next_tile("load_next_tile")
        , output_load_start("output_load_start")
        , output_store_start("output_store_start")
        , load_output_done("load_output_done")
        , store_output_done("store_output_done")
        {
            // Signal binding
            cfg.bind_with(*this);
            output_done.bind_with<DMA_WIDTH>(*this);
            load_compute_cfg_done.bind_with<DMA_WIDTH>(*this);
            load_store_cfg_done.bind_with<DMA_WIDTH>(*this);
            load_done.bind_with<DMA_WIDTH>(*this);
            store_done.bind_with<DMA_WIDTH>(*this);
            compute_done.bind_with<DMA_WIDTH>(*this);
            load_next_tile.bind_with<DMA_WIDTH>(*this);
            
            SC_CTHREAD(input_asi_controller, this->clk.pos());
            this->reset_signal_is(this->rst, false);
            SC_CTHREAD(output_asi_controller, this->clk.pos());
            this->reset_signal_is(this->rst, false);


            HLS_PRESERVE_SIGNAL(input_state_req_dbg);
            HLS_PRESERVE_SIGNAL(output_state_req_dbg);
            HLS_PRESERVE_SIGNAL(op_asi_state_dbg);
            HLS_PRESERVE_SIGNAL(compute_state_dbg);
            HLS_PRESERVE_SIGNAL(asi_state_dbg);
            HLS_PRESERVE_SIGNAL(load_iter_dbg);
            HLS_PRESERVE_SIGNAL(store_iter_dbg);
            HLS_PRESERVE_SIGNAL(load_state_dbg);
            HLS_PRESERVE_SIGNAL(store_state_dbg);
            HLS_PRESERVE_SIGNAL(load_unit_sp_write_dbg);
            HLS_PRESERVE_SIGNAL(store_unit_sp_read_dbg);


            HLS_PRESERVE_SIGNAL(size_matrix_out_sig);
            HLS_PRESERVE_SIGNAL(size_matrix1_sig);
            HLS_PRESERVE_SIGNAL(size_matrix2_sig);
            HLS_PRESERVE_SIGNAL(matrix_chk_in_sig);
            HLS_PRESERVE_SIGNAL(matrix_rem_in1_sig);
            HLS_PRESERVE_SIGNAL(matrix_rem_in2_sig);
            HLS_PRESERVE_SIGNAL(matrix_chk_out_sig);
            HLS_PRESERVE_SIGNAL(matrix_rem_out_sig);
            HLS_PRESERVE_SIGNAL(load_cfg_sig);
            HLS_PRESERVE_SIGNAL(loadable_rows_sig);
            HLS_PRESERVE_SIGNAL(loadable_chunk_sig);
            HLS_PRESERVE_SIGNAL(index_d1_incr_sig);
            HLS_PRESERVE_SIGNAL(gemm_st_offset);
            HLS_PRESERVE_SIGNAL(ninputs_sig);
            HLS_PRESERVE_SIGNAL(d1_sig);
            HLS_PRESERVE_SIGNAL(d2_sig);
            HLS_PRESERVE_SIGNAL(d3_sig);
            HLS_PRESERVE_SIGNAL(transpose_sig);
            HLS_PRESERVE_SIGNAL(do_relu_sig);

            HLS_PRESERVE_SIGNAL(pingpong_m1_sig);
            HLS_PRESERVE_SIGNAL(pingpong_m2_sig);

            #if (PARALLELISM >= 8)
            HLS_PRESERVE_SIGNAL(row_0);
            HLS_PRESERVE_SIGNAL(row_1);
            HLS_PRESERVE_SIGNAL(row_2);
            HLS_PRESERVE_SIGNAL(row_3);
            HLS_PRESERVE_SIGNAL(row_4);
            HLS_PRESERVE_SIGNAL(row_5);
            HLS_PRESERVE_SIGNAL(row_6);
            HLS_PRESERVE_SIGNAL(row_7);
            HLS_PRESERVE_SIGNAL(col_0);
            HLS_PRESERVE_SIGNAL(col_1);
            HLS_PRESERVE_SIGNAL(col_2);
            HLS_PRESERVE_SIGNAL(col_3);
            HLS_PRESERVE_SIGNAL(col_4);
            HLS_PRESERVE_SIGNAL(col_5);
            HLS_PRESERVE_SIGNAL(col_6);
            HLS_PRESERVE_SIGNAL(col_7);

            HLS_PRESERVE_SIGNAL(mult_out_sig_0);
            HLS_PRESERVE_SIGNAL(mult_out_sig_1);
            HLS_PRESERVE_SIGNAL(mult_out_sig_2);
            HLS_PRESERVE_SIGNAL(mult_out_sig_3);
            HLS_PRESERVE_SIGNAL(mult_out_sig_4);
            HLS_PRESERVE_SIGNAL(mult_out_sig_5);
            HLS_PRESERVE_SIGNAL(mult_out_sig_6);
            HLS_PRESERVE_SIGNAL(mult_out_sig_7);
            #endif

            // Flatten arrays
            HLS_FLATTEN_ARRAY(mult_out);
            HLS_FLATTEN_ARRAY(row);
            HLS_FLATTEN_ARRAY(col);

            // Map memories
            HLS_MAP_plm(input0, IN_PLM_NAME);
            HLS_MAP_plm(input1, IN_PLM_NAME);
            HLS_MAP_plm(input2, IN_PLM_NAME);
            HLS_MAP_plm(input3, IN_PLM_NAME);
            HLS_MAP_plm(output0, OUT_PLM_NAME);
            HLS_MAP_plm(output1, OUT_PLM_NAME);
        }

    // Processes

    // Load the input data
    void load_input();

    // Computation
    void compute_kernel();

    // Store the output data
    void store_output();

    // Configure gemm
    esp_config_proc cfg;

    // Custom handshakes
    handshake_t output_done;
    handshake_t load_compute_cfg_done;
    handshake_t load_store_cfg_done;

    sc_signal< sc_int<64> > asi_state_dbg;
    sc_signal< sc_int<64> > compute_state_dbg;
    sc_signal< sc_int<64> > op_asi_state_dbg;
    sc_signal< sc_int<64> > load_iter_dbg;
    sc_signal< sc_int<64> > store_iter_dbg;
    sc_signal< sc_int<64> > load_state_dbg;
    sc_signal< sc_int<64> > store_state_dbg;
    sc_signal< sc_int<64> > load_unit_sp_write_dbg;
    sc_signal< sc_int<64> > store_unit_sp_read_dbg;

    sc_signal< sc_int<1> > pingpong_m1_sig;
    sc_signal< sc_int<1> > pingpong_m2_sig;
    #if (PARALLELISM >= 8)
    sc_signal< sc_int<32> > row_0;
    sc_signal< sc_int<32> > row_1;
    sc_signal< sc_int<32> > row_2;
    sc_signal< sc_int<32> > row_3;
    sc_signal< sc_int<32> > row_4;
    sc_signal< sc_int<32> > row_5;
    sc_signal< sc_int<32> > row_6;
    sc_signal< sc_int<32> > row_7;
    sc_signal< sc_int<32> > col_0;
    sc_signal< sc_int<32> > col_1;
    sc_signal< sc_int<32> > col_2;
    sc_signal< sc_int<32> > col_3;
    sc_signal< sc_int<32> > col_4;
    sc_signal< sc_int<32> > col_5;
    sc_signal< sc_int<32> > col_6;
    sc_signal< sc_int<32> > col_7;

    sc_signal< sc_int<32> > mult_out_sig_0;
    sc_signal< sc_int<32> > mult_out_sig_1;
    sc_signal< sc_int<32> > mult_out_sig_2;
    sc_signal< sc_int<32> > mult_out_sig_3;
    sc_signal< sc_int<32> > mult_out_sig_4;
    sc_signal< sc_int<32> > mult_out_sig_5;
    sc_signal< sc_int<32> > mult_out_sig_6;
    sc_signal< sc_int<32> > mult_out_sig_7;
    #endif

    sc_int<32> load_state;
    sc_int<32> store_state;
    sc_int<32> last_task;

    // Functions

    // Calculate the number of chunks and remaining cols
    inline void calculate_config(uint24_t ninputs,
				 uint24_t matrix_d1,
				 uint24_t matrix_d2,
				 uint24_t matrix_d3,
				 bool transpose,
				 uint32_t& size_matrix1,
				 uint32_t& size_matrix2,
				 uint32_t& size_matrix_out,
				 uint24_t& matrix_chk_in,
				 uint16_t& matrix_rem_in1,
				 uint16_t& matrix_rem_in2,
				 uint24_t& matrix_chk_out,
				 uint16_t& matrix_rem_out,
				 uint8_t& load_cfg,
				 uint16_t& loadable_rows,
				 uint16_t& loadable_chunk,
				 uint16_t& index_d1_incr,
				 uint16_t& m2_loop_iters,
				 uint16_t& m2_plm_incr);
    inline void calculate_chunks(uint24_t &matrix_chk, uint16_t &matrix_rem,
				 uint32_t matrix_d2, bool in_or_out);

    // Synchronize compute_kernel and store_output processes
    inline void sync_compute_store(uint16_t &count, uint16_t loaded_rows,
				   uint8_t load_cfg, uint16_t loadable_rows,
				   bool &pingpong
                   , bool &pingpong_m1, bool &pingpong_m2, sc_signal< sc_int<64> >& compute_state_dbg);



    // sc_int<32> load_state;
    // sc_int<32> store_state;

    sc_signal< sc_int<32> > load_state_req_dbg;
    sc_signal< sc_int<32> > store_state_req_dbg;
    sc_signal< sc_int<32> > input_state_req_dbg;
    sc_signal< sc_int<32> > output_state_req_dbg;
    sc_int<32> prod_valid;
    sc_int<32> flt_valid;
    sc_int<32> cons_ready;
    sc_int<32> end_acc;
    // sc_int<32> last_task;

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


    // BM Load utility functions
    inline void calc_load_len(uint16_t & length1, uint16_t & length2, uint16_t load_cfg, uint16_t chk, 
                                uint16_t loadable_rows, uint16_t d1, uint16_t d2,
                                uint16_t matrix_rem_in1, uint16_t matrix_rem_in2,uint24_t  matrix_chk_in,
                                uint24_t matrix_d1, uint24_t matrix_d3 );
    inline void load_input_1(bool &pingpong, uint32_t offset, uint16_t length, PLM_WORD (&ping_sp)[DMA_CHUNK], PLM_WORD (&pong_sp)[DMA_CHUNK], int16_t load_cfg);
    inline void load_config(int32_t offset, 
								uint24_t &ninputs,
								uint24_t& matrix_d1,
								uint24_t& matrix_d2,
								uint24_t& matrix_d3,
								uint32_t& ld_offset1,
								// uint32_t ld_offset2;
								uint32_t& st_offset,
								bool &transpose,
								bool &do_relu,
								sc_dt::sc_bv<DMA_WIDTH>& dataBvin
							);
    inline void store_output_body(uint24_t st_offset, uint24_t length, bool& pingpong, PLM_WORD (&ping_sp)[OUT_DMA_CHUNK], PLM_WORD (&pong_sp)[OUT_DMA_CHUNK]);

    //ASI Functions
    inline void poll_flag(sc_dt::sc_bv<DMA_WIDTH> &dataBvin, int sync_offset, int sync_len, sc_int<32> &var);
    // inline void chk_last_task(sc_dt::sc_bv<DMA_WIDTH> &dataBvin);
    inline void update_flag(int32_t sync_offset, int32_t sync_len, bool sync_flag, sc_dt::sc_bv<DMA_WIDTH>& dataBv);
    inline void update_last_task(sc_dt::sc_bv<DMA_WIDTH> &dataBvin);
    inline void propagate_flag();

     // ASI submodule for input
    void input_asi_controller();

    // ASI submodule for output
    void output_asi_controller();

    //ASI FSM <-> Modules
    inline void arbitrate_load_state(int8_t &task_arbiter, bool &continue_arb);
    inline void arbitrate_store_state();
    inline void input_asi_flag_update(int16_t update_stage);
    inline void output_asi_flag_update(int16_t update_stage);
    inline void input_asi_flag_poll(int16_t poll_stage);
    inline void release_outputs(sc_signal< sc_int<64> >& compute_state_dbg);

    // Handshake callable from compute_kernel
    inline void compute_store_2_handshake();

    // Handshake callable from store_output
    inline void store_compute_2_handshake();

    // Configuration handshakes
    inline void load_compute_cfg_handshake();
    inline void compute_load_cfg_handshake();
    inline void load_store_cfg_handshake();
    inline void store_load_cfg_handshake();

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

    handshake_t compute_done;

    inline void compute_done_req();
    inline void compute_done_ack();
    inline void load_done_req();
    inline void load_done_ack();
    inline void store_done_req();
    inline void store_done_ack();
    inline void load_next_tile_req(); 
    inline void load_next_tile_ack();
    inline void load_output_done_handshake();
    inline void output_load_done_handshake();
    inline void store_output_done_handshake();
    inline void output_store_done_handshake(); 
    inline void output_load_start_handshake();
    inline void load_output_start_handshake();
    inline void output_store_start_handshake();
    inline void store_output_start_handshake();


    // Private local memories
    PLM_WORD input0[DMA_CHUNK];
    PLM_WORD input1[DMA_CHUNK];
    PLM_WORD input2[DMA_CHUNK];
    PLM_WORD input3[DMA_CHUNK];
    PLM_WORD output0[OUT_DMA_CHUNK];
    PLM_WORD output1[OUT_DMA_CHUNK];
    FPDATA row[PARALLELISM];
    FPDATA col[PARALLELISM];
    FPDATA mult_out[PARALLELISM];
    FPDATA accumulator;

    // Custom configuration signals
    sc_signal<uint32_t> size_matrix_out_sig;
    sc_signal<uint32_t> size_matrix1_sig;
    sc_signal<uint32_t> size_matrix2_sig;
    sc_signal<uint24_t> matrix_chk_in_sig;
    sc_signal<uint16_t> matrix_rem_in1_sig;
    sc_signal<uint16_t> matrix_rem_in2_sig;
    sc_signal<uint24_t> matrix_chk_out_sig;
    sc_signal<uint16_t> matrix_rem_out_sig;
    sc_signal<uint8_t> load_cfg_sig;
    sc_signal<uint16_t> loadable_rows_sig;
    sc_signal<uint16_t> loadable_chunk_sig;
    sc_signal<uint16_t> index_d1_incr_sig;
    sc_signal<uint32_t> gemm_st_offset;

	sc_signal<uint24_t> ninputs_sig;
    sc_signal<uint24_t> d1_sig;
    sc_signal<uint24_t> d2_sig;
    sc_signal<uint24_t> d3_sig;
    sc_signal<uint8_t> transpose_sig;
    sc_signal<uint8_t> do_relu_sig;

    
};

#endif /* __GEMM_HPP__ */
