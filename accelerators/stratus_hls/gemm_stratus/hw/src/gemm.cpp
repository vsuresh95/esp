// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "gemm.hpp"
#include "gemm_functions.hpp"

// Processes


void gemm::input_asi_controller(){
	bool new_task;
    {
        HLS_DEFINE_PROTOCOL("input-asi_controller-reset");
        load_next_tile.req.reset_req(); //invoke load
        load_done.ack.reset_ack();
        input_ready.req.reset_req(); //invoke compute
        store_done.ack.reset_ack();

		input_state_req_dbg.write(0);
        asi_state_dbg.write(0);

        end_acc = 0;
        input_load_req = 0;
        input_store_req = 0;
        input_load_req_valid = false;
        input_store_req_valid = false;
		new_task = true;
        // this->reset_accelerator_done();
        wait();
    }
    conf_info_t config;
    int32_t num_tiles;
    {
        HLS_DEFINE_PROTOCOL("input-asi_controller-config");
        cfg.wait_for_config(); // config process
        config = this->conf_info.read();
        num_tiles = 0;
        // User-defined config code
        /* <<--local-params-->> */
        // num_tiles = config.num_tiles;
    }

    while(true){ 
        // Test producer's valid for new task
        {
            HLS_DEFINE_PROTOCOL("test-prod-valid");
			asi_state_dbg.write(POLL_PROD_VALID_REQ);
			input_asi_flag_poll(POLL_PROD_VALID_REQ);
        }

        {
            // HLS_DEFINE_PROTOCOL("prod-valid-check");
            if (prod_valid == 1)
            {
				if(new_task){
					{
            			HLS_DEFINE_PROTOCOL("get-config");
						asi_state_dbg.write(LOAD_CONFIG);
						input_asi_flag_poll(LOAD_CONFIG);
						wait();
					}
				}
				else{
					// Load input data
					{
						{
							HLS_DEFINE_PROTOCOL("load-input-data");
							asi_state_dbg.write(LOAD_DATA_REQ);
							input_asi_flag_poll(LOAD_DATA_REQ);
						}
						num_tiles = num_tiles+1;
						if(last_task) end_acc = num_tiles;

					}
					// Inform GeMM to start
					{
						HLS_DEFINE_PROTOCOL("inform-compute-start");

						asi_state_dbg.write(COMPUTE);
						this->load_compute_handshake();
						wait();
					}
				}

                prod_valid = 0;
				if(new_task)
				{
					{
						HLS_DEFINE_PROTOCOL("update-condig");
						asi_state_dbg.write(UPDATE_CONFIG);
						input_asi_flag_update(UPDATE_CONFIG);
					}
					new_task = 0;
				}
				// else
				{
					// Reset producer's valid
					{
						HLS_DEFINE_PROTOCOL("update-prod-valid");
						asi_state_dbg.write(UPDATE_PROD_VALID_REQ);
						input_asi_flag_update(UPDATE_PROD_VALID_REQ);
					}
				}
                // update producer's ready to accept new data
                {
					{
						HLS_DEFINE_PROTOCOL("update-prod-ready");
						asi_state_dbg.write(UPDATE_PROD_READY_REQ);
						input_asi_flag_update(UPDATE_PROD_READY_REQ);
					}
                }
            }
        }
    }
}



void gemm::output_asi_controller(){
     {
        HLS_DEFINE_PROTOCOL("input-asi_controller-reset");
        output_load_start.req.reset_req(); //invoke load
        load_output_done.ack.reset_ack();
        compute_done.ack.reset_ack(); //ack compute
        output_store_start.req.reset_req(); //invoke store
        store_output_done.ack.reset_ack();

        // asi_state_dbg.write(0);
		output_state_req_dbg.write(0);
		op_asi_state_dbg.write(0);
        output_load_req = 0;
        output_store_req = 0;
        output_load_req_valid = false;
        output_store_req_valid = false;

        this->reset_accelerator_done();
        wait();
    }
    conf_info_t config;
    int32_t num_tiles;
    {
        HLS_DEFINE_PROTOCOL("input-asi_controller-config");
        cfg.wait_for_config(); // config process
        config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        num_tiles = 0;
    }

    while(true){
        // HLS_DEFINE_PROTOCOL("input-asi_controller-body");
        HLS_UNROLL_LOOP(OFF);
        {
            HLS_DEFINE_PROTOCOL("wait-for-compute");

			op_asi_state_dbg.write(1);
            this->compute_done_ack();
            wait();
        }
 
        // Poll consumer's ready to know if we can send new data
        {
            HLS_DEFINE_PROTOCOL("poll-for-cons-ready");

            while (cons_ready == 0)
            {
				op_asi_state_dbg.write(3);
                output_load_req = POLL_CONS_READY_REQ;
                output_load_req_valid = true;

                output_state_req_dbg.write(POLL_CONS_READY_REQ);

                this->output_load_start_handshake();
                wait();
                output_load_req_valid = false;
                wait();
                this->output_load_done_handshake();
                wait();
            }
        }//if cons rdy
        {
            // HLS_DEFINE_PROTOCOL("cons-ready-check");
            cons_ready = 0;
            // Reset consumer's ready
            {
                HLS_DEFINE_PROTOCOL("update-cons-ready");
				output_asi_flag_update(UPDATE_CONS_READY_REQ);

            } // Store output data
            {
                HLS_DEFINE_PROTOCOL("store-output-data");
				output_asi_flag_update(STORE_DATA_REQ);
            }
            // update consumer's valid for new data available
            {
                HLS_DEFINE_PROTOCOL("update-cons-valid");
				output_asi_flag_update(UPDATE_CONS_VALID_REQ);
            }
        }


        // if(end_acc>0 && num_tiles == end_acc){ 
        //     HLS_DEFINE_PROTOCOL("end-acc");
        //     this->accelerator_done();
        //     this->process_done();
        // }
    }
}

void gemm::load_input()
{
    bool pingpong_m1;
    bool pingpong_m2;
	bool new_task;
    int8_t task_arbiter;
    uint16_t i;
    uint24_t ninputs;
    uint16_t length1;
    uint16_t length2;
    uint32_t index_d1;
    uint32_t index_d1_n;
    uint32_t index_d2;
    uint32_t index_d1_tmp;
    uint32_t index_d2_tmp;
    uint24_t matrix_d1;
    uint24_t matrix_d2;
    uint24_t matrix_d3;
    uint32_t size_matrix_out;
    uint32_t size_matrix1;
    uint32_t size_matrix2;
    uint24_t matrix_chk_in;
    uint16_t matrix_rem_in1;
    uint16_t matrix_rem_in2;
    uint24_t matrix_chk_out;
    uint16_t matrix_rem_out;
    uint8_t load_cfg;
    uint16_t loadable_rows;
    uint16_t loadable_chunk;
    uint16_t index_d1_incr;
    uint32_t ld_offset1;
    uint32_t ld_offset2;
	uint32_t st_offset;
    bool transpose;
    bool do_relu;
    uint16_t m2_loop_iters, length_m2_dma;
    uint32_t index_m2_incr;
    uint16_t m2_plm_incr;
    int32_t input_offset;
    int32_t cons_rdy_offset;
    int32_t prod_valid_offset ;

    // Reset
    {
		HLS_DEFINE_PROTOCOL("load-reset");

		// this->reset_load_input();

    	this->reset_dma_read();
		load_compute_cfg_done.req.reset_req();
		load_store_cfg_done.req.reset_req();

        load_done.req.reset_req();
        load_next_tile.ack.reset_ack();

        output_load_start.ack.reset_ack(); //invoke load

        load_iter_dbg.write(0);
        load_state_dbg.write(0);
        load_unit_sp_write_dbg.write(0);
		// PLM memories reset

		// User-defined reset code
		i = 0;
		ninputs = 0;
        length1 = 0; 
        length2 = 0; 
        index_d1 = 0;
        index_d1_n = 0;
        index_d2 = 0;
        index_d1_tmp = 0;
        index_d2_tmp = 0;
        matrix_d1 = 0;
        matrix_d2 = 0;
        matrix_d3 = 0;
        size_matrix_out = 0;
		size_matrix1 = 0;
		size_matrix2 = 0;
        matrix_chk_in = 0;
        matrix_rem_in1 = 0;
        matrix_rem_in2 = 0;
        matrix_chk_out = 0;
        matrix_rem_out = 0;
		load_cfg = 0;
		loadable_rows = 0;
		loadable_chunk = 0;
		index_d1_incr = 0;
        ld_offset1 = 0;
        ld_offset2 = 0;
		transpose = 0;
		pingpong_m1 = false;
		pingpong_m2 = false;
		m2_loop_iters = 0;
		length_m2_dma = 0;
		index_m2_incr = 0;
		m2_plm_incr = 0;
		new_task = 1;
		task_arbiter = 0;
		do_relu = false;
		wait();
    }

    // Config
    {
		HLS_DEFINE_PROTOCOL("load-config");

		cfg.wait_for_config(); // config process
		conf_info_t config = this->conf_info.read();

		// User-defined config code
        cons_rdy_offset = config.cons_rdy_offset  ;
        prod_valid_offset  = config.prod_valid_offset   ;
		input_offset = config.input_offset;
		// ninputs = config.ninputs;
        // matrix_d1 = config.d1;
        // matrix_d2 = config.d2;
        // matrix_d3 = config.d3;
        // ld_offset1 = config.ld_offset1;
        // ld_offset2 = config.ld_offset2;
		// transpose = config.transpose;
    }

	while(true)
	{
		bool cont_arb = false;
		arbitrate_load_state(task_arbiter, cont_arb);
		if(cont_arb) continue;
		
		sc_dt::sc_bv<DMA_WIDTH> dataBvin;
		switch(load_state){
			case POLL_PROD_VALID_REQ:{
				{
					HLS_DEFINE_PROTOCOL("poll-prod-valid");
					poll_flag(dataBvin, prod_valid_offset, 2, prod_valid);
					load_unit_sp_write_dbg.write(16); 
					wait();
					// chk_last_task(dataBvin);
					break;
				}
			}
			case POLL_CONS_READY_REQ:{
				{
					HLS_DEFINE_PROTOCOL("poll-cons-valid");
					poll_flag(dataBvin, cons_rdy_offset, 1, cons_ready);
					wait();
					break;
				}
			}
			case LOAD_CONFIG:{
				{
					HLS_DEFINE_PROTOCOL("load-config");
					load_config(input_offset, ninputs, matrix_d1, 
					matrix_d2, matrix_d3, ld_offset1, st_offset, transpose, do_relu, dataBvin);
				}
				calculate_config(ninputs, matrix_d1, matrix_d2, matrix_d3, transpose,
						size_matrix1, size_matrix2, size_matrix_out, 
						matrix_chk_in, matrix_rem_in1, matrix_rem_in2,
						matrix_chk_out, matrix_rem_out,
						load_cfg, loadable_rows, loadable_chunk, index_d1_incr,
						m2_loop_iters, m2_plm_incr);

				{
					HLS_DEFINE_PROTOCOL("load-config-sig");

					load_unit_sp_write_dbg.write(0xd8add8ad); 
					wait();
					size_matrix_out_sig.write(size_matrix_out);
					size_matrix1_sig.write(size_matrix1);
					size_matrix2_sig.write(size_matrix2);
					matrix_chk_in_sig.write(matrix_chk_in);
					matrix_rem_in1_sig.write(matrix_rem_in1);
					matrix_rem_in2_sig.write(matrix_rem_in2);
					matrix_chk_out_sig.write(matrix_chk_out);
					matrix_rem_out_sig.write(matrix_rem_out);
					load_cfg_sig.write(load_cfg);
					loadable_rows_sig.write(loadable_rows);
					loadable_chunk_sig.write(loadable_chunk);
					index_d1_incr_sig.write(index_d1_incr);
					gemm_st_offset.write(st_offset);
					ninputs_sig.write(ninputs);
					d1_sig.write(matrix_d1);
					d2_sig.write(matrix_d2);
					d3_sig.write(matrix_d3);
					transpose_sig.write(transpose);
					do_relu_sig.write(do_relu);
					wait();
					load_unit_sp_write_dbg.write(0xc8dec8de); 
					wait();

					index_d1_n = ld_offset1;
					index_d2 = ld_offset2;
					index_m2_incr = matrix_d3;
					length_m2_dma = 1;

					wait();
					load_compute_cfg_handshake();
					// load_store_cfg_handshake();
					break;
				}
			}
			case LOAD_DATA_REQ: {

				length1 = loadable_chunk; //matrix_d1*matrix_d2;
				length2 = loadable_chunk; //matrix_d2*matrix_d3;
				load_input_1(pingpong_m1, ld_offset1, length1, input0, input1,  load_cfg);
				load_input_1(pingpong_m2, ld_offset1 + length1, length2, input2, input3,  load_cfg);
				// load_compute_handshake();
				break;
			}
		}

		 {
            HLS_DEFINE_PROTOCOL("load-dma-handshake");
            // monolithic change 
            if (load_state_req_module == INPUT_ASI) {
                load_unit_sp_write_dbg.write(0xc0dec0de);
                this->load_done_req();
            } else if (load_state_req_module == OUTPUT_ASI) {
                load_unit_sp_write_dbg.write(0xdeadbeef);
                this->load_output_done_handshake();
            }

            load_state_req_module = 0;
            // this->load_done_req();
            wait();
        }
	}// while loop
    // Conclude
    {
	HLS_DEFINE_PROTOCOL("load-done");
	this->process_done();
    }
}

void gemm::store_output()
{
    uint16_t i;
    bool pingpong;
	bool task_arbiter;
    uint24_t ninputs;
    uint24_t matrix_d1;
    uint24_t matrix_d2;
    uint24_t matrix_d3;
    uint32_t st_offset;
    uint24_t matrix_chk_out;
    uint16_t matrix_rem_out;
    uint32_t size_matrix_out;
    uint8_t load_cfg;
    uint16_t loadable_rows;
	int32_t cons_valid_offset;
    int32_t prod_rdy_offset ;
    int32_t cons_rdy_offset;
    int32_t prod_valid_offset ;

    // Reset
    {
    	HLS_DEFINE_PROTOCOL("store-reset");

    	// this->reset_store_output()
    	this->reset_dma_write();;
        output_done.req.reset_req();
        store_done.req.reset_req();
        output_ready.ack.reset_ack();
		load_store_cfg_done.ack.reset_ack();
		store_unit_sp_read_dbg.write(0);
    	// PLM memories reset

    	// User-defined reset code
		i = 0;
		pingpong = false;
		ninputs = 0;
        matrix_d1 = 0;
        matrix_d2 = 0;
        matrix_d3 = 0;
        st_offset = 0;
        matrix_chk_out = 0;
        matrix_rem_out = 0;
        size_matrix_out = 0;
		load_cfg = 0;
		loadable_rows = 0;
		task_arbiter = 0;

    	wait();
    }

    // Config
    {
    	HLS_DEFINE_PROTOCOL("store-config");

    	cfg.wait_for_config(); // config process
    	conf_info_t config = this->conf_info.read();

    	// User-defined config code
    	// ninputs = config.ninputs;
        // matrix_d1 = config.d1;
        // matrix_d2 = config.d2;
        // matrix_d3 = config.d3;
        // st_offset = config.st_offset;
		prod_valid_offset = config.prod_valid_offset;
		prod_rdy_offset = config.prod_rdy_offset;
		cons_valid_offset = config.cons_valid_offset;
		cons_rdy_offset = config.cons_rdy_offset;
	}

	while(true)
	{
		arbitrate_store_state();
		store_unit_sp_read_dbg.write(store_state);
		sc_dt::sc_bv<DMA_WIDTH> dataBv;
		switch(store_state){
            case UPDATE_CONS_VALID_REQ: 
			{
				{
					HLS_DEFINE_PROTOCOL("update-cons-valid");
					update_flag(cons_valid_offset, 2, 1, dataBv); // len 2 (incl last task), set cons valid flag
					// update_last_task(dataBv);
					propagate_flag();
				}
				break;
			}
			case UPDATE_CONFIG:
			{
				{
					HLS_DEFINE_PROTOCOL("store-config-sig");
					matrix_chk_out = matrix_chk_out_sig.read();
					matrix_rem_out = matrix_rem_out_sig.read();
					size_matrix_out = size_matrix_out_sig.read();
					load_cfg = load_cfg_sig.read();
					loadable_rows = loadable_rows_sig.read();

					ninputs = ninputs_sig.read();
					matrix_d1 = d1_sig.read();
					matrix_d2 = d2_sig.read();
					matrix_d3 = d3_sig.read();
					// do_relu = do_relu_sig.read();
					st_offset = gemm_st_offset.read();
					wait();
				}
				break;
			}

            case UPDATE_PROD_VALID_REQ:
			{
				{
					HLS_DEFINE_PROTOCOL("update-prod-valid");
					update_flag(prod_valid_offset, 1, 0, dataBv); // reset flag by writing 0
					propagate_flag();
				}
				break;
			}
            case UPDATE_CONS_READY_REQ:
			{
				{
					HLS_DEFINE_PROTOCOL("update-cons-ready");
					update_flag(cons_rdy_offset, 1, 0, dataBv); // reset flag by writing 0
					propagate_flag();
				}
				break;
			}
            case UPDATE_PROD_READY_REQ:
			{
				{
					HLS_DEFINE_PROTOCOL("update-prod-ready");
					update_flag(prod_rdy_offset, 1, 1, dataBv); // set prod rdy flag
					propagate_flag();
				}
				break;
			}
			case STORE_DATA_REQ:{
				int32_t length = OUT_DMA_CHUNK; //matrix_d1*matrix_d3; //TODO
				store_output_body(st_offset, length, pingpong, output0, output1);
			}
			
		 }
		  {
            HLS_DEFINE_PROTOCOL("store-dma-handshake");

            if (store_state_req_module == INPUT_ASI) {    
                store_state_dbg.write(0xc0dec0de);
                this->store_done_req();
            } else if(store_state_req_module == OUTPUT_ASI) {
                store_state_dbg.write(0xdeadbeef);
                this->store_output_done_handshake();
            }
            store_state_req_module = 0;
            // this->store_done_req();
            wait();
        }
	}
}


void gemm::compute_kernel()
{
    bool pingpong_m1, pingpong_m2, pingpong_out;
    uint24_t ninputs;
    uint16_t length;
    uint24_t matrix_d1;
    uint24_t matrix_d2;
    uint24_t matrix_d3;
    bool do_relu;
    uint24_t matrix_chk_in;
    uint16_t matrix_rem_in1;
    uint16_t matrix_rem_in2;
    uint16_t store_count;
    uint8_t load_cfg;
    uint16_t loadable_rows;

    // Reset
    {
    	HLS_DEFINE_PROTOCOL("compute-reset");

    	// this->reset_compute_kernel();
		output_done.ack.reset_ack();
		load_compute_cfg_done.ack.reset_ack();

    	// PLM memories reset
		input_ready.ack.reset_ack();
		compute_done.req.reset_req();
        compute_state_dbg.write(0);

    	// User-defined reset code
        pingpong_m1 = true;
        pingpong_m2 = true;
        pingpong_out = false;
    	ninputs = 0;
        matrix_d1 = 0;
        matrix_d2 = 0;
        matrix_d3 = 0;
		do_relu = 0;
        matrix_chk_in = 0;
        matrix_rem_in1 = 0;
        matrix_rem_in2 = 0;
        store_count = 0;
		load_cfg = 0;
		loadable_rows = 0;


		pingpong_m1_sig.write(0);
		pingpong_m2_sig.write(0);

        #if (PARALLELISM >= 8)
		row_0.write(0);
		row_1.write(0);
		row_2.write(0);
		row_3.write(0);
		row_4.write(0);
		row_5.write(0);
		row_6.write(0);
		row_7.write(0);
		col_0.write(0);
		col_1.write(0);
		col_2.write(0);
		col_3.write(0);
		col_4.write(0);
		col_5.write(0);
		col_6.write(0);
		col_7.write(0);
		#endif

    	wait();
    }

    // Config
    {
    	HLS_DEFINE_PROTOCOL("compute-config");

    	cfg.wait_for_config(); // config process
    	conf_info_t config = this->conf_info.read();

    	// User-defined config code
    	// ninputs = config.ninputs;
        // matrix_d1 = config.d1;
        // matrix_d2 = config.d2;
        // matrix_d3 = config.d3;
    	// do_relu = config.do_relu;

    	// compute_load_cfg_handshake();

    	// matrix_chk_in = matrix_chk_in_sig.read();
    	// matrix_rem_in1 = matrix_rem_in1_sig.read();
    	// matrix_rem_in2 = matrix_rem_in2_sig.read();
    	// load_cfg = load_cfg_sig.read();
    	// loadable_rows = loadable_rows_sig.read();

		wait();

        // ESP_REPORT_INFO("COMPUTE %u %u %u %u %u",
    	// 		(unsigned) matrix_chk_in, (unsigned) matrix_rem_in1, (unsigned) matrix_rem_in2,
    	// 		(unsigned) load_cfg, (unsigned) loadable_rows);
    }

	while(true){
 		{
			HLS_DEFINE_PROTOCOL("reset-config-signals");
			compute_load_cfg_handshake();
			matrix_chk_in = matrix_chk_in_sig.read();
			matrix_rem_in1 = matrix_rem_in1_sig.read();
			matrix_rem_in2 = matrix_rem_in2_sig.read();
			load_cfg = load_cfg_sig.read();
			loadable_rows = loadable_rows_sig.read();
			ninputs = ninputs_sig.read();
			matrix_d1 = d1_sig.read();
			matrix_d2 = d2_sig.read();
			matrix_d3 = d3_sig.read();
			do_relu = do_relu_sig.read();
			// wait();
			// length = matrix_d2;
			wait();
		}
		// Compute
		for (uint24_t a = 0; a < ninputs; a++)
		{
			uint16_t plm_offset_m1 = 0;
			uint16_t plm_offset_m2 = 0;

			// {
			// 	HLS_DEFINE_PROTOCOL("compute-block-input-handshake");
			// 	this->compute_load_handshake(); // Ack new input tile
			// 	wait();
			// 	compute_state_dbg.write(1);
			// 	wait();
			// }

			for (uint24_t d1 = 0; d1 < matrix_d1; d1 += loadable_rows)
			{
				uint16_t loaded_rows_d1 = min(loadable_rows, matrix_d1 - d1);

				for (uint24_t d2 = 0; d2 < matrix_d3; d2 += loadable_rows)
				{
					uint16_t loaded_rows_d3 = min(loadable_rows, matrix_d3 - d2);

					// if (load_cfg != LESS_THAN_ROW)
					// 	compute_load_handshake();
					if (load_cfg != LESS_THAN_ROW)
					{
						HLS_DEFINE_PROTOCOL("compute-block-input-handshake");
						this->compute_load_handshake(); // Ack new input tile
						wait();
						compute_state_dbg.write(1);
						wait();
					}

					for (uint16_t d1i = 0; d1i < loaded_rows_d1; d1i++)
					{
						plm_offset_m1 = d1i * matrix_d2;

						for (uint16_t d2i = 0; d2i < loaded_rows_d3; d2i++)
						{
							plm_offset_m2 = d2i * matrix_d2;

							// reset accumulator register
							accumulator = 0;

							for (uint24_t chk = 0; chk < matrix_chk_in; ++chk)
							{
								// If true the next is the last (smaller) chunk
								// BM
								// pingpong_m1 = !pingpong_m1;
								// pingpong_m2 = !pingpong_m2;
								// compute_load_handshake();

								if (load_cfg == LESS_THAN_ROW) {
									if (chk == matrix_chk_in - 1 && matrix_rem_in2 != 0) {
										length = matrix_rem_in2;
									} else {
										length = DMA_CHUNK;
									}
									pingpong_m1 = !pingpong_m1;
									pingpong_m2 = !pingpong_m2;

									{
										HLS_DEFINE_PROTOCOL("compute-block-input-handshake");
										this->compute_load_handshake(); // Ack new input tile
										wait();
										compute_state_dbg.write(11);
										wait();
									}
								// compute_load_handshake();
								} else if (load_cfg == LESS_THAN_MATRIX2) {
									length = matrix_d2;
									if (!d1i && !d2i) {
										if (!d2)
										pingpong_m1 = !pingpong_m1;
										pingpong_m2 = !pingpong_m2;
									}
								} else { // load_cfg ==  MORE_THAN_MATRIX2
									length = matrix_d2;
									if (!d1i && !d2i) {
										pingpong_m1 = !pingpong_m1;
										pingpong_m2 = !pingpong_m2;
									}
								}

								uint16_t plm_i_row = plm_offset_m1;
								uint16_t plm_i_col = plm_offset_m2;
								for (uint16_t k = 0; k < (length + PARALLELISM - 1) / PARALLELISM; ++k)
								{
									//HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "constrain-mac");
									#ifdef FIXED_POINT
									HLS_PIPELINE_LOOP(HARD_STALL, 2, "pipeline-mac-fixed");
									#else
									HLS_PIPELINE_LOOP(HARD_STALL, 2, "pipeline-mac-float");
									#endif
									HLS_BREAK_ARRAY_DEPENDENCY(input0);
									HLS_BREAK_ARRAY_DEPENDENCY(input1);
									HLS_BREAK_ARRAY_DEPENDENCY(input2);
									HLS_BREAK_ARRAY_DEPENDENCY(input3);

									{
										HLS_DEFINE_PROTOCOL("compute-ping");
										compute_state_dbg.write(0xcccc);
										pingpong_m1_sig.write(pingpong_m1);
										pingpong_m2_sig.write(pingpong_m2);
										wait();
									}

									if (pingpong_m1) {
										row[0] = INT2FP(input0[plm_i_row++]);
										row[1] = INT2FP(input0[plm_i_row++]);
										#if (PARALLELISM >= 4)
										row[2] = INT2FP(input0[plm_i_row++]);
										row[3] = INT2FP(input0[plm_i_row++]);
										#endif
										#if (PARALLELISM >= 8)
										row[4] = INT2FP(input0[plm_i_row++]);
										row[5] = INT2FP(input0[plm_i_row++]);
										row[6] = INT2FP(input0[plm_i_row++]);
										row[7] = INT2FP(input0[plm_i_row++]);
										#endif
										#if (PARALLELISM >= 16)
										row[8] = INT2FP(input0[plm_i_row++]);
										row[9] = INT2FP(input0[plm_i_row++]);
										row[10] = INT2FP(input0[plm_i_row++]);
										row[11] = INT2FP(input0[plm_i_row++]);
										row[12] = INT2FP(input0[plm_i_row++]);
										row[13] = INT2FP(input0[plm_i_row++]);
										row[14] = INT2FP(input0[plm_i_row++]);
										row[15] = INT2FP(input0[plm_i_row++]);
										#endif
									} else {
										row[0] = INT2FP(input1[plm_i_row++]);
										row[1] = INT2FP(input1[plm_i_row++]);
										#if (PARALLELISM >= 4)
															row[2] = INT2FP(input1[plm_i_row++]);
															row[3] = INT2FP(input1[plm_i_row++]);
										#endif
										#if (PARALLELISM >= 8)
															row[4] = INT2FP(input1[plm_i_row++]);
															row[5] = INT2FP(input1[plm_i_row++]);
															row[6] = INT2FP(input1[plm_i_row++]);
															row[7] = INT2FP(input1[plm_i_row++]);
										#endif
										#if (PARALLELISM >= 16)
										row[8] = INT2FP(input1[plm_i_row++]);
										row[9] = INT2FP(input1[plm_i_row++]);
										row[10] = INT2FP(input1[plm_i_row++]);
										row[11] = INT2FP(input1[plm_i_row++]);
										row[12] = INT2FP(input1[plm_i_row++]);
										row[13] = INT2FP(input1[plm_i_row++]);
										row[14] = INT2FP(input1[plm_i_row++]);
										row[15] = INT2FP(input1[plm_i_row++]);
										#endif

									}
									if (pingpong_m2) {
										col[0] = INT2FP(input2[plm_i_col++]);
										col[1] = INT2FP(input2[plm_i_col++]);
										#if (PARALLELISM >= 4)
															col[2] = INT2FP(input2[plm_i_col++]);
															col[3] = INT2FP(input2[plm_i_col++]);
										#endif
										#if (PARALLELISM >= 8)
															col[4] = INT2FP(input2[plm_i_col++]);
															col[5] = INT2FP(input2[plm_i_col++]);
															col[6] = INT2FP(input2[plm_i_col++]);
															col[7] = INT2FP(input2[plm_i_col++]);
										#endif
										#if (PARALLELISM >= 16)
															col[8] = INT2FP(input2[plm_i_col++]);
															col[9] = INT2FP(input2[plm_i_col++]);
															col[10] = INT2FP(input2[plm_i_col++]);
															col[11] = INT2FP(input2[plm_i_col++]);
															col[12] = INT2FP(input2[plm_i_col++]);
															col[13] = INT2FP(input2[plm_i_col++]);
															col[14] = INT2FP(input2[plm_i_col++]);
															col[15] = INT2FP(input2[plm_i_col++]);
										#endif
									} else {
										col[0] = INT2FP(input3[plm_i_col++]);
										col[1] = INT2FP(input3[plm_i_col++]);
										#if (PARALLELISM >= 4)
															col[2] = INT2FP(input3[plm_i_col++]);
															col[3] = INT2FP(input3[plm_i_col++]);
										#endif
										#if (PARALLELISM >= 8)
															col[4] = INT2FP(input3[plm_i_col++]);
															col[5] = INT2FP(input3[plm_i_col++]);
															col[6] = INT2FP(input3[plm_i_col++]);
															col[7] = INT2FP(input3[plm_i_col++]);
										#endif
										#if (PARALLELISM >= 16)
															col[8] = INT2FP(input3[plm_i_col++]);
															col[9] = INT2FP(input3[plm_i_col++]);
															col[10] = INT2FP(input3[plm_i_col++]);
															col[11] = INT2FP(input3[plm_i_col++]);
															col[12] = INT2FP(input3[plm_i_col++]);
															col[13] = INT2FP(input3[plm_i_col++]);
															col[14] = INT2FP(input3[plm_i_col++]);
															col[15] = INT2FP(input3[plm_i_col++]);
										#endif
									}

									uint16_t plm_i = k * PARALLELISM + 1;
									mult_out[0] = row[0] * col[0];

									{
										HLS_DEFINE_PROTOCOL("compute-mult");
										
										compute_state_dbg.write(0xdddd);

										#if (PARALLELISM >= 8)
										row_0.write(row[0]);
										row_1.write(row[1]);
										row_2.write(row[2]);
										row_3.write(row[3]);
										row_4.write(row[4]);
										row_5.write(row[5]);
										row_6.write(row[6]);
										row_7.write(row[7]);
										col_0.write(col[0]);
										col_1.write(col[1]);
										col_2.write(col[2]);
										col_3.write(col[3]);
										col_4.write(col[4]);
										col_5.write(col[5]);
										col_6.write(col[6]);
										col_7.write(col[7]);
										#endif
										wait();
									}
									if (plm_i < length)
										mult_out[1] =  row[1] * col[1];
									else
										mult_out[1] = 0;
									#if (PARALLELISM >= 4)
									if (plm_i + 1 < length)
										mult_out[2] = row[2] * col[2];
									else
										mult_out[2] = 0;
									if (plm_i + 2 < length)
										mult_out[3] = row[3] * col[3];
									else
										mult_out[3] = 0;
									#endif
									#if (PARALLELISM >= 8)
									if (plm_i + 3 < length)
										mult_out[4] = row[4] * col[4];
									else
										mult_out[4] = 0;
									if (plm_i + 4 < length)
										mult_out[5] = row[5] * col[5];
									else
										mult_out[5] = 0;
									if (plm_i + 5 < length)
										mult_out[6] = row[6] * col[6];
									else
										mult_out[6] = 0;
									if (plm_i + 6 < length)
										mult_out[7] = row[7] * col[7];
									else
										mult_out[7] = 0;
									#endif
									#if (PARALLELISM >= 16)
									if (plm_i + 7 < length)
										mult_out[8] = row[8] * col[8];
									else
										mult_out[8] = 0;
									if (plm_i + 8 < length)
										mult_out[9] = row[9] * col[9];
									else
										mult_out[9] = 0;
									if (plm_i + 9 < length)
										mult_out[10] = row[10] * col[10];
									else
										mult_out[10] = 0;
									if (plm_i + 10 < length)
										mult_out[11] = row[11] * col[11];
									else
										mult_out[11] = 0;
									if (plm_i + 11 < length)
										mult_out[12] = row[12] * col[12];
									else
										mult_out[12] = 0;
									if (plm_i + 12 < length)
										mult_out[13] = row[13] * col[13];
									else
										mult_out[13] = 0;
									if (plm_i + 13 < length)
										mult_out[14] = row[14] * col[14];
									else
										mult_out[14] = 0;
									if (plm_i + 14 < length)
										mult_out[15] = row[15] * col[15];
									else
										mult_out[15] = 0;
									#endif

									#if (PARALLELISM == 2)
									accumulator += mult_out[0] + mult_out[1];
									#elif (PARALLELISM == 4)
									FPDATA add_tmp0 = mult_out[0] + mult_out[1];
									FPDATA add_tmp1 = mult_out[2] + mult_out[3];
									accumulator += add_tmp0 + add_tmp1;
									#elif (PARALLELISM == 8)
									FPDATA add_tmp0 = mult_out[0] + mult_out[1];
									FPDATA add_tmp1 = mult_out[2] + mult_out[3];
									FPDATA add_tmp2 = mult_out[4] + mult_out[5];
									FPDATA add_tmp3 = mult_out[6] + mult_out[7];
									FPDATA add_tmp4 = add_tmp0 + add_tmp1;
									FPDATA add_tmp5 = add_tmp2 + add_tmp3;
									accumulator += add_tmp4 + add_tmp5;
									#elif (PARALLELISM == 16)
									FPDATA add_tmp0 = mult_out[0] + mult_out[1];
									FPDATA add_tmp1 = mult_out[2] + mult_out[3];
									FPDATA add_tmp2 = mult_out[4] + mult_out[5];
									FPDATA add_tmp3 = mult_out[6] + mult_out[7];
									FPDATA add_tmp4 = mult_out[8] + mult_out[9];
									FPDATA add_tmp5 = mult_out[10] + mult_out[11];
									FPDATA add_tmp6 = mult_out[12] + mult_out[13];
									FPDATA add_tmp7 = mult_out[14] + mult_out[15];
									FPDATA add_tmp8 = add_tmp0 + add_tmp1;
									FPDATA add_tmp9 = add_tmp2 + add_tmp3;
									FPDATA add_tmp10 = add_tmp4 + add_tmp5;
									FPDATA add_tmp11 = add_tmp6 + add_tmp7;
									FPDATA add_tmp12 = add_tmp8 + add_tmp9;
									FPDATA add_tmp13 = add_tmp10 + add_tmp11;
									accumulator += add_tmp12 + add_tmp13;
									#else
									accumulator +=  mult_out[0];
									#endif
								}
							}

							{
								HLS_DEFINE_PROTOCOL("compute-relu");
								compute_state_dbg.write(0x7e70);
								wait();
							}
							// ReLU
							accumulator = (do_relu && accumulator < (FPDATA) 0) ? (FPDATA) 0 : accumulator;
							
							{
								HLS_DEFINE_PROTOCOL("compute-relu-done");
								compute_state_dbg.write(0x7e7d);
								wait();
							}
							// Write to output PLM
							if (pingpong_out) {
								output0[store_count] = FP2INT(accumulator);
							} else {
								output1[store_count] = FP2INT(accumulator);
							}

								// Call the store_output process and wait for the store_output process
								// -> output PLM is not in pingpong
							// //todo
							sync_compute_store(store_count, loaded_rows_d3, load_cfg,
							loadable_rows, pingpong_out,pingpong_m1, pingpong_m2,compute_state_dbg);
						}
					}
				}
			}

			// Force to store the last chunk
			if (store_count) {
				store_count = OUT_DMA_CHUNK - 1;
				sync_compute_store(store_count, 1, load_cfg, loadable_rows, pingpong_out
				,pingpong_m1, pingpong_m2,compute_state_dbg);
			}
		}

		// // Force to store the last chunk
		// if (store_count) {
		// 	store_count = OUT_DMA_CHUNK - 1;
		// 	sync_compute_store(store_count, 1, load_cfg, loadable_rows, pingpong_out
		// 	,pingpong_m1, pingpong_m2,compute_state_dbg);
		// }


        //add return handshake
        // {
        //     HLS_DEFINE_PROTOCOL("compute-block-output-handshake");
        //     compute_state_dbg.write(2);
        //     wait();
        //     // ping = !ping;
		// 	pingpong_m1 = !pingpong_m1;
		// 	pingpong_m2 = !pingpong_m2;
		// 	pingpong_out = !pingpong_out;
        //     this->compute_done_req();
        //     wait();
        //     compute_state_dbg.write(3);
        //     wait();
        // }
	} // while(true)

    // Conclude
    {
	this->process_done();
    }
}
