// Copyright (c) 2011-2019 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef ENABLE_PP

#include "audio_ffi.hpp"
#include "audio_ffi_directives.hpp"

// Functions

#include "audio_ffi_functions.hpp"

// Processes

void audio_ffi::load_input()
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
    int32_t filter_queue_base;
    int32_t input_valid_offset;
    int32_t output_valid_offset;
    int32_t filter_valid_offset;
    int32_t input_payload_offset;
    int32_t filter_payload_offset;
    int32_t twiddle_payload_offset;
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
            HLS_FLATTEN_ARRAY(config.logn_samples);
            HLS_FLATTEN_ARRAY(config.do_shift);
            HLS_FLATTEN_ARRAY(config.input_queue_base);
            HLS_FLATTEN_ARRAY(config.output_queue_base);
            HLS_FLATTEN_ARRAY(config.filter_queue_base);

            // User-defined config code
            /* <<--local-params-->> */
            logn_samples = config.logn_samples[current_context_int];
            num_samples = 1 << logn_samples;

            // Configured shared memory base addresses for input and output queues
            input_queue_base = config.input_queue_base[current_context_int];
            output_queue_base = config.output_queue_base[current_context_int];
            filter_queue_base = config.filter_queue_base[current_context_int];

            // offsets for input valid and output valid based on preset values
            input_valid_offset = input_queue_base + VALID_OFFSET;
            output_valid_offset = output_queue_base + VALID_OFFSET;
            filter_valid_offset = filter_queue_base + VALID_OFFSET;

            // offsets for input valid and output payload based on preset values
            input_payload_offset = input_queue_base + PAYLOAD_OFFSET;
            filter_payload_offset = filter_queue_base + PAYLOAD_OFFSET;

            twiddle_payload_offset = filter_payload_offset + 2 * (num_samples + 1);

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
            case POLL_FILTER_IS_FULL:
            {
                dma_info_t dma_info(filter_valid_offset / DMA_WORD_PER_BEAT, TEST_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                int32_t filter_is_full = 0;

                wait();

                // Wait for producer to send new data
                while (filter_is_full != 1)
                {
                    HLS_UNROLL_LOOP(OFF);
                    this->dma_read_ctrl.put(dma_info);
                    dataBv = this->dma_read_chnl.get();
                    wait();
                    filter_is_full = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
                }
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
            // Load filters
            {
                dma_info_t dma_info(filter_payload_offset / DMA_WORD_PER_BEAT, 2 * (num_samples + 1)/ DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                this->dma_read_ctrl.put(dma_info);

                for (int i = 0; i < 2 * (num_samples + 1); i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(F0);

                    dataBv = this->dma_read_chnl.get();
                    wait();
                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        F0[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                    }
                }
            }
            // Load twiddle factors 
            {
                dma_info_t dma_info(twiddle_payload_offset / DMA_WORD_PER_BEAT, num_samples / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                this->dma_read_ctrl.put(dma_info);

                for (int i = 0; i < num_samples; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(T0);

                    dataBv = this->dma_read_chnl.get();
                    wait();
                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        T0[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
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

void audio_ffi::store_output()
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
    int32_t num_samples;
    int32_t input_queue_base;
    int32_t output_queue_base;
    int32_t filter_queue_base;
    int32_t input_valid_offset;
    int32_t output_valid_offset;
    int32_t filter_valid_offset;
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
            HLS_FLATTEN_ARRAY(config.logn_samples);
            HLS_FLATTEN_ARRAY(config.input_queue_base);
            HLS_FLATTEN_ARRAY(config.output_queue_base);
            HLS_FLATTEN_ARRAY(config.filter_queue_base);

            // User-defined config code
            /* <<--local-params-->> */
            logn_samples = config.logn_samples[current_context_int];
            num_samples = 1 << logn_samples;

            // Configured shared memory base addresses for input and output queues
            input_queue_base = config.input_queue_base[current_context_int];
            output_queue_base = config.output_queue_base[current_context_int];
            filter_queue_base = config.filter_queue_base[current_context_int];

            // offsets for input valid and output valid based on preset values
            input_valid_offset = input_queue_base + VALID_OFFSET;
            output_valid_offset = output_queue_base + VALID_OFFSET;
            filter_valid_offset = filter_queue_base + VALID_OFFSET;

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
            case UPDATE_FILTER_IS_EMPTY:
            {
                dma_info_t dma_info(filter_valid_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
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

void audio_ffi::compute_kernel()
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

        start_cycles = 0;
        cycles_elapsed = 0;

        start_cycles_dbg.write(0);
        cycles_elapsed_dbg.write(0);

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    int32_t do_shift;
    uint32_t context_quota;
    uint32_t valid_contexts;
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
        {
            HLS_PROTO("check-new-context");

            // Is the quota of current context complete?
            cycles_elapsed = accel_cycles - start_cycles;
            cycles_elapsed_dbg.write(cycles_elapsed);

            conf_info_t config = this->conf_info.read();        
            HLS_FLATTEN_ARRAY(config.logn_samples);
            HLS_FLATTEN_ARRAY(config.do_shift);
            HLS_FLATTEN_ARRAY(config.input_queue_base);
            HLS_FLATTEN_ARRAY(config.output_queue_base);
            HLS_FLATTEN_ARRAY(config.filter_queue_base);
            
            context_quota = config.context_quota;
            valid_contexts = config.valid_contexts;

            wait();

            if (cycles_elapsed > context_quota) {
                sc_uint<MAX_CONTEXTS> v = valid_contexts;
                sc_uint<MAX_CONTEXTS_BITS> idx = current_context_int + 1;

                for (int i = 0; i < MAX_CONTEXTS; i++) {
                    if (v[idx] == 1) {
                        current_context_int = idx;
                        break;
                    }
                    idx++;
                }

                {
                    HLS_PROTO("read-compute-config");
                    // User-defined config code
                    /* <<--local-params-->> */
                    logn_samples = config.logn_samples[current_context_int];
                    num_samples = 1 << logn_samples;
                    do_shift = config.do_shift[current_context_int];

                    // Set the start cycles for this context to current cycle value.
                    start_cycles = accel_cycles;
                    start_cycles_dbg.write(start_cycles);
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
                HLS_PROTO("check-cycles-elapsed");

                // Is the quota of current context complete?
                cycles_elapsed = accel_cycles - start_cycles;
                cycles_elapsed_dbg.write(cycles_elapsed);

                wait();

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

        // Poll filter queue is full for new task
        {
            HLS_PROTO("poll-filter-is-full");

            load_state_req = POLL_FILTER_IS_FULL;

            compute_state_req_dbg.write(POLL_FILTER_IS_FULL);

            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();
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

        // Update filter queue to be empty
        {
            HLS_PROTO("update-filter-is-empty");

            store_state_req = UPDATE_FILTER_IS_EMPTY;

            compute_state_req_dbg.write(UPDATE_FILTER_IS_EMPTY);

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
        // Compute - FFT
        {
            unsigned offset = 0;  // Offset into Mem for start of this FFT
            int sin_sign = 1; // This modifes the mySin

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
                                HLS_PROTO("compute_write_A0_fft");
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

            if (do_shift) {
                fft2_do_shift(offset, num_samples, logn_samples);
            }
        }
        // Compute - FIR
        {
            CompNum fpnk, fpk, f1k, f2k, tw, tdc;
            CompNum tf, if0, ifn, of0, flt0, fltn, t0, tn;
            CompNum fk, fnkc, fek, fok, tmp;
            CompNum tmpbuf0, tmpbufn;

            // first and last element
            {
                // Post-process first and last element
                tdc.re = int2fp<FPDATA, WORD_SIZE>(A0[0]);
                tdc.im = int2fp<FPDATA, WORD_SIZE>(A0[1]);

                if0.re = tdc.re + tdc.im;
                if0.im = 0;
                ifn.re = tdc.re - tdc.im;
                ifn.im = 0;

                // Reading filter values
                flt0.re = int2fp<FPDATA, WORD_SIZE>(F0[0]);
                flt0.im = int2fp<FPDATA, WORD_SIZE>(F0[1]);
                fltn.re = int2fp<FPDATA, WORD_SIZE>(F0[(2 * num_samples)]);
                fltn.im = int2fp<FPDATA, WORD_SIZE>(F0[(2 * num_samples) + 1]);

                // fir
                compMul(if0, flt0, t0);
                compMul(ifn, fltn, tn);

                // Pre-process first and last element
                of0.re = t0.re + tn.re;
                of0.im = t0.re - tn.re;

                // Write back element 0 to memory
                {
                    HLS_PROTO("write-back-elem-0");
                    HLS_BREAK_DEP(A0);
                    wait();
                    A0[0] = fp2int<FPDATA, WORD_SIZE>(of0.re);
                    A0[1] = fp2int<FPDATA, WORD_SIZE>(of0.im);
                }
            }

            // Remaining elements
            for (unsigned k = 2; k <= num_samples; k+=2)
            {
                // Read FFT output
                fpk.re = int2fp<FPDATA, WORD_SIZE>(A0[k]);
                fpk.im = int2fp<FPDATA, WORD_SIZE>(A0[k + 1]);
                fpnk.re = int2fp<FPDATA, WORD_SIZE>(A0[(2 * num_samples) - k]);
                fpnk.im = - (int2fp<FPDATA, WORD_SIZE>(A0[(2 * num_samples) - k + 1]));

                // Read twiddle factors
                tf.re = int2fp<FPDATA, WORD_SIZE>(T0[k - 2]);
                tf.im = int2fp<FPDATA, WORD_SIZE>(T0[k - 1]);

                compAdd(fpk, fpnk, f1k);
                compSub(fpk, fpnk, f2k);
                compMul(f2k, tf, tw);

                // Computing freqdata's
                compAdd(f1k, tw, if0);
                if0.re /= 2; if0.im /= 2;

                compSub(f1k, tw, ifn);
                ifn.re /= 2; ifn.im /= 2;
                ifn.im *= -1;

                // Reading filter values
                flt0.re = int2fp<FPDATA, WORD_SIZE>(F0[k]);
                flt0.im = int2fp<FPDATA, WORD_SIZE>(F0[k + 1]);
                fltn.re = int2fp<FPDATA, WORD_SIZE>(F0[(2 * num_samples) - k]);
                fltn.im = int2fp<FPDATA, WORD_SIZE>(F0[(2 * num_samples) - k + 1]);

                // fir
                compMul(if0, flt0, t0);
                compMul(ifn, fltn, tn);

                if (k == num_samples) {
                    fk = tn;
                } else {
                    fk = t0;
                }

                fnkc.re = tn.re;
                fnkc.im = - (tn.im);

                tf.im *= -1;

                compAdd(fk, fnkc, fek);
                compSub(fk, fnkc, tmp);
                compMul(tmp, tf, fok);

                compAdd(fek, fok, tmpbuf0);
                compSub(fek, fok, tmpbufn);

                {
                    HLS_PROTO("write-back-elem-k");
                    HLS_BREAK_DEP(A0);
                    wait();
                    A0[k] = fp2int<FPDATA, WORD_SIZE>(tmpbuf0.re);
                    A0[k + 1] = fp2int<FPDATA, WORD_SIZE>(tmpbuf0.im);
                    wait();
                    A0[(2 * num_samples) - k] = fp2int<FPDATA, WORD_SIZE>(tmpbufn.re);
                    A0[(2 * num_samples) - k + 1] = - (fp2int<FPDATA, WORD_SIZE>(tmpbufn.im));
                }
            } // for (k = 0 .. num_samples)
        }
        // Compute - IFFT
        {
            unsigned offset = 0;  // Offset into Mem for start of this FFT
            int sin_sign = -1; // This modifes the mySin
                                                  // values used below
            if (do_shift) {
                fft2_do_shift(offset, num_samples, logn_samples);
            }

            // Do the bit-reverse
            fft2_bit_reverse(offset, num_samples, logn_samples);

            // Computing phase implementation
            int m = 1;  // iterative FFT

            IFFT2_SINGLE_L1:
                for(unsigned s = 1; s <= logn_samples; s++) {
                    m = 1 << s;
                    CompNum wm(myCos(s), sin_sign*mySin(s));

                IFFT2_SINGLE_L2:
                    for(unsigned k = 0; k < num_samples; k +=m) {

                        CompNum w((FPDATA) 1, (FPDATA) 0);
                        int md2 = m / 2;

                    IFFT2_SINGLE_L3:
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
                                HLS_PROTO("compute_write_A0_ifft");
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

#include "audio_ffi_pipelined.cpp"

#endif // ENABLE_PP