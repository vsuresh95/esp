// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "gemm.hpp"

// Optional application-specific helper functions

//
// Utility functions
//

inline void gemm::calculate_config(uint24_t ninputs,
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
				   uint16_t& m2_plm_incr)
{
    size_matrix1 = matrix_d1 * matrix_d2;
    size_matrix2 = matrix_d2 * matrix_d3;
    size_matrix_out = matrix_d1 * matrix_d3 * ninputs;

    m2_loop_iters = 1;
    m2_plm_incr = 1;

    bool d3_odd = matrix_d3 % 2;
    bool is_less_than_matrix2 = (size_matrix2 > DMA_CHUNK || !transpose);

    if ((matrix_d2 > DMA_CHUNK) || (is_less_than_matrix2 && d3_odd)) {
	load_cfg = LESS_THAN_ROW;
	loadable_rows = 1;
	loadable_chunk = DMA_CHUNK;
	calculate_chunks(matrix_chk_in, matrix_rem_in1, matrix_d2, 0);
	matrix_rem_in2 = matrix_rem_in1;
	index_d1_incr = matrix_d2;
    } else if (is_less_than_matrix2) {
	load_cfg = LESS_THAN_MATRIX2;
	if (size_matrix2 > DMA_CHUNK) {
	    loadable_rows = DMA_CHUNK / matrix_d2;
	    if (loadable_rows != 1)
		loadable_rows = (loadable_rows >> 1) << 1;
	} else {
	    loadable_rows = matrix_d3;
	}
	loadable_chunk = loadable_rows * matrix_d2;
	matrix_chk_in = 1;
	matrix_rem_in1 = size_matrix1 % loadable_chunk;
	matrix_rem_in2 = size_matrix2 % loadable_chunk;
	index_d1_incr = loadable_chunk;
	if (!transpose) {
	    m2_loop_iters = matrix_d2;
	    m2_plm_incr = matrix_d2;
	}
    } else {
	load_cfg = MORE_THAN_MATRIX2;
	loadable_rows = matrix_d3;
	loadable_chunk = size_matrix2;
	matrix_chk_in = 1;
	matrix_rem_in1 = size_matrix1 % loadable_chunk;
	matrix_rem_in2 = size_matrix2;
	index_d1_incr = loadable_chunk;
    }

    calculate_chunks(matrix_chk_out, matrix_rem_out, size_matrix_out, 1);
}

inline void gemm::calculate_chunks(uint24_t  &matrix_chk,
				   uint16_t &matrix_rem,
				   uint32_t matrix_d2,
				   bool in_or_out)
{
     uint32_t matrix_mul;
     {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "calc-chunks");

	if (!in_or_out) {
	    // calculating the number of chunks (ceil)
	    matrix_chk = matrix_d2 >> DMA_CHUNK_LOG;
	    // calculating the number of cols (covered the by the chunks)
	    matrix_mul = matrix_chk << DMA_CHUNK_LOG;
	} else {
	    // calculating the number of chunks (ceil)
	    matrix_chk = matrix_d2 >> OUT_DMA_CHUNK_LOG;
	    // calculating the number of cols (covered the by the chunks)
	    matrix_mul = matrix_chk << OUT_DMA_CHUNK_LOG;
	}

        // calculating the remaining cols (size of the last chunk)
        matrix_rem = matrix_d2 - matrix_mul;

        // adding the last chunk if it is necessary
        if (matrix_rem != 0) { ++matrix_chk; }
    }
}

inline void gemm::sync_compute_store(uint16_t &count, uint16_t loaded_rows,
				     uint8_t load_cfg, uint16_t loadable_rows,
				     bool &pingpong)
{
    count++;
    if (load_cfg == LESS_THAN_MATRIX2 && loadable_rows != 1) {
	if (count == loaded_rows) {
            count = 0;
	    // ESP_REPORT_INFO("COMPUTE2: before store hs %u", (unsigned) count);
            // Call the store_output process
            compute_store_handshake();
	    // ESP_REPORT_INFO("COMPUTE2: after store hs %u", (unsigned) count);
	    pingpong = !pingpong;
	}
    } else {
        if (count == OUT_DMA_CHUNK) {
            count = 0;
	    // ESP_REPORT_INFO("COMPUTE: before store hs");
            // Call the store_output process
            compute_store_handshake();
	    // ESP_REPORT_INFO("COMPUTE: after store hs");
	    pingpong = !pingpong;
        }
    }
}


// inline void gemm::calc_load_len(uint16_t & length1, uint16_t & length2, uint16_t load_cfg, 
// uint16_t chk, uint16_t loadable_rows, uint16_t d1, uint16_t d2,  uint16_t matrix_rem_in1, uint16_t matrix_rem_in2,uint24_t  matrix_chk_in,
// uint24_t matrix_d1, uint24_t matrix_d3 )
// {
// 	if (load_cfg == LESS_THAN_ROW && chk == matrix_chk_in - 1 &&
// 		matrix_rem_in2 != 0) {
// 		length1 = matrix_rem_in1;
// 		length2 = matrix_rem_in2;
// 		} else if (load_cfg != LESS_THAN_ROW) {
// 		if (d1 + loadable_rows > matrix_d1)
// 			length1 = matrix_rem_in1;
// 		if (d2 + loadable_rows > matrix_d3)
// 			length2 = matrix_rem_in2;
// 		}
// }

inline void gemm::load_input_1(bool &pingpong, uint32_t offset, uint16_t length, PLM_WORD (&ping_sp)[DMA_CHUNK], PLM_WORD (&pong_sp)[DMA_CHUNK],  int16_t load_cfg)
{
	// if (!(d2 && load_cfg == LESS_THAN_MATRIX2)) {

    			{
    			    HLS_DEFINE_PROTOCOL("load-matrix1-info");
    			    dma_info_t dma_info(offset >> WORDS_PER_DMA_LOG,
						round_up(length, WORDS_PER_DMA) >> WORDS_PER_DMA_LOG,
						SIZE_WORD);
    			    this->dma_read_ctrl.put(dma_info);

                            // ESP_REPORT_INFO("load m1 %u %u",
                            // 		    (unsigned) index_d1_tmp, (unsigned) round_up(length1, WORDS_PER_DMA));
    			}

    		int i = 0;

			bool misaligned = 0;// index_d1_tmp & 1 & (WORDS_PER_DMA - 1);

    			for (uint16_t k = 0; k < round_up(length + misaligned, WORDS_PER_DMA) >> WORDS_PER_DMA_LOG; ++k)
    			{
    			    sc_dt::sc_bv<DMA_WIDTH> data = this->dma_read_chnl.get();

    			    {
    				// This ensures the maximum throughput
						HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "load-matrix");
						HLS_BREAK_ARRAY_DEPENDENCY(ping_sp);
						HLS_BREAK_ARRAY_DEPENDENCY(pong_sp);

#if (WORDS_PER_DMA == 2)                                
				if (pingpong) {

				    if (!(misaligned && !k))
					ping_sp[i++] = data.range(31,0).to_uint();
				    if (i < DMA_CHUNK)
					ping_sp[i++] = data.range(63,32).to_uint();
				} else {
				    if (!(misaligned && !k))
					pong_sp[i++] = data.range(31,0).to_uint();
				    if (i < DMA_CHUNK)
					pong_sp[i++] = data.range(63,32).to_uint();
				}
#else
				if (pingpong) {
                                    ping_sp[i++] = data.to_uint();
				} else {
                                    pong_sp[i++] = data.to_uint();
				}
#endif
				// i+=2;
    			    }
			    wait(); // Only considered in behavioral simulation
    			}
    			pingpong = !pingpong;
    		    // }
}

inline void gemm::store_output_body(uint24_t st_offset, uint24_t length, bool& pingpong, PLM_WORD (&ping_sp)[OUT_DMA_CHUNK], PLM_WORD (&pong_sp)[OUT_DMA_CHUNK]){
	{
		HLS_DEFINE_PROTOCOL("store-matrix-info");
		dma_info_t dma_info(st_offset >> WORDS_PER_DMA_LOG,
			round_up(length, WORDS_PER_DMA) >> WORDS_PER_DMA_LOG,
			SIZE_WORD);
		this->dma_write_ctrl.put(dma_info);

		// ESP_REPORT_INFO("STORE index %u length %u",
		// 		    (unsigned) index, (unsigned) length);
	}
	uint24_t i = 0;
	for (uint16_t k = 0; k < round_up(length, WORDS_PER_DMA) >> WORDS_PER_DMA_LOG; ++k)
	{
		sc_dt::sc_bv<DMA_WIDTH> data = 0;

		{
			// This ensures the maximum throughput
			HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "store-matrix");
	
			if (pingpong) {
				data.range(31,0) = ping_sp[i++];
				#if (WORDS_PER_DMA == 2)
				data.range(63,32) = ping_sp[i++];
				#endif
			} else {
				data.range(31,0) = pong_sp[i++];
				#if (WORDS_PER_DMA == 2)
				data.range(63,32) = pong_sp[i++];
				#endif
			}

		wait(); // Only considered in behavioral simulation
		}

		this->dma_write_chnl.put(data);
	}

	// toggle pingpong
	pingpong = !pingpong;

}


inline void gemm::arbitrate_load_state(int8_t &task_arbiter, bool &continue_arb){
	{
            HLS_DEFINE_PROTOCOL("load-next-tile");

            // monolithic add
            // Wait for either input ASI or output ASI
            while (!(input_load_req_valid || output_load_req_valid)) wait();
            if (task_arbiter == 0 && input_load_req_valid) {
                // this->load_input_start_handshake();
                this->load_next_tile_ack();
                load_state = input_load_req;
                load_state_req_module = INPUT_ASI;
                task_arbiter++;
				continue_arb = false;
            } else if (task_arbiter == 1 && output_load_req_valid) {
                this->load_output_start_handshake();
                load_state = output_load_req;
                load_state_req_module = OUTPUT_ASI;
                task_arbiter = 0;
				continue_arb = false;
            } else {
                if (task_arbiter == 1) task_arbiter = 0;
                else task_arbiter++;
                // continue;
				continue_arb = true;
            }
            // monolithic add
            // this->load_next_tile_ack();
            wait();

            load_state_dbg.write(load_state);
            // load_iter_dbg.write(curr_tile);
            wait();
        }
}


inline void gemm::arbitrate_store_state()//int32_t &store_state, int32_t &store_state_req_module
{
	{
		HLS_DEFINE_PROTOCOL("arbitrate_store_state");
		while (!(input_store_req_valid ||  output_store_req_valid)) wait();

		if (input_store_req_valid) {
			this->store_compute_handshake();
			store_state = input_store_req;
			store_state_req_module = INPUT_ASI;
		} else {
			this->store_output_start_handshake();
			store_state = output_store_req;
			store_state_req_module = OUTPUT_ASI;
		}
		// this->store_compute_handshake();
		store_state_dbg.write(store_state);
		// store_iter_dbg.write(curr_tile);
	}
}

inline void gemm::update_flag(int32_t sync_offset, int32_t sync_len, bool sync_flag, sc_dt::sc_bv<DMA_WIDTH>& dataBv)
{
	// HLS_DEFINE_PROTOCOL("store-dma-flag");
	{
		HLS_DEFINE_PROTOCOL("update_flag");
		dma_info_t dma_info(sync_offset / WORDS_PER_DMA, 
		round_up(sync_len, WORDS_PER_DMA) >> WORDS_PER_DMA_LOG, SIZE_WORD);
		// sc_dt::sc_bv<DMA_WIDTH> dataBv;
		// dataBv.range(DMA_WIDTH - 1, 0) = sync_flag;
		this->dma_write_ctrl.put(dma_info);
		wait();
		#if (WORDS_PER_DMA == 2)
			dataBv.range(31, 0) = sync_flag;
			if(store_state == UPDATE_CONS_VALID_REQ)
				dataBv.range(63,32) = last_task;
			this->dma_write_chnl.put(dataBv);
			wait();
		#else
			dataBv.range(DMA_WIDTH - 1, 0) = sync_flag;
			this->dma_write_chnl.put(dataBv);
			wait();
			if(store_state == UPDATE_CONS_VALID_REQ){
				dataBv.range(DMA_WIDTH - 1, 0) = last_task;
				this->dma_write_chnl.put(dataBv);
				wait();
			}
		#endif
	}
}
      
inline void gemm::update_last_task(sc_dt::sc_bv<DMA_WIDTH> &dataBv){
	dataBv.range(DMA_WIDTH - 1, 0) = last_task;
	this->dma_write_chnl.put(dataBv);
	wait();
}      

inline void gemm::propagate_flag()
{
	{
		HLS_DEFINE_PROTOCOL("propagate_flag");
		// Wait till the write is accepted at the cache (and previous fences)
		while (!(this->dma_write_chnl.ready)) wait();
		wait();

		//FENCE
		this->acc_fence.put(0x2);
		wait();
		while (!(this->acc_fence.ready)) wait();
		wait();
	}
}

inline void gemm::poll_flag(sc_dt::sc_bv<DMA_WIDTH> &dataBvin, int sync_offset, int sync_len, sc_int<32> &var)
{
	{
		HLS_DEFINE_PROTOCOL("poll_flag");
		dma_info_t dma_info2(sync_offset>> WORDS_PER_DMA_LOG, round_up(sync_len, WORDS_PER_DMA) >> WORDS_PER_DMA_LOG, SIZE_WORD);
		this->dma_read_ctrl.put(dma_info2);
		wait();
		load_unit_sp_write_dbg.write(1); 
		dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
		wait();
		load_unit_sp_write_dbg.write(2); 
		var = dataBvin.range(31, 0).to_uint();
		wait();
		load_unit_sp_write_dbg.write(3); 

		if(sync_len == 2){
			#if (WORDS_PER_DMA == 2)
			last_task = dataBvin.range(63,32).to_uint();
			#else
				dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
				wait();
				last_task = dataBvin.range(31, 0).to_uint();
			#endif
			// dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
			// last_task = dataBvin.range(31, 0).to_int64();
			// wait();
			// load_unit_sp_write_dbg.write(4); 
			// wait();
		}
	}
}

// inline void gemm::chk_last_task(sc_dt::sc_bv<DMA_WIDTH> &dataBvin)
// {
// 	{
// 		HLS_DEFINE_PROTOCOL("chk_last_task");
// 		dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
// 		last_task = dataBvin.range(31, 0).to_int64();
// 		wait();
// 		load_unit_sp_write_dbg.write(4); 
// 		wait();
// 	}
// }

inline void gemm::input_asi_flag_update(int16_t update_stage)
{
	{
		HLS_DEFINE_PROTOCOL("input_asi_flag_update");

		input_store_req = update_stage; //UPDATE_CONFIG;
		input_store_req_valid = true;

		input_state_req_dbg.write(update_stage);

		this->compute_store_handshake();
		wait();
		input_store_req_valid = false;
		wait();
		this->store_done_ack();
		wait();
	}
}

inline void gemm::load_config(int32_t offset, 
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
							)
{
	{
		HLS_DEFINE_PROTOCOL("load_config");
		dma_info_t dma_info2(offset>> WORDS_PER_DMA_LOG, 8>>WORDS_PER_DMA_LOG, SIZE_WORD);
		this->dma_read_ctrl.put(dma_info2);
		wait();
		load_unit_sp_write_dbg.write(LOAD_CONFIG); 
		dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
		wait();
		ninputs = dataBvin.range(31, 0).to_uint();
		#if (WORDS_PER_DMA == 2)
			matrix_d1 = dataBvin.range(63,32).to_uint();
		#else
			dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
			wait();
			matrix_d1 = dataBvin.range(31, 0).to_uint();
		#endif
		wait();
		dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
		wait();
		matrix_d2 = dataBvin.range(31, 0).to_uint();
		#if (WORDS_PER_DMA == 2)
			matrix_d3 = dataBvin.range(63,32).to_uint();
		#else
			dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
			wait();
			matrix_d3 = dataBvin.range(31, 0).to_uint();
		#endif
		wait();
		dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
		wait();
		ld_offset1 = dataBvin.range(31, 0).to_uint();
		#if (WORDS_PER_DMA == 2)
			st_offset = dataBvin.range(63,32).to_uint();
		#else
			dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
			wait();
			st_offset = dataBvin.range(31, 0).to_uint();
		#endif

		wait();

		dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
		wait();
		transpose = dataBvin.range(31, 0).to_uint();
		#if (WORDS_PER_DMA == 2)
			do_relu = dataBvin.range(63,32).to_uint();
		#else
			dataBvin.range(DMA_WIDTH - 1, 0) = this->dma_read_chnl.get();
			wait();
			do_relu = dataBvin.range(31, 0).to_uint();
		#endif
	}
}


inline void gemm::output_asi_flag_update(int16_t update_stage)
{
	{
		HLS_DEFINE_PROTOCOL("output_asi_flag_update");
		output_store_req = update_stage; //UPDATE_CONS_READY_REQ;
		output_store_req_valid = true;

		output_state_req_dbg.write(update_stage);

		this->output_store_start_handshake();
		wait();
		output_store_req_valid = false;
		wait();
		this->output_store_done_handshake();
		wait();
	}
}

inline void gemm::input_asi_flag_poll(int16_t poll_stage)
{
	{
		HLS_DEFINE_PROTOCOL("input_asi_flag_poll");
		input_load_req = poll_stage;
		input_load_req_valid = true;

		input_state_req_dbg.write(poll_stage);

		this->load_next_tile_req();
		wait();
		input_load_req_valid = false;
		wait();
		this->load_done_ack();
		wait();
	}
}


inline void gemm::load_compute_cfg_handshake()
{
    HLS_DEFINE_PROTOCOL("load-compute-cfg-handshake");

    load_compute_cfg_done.req.req();
}

inline void gemm::compute_load_cfg_handshake()
{
    HLS_DEFINE_PROTOCOL("compute-load-cfg-handshake");

    load_compute_cfg_done.ack.ack();
}

inline void gemm::load_store_cfg_handshake()
{
    HLS_DEFINE_PROTOCOL("load-store-cfg-handshake");

    load_store_cfg_done.req.req();
}

inline void gemm::store_load_cfg_handshake()
{
    HLS_DEFINE_PROTOCOL("store-load-cfg-handshake");

    load_store_cfg_done.ack.ack();
}

inline void gemm::store_done_req()
{
    HLS_DEFINE_PROTOCOL("store-done-req-handshake");

    store_done.req.req();
}

inline void gemm::store_done_ack()
{
    HLS_DEFINE_PROTOCOL("store-done-ack-handshake");

    store_done.ack.ack();
}

inline void gemm::load_done_req()
{
    HLS_DEFINE_PROTOCOL("load-done-req-handshake");

    load_done.req.req();
}

inline void gemm::load_done_ack()
{
    HLS_DEFINE_PROTOCOL("load-done-ack-handshake");

    load_done.ack.ack();
}

inline void gemm::load_next_tile_req()
{
    HLS_DEFINE_PROTOCOL("load-next-tile-req-handshake");

    load_next_tile.req.req();
}

inline void gemm::load_next_tile_ack()
{
    HLS_DEFINE_PROTOCOL("load-next-tile-ack-handshake");

    load_next_tile.ack.ack();
}


inline void gemm::compute_done_req()
{
    HLS_DEFINE_PROTOCOL("compute-done-ack-handshake");

    compute_done.req.req();
}

inline void gemm::compute_done_ack()
{
    HLS_DEFINE_PROTOCOL("compute-done-req-handshake");

    compute_done.ack.ack();
}


// inline void gemm::_req(){
//     {
//         HLS_DEFINE_PROTOCOL("-req-handshake");
//         .req.req();
//     }
// }
// inline void gemm::_ack(){
//     {
//         HLS_DEFINE_PROTOCOL("-ack-handshake");
//         .ack.ack();
//     }
// }

//////////////////////////////////////////////////////
// OUTPUT -> LOAD/STORE START HANDSHAKES
//////////////////////////////////////////////////////

inline void gemm::output_load_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("output-load-start-handshake");

        output_load_start.req.req();
    }
}

inline void gemm::load_output_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-output-start-handshake");

        output_load_start.ack.ack();
    }
}

inline void gemm::output_store_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("output-store-start-handshake");

        output_store_start.req.req();
    }
}

inline void gemm::store_output_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-output-start-handshake");

        output_store_start.ack.ack();
    }
}

//////////////////////////////////////////////////////
// LOAD/STORE -> OUTPUT DONE HANDSHAKES
//////////////////////////////////////////////////////

inline void gemm::output_load_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("output-load-done-handshake");

        load_output_done.ack.ack();
    }
}

inline void gemm::load_output_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-output-done-handshake");

        load_output_done.req.req();
    }
}

inline void gemm::output_store_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("output-store-done-handshake");

        store_output_done.ack.ack();
    }
}

inline void gemm::store_output_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-output-done-handshake");

        store_output_done.req.req();
    }
}
