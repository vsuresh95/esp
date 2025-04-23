// Copyright (c) 2011-2019 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef ENABLE_PP

#include "audio_fft.hpp"
#include "audio_fft_directives.hpp"

// Functions

#include "audio_fft_functions.hpp"

// Processes

void audio_fft::load_input()
{
    // Reset
    {
        HLS_PROTO("load-reset");

        this->reset_load_input();

        load_state_req_dbg.write(0);

        load_ready.ack.reset_ack();
        load_done.req.reset_req();

        input_is_full = 0;

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    int32_t input_queue_base;
    int32_t output_queue_base;
    int32_t input_valid_offset;
    int32_t output_valid_offset;
    int32_t input_payload_offset;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process

        wait();
    }

    // Load
    while(true)
    {
        HLS_PROTO("load-dma");

        wait();

        this->load_compute_ready_handshake();

        load_state_req_dbg.write(load_state_req);

        // Read config information for current context
        {
            HLS_PROTO("read-load-config");

            conf_info_t config = this->conf_info.read();        
            HLS_FLATTEN_ARRAY(config.do_inverse);
            HLS_FLATTEN_ARRAY(config.logn_samples);
            HLS_FLATTEN_ARRAY(config.do_shift);
            HLS_FLATTEN_ARRAY(config.input_queue_base);
            HLS_FLATTEN_ARRAY(config.output_queue_base);

            // User-defined config code
            /* <<--local-params-->> */
            logn_samples = config.logn_samples[current_context_int];
            num_samples = 1 << logn_samples;

            // Configured shared memory base addresses for input and output queues
            input_queue_base = config.input_queue_base[current_context_int];
            output_queue_base = config.output_queue_base[current_context_int];

            // offsets for input valid and output valid based on preset values
            input_valid_offset = input_queue_base + VALID_OFFSET;
            output_valid_offset = output_queue_base + VALID_OFFSET;

            // offsets for input valid and output payload based on preset values
            input_payload_offset = input_queue_base + PAYLOAD_OFFSET;

            wait();
        }

        switch (load_state_req)
        {
#ifdef ENABLE_SM
            case TEST_INPUT_IS_FULL:
            {
                dma_info_t dma_info(input_valid_offset / DMA_WORD_PER_BEAT, TEST_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                // Check if producer has new data
                this->dma_read_ctrl.put(dma_info);
                dataBv = this->dma_read_chnl.get();
                wait();
                input_is_full = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
            }
            break;
            case POLL_OUTPUT_IS_EMPTY:
            {
                dma_info_t dma_info(output_valid_offset / DMA_WORD_PER_BEAT, TEST_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                int32_t output_is_empty = 1;

                wait();

                // Wait for consumer to accept new data
                while (output_is_empty != 0)
                {
                    HLS_UNROLL_LOOP(OFF);
                    this->dma_read_ctrl.put(dma_info);
                    dataBv = this->dma_read_chnl.get();
                    wait();
                    output_is_empty = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
                }
            }
            break;
#endif
            case LOAD_DATA_REQ:
            // Load input data
            {
                dma_info_t dma_info(input_payload_offset / DMA_WORD_PER_BEAT, 2 * num_samples / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                this->dma_read_ctrl.put(dma_info);

                for (int i = 0; i < 2 * num_samples; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(A0);

                    dataBv = this->dma_read_chnl.get();
                    wait();
                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        A0[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                    }
                }
            }
            break;
            default:
            break;
        }

        wait();

        this->load_compute_done_handshake();
    }
} // Function : load_input

void audio_fft::store_output()
{
    // Reset
    {
        HLS_PROTO("store-reset");

        this->reset_store_output();

        store_state_req_dbg.write(0);

        store_ready.ack.reset_ack();
        store_done.req.reset_req();

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;;
    int32_t input_queue_base;
    int32_t output_queue_base;
    int32_t input_valid_offset;
    int32_t output_valid_offset;
    int32_t output_payload_offset;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process

        wait();
    }

    // Store
    while(true)
    {
        HLS_PROTO("store-dma");

        wait();

        this->store_compute_ready_handshake();

        store_state_req_dbg.write(store_state_req);

        // Read config information for current context
        {
            HLS_PROTO("read-store-config");

            conf_info_t config = this->conf_info.read();        
            HLS_FLATTEN_ARRAY(config.do_inverse);
            HLS_FLATTEN_ARRAY(config.logn_samples);
            HLS_FLATTEN_ARRAY(config.do_shift);
            HLS_FLATTEN_ARRAY(config.input_queue_base);
            HLS_FLATTEN_ARRAY(config.output_queue_base);

            // User-defined config code
            /* <<--local-params-->> */
            logn_samples = config.logn_samples[current_context_int];
            num_samples = 1 << logn_samples;

            // Configured shared memory base addresses for input and output queues
            input_queue_base = config.input_queue_base[current_context_int];
            output_queue_base = config.output_queue_base[current_context_int];

            // offsets for input valid and output valid based on preset values
            input_valid_offset = input_queue_base + VALID_OFFSET;
            output_valid_offset = output_queue_base + VALID_OFFSET;

            // offsets for input valid and output payload based on preset values
            output_payload_offset = output_queue_base + PAYLOAD_OFFSET;

            wait();
        }

        switch (store_state_req)
        {
#ifdef ENABLE_SM
            case UPDATE_INPUT_IS_EMPTY:
            {
                dma_info_t dma_info(input_valid_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
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
            case UPDATE_OUTPUT_IS_FULL:
            {
                dma_info_t dma_info(output_valid_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
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
#endif
            case STORE_DATA_REQ:
            {
                dma_info_t dma_info(output_payload_offset / DMA_WORD_PER_BEAT, 2 * num_samples / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                this->dma_write_ctrl.put(dma_info);

                for (int i = 0; i < 2 * num_samples; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(A0);

                    wait();

                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = A0[i + k];
                    }

                    this->dma_write_chnl.put(dataBv);
                }

                // Wait till the last write is accepted at the cache
                wait();
                while (!(this->dma_write_chnl.ready)) wait();
            }
            break;
            case STORE_FENCE:
            {
                // Block till L2 to be ready to receive a fence, then send
                this->acc_fence.put(0x2);
                wait();
            }
            break;
            case ACC_DONE:
            {
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

        wait();

        this->store_compute_done_handshake();
    }
} // Function : store_output

void audio_fft::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-reset");

        this->reset_compute_kernel();

        compute_state_req_dbg.write(0);

        load_ready.req.reset_req();
        load_done.ack.reset_ack();
        store_ready.req.reset_req();
        store_done.ack.reset_ack();

        load_state_req = 0;
        store_state_req = 0;

        current_context_int = 0;

        switch_context_dbg.write(0);
        current_context_int_dbg.write(0);

        current_context.write(0);

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    int32_t do_inverse;
    int32_t do_shift;
    uint32_t context_quota;
    uint32_t valid_contexts;
    uint32_t start_cycles;
    bool switch_context;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process

        switch_context = false;
        
        wait();
    }

    while(true)
    {
#ifdef ENABLE_SM
        // At the start of every iteration, check if there is a need to switch context
        // This could be based on amount of time this context has already gotten, for starters.
        // We need a way to check number of active contexts in the regbank and if it is > 1, then
        // we can switch to another context in round robin manner. Limit the number of possible
        // contexts to 4. DMA load and store must be overloaded to take ASID also. This ASID must
        // be sent my esp_acc_dma wrapper to the TLB to get the translation. Need to check if 
        // esp_acc_dma also caches the latest TLB entry used. If yes, that needs to be cleared
        // and a TLB entry must be fetched again. If there is a TLB miss, a new memory access
        // must be sent for the PT_ADDRESS location.
        {
            HLS_PROTO("check-new-context");

            // Is the quota of current context complete?
            uint32_t cycles_elapsed = accel_cycles - start_cycles;

            conf_info_t config = this->conf_info.read();        
            HLS_FLATTEN_ARRAY(config.do_inverse);
            HLS_FLATTEN_ARRAY(config.logn_samples);
            HLS_FLATTEN_ARRAY(config.do_shift);
            HLS_FLATTEN_ARRAY(config.input_queue_base);
            HLS_FLATTEN_ARRAY(config.output_queue_base);
            
            context_quota = config.context_quota;
            valid_contexts = config.valid_contexts;

            wait();

            if (cycles_elapsed > context_quota) {
                sc_uint<MAX_CONTEXTS_BITS> v = valid_contexts;

                if (current_context_int == v - 1) {
                    current_context_int = 0;
                } else {
                    current_context_int++;
                }

                {
                    HLS_PROTO("read-compute-config");
                    // User-defined config code
                    /* <<--local-params-->> */
                    logn_samples = config.logn_samples[current_context_int];
                    num_samples = 1 << logn_samples;
                    do_inverse = config.do_inverse[current_context_int];
                    do_shift = config.do_shift[current_context_int];

                    // Set the start cycles for this context to current cycle value.
                    start_cycles = accel_cycles;
                    wait();
                }
            }

            current_context_int_dbg.write(current_context_int);
            current_context.write(current_context_int);
            wait();
        }

        // Poll input queue is full for new task
        while (1)
        {
            HLS_PROTO("test-input-is-full");

            load_state_req = TEST_INPUT_IS_FULL;

            compute_state_req_dbg.write(TEST_INPUT_IS_FULL);

            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();

            if (input_is_full == 1) {
                input_is_full = 0;
                break;
            } else {              
                // Is the quota of current context complete?
                uint32_t cycles_elapsed = accel_cycles - start_cycles;

                if (cycles_elapsed > context_quota) {
                    HLS_PROTO("switch-context-1");
                    switch_context = true;
                    switch_context_dbg.write(1);
                    wait();
                    break;
                } else {
                    HLS_PROTO("no-switch-context");
                    wait();
                    continue;
                }
            }

            // If input is not full and if quota is not exceeded, we will continue spinning.
        }

        // If spinning for long time, switch context
        if (switch_context) {
            HLS_PROTO("switch-context-0");
            switch_context = false;
            switch_context_dbg.write(0);
            wait();
            continue;
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
        }
#ifdef ENABLE_SM
        // Update input queue to be empty
        {
            HLS_PROTO("update-input-is-empty");

            store_state_req = UPDATE_INPUT_IS_EMPTY;

            compute_state_req_dbg.write(UPDATE_INPUT_IS_EMPTY);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_state_req_dbg.write(STORE_FENCE);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();
        }
#endif
        // Compute FFT
        {
            unsigned offset = 0;  // Offset into Mem for start of this FFT
            int sin_sign = (do_inverse) ? -1 : 1; // This modifes the mySin
                                                  // values used below
            if (do_inverse && do_shift) {
                fft2_do_shift(offset, num_samples, logn_samples);
            }

            // Do the bit-reverse
            fft2_bit_reverse(offset, num_samples, logn_samples);

            // Computing phase implementation
            int m = 1;  // iterative FFT

            FFT2_SINGLE_L1:
                for(unsigned s = 1; s <= logn_samples; s++) {
                    m = 1 << s;
                    CompNum wm(myCos(s), sin_sign*mySin(s));

                FFT2_SINGLE_L2:
                    for(unsigned k = 0; k < num_samples; k +=m) {

                        CompNum w((FPDATA) 1, (FPDATA) 0);
                        int md2 = m / 2;

                    FFT2_SINGLE_L3:
                        for(int j = 0; j < md2; j++) {

                            int kj = offset + k + j;
                            int kjm = offset + k + j + md2;
                            CompNum akj, akjm;
                            CompNum bkj, bkjm;

                            akj.re = int2fp<FPDATA, WORD_SIZE>(A0[2 * kj]);
                            akj.im = int2fp<FPDATA, WORD_SIZE>(A0[2 * kj + 1]);
                            akjm.re = int2fp<FPDATA, WORD_SIZE>(A0[2 * kjm]);
                            akjm.im = int2fp<FPDATA, WORD_SIZE>(A0[2 * kjm + 1]);

                            CompNum t;
                            compMul(w, akjm, t);
                            CompNum u(akj.re, akj.im);
                            compAdd(u, t, bkj);
                            compSub(u, t, bkjm);
                            CompNum wwm;
                            wwm.re = w.re - (wm.im * w.im + wm.re * w.re);
                            wwm.im = w.im + (wm.im * w.re - wm.re * w.im);
                            w = wwm;

                            {
                                HLS_PROTO("compute_write_A0");
                                HLS_BREAK_DEP(A0);
                                wait();
                                A0[2 * kj] = fp2int<FPDATA, WORD_SIZE>(bkj.re);
                                A0[2 * kj + 1] = fp2int<FPDATA, WORD_SIZE>(bkj.im);
                                wait();
                                A0[2 * kjm] = fp2int<FPDATA, WORD_SIZE>(bkjm.re);
                                A0[2 * kjm + 1] = fp2int<FPDATA, WORD_SIZE>(bkjm.im);
                            }
                        } // for (j = 0 .. md2)
                    } // for (k = 0 .. num_samples)
                } // for (s = 1 .. logn_samples)

            if ((!do_inverse) && (do_shift)) {
                fft2_do_shift(offset, num_samples, logn_samples);
            }
        } // Compute
#ifdef ENABLE_SM
        // Poll output queue is empty for new task
        {
            HLS_PROTO("poll-output-is-empty");

            load_state_req = POLL_OUTPUT_IS_EMPTY;

            compute_state_req_dbg.write(POLL_OUTPUT_IS_EMPTY);

            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();
        }
#endif
        // Store output data
        {
            HLS_PROTO("store-output-data");

            store_state_req = STORE_DATA_REQ;

            this->compute_store_ready_handshake();

            compute_state_req_dbg.write(STORE_DATA_REQ);

            wait();
            this->compute_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_state_req_dbg.write(STORE_FENCE);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();
        }
#ifdef ENABLE_SM
        // Update output queue to be full 
        {
            HLS_PROTO("update-output-is-full");

            store_state_req = UPDATE_OUTPUT_IS_FULL;

            compute_state_req_dbg.write(UPDATE_OUTPUT_IS_FULL);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_state_req_dbg.write(STORE_FENCE);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();
        }
#endif
    } // while (true)
} // Function : compute_kernel

#else // ENABLE_PP

#include "audio_fft_pipelined.cpp"

#endif // ENABLE_PP
