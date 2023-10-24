// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "sort.hpp"
#include "sort_directives.hpp"

// Functions

#include "sort_functions.hpp"

// Processes

void sort::load_input()
{
	unsigned len; // from conf_info.len
	unsigned bursts; // from conf_info.batch
	// unsigned index;

	// Reset
	{
		HLS_LOAD_RESET;

		this->reset_load_input();

 		// index = 0;
		len = 0;
		bursts = 0;

		load_state_req_dbg.write(0);

        load_ready.ack.reset_ack();
        load_done.req.reset_req();

		wait();
	}

	// Config
	int32_t prod_valid_offset;
    int32_t prod_ready_offset;
    int32_t cons_valid_offset;
    int32_t cons_ready_offset;
    int32_t input_offset;
    int32_t output_offset;
	{
		HLS_LOAD_CONFIG;

		cfg.wait_for_config(); // config process
		conf_info_t config = this->conf_info.read();
		len = config.len;
		bursts = config.batch;

		// Configured shared memory offsets for sync flags
        prod_valid_offset = config.prod_valid_offset;
        prod_ready_offset = config.prod_ready_offset;
        cons_valid_offset = config.cons_valid_offset;
        cons_ready_offset = config.cons_ready_offset;
        input_offset = config.input_offset;
        output_offset = config.output_offset;
	}

	// Load
	while (true)
	{
        {
			HLS_PROTO("load-dma-start");
			wait();

			this->load_compute_ready_handshake();

			load_state_req_dbg.write(load_state_req);
		}

		switch (load_state_req)
        {
#ifdef ENABLE_SM
            case POLL_PROD_VALID_REQ:
            {
        		HLS_PROTO("load-dma-prod-valid");

                dma_info_t dma_info(prod_valid_offset / DMA_WORD_PER_BEAT, 2 * TEST_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                int32_t valid_task = 0;

                wait();

                // Wait for producer to send new data
                while (valid_task != 1)
                {
                    HLS_UNROLL_LOOP(OFF);
                    this->dma_read_ctrl.put(dma_info);
                    dataBv = this->dma_read_chnl.get();
                    wait();
                    valid_task = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
                    dataBv = this->dma_read_chnl.get();
                    wait();
                    last_task = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
                }
            }
            break;
            case POLL_CONS_READY_REQ:
            {
                HLS_PROTO("load-dma-cons-ready");
				
				dma_info_t dma_info(cons_ready_offset / DMA_WORD_PER_BEAT, TEST_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                int32_t ready_for_task = 0;

                wait();

                // Wait for consumer to accept new data
                while (ready_for_task != 1)
                {
                    HLS_UNROLL_LOOP(OFF);
                    this->dma_read_ctrl.put(dma_info);
                    dataBv = this->dma_read_chnl.get();
                    wait();
                    ready_for_task = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
                }
            }
            break;
#endif
            case LOAD_DATA_REQ:
			{
				// for (uint16_t b = 0; b < bursts; b++)
				{
					// HLS_LOAD_INPUT_BATCH_LOOP;

					{
						HLS_LOAD_DMA;
						
						dma_info_t dma_info(input_offset / (DMA_WIDTH / 32), len / (DMA_WIDTH / 32), SIZE_WORD);

						wait();

						this->dma_read_ctrl.put(dma_info);
					}
					for (uint16_t i = 0; i < len; i += 2)
					{
						HLS_LOAD_INPUT_LOOP;

						sc_dt::sc_bv<64> data_bv = this->dma_read_chnl.get();
						{
							HLS_LOAD_INPUT_PLM_WRITE;
							uint32_t data_1 = data_bv.range(31, 0).to_uint();
							uint32_t data_2 = data_bv.range(63, 32).to_uint();
							A0[i] = data_1;
							A0[i + 1] = data_2;
							wait();
						}
					}
				}
			}
			break;
			default:
			break;
		}

		// End Operation
		{
			HLS_PROTO("end-load");
			wait();

			this->load_compute_done_handshake();

			// debug
			load_state_req_dbg.write(0x10);

#ifdef ENABLE_SM
            if (last_task == 1 && load_state_req == POLL_CONS_READY_REQ)
            {
#endif
				// debug
				load_state_req_dbg.write(0x20);

                this->process_done();
#ifdef ENABLE_SM
            }
#endif
		}

	}
}



void sort::store_output()
{
	unsigned len; // from conf_info.len
	unsigned bursts; // from conf_info.batch
	// unsigned index;

	// Reset
	{
		HLS_STORE_RESET;

		this->reset_store_output();

 		// index = 0;
		len = 0;
		bursts = 0;

		store_state_req_dbg.write(0);

        store_ready.ack.reset_ack();
        store_done.req.reset_req();

		wait();
	}

	// Config
	int32_t prod_valid_offset;
    int32_t prod_ready_offset;
    int32_t cons_valid_offset;
    int32_t cons_ready_offset;
    int32_t input_offset;
    int32_t output_offset;

	{
		HLS_STORE_CONFIG;

		cfg.wait_for_config(); // config process
		conf_info_t config = this->conf_info.read();
		len = config.len;
		bursts = config.batch;

		// Configured shared memory offsets for sync flags
		prod_valid_offset = config.prod_valid_offset;
        prod_ready_offset = config.prod_ready_offset;
        cons_valid_offset = config.cons_valid_offset;
        cons_ready_offset = config.cons_ready_offset;
        input_offset = config.input_offset;
        output_offset = config.output_offset;
	}

	// Store
	while (true)
	{
        {
			HLS_PROTO("store-dma-start");
			wait();

        	this->store_compute_2_ready_handshake();

        	store_state_req_dbg.write(store_state_req);
		}

		switch (store_state_req)
        {
#ifdef ENABLE_SM
            case UPDATE_PROD_READY_REQ:
            {
        		HLS_PROTO("store-dma-prod-ready");

                dma_info_t dma_info(prod_ready_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                dataBv.range(DMA_WIDTH - 1, 0) = 1;

                this->dma_write_ctrl.put(dma_info);
                wait();
                this->dma_write_chnl.put(dataBv);
                wait();

                // Wait till the write is accepted at the cache (and previous fences)
                while (!(this->dma_write_chnl.ready)) wait();
                wait();
            }
            break;
            case UPDATE_PROD_VALID_REQ:
            {
                HLS_PROTO("store-dma-prod-valid");
				
				dma_info_t dma_info(prod_valid_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                dataBv.range(DMA_WIDTH - 1, 0) = 0;

                this->dma_write_ctrl.put(dma_info);
                wait();
                this->dma_write_chnl.put(dataBv);
                wait();

                // Wait till the write is accepted at the cache (and previous fences)
                while (!(this->dma_write_chnl.ready)) wait();
                wait();
            }
            break;
            case UPDATE_CONS_VALID_REQ:
            {
                HLS_PROTO("store-dma-cons-valid");
				
				dma_info_t dma_info(cons_valid_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                dataBv.range(DMA_WIDTH - 1, 0) = 1;

                this->dma_write_ctrl.put(dma_info);
                wait();
                this->dma_write_chnl.put(dataBv);
                wait();

                // Wait till the write is accepted at the cache (and previous fences)
                while (!(this->dma_write_chnl.ready)) wait();
                wait();
            }
            break;
            case UPDATE_CONS_READY_REQ:
            {
                HLS_PROTO("store-dma-cons-ready");
				
				dma_info_t dma_info(cons_ready_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                dataBv.range(DMA_WIDTH - 1, 0) = 0;

                this->dma_write_ctrl.put(dma_info);
                wait();
                this->dma_write_chnl.put(dataBv);
                wait();

                // Wait till the write is accepted at the cache (and previous fences)
                while (!(this->dma_write_chnl.ready)) wait();
                wait();
            }
            break;
#endif
            case STORE_DATA_REQ:
			{
				// for (uint16_t b = 0; b < bursts; b++)
				{
					// HLS_STORE_OUTPUT_BATCH_LOOP;


					{
						HLS_STORE_DMA;

						dma_info_t dma_info(output_offset / (DMA_WIDTH / 32), len / (DMA_WIDTH / 32), SIZE_WORD);

						wait();

						this->dma_write_ctrl.put(dma_info);
					}
					for (uint16_t i = 0; i < len; i += 2)
					{
						HLS_STORE_OUTPUT_LOOP;
                    	
						// wait();

						sc_dt::sc_bv<64> data_bv;
						{
							HLS_STORE_OUTPUT_PLM_READ;
							data_bv.range(31, 0)  = B0[i];
							data_bv.range(63, 32) = B0[i + 1];
							wait();
						}
						this->dma_write_chnl.put(data_bv);
					}

                	// Wait till the last write is accepted at the cache
					{
						HLS_PROTO("store-last-accept");
						wait();
                		while (!(this->dma_write_chnl.ready)) wait();
					}
				}
			}
			break;
            case STORE_FENCE:
            {
				HLS_PROTO("store-dma-store-fence");
                // Block till L2 to be ready to receive a fence, then send
                this->acc_fence.put(0x2);
                wait();
            }
            break;
            case ACC_DONE:
            {
                HLS_PROTO("store-dma-acc-done");
				// Ensure the previous fence was accepted, then acc_done
                while (!(this->acc_fence.ready)) wait();
                wait();
                this->accelerator_done();
                wait();
            }
			break;
			default:
			break;
		}

		// End Operation
		{
			HLS_PROTO("end-store");
			wait();
        	
			this->store_compute_2_done_handshake();

			// debug
			store_state_req_dbg.write(0x10);
			
#ifdef ENABLE_SM
            if (last_task == 1 && store_state_req == ACC_DONE)
#else
			if (store_state_req == ACC_DONE)
#endif
            {
				// debug
				store_state_req_dbg.write(0x20);

                this->process_done();
            }
		}

	}
}


void sort::compute_kernel()
{
	// Bi-tonic sort
	unsigned len; // from conf_info.len
	unsigned bursts; // from conf_info.batch

	// Reset
	{
		HLS_RB_RESET;

		this->reset_compute_1_kernel();
		this->reset_compute_2_kernel();

		len = 0;
		bursts = 0;

		compute_state_req_dbg.write(0);
		compute_2_state_req_dbg.write(0);

		// // for debugging
		compute_stage_dbg.write(0);

        load_ready.req.reset_req();
        load_done.ack.reset_ack();
		store_ready.req.reset_req();
        store_done.ack.reset_ack();

        load_state_req = 0;
        store_state_req = 0;

		wait();
	}

	// Config
	{
		HLS_RB_CONFIG;

		cfg.wait_for_config(); // config process
		conf_info_t config = this->conf_info.read();
		len = config.len;
		bursts = config.batch;
	}


	// Compute
	while (true)
	{
#ifdef ENABLE_SM
        // Poll producer's valid for new task
        {
            HLS_PROTO("poll-prod-valid");

            load_state_req = POLL_PROD_VALID_REQ;

            compute_state_req_dbg.write(POLL_PROD_VALID_REQ);

            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();

			// debug
            compute_state_req_dbg.write(0x10);
        }

        // Reset producer's valid
        {
            HLS_PROTO("update-prod-valid");

            store_state_req = UPDATE_PROD_VALID_REQ;

            compute_state_req_dbg.write(UPDATE_PROD_VALID_REQ);

            this->compute_2_store_ready_handshake();
            wait();
            this->compute_2_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_state_req_dbg.write(STORE_FENCE);

            this->compute_2_store_ready_handshake();
            wait();
            this->compute_2_store_done_handshake();
            wait();

			// debug
            compute_state_req_dbg.write(0x20);
        }
#endif
        // Load input data
        {
            HLS_PROTO("load-input-data");

            load_state_req = LOAD_DATA_REQ;

            compute_state_req_dbg.write(LOAD_DATA_REQ);

            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();

			// debug
            compute_state_req_dbg.write(0x30);
        }
#ifdef ENABLE_SM
        // update producer's ready to accept new data
        {
            HLS_PROTO("update-prod-ready");

            store_state_req = UPDATE_PROD_READY_REQ;

            compute_state_req_dbg.write(UPDATE_PROD_READY_REQ);

            this->compute_2_store_ready_handshake();
            wait();
            this->compute_2_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_state_req_dbg.write(STORE_FENCE);

            this->compute_2_store_ready_handshake();
            wait();
            this->compute_2_store_done_handshake();
            wait();

            compute_state_req_dbg.write(COMPUTE);

			// debug
            compute_state_req_dbg.write(0x40);
        }
#endif
		// Compute 1
		{
			// // for debugging
			compute_stage_dbg.write(1);

			// for (uint16_t b = 0; b < bursts; b++)
			{
				// HLS_RB_SORT_LOOP;

				// // for debugging
				compute_stage_dbg.write(2);

				// Flatten array
				unsigned regs[NUM];
				HLS_RB_MAP_REGS;

				uint8_t wchunk = 0;

				// Break the following
				for (uint16_t chunk = 0; chunk < LEN / NUM; chunk++)
				{
					HLS_RB_MAIN;

					// // for debugging
					compute_stage_dbg.write(3);

					if (chunk * NUM == len)
						break;

					//Break the following
					for (uint8_t i = 0; i < NUM; i++)
					{
						HLS_RB_RW_CHUNK;

						unsigned elem;
						elem = A0[chunk * NUM + i];
						C0[wchunk][i] = regs[i];
						regs[i] = elem;
					}
					if (chunk != 0)
						wchunk++;

					// // for debugging
					compute_stage_dbg.write(4);

					//Break the following
					for (uint8_t k = 0; k < NUM; k++)
					{
						HLS_RB_INSERTION_OUTER;
						//Unroll the following
						for (uint8_t i = 0; i < NUM; i += 2)
						{
							HLS_RB_INSERTION_EVEN;
							if (!lt_float(regs[i], regs[i + 1]))
							{
								unsigned tmp = regs[i];
								regs[i]      = regs[i + 1];
								regs[i + 1]  = tmp;
							}
						}

						// // for debugging
						compute_stage_dbg.write(5);

						//Unroll the following
						for (uint8_t i = 1; i < NUM - 1; i += 2)
						{
							HLS_RB_INSERTION_ODD;
							if (!lt_float(regs[i], regs[i + 1]))
							{
								unsigned tmp = regs[i];
								regs[i]      = regs[i + 1];
								regs[i + 1]  = tmp;
							}
						}

						// // for debugging
						compute_stage_dbg.write(6);
					}
				}

				{
					HLS_RB_BREAK_FALSE_DEP;
				}

				for (uint8_t i = 0; i < NUM; i++)
				{
					HLS_RB_W_LAST_CHUNKS_INNER;
					uint32_t elem = regs[i];
					C0[wchunk][i] = elem;
				}

				// // for debugging
				compute_stage_dbg.write(7);

			}
		}	// Compute 1

		// Compute 2
		{
			const uint8_t chunk_max = (uint8_t) (len >> lgNUM);

			// for (uint16_t b = 0; b < bursts; b++)
			{
				// HLS_MERGE_SORT_LOOP;
				unsigned head[LEN / NUM];  // Fifo output
				unsigned fidx[LEN / NUM];  // Fifo index
				bool pop_array[32];
				sc_dt::sc_bv<32> pop_bv(0);
				unsigned pop;              // pop from ith fifo
				bool shift_array[32];
				sc_dt::sc_bv<32> shift_bv(0);
				unsigned shift;            // shift from ith fifo
				unsigned regs2[LEN / NUM];  // State
				unsigned regs_in[LEN / NUM]; // Next state
				HLS_MERGE_SORT_MAP_REGS;

				//Should not be a combinational loop. BTW unroll.
				for (uint8_t i = 0; i < LEN / NUM; i++)
				{
					HLS_MERGE_INIT_ZERO_FIDX;

					fidx[i] = 0;
					shift_array[i] = false;
					pop_array[i] = false;
				}

				// this->compute_2_compute_handshake();

				// // for debugging
				compute_2_stage_dbg.write(1);

				if (chunk_max > 1)  //MERGE is needed
				{

					//Break the following
					for (uint8_t chunk = 0; chunk < LEN / NUM; chunk++)
					{
						HLS_MERGE_RD_FIRST_ELEMENTS;

						unsigned elem;
						elem = C0[chunk][0];

						if (chunk < chunk_max) {
							head[chunk] = elem;
							fidx[chunk]++;
						}
					}

					// // for debugging
					compute_2_stage_dbg.write(2);

					regs2[0] = head[0];
					{
						HLS_MERGE_RD_NEXT_ELEMENT;
						head[0] = C0[0][1];
					}
					fidx[0]++;

					// // for debugging
					compute_2_stage_dbg.write(3);

					uint8_t cur = 2;
					uint16_t cnt = 0;
					//Break the following
					while(true)
					{
						HLS_MERGE_MAIN;
						//Unroll the following
						for (uint8_t chunk = 1; chunk < LEN / NUM; chunk++)
						{
							HLS_MERGE_COMPARE;

							if ((chunk < cur) && !lt_float(regs2[chunk - 1], head[chunk]))
								shift_array[chunk] = 1;
						}
						for (uint8_t chunk = 0; chunk < LEN / NUM; chunk++)
						{
							HLS_MERGE_SHIFT_ARRAY;

								shift_bv[chunk] = shift_array[chunk];
						}
						shift = shift_bv.to_uint();

						const int DeBruijn32[32] =
							{
								0, 1, 28, 2, 29, 14, 24, 3,
								30, 22, 20, 15, 25, 17, 4, 8,
								31, 27, 13, 23, 21, 19, 16, 7,
								26, 12, 18, 6, 11, 5, 10, 9
							};
						HLS_MERGE_DEBRUIJN32;

						sc_dt::sc_bv<32> shift_rev_bv;
						//Unroll the following
						for (uint8_t i = 0; i < 32; i++) {
							HLS_MERGE_SHIFT_REV;

							const uint8_t index_rev = 31 - i;
							shift_rev_bv[index_rev] = shift_bv[i];
						}
						unsigned shift_rev = shift_rev_bv.to_uint();
						unsigned shift_msb;
						{
							int v = shift_rev;
							shift_msb = 31 - DeBruijn32[((unsigned)((v & -v) * 0x077CB531U)) >> 27];
							shift_msb = shift != 0 ? shift_msb : 0;
						}

						regs_in[0] = regs2[0];
						//Unroll the following
						for (uint8_t chunk =  LEN / NUM - 1; chunk > 1; chunk--)
						{
							HLS_MERGE_SHIFT;

							const uint32_t mask = 1 << chunk;
							if (chunk < cur && chunk >= shift_msb) {
								if (shift & mask)
								{
									regs_in[chunk] = head[chunk];
									pop_array[chunk] = 1;
								}
								else
								{
									regs_in[chunk] = regs2[chunk - 1];
								}
							}
						}

						if (shift_msb <= 1) {
							if (shift & 2)
							{
								regs_in[1] = head[1];
								pop_array[1]  = 1;
							}
							else
							{
								regs_in[1] = regs2[0];
								regs_in[0] = head[0];
								pop_array[0]  = 1;
							}
						}

						for (uint8_t chunk = 0; chunk < LEN / NUM; chunk++)
						{
							HLS_MERGE_POP_ARRAY;

								pop_bv[chunk] = pop_array[chunk];
						}
						pop = pop_bv.to_uint();

						//Unroll the following
						for (uint8_t chunk = 0; chunk < LEN / NUM; chunk++)
						{
							HLS_MERGE_SEQ;

							if (chunk < cur)
								regs2[chunk] = regs_in[chunk];
						}

						if (cur == chunk_max)
						{
							HLS_MERGE_WR_LAST_ELEMENTS;
							// write output
							B0[cnt] = regs2[chunk_max - 1];
							cnt++;
						}

						//Notice that only one pop[i] will be true at any time
						int pop_idx = -1;
						{
							unsigned pop_msb;
							int v = pop;
							pop_msb = DeBruijn32[((unsigned)((v & -v) * 0x077CB531U)) >> 27];
							pop_idx = pop != 0 ? pop_msb : -1;
						}


						if (pop_idx != -1)
							if (fidx[pop_idx] >= NUM) {
								head[pop_idx] = 0x7f800000; // +INF
								pop_idx = -1;
							}
						
						// // for debugging
						compute_2_stage_dbg.write(4);

						if (pop_idx != -1)
						{
							HLS_MERGE_DO_POP;
							head[pop_idx] = C0[pop_idx][fidx[pop_idx]];
							fidx[pop_idx]++;
						}

						// // for debugging
						compute_2_stage_dbg.write(5);

						for (uint8_t chunk = 0; chunk < LEN / NUM; chunk++)
						{
							HLS_MERGE_ZERO;
							shift_array[chunk] = false;
							pop_array[chunk] = false;
						}

						// DEBUG
						// cout << "heads after pop: ";
						// for (int chunk = 0; chunk < LEN/NUM; chunk++) {
						//    if (chunk == chunk_max)
						//       break;
						//    cout << chunk << ": " << std::hex << head[chunk] << "; ";
						// }
						// cout << std::dec << endl << endl;

						if (cur < chunk_max)
							cur++;

						if (cnt == len)
							break;
					}
				}
				else     // MERGE is not required
				{
					for (uint8_t chunk = 0; chunk < LEN / NUM; chunk++)
					{
						HLS_MERGE_NO_MERGE_OUTER;
						if (chunk == chunk_max)
							break;

						// // for debugging
						compute_2_stage_dbg.write(6);

						for (uint8_t i = 0; i < NUM; i++)
						{
							HLS_MERGE_NO_MERGE_INNER;

							unsigned elem;
							{
								elem = C0[chunk][i];
							}
							{
								B0[i] = elem;
							}
						}

						// // for debugging
						compute_2_stage_dbg.write(7);
					}
				}
			}
		}	// Compute 2

#ifdef ENABLE_SM
        // Poll consumer's ready to know if we can send new data
        {
            HLS_PROTO("poll-for-cons-ready");

            load_state_req = POLL_CONS_READY_REQ;

            compute_2_state_req_dbg.write(POLL_CONS_READY_REQ);

            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();

			// debug
            compute_2_state_req_dbg.write(0x50);
        }

        // Reset consumer's ready
        {
            HLS_PROTO("update-cons-ready");

            store_state_req = UPDATE_CONS_READY_REQ;

            compute_2_state_req_dbg.write(UPDATE_CONS_READY_REQ);

            this->compute_2_store_ready_handshake();
            wait();
            this->compute_2_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_2_state_req_dbg.write(STORE_FENCE);

            this->compute_2_store_ready_handshake();
            wait();
            this->compute_2_store_done_handshake();
            wait();


			// debug
            compute_2_state_req_dbg.write(0x60);
        }
#endif
        // Store output data
        {
            HLS_PROTO("store-output-data");

            store_state_req = STORE_DATA_REQ;

            this->compute_2_store_ready_handshake();

            compute_2_state_req_dbg.write(STORE_DATA_REQ);

            wait();
            this->compute_2_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_2_state_req_dbg.write(STORE_FENCE);

            this->compute_2_store_ready_handshake();
            wait();
            this->compute_2_store_done_handshake();
            wait();


			// debug
            compute_2_state_req_dbg.write(0x70);
        }
#ifdef ENABLE_SM
        // update consumer's ready for new data available
        {
            HLS_PROTO("update-cons-valid");

            store_state_req = UPDATE_CONS_VALID_REQ;

            compute_2_state_req_dbg.write(UPDATE_CONS_VALID_REQ);

            this->compute_2_store_ready_handshake();
            wait();
            this->compute_2_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_2_state_req_dbg.write(STORE_FENCE);

            this->compute_2_store_ready_handshake();
            wait();
            this->compute_2_store_done_handshake();
            wait();


			// debug
            compute_2_state_req_dbg.write(0x80);
        }
#endif
        // End operation
        {
            HLS_PROTO("end-acc");


			// debug
            compute_2_state_req_dbg.write(0x90);

#ifdef ENABLE_SM
            if (last_task == 1)
            {
#endif
                store_state_req = ACC_DONE;

                compute_2_state_req_dbg.write(ACC_DONE);
				
				this->compute_2_store_ready_handshake();
				wait();
				this->compute_2_store_done_handshake();
				wait();
				this->process_done();
#ifdef ENABLE_SM
			}
#endif
		}

	
// 		// End operation
//         {
//             HLS_PROTO("end-compute-1");
// 			this->compute_compute_2_handshake();

// #ifdef ENABLE_SM
//             if (last_task == 1)
//             {
// #endif
//                 this->process_done();
// #ifdef ENABLE_SM
//             }
// #endif
//         }
// 		// // Conclude
// 		// {
// 		// 	this->process_done();
// 		// }
	}
}


// void sort::compute_2_kernel()
// {
// 	// Bi-tonic sort
// 	unsigned len; // from conf_info.len
// 	unsigned bursts; // from conf_info.batch

// 	// Reset
// 	{
// 		HLS_MERGE_RESET;

// 		this->reset_compute_2_kernel();
// 		compute_2_state_req_dbg.write(0);

// 		len = 0;
// 		bursts = 0;
		
// 		wait();
// 	}

// 	// Config
// 	{
// 		HLS_MERGE_CONFIG;

// 		cfg.wait_for_config(); // config process
// 		conf_info_t config = this->conf_info.read();
// 		len = config.len;
// 		bursts = config.batch;
// 	}


// 	// Compute 2
// 	while (true)
// 	{
// 		const uint8_t chunk_max = (uint8_t) (len >> lgNUM);
// 		{
// 			// // for debugging
// 			compute_2_stage_dbg.write(0);

// 			// for (uint16_t b = 0; b < bursts; b++)
// 			{
// 				// HLS_MERGE_SORT_LOOP;
// 				unsigned head[LEN / NUM];  // Fifo output
// 				unsigned fidx[LEN / NUM];  // Fifo index
// 				bool pop_array[32];
// 				sc_dt::sc_bv<32> pop_bv(0);
// 				unsigned pop;              // pop from ith fifo
// 				bool shift_array[32];
// 				sc_dt::sc_bv<32> shift_bv(0);
// 				unsigned shift;            // shift from ith fifo
// 				unsigned regs[LEN / NUM];  // State
// 				unsigned regs_in[LEN / NUM]; // Next state
// 				HLS_MERGE_SORT_MAP_REGS;

// 				//Should not be a combinational loop. BTW unroll.
// 				for (uint8_t i = 0; i < LEN / NUM; i++)
// 				{
// 					HLS_MERGE_INIT_ZERO_FIDX;

// 					fidx[i] = 0;
// 					shift_array[i] = false;
// 					pop_array[i] = false;
// 				}

// 				this->compute_2_compute_handshake();

// 				// // for debugging
// 				compute_2_stage_dbg.write(1);

// 				if (chunk_max > 1)  //MERGE is needed
// 				{

// 					//Break the following
// 					for (uint8_t chunk = 0; chunk < LEN / NUM; chunk++)
// 					{
// 						HLS_MERGE_RD_FIRST_ELEMENTS;

// 						unsigned elem;
// 						elem = C0[chunk][0];

// 						if (chunk < chunk_max) {
// 							head[chunk] = elem;
// 							fidx[chunk]++;
// 						}
// 					}

// 					// // for debugging
// 					compute_2_stage_dbg.write(2);

// 					regs[0] = head[0];
// 					{
// 						HLS_MERGE_RD_NEXT_ELEMENT;
// 						head[0] = C0[0][1];
// 					}
// 					fidx[0]++;

// 					// // for debugging
// 					compute_2_stage_dbg.write(3);

// 					uint8_t cur = 2;
// 					uint16_t cnt = 0;
// 					//Break the following
// 					while(true)
// 					{
// 						HLS_MERGE_MAIN;
// 						//Unroll the following
// 						for (uint8_t chunk = 1; chunk < LEN / NUM; chunk++)
// 						{
// 							HLS_MERGE_COMPARE;

// 							if ((chunk < cur) && !lt_float(regs[chunk - 1], head[chunk]))
// 								shift_array[chunk] = 1;
// 						}
// 						for (uint8_t chunk = 0; chunk < LEN / NUM; chunk++)
// 						{
// 							HLS_MERGE_SHIFT_ARRAY;

// 								shift_bv[chunk] = shift_array[chunk];
// 						}
// 						shift = shift_bv.to_uint();

// 						const int DeBruijn32[32] =
// 							{
// 								0, 1, 28, 2, 29, 14, 24, 3,
// 								30, 22, 20, 15, 25, 17, 4, 8,
// 								31, 27, 13, 23, 21, 19, 16, 7,
// 								26, 12, 18, 6, 11, 5, 10, 9
// 							};
// 						HLS_MERGE_DEBRUIJN32;

// 						sc_dt::sc_bv<32> shift_rev_bv;
// 						//Unroll the following
// 						for (uint8_t i = 0; i < 32; i++) {
// 							HLS_MERGE_SHIFT_REV;

// 							const uint8_t index_rev = 31 - i;
// 							shift_rev_bv[index_rev] = shift_bv[i];
// 						}
// 						unsigned shift_rev = shift_rev_bv.to_uint();
// 						unsigned shift_msb;
// 						{
// 							int v = shift_rev;
// 							shift_msb = 31 - DeBruijn32[((unsigned)((v & -v) * 0x077CB531U)) >> 27];
// 							shift_msb = shift != 0 ? shift_msb : 0;
// 						}

// 						regs_in[0] = regs[0];
// 						//Unroll the following
// 						for (uint8_t chunk =  LEN / NUM - 1; chunk > 1; chunk--)
// 						{
// 							HLS_MERGE_SHIFT;

// 							const uint32_t mask = 1 << chunk;
// 							if (chunk < cur && chunk >= shift_msb) {
// 								if (shift & mask)
// 								{
// 									regs_in[chunk] = head[chunk];
// 									pop_array[chunk] = 1;
// 								}
// 								else
// 								{
// 									regs_in[chunk] = regs[chunk - 1];
// 								}
// 							}
// 						}

// 						if (shift_msb <= 1) {
// 							if (shift & 2)
// 							{
// 								regs_in[1] = head[1];
// 								pop_array[1]  = 1;
// 							}
// 							else
// 							{
// 								regs_in[1] = regs[0];
// 								regs_in[0] = head[0];
// 								pop_array[0]  = 1;
// 							}
// 						}

// 						for (uint8_t chunk = 0; chunk < LEN / NUM; chunk++)
// 						{
// 							HLS_MERGE_POP_ARRAY;

// 								pop_bv[chunk] = pop_array[chunk];
// 						}
// 						pop = pop_bv.to_uint();

// 						//Unroll the following
// 						for (uint8_t chunk = 0; chunk < LEN / NUM; chunk++)
// 						{
// 							HLS_MERGE_SEQ;

// 							if (chunk < cur)
// 								regs[chunk] = regs_in[chunk];
// 						}

// 						if (cur == chunk_max)
// 						{
// 							HLS_MERGE_WR_LAST_ELEMENTS;
// 							// write output
// 							B0[cnt] = regs[chunk_max - 1];
// 							cnt++;
// 						}

// 						//Notice that only one pop[i] will be true at any time
// 						int pop_idx = -1;
// 						{
// 							unsigned pop_msb;
// 							int v = pop;
// 							pop_msb = DeBruijn32[((unsigned)((v & -v) * 0x077CB531U)) >> 27];
// 							pop_idx = pop != 0 ? pop_msb : -1;
// 						}


// 						if (pop_idx != -1)
// 							if (fidx[pop_idx] >= NUM) {
// 								head[pop_idx] = 0x7f800000; // +INF
// 								pop_idx = -1;
// 							}
						
// 						// // for debugging
// 						compute_2_stage_dbg.write(4);

// 						if (pop_idx != -1)
// 						{
// 							HLS_MERGE_DO_POP;
// 							head[pop_idx] = C0[pop_idx][fidx[pop_idx]];
// 							fidx[pop_idx]++;
// 						}

// 						// // for debugging
// 						compute_2_stage_dbg.write(5);

// 						for (uint8_t chunk = 0; chunk < LEN / NUM; chunk++)
// 						{
// 							HLS_MERGE_ZERO;
// 							shift_array[chunk] = false;
// 							pop_array[chunk] = false;
// 						}

// 						// DEBUG
// 						// cout << "heads after pop: ";
// 						// for (int chunk = 0; chunk < LEN/NUM; chunk++) {
// 						//    if (chunk == chunk_max)
// 						//       break;
// 						//    cout << chunk << ": " << std::hex << head[chunk] << "; ";
// 						// }
// 						// cout << std::dec << endl << endl;

// 						if (cur < chunk_max)
// 							cur++;

// 						if (cnt == len)
// 							break;
// 					}
// 				}
// 				else     // MERGE is not required
// 				{
// 					for (uint8_t chunk = 0; chunk < LEN / NUM; chunk++)
// 					{
// 						HLS_MERGE_NO_MERGE_OUTER;
// 						if (chunk == chunk_max)
// 							break;

// 						// // for debugging
// 						compute_2_stage_dbg.write(6);

// 						for (uint8_t i = 0; i < NUM; i++)
// 						{
// 							HLS_MERGE_NO_MERGE_INNER;

// 							unsigned elem;
// 							{
// 								elem = C0[chunk][i];
// 							}
// 							{
// 								B0[i] = elem;
// 							}
// 						}

// 						// // for debugging
// 						compute_2_stage_dbg.write(7);
// 					}
// 				}
// 			}
// 		}
// #ifdef ENABLE_SM
//         // Poll consumer's ready to know if we can send new data
//         {
//             HLS_PROTO("poll-for-cons-ready");

//             load_state_req = POLL_CONS_READY_REQ;

//             compute_2_state_req_dbg.write(POLL_CONS_READY_REQ);

//             this->compute_load_ready_handshake();
//             wait();
//             this->compute_load_done_handshake();
//             wait();
//         }

//         // Reset consumer's ready
//         {
//             HLS_PROTO("update-cons-ready");

//             store_state_req = UPDATE_CONS_READY_REQ;

//             compute_2_state_req_dbg.write(UPDATE_CONS_READY_REQ);

//             this->compute_2_store_ready_handshake();
//             wait();
//             this->compute_2_store_done_handshake();
//             wait();

//             // Wait for all writes to be done and then issue fence
//             store_state_req = STORE_FENCE;

//             compute_2_state_req_dbg.write(STORE_FENCE);

//             this->compute_2_store_ready_handshake();
//             wait();
//             this->compute_2_store_done_handshake();
//             wait();
//         }
// #endif
//         // Store output data
//         {
//             HLS_PROTO("store-output-data");

//             store_state_req = STORE_DATA_REQ;

//             this->compute_2_store_ready_handshake();

//             compute_2_state_req_dbg.write(STORE_DATA_REQ);

//             wait();
//             this->compute_2_store_done_handshake();
//             wait();

//             // Wait for all writes to be done and then issue fence
//             store_state_req = STORE_FENCE;

//             compute_2_state_req_dbg.write(STORE_FENCE);

//             this->compute_2_store_ready_handshake();
//             wait();
//             this->compute_2_store_done_handshake();
//             wait();
//         }
// #ifdef ENABLE_SM
//         // update consumer's ready for new data available
//         {
//             HLS_PROTO("update-cons-valid");

//             store_state_req = UPDATE_CONS_VALID_REQ;

//             compute_2_state_req_dbg.write(UPDATE_CONS_VALID_REQ);

//             this->compute_2_store_ready_handshake();
//             wait();
//             this->compute_2_store_done_handshake();
//             wait();

//             // Wait for all writes to be done and then issue fence
//             store_state_req = STORE_FENCE;

//             compute_2_state_req_dbg.write(STORE_FENCE);

//             this->compute_2_store_ready_handshake();
//             wait();
//             this->compute_2_store_done_handshake();
//             wait();
//         }
// #endif
//         // End operation
//         {
//             HLS_PROTO("end-acc");

// #ifdef ENABLE_SM
//             if (last_task == 1)
//             {
// #endif
//                 store_state_req = ACC_DONE;

//                 compute_2_state_req_dbg.write(ACC_DONE);
				
// 				this->compute_2_store_ready_handshake();
// 				wait();
// 				this->compute_2_store_done_handshake();
// 				wait();
// 				this->process_done();
// #ifdef ENABLE_SM
// 			}
// #endif

// 		// Conclude
// 		// {
// 		// 	this->process_done();
// 		// }
// 		}
// 	}
// }
