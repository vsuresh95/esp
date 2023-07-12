// Copyright (c) 2011-2019 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "audio_fir_pipelined.hpp"
#include "audio_fir_directives.hpp"

// Functions

#include "audio_fir_pipelined_functions.hpp"

// Processes

void audio_fir::load_input()
{
    // Reset
    {
        HLS_PROTO("load-reset");

        this->reset_load_input();

        load_state_req_dbg.write(0);

        input_load_start.ack.reset_ack();
        filters_load_start.ack.reset_ack();
        output_load_start.ack.reset_ack();
        load_input_done.req.reset_req();
        load_filters_done.req.reset_req();
        load_output_done.req.reset_req();

        prod_valid = 0;
        flt_valid = 0;
        cons_ready = 0;
        load_state_req = 0;
        load_state_req_module = 0;

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    int32_t prod_valid_offset;
    int32_t prod_ready_offset;
    int32_t flt_prod_valid_offset;
    int32_t flt_prod_ready_offset;
    int32_t cons_ready_offset;
    int32_t cons_valid_offset;
    int32_t load_data_offset;
    int32_t flt_load_data_offset;
    int32_t twd_load_data_offset;
    int32_t store_data_offset;
    bool pingpong;
    bool flt_pingpong;
    int32_t task_arbiter;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        logn_samples = config.logn_samples;
        num_samples = 1 << logn_samples;

        // Configured shared memory offsets for sync flags
        prod_valid_offset = config.prod_valid_offset;
        prod_ready_offset = config.prod_ready_offset;
        flt_prod_valid_offset = config.flt_prod_valid_offset;
        flt_prod_ready_offset = config.flt_prod_ready_offset;
        cons_ready_offset = config.cons_ready_offset;
        cons_valid_offset = config.cons_valid_offset;
        load_data_offset = config.load_data_offset;
        flt_load_data_offset = config.flt_load_data_offset;
        twd_load_data_offset = config.twd_load_data_offset;
        store_data_offset = config.store_data_offset;

        pingpong = false;
        flt_pingpong = false;
        task_arbiter = 0;
    }

    // Load
    while(true)
    {
        HLS_PROTO("load-dma");

        wait();

        // Wait for either input ASI or output ASI
        while (!(input_load_req_valid || filters_load_req_valid || output_load_req_valid)) wait();

        if (task_arbiter == 0 && input_load_req_valid) {
            this->load_input_start_handshake();
            load_state_req = input_load_req;
            load_state_req_module = INPUT_ASI;
            task_arbiter++;
        } else if (task_arbiter == 1 && filters_load_req_valid) {
            this->load_filters_start_handshake();
            load_state_req = filters_load_req;
            load_state_req_module = FILTERS_ASI;
            task_arbiter++;
        } else if (task_arbiter == 2 && output_load_req_valid) {
            this->load_output_start_handshake();
            load_state_req = output_load_req;
            load_state_req_module = OUTPUT_ASI;
            task_arbiter = 0;
        } else {
            if (task_arbiter == 2) task_arbiter = 0;
            else task_arbiter++;
            continue;
        }

        load_state_req_dbg.write(load_state_req);

        switch (load_state_req)
        {
            case TEST_PROD_VALID_REQ:
            {
                // Test for producer to send new data
                dma_info_t dma_info(prod_valid_offset / DMA_WORD_PER_BEAT, 2 * TEST_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();
                this->dma_read_ctrl.put(dma_info);

                dataBv = this->dma_read_chnl.get();
                wait();
                prod_valid = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
                dataBv = this->dma_read_chnl.get();
                wait();
                last_task = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
            }
            break;
            case TEST_FLT_PROD_VALID_REQ:
            {
                dma_info_t dma_info(flt_prod_valid_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();
                this->dma_read_ctrl.put(dma_info);

                dataBv = this->dma_read_chnl.get();
                wait();
                flt_valid = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
            }
            break;
            case TEST_CONS_READY_REQ:
            {
                // Test for consumer to accept new data
                dma_info_t dma_info(cons_ready_offset / DMA_WORD_PER_BEAT, TEST_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();
                this->dma_read_ctrl.put(dma_info);

                dataBv = this->dma_read_chnl.get();
                wait();
                cons_ready = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
            }
            break;
            case LOAD_DATA_REQ:
            {
                dma_info_t dma_info(load_data_offset / DMA_WORD_PER_BEAT, 2 * num_samples / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                this->dma_read_ctrl.put(dma_info);

                for (int i = 0; i < 2 * num_samples; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(A0);
                    HLS_BREAK_DEP(A1);

                    dataBv = this->dma_read_chnl.get();
                    wait();
                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        if (!pingpong)
                            A0[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                        else
                            A1[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                    }
                }
            }

            pingpong = !pingpong;
            break;
            case LOAD_FILTERS_REQ:
            // Load filters
            {
                dma_info_t dma_info(flt_load_data_offset / DMA_WORD_PER_BEAT, 2 * (num_samples + 1)/ DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                this->dma_read_ctrl.put(dma_info);

                for (int i = 0; i < 2 * (num_samples + 1); i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(F0);
                    HLS_BREAK_DEP(F1);

                    dataBv = this->dma_read_chnl.get();
                    wait();
                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        if (!flt_pingpong)
                            F0[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                        else
                            F1[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                    }
                }
            }
            // Load twiddle factors 
            {
                dma_info_t dma_info(twd_load_data_offset / DMA_WORD_PER_BEAT, num_samples / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                this->dma_read_ctrl.put(dma_info);

                for (int i = 0; i < num_samples; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(T0);
                    HLS_BREAK_DEP(T1);

                    dataBv = this->dma_read_chnl.get();
                    wait();
                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        if (!flt_pingpong)
                            T0[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                        else
                            T1[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
                    }
                }
            }

            flt_pingpong = !flt_pingpong;
            break;
            default:
            break;
        }

        wait();

        if (load_state_req_module == INPUT_ASI) {
            this->load_input_done_handshake();
        } else if (load_state_req_module == FILTERS_ASI) {
            this->load_filters_done_handshake();
        } else {
            this->load_output_done_handshake();
        }

        load_state_req_module = 0;
    }
} // Function : load_input

void audio_fir::store_output()
{
    // Reset
    {
        HLS_PROTO("store-reset");

        this->reset_store_output();

        store_state_req_dbg.write(0);

        input_store_start.ack.reset_ack();
        filters_store_start.ack.reset_ack();
        output_store_start.ack.reset_ack();
        store_input_done.req.reset_req();
        store_filters_done.req.reset_req();
        store_output_done.req.reset_req();

        store_state_req = 0;
        store_state_req_module = 0;

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    int32_t prod_valid_offset;
    int32_t prod_ready_offset;
    int32_t flt_prod_valid_offset;
    int32_t flt_prod_ready_offset;
    int32_t cons_valid_offset;
    int32_t cons_ready_offset;
    int32_t load_data_offset;
    int32_t flt_load_data_offset;
    int32_t twd_load_data_offset;
    int32_t store_data_offset;
    bool pingpong;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process

        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        logn_samples = config.logn_samples;
        num_samples = 1 << logn_samples;

        // Configured shared memory offsets for sync flags
        prod_valid_offset = config.prod_valid_offset;
        prod_ready_offset = config.prod_ready_offset;
        flt_prod_valid_offset = config.flt_prod_valid_offset;
        flt_prod_ready_offset = config.flt_prod_ready_offset;
        cons_valid_offset = config.cons_valid_offset;
        cons_ready_offset = config.cons_ready_offset;
        load_data_offset = config.load_data_offset;
        flt_load_data_offset = config.flt_load_data_offset;
        twd_load_data_offset = config.twd_load_data_offset;
        store_data_offset = config.store_data_offset;

        pingpong = false;
    }

    // Store
    while(true)
    {
        HLS_PROTO("store-dma");

        wait();

        // Wait for either input ASI or output ASI
        while (!(input_store_req_valid || filters_store_req_valid || output_store_req_valid)) wait();

        if (input_store_req_valid) {
            this->store_input_start_handshake();
            store_state_req = input_store_req;
            store_state_req_module = INPUT_ASI;
        } else if (filters_store_req_valid) {
            this->store_filters_start_handshake();
            store_state_req = filters_store_req;
            store_state_req_module = FILTERS_ASI;
        } else {
            this->store_output_start_handshake();
            store_state_req = output_store_req;
            store_state_req_module = OUTPUT_ASI;
        }

        store_state_req_dbg.write(store_state_req);

        switch (store_state_req)
        {
            case UPDATE_PROD_READY_REQ:
            {
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
            case UPDATE_FLT_PROD_READY_REQ:
            {
                dma_info_t dma_info(flt_prod_ready_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
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
            case UPDATE_FLT_PROD_VALID_REQ:
            {
                dma_info_t dma_info(flt_prod_valid_offset / DMA_WORD_PER_BEAT, UPDATE_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
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
            case STORE_DATA_REQ:
            {
                dma_info_t dma_info(store_data_offset / DMA_WORD_PER_BEAT, 2 * num_samples / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                this->dma_write_ctrl.put(dma_info);

                for (int i = 0; i < 2 * num_samples; i += DMA_WORD_PER_BEAT)
                {
                    HLS_BREAK_DEP(A0);
                    HLS_BREAK_DEP(A1);

                    wait();

                    for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
                    {
                        HLS_UNROLL_SIMPLE;
                        if (!pingpong)
                            dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = A0[i + k];
                        else
                            dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = A1[i + k];
                    }

                    this->dma_write_chnl.put(dataBv);
                }

                // Wait till the last write is accepted at the cache
                wait();
                while (!(this->dma_write_chnl.ready)) wait();
            }

            pingpong = !pingpong;
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

        if (store_state_req_module == INPUT_ASI) {
            this->store_input_done_handshake();
        } else if (store_state_req_module == FILTERS_ASI) {
            this->store_filters_done_handshake();
        } else {
            this->store_output_done_handshake();
        }

        store_state_req_module = 0;
    }
} // Function : store_output

void audio_fir::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-reset");

        this->reset_compute_kernel();

        compute_state_req_dbg.write(0);

        input_to_compute.ack.reset_ack();
        compute_to_output.req.reset_req();

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    int32_t do_inverse;
    int32_t do_shift;
    bool pingpong;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        logn_samples = config.logn_samples;
        num_samples = 1 << logn_samples;
        do_inverse = config.do_inverse;
        do_shift = config.do_shift;
        pingpong = false;
    }

    while(true)
    {
        // Wait for load to be complete
        {
            HLS_PROTO("wait-for-load");

            this->compute_input_handshake();
            wait();
        }

        // Wait for filters to be complete
        {
            HLS_PROTO("wait-for-filters");

            this->compute_filters_handshake();
            wait();

            compute_state_req_dbg.write(COMPUTE);
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
                if (!pingpong) {
                    tdc.re = int2fp<FPDATA, WORD_SIZE>(A0[0]);
                    tdc.im = int2fp<FPDATA, WORD_SIZE>(A0[1]);
                } else {
                    tdc.re = int2fp<FPDATA, WORD_SIZE>(A1[0]);
                    tdc.im = int2fp<FPDATA, WORD_SIZE>(A1[1]);
                }

                if0.re = tdc.re + tdc.im;
                if0.im = 0;
                ifn.re = tdc.re - tdc.im;
                ifn.im = 0;

                // Reading filter values
                if (!pingpong) {
                    flt0.re = int2fp<FPDATA, WORD_SIZE>(F0[0]);
                    flt0.im = int2fp<FPDATA, WORD_SIZE>(F0[1]);
                    fltn.re = int2fp<FPDATA, WORD_SIZE>(F0[(2 * num_samples)]);
                    fltn.im = int2fp<FPDATA, WORD_SIZE>(F0[(2 * num_samples) + 1]);
                } else {
                    flt0.re = int2fp<FPDATA, WORD_SIZE>(F1[0]);
                    flt0.im = int2fp<FPDATA, WORD_SIZE>(F1[1]);
                    fltn.re = int2fp<FPDATA, WORD_SIZE>(F1[(2 * num_samples)]);
                    fltn.im = int2fp<FPDATA, WORD_SIZE>(F1[(2 * num_samples) + 1]);
                }

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
                    HLS_BREAK_DEP(A1);

                    if (!pingpong) {
                        wait();
                        A0[0] = fp2int<FPDATA, WORD_SIZE>(of0.re);
                        A0[1] = fp2int<FPDATA, WORD_SIZE>(of0.im);
                    } else {
                        wait();
                        A1[0] = fp2int<FPDATA, WORD_SIZE>(of0.re);
                        A1[1] = fp2int<FPDATA, WORD_SIZE>(of0.im);
                    }
                }
            }

            // Remaining elements
            for (unsigned k = 2; k <= num_samples; k+=2)
            {
                // Read FFT output
                if (!pingpong) {
                    fpk.re = int2fp<FPDATA, WORD_SIZE>(A0[k]);
                    fpk.im = int2fp<FPDATA, WORD_SIZE>(A0[k + 1]);
                    fpnk.re = int2fp<FPDATA, WORD_SIZE>(A0[(2 * num_samples) - k]);
                    fpnk.im = - (int2fp<FPDATA, WORD_SIZE>(A0[(2 * num_samples) - k + 1]));
                } else {
                    fpk.re = int2fp<FPDATA, WORD_SIZE>(A1[k]);
                    fpk.im = int2fp<FPDATA, WORD_SIZE>(A1[k + 1]);
                    fpnk.re = int2fp<FPDATA, WORD_SIZE>(A1[(2 * num_samples) - k]);
                    fpnk.im = - (int2fp<FPDATA, WORD_SIZE>(A1[(2 * num_samples) - k + 1]));
                }

                // Read twiddle factors
                if (!pingpong) {
                    tf.re = int2fp<FPDATA, WORD_SIZE>(T0[k - 2]);
                    tf.im = int2fp<FPDATA, WORD_SIZE>(T0[k - 1]);
                } else {
                    tf.re = int2fp<FPDATA, WORD_SIZE>(T1[k - 2]);
                    tf.im = int2fp<FPDATA, WORD_SIZE>(T1[k - 1]);
                }

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
                if (!pingpong) {
                    flt0.re = int2fp<FPDATA, WORD_SIZE>(F0[k]);
                    flt0.im = int2fp<FPDATA, WORD_SIZE>(F0[k + 1]);
                    fltn.re = int2fp<FPDATA, WORD_SIZE>(F0[(2 * num_samples) - k]);
                    fltn.im = int2fp<FPDATA, WORD_SIZE>(F0[(2 * num_samples) - k + 1]);
                } else {
                    flt0.re = int2fp<FPDATA, WORD_SIZE>(F1[k]);
                    flt0.im = int2fp<FPDATA, WORD_SIZE>(F1[k + 1]);
                    fltn.re = int2fp<FPDATA, WORD_SIZE>(F1[(2 * num_samples) - k]);
                    fltn.im = int2fp<FPDATA, WORD_SIZE>(F1[(2 * num_samples) - k + 1]);
                }

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
                    HLS_BREAK_DEP(A1);
                    
                    if (!pingpong) {
                        wait();
                        A0[k] = fp2int<FPDATA, WORD_SIZE>(tmpbuf0.re);
                        A0[k + 1] = fp2int<FPDATA, WORD_SIZE>(tmpbuf0.im);
                        wait();
                        A0[(2 * num_samples) - k] = fp2int<FPDATA, WORD_SIZE>(tmpbufn.re);
                        A0[(2 * num_samples) - k + 1] = - (fp2int<FPDATA, WORD_SIZE>(tmpbufn.im));
                    } else {
                        wait();
                        A1[k] = fp2int<FPDATA, WORD_SIZE>(tmpbuf0.re);
                        A1[k + 1] = fp2int<FPDATA, WORD_SIZE>(tmpbuf0.im);
                        wait();
                        A1[(2 * num_samples) - k] = fp2int<FPDATA, WORD_SIZE>(tmpbufn.re);
                        A1[(2 * num_samples) - k + 1] = - (fp2int<FPDATA, WORD_SIZE>(tmpbufn.im));
                    }
                }
            } // for (k = 0 .. num_samples)
        }

        // Inform output to start
        {
            HLS_PROTO("inform-output-start");

            compute_state_req_dbg.write(0);

            this->compute_output_handshake();
            wait();
        }

        pingpong = !pingpong;
    } // while (true)
} // Function : compute_kernel


void audio_fir::input_asi_kernel()
{
    // Reset
    {
        HLS_PROTO("input-asi-reset");

        input_state_req_dbg.write(0);

        input_load_start.req.reset_req();
        input_store_start.req.reset_req();
        load_input_done.ack.reset_ack();
        store_input_done.ack.reset_ack();
        input_to_compute.req.reset_req();

        input_load_req = 0;
        input_store_req = 0;
        input_load_req_valid = false;
        input_store_req_valid = false;

        end_acc = 0;

        wait();
    }

    // Config
    /* <<--params-->> */
    {
        HLS_PROTO("input-asi-config");

        cfg.wait_for_config(); // config process

        // User-defined config code
        /* <<--local-params-->> */
    }

    while(true)
    {
        // Test producer's valid for new task
        {
            HLS_PROTO("test-prod-valid");

            input_load_req = TEST_PROD_VALID_REQ;
            input_load_req_valid = true;

            input_state_req_dbg.write(TEST_PROD_VALID_REQ);

            this->input_load_start_handshake();
            wait();
            input_load_req_valid = false;
            wait();
            this->input_load_done_handshake();
            wait();
        }

        {
            HLS_PROTO("prod-valid-check");

            if (prod_valid == 1)
            {
                prod_valid = 0;
                end_acc = last_task;

                // Reset producer's valid
                {
                    HLS_PROTO("update-prod-valid");

                    input_store_req = UPDATE_PROD_VALID_REQ;
                    input_store_req_valid = true;

                    input_state_req_dbg.write(UPDATE_PROD_VALID_REQ);

                    this->input_store_start_handshake();
                    wait();
                    input_store_req_valid = false;
                    wait();
                    this->input_store_done_handshake();
                    wait();

                    // Wait for all writes to be done and then issue fence
                    input_store_req = STORE_FENCE;
                    input_store_req_valid = true;

                    input_state_req_dbg.write(STORE_FENCE);

                    this->input_store_start_handshake();
                    wait();
                    input_store_req_valid = false;
                    wait();
                    this->input_store_done_handshake();
                    wait();
                }

                // Load input data
                {
                    HLS_PROTO("load-input-data");

                    input_load_req = LOAD_DATA_REQ;
                    input_load_req_valid = true;

                    input_state_req_dbg.write(LOAD_DATA_REQ);

                    this->input_load_start_handshake();
                    wait();
                    input_load_req_valid = false;
                    wait();
                    this->input_load_done_handshake();
                    wait();
                }

                // update producer's ready to accept new data
                {
                    HLS_PROTO("update-prod-ready");

                    input_store_req = UPDATE_PROD_READY_REQ;
                    input_store_req_valid = true;

                    input_state_req_dbg.write(UPDATE_PROD_READY_REQ);

                    this->input_store_start_handshake();
                    wait();
                    input_store_req_valid = false;
                    wait();
                    this->input_store_done_handshake();
                    wait();

                    // Wait for all writes to be done and then issue fence
                    input_store_req = STORE_FENCE;
                    input_store_req_valid = true;

                    input_state_req_dbg.write(STORE_FENCE);

                    this->input_store_start_handshake();
                    wait();
                    input_store_req_valid = false;
                    wait();
                    this->input_store_done_handshake();
                    wait();
                }

                // Inform compute to start
                {
                    HLS_PROTO("inform-compute-start");

                    this->input_compute_handshake();
                    wait();
                }
            }
        }
    } // while (true)
} // Function : input_asi_kernel 

void audio_fir::filters_asi_kernel()
{
    // Reset
    {
        HLS_PROTO("filters-asi-reset");

        filters_state_req_dbg.write(0);

        filters_load_start.req.reset_req();
        filters_store_start.req.reset_req();
        load_filters_done.ack.reset_ack();
        store_filters_done.ack.reset_ack();
        filters_to_compute.req.reset_req();

        filters_load_req = 0;
        filters_store_req = 0;
        filters_load_req_valid = false;
        filters_store_req_valid = false;

        wait();
    }

    // Config
    /* <<--params-->> */
    {
        HLS_PROTO("filters-asi-config");

        cfg.wait_for_config(); // config process

        // User-defined config code
        /* <<--local-params-->> */
    }

    while(true)
    {
        // Test filter producer's valid for new filters
        {
            HLS_PROTO("test-flt-valid");

            filters_load_req = TEST_FLT_PROD_VALID_REQ;
            filters_load_req_valid = true;

            filters_state_req_dbg.write(TEST_FLT_PROD_VALID_REQ);

            this->filters_load_start_handshake();
            wait();
            filters_load_req_valid = false;
            wait();
            this->filters_load_done_handshake();
            wait();
        }

        {
            HLS_PROTO("flt-valid-check");

            if (flt_valid == 1)
            {
                flt_valid = 0;

                // Reset filter producer's valid
                {
                    HLS_PROTO("update-flt-prod-valid");

                    filters_store_req = UPDATE_FLT_PROD_VALID_REQ;
                    filters_store_req_valid = true;

                    filters_state_req_dbg.write(UPDATE_FLT_PROD_VALID_REQ);

                    this->filters_store_start_handshake();
                    wait();
                    filters_store_req_valid = false;
                    wait();
                    this->filters_store_done_handshake();
                    wait();

                    // Wait for all writes to be done and then issue fence
                    filters_store_req = STORE_FENCE;
                    filters_store_req_valid = true;

                    filters_state_req_dbg.write(STORE_FENCE);

                    this->filters_store_start_handshake();
                    wait();
                    filters_store_req_valid = false;
                    wait();
                    this->filters_store_done_handshake();
                    wait();
                }

                // Load filters data
                {
                    HLS_PROTO("load-filters-data");

                    filters_load_req = LOAD_FILTERS_REQ;
                    filters_load_req_valid = true;

                    filters_state_req_dbg.write(LOAD_FILTERS_REQ);

                    this->filters_load_start_handshake();
                    wait();
                    filters_load_req_valid = false;
                    wait();
                    this->filters_load_done_handshake();
                    wait();
                }

                // update filter producer's ready to accept new filters
                {
                    HLS_PROTO("update-flt-prod-ready");

                    filters_store_req = UPDATE_FLT_PROD_READY_REQ;
                    filters_store_req_valid = true;

                    filters_state_req_dbg.write(UPDATE_FLT_PROD_READY_REQ);

                    this->filters_store_start_handshake();
                    wait();
                    filters_store_req_valid = false;
                    wait();
                    this->filters_store_done_handshake();
                    wait();

                    // Wait for all writes to be done and then issue fence
                    filters_store_req = STORE_FENCE;
                    filters_store_req_valid = true;

                    filters_state_req_dbg.write(STORE_FENCE);

                    this->filters_store_start_handshake();
                    wait();
                    filters_store_req_valid = false;
                    wait();
                    this->filters_store_done_handshake();
                    wait();
                }

                // Inform compute to start
                {
                    HLS_PROTO("inform-compute-start-flt");

                    this->filters_compute_handshake();
                    wait();
                }
            }
        }
    }
}

void audio_fir::output_asi_kernel()
{
    // Reset
    {
        HLS_PROTO("output-asi-reset");

        output_state_req_dbg.write(0);

        output_load_start.req.reset_req();
        output_store_start.req.reset_req();
        load_output_done.ack.reset_ack();
        store_output_done.ack.reset_ack();
        compute_to_output.ack.reset_ack();

        output_load_req = 0;
        output_store_req = 0;
        output_load_req_valid = false;
        output_store_req_valid = false;

        wait();
    }

    // Config
    /* <<--params-->> */
    {
        HLS_PROTO("output-asi-config");

        cfg.wait_for_config(); // config process

        // User-defined config code
        /* <<--local-params-->> */
    }

    while(true)
    {
        // Wait for compute to be complete
        {
            HLS_PROTO("wait-for-compute");

            this->output_compute_handshake();
            wait();
        }

        // Poll consumer's ready to know if we can send new data
        {
            HLS_PROTO("poll-for-cons-ready");

            while (cons_ready == 0)
            {
                output_load_req = TEST_CONS_READY_REQ;
                output_load_req_valid = true;

                output_state_req_dbg.write(TEST_CONS_READY_REQ);

                this->output_load_start_handshake();
                wait();
                output_load_req_valid = false;
                wait();
                this->output_load_done_handshake();
                wait();
            }
        }

        {
            HLS_PROTO("cons-ready-check");

            cons_ready = 0;

            // Reset consumer's ready
            {
                HLS_PROTO("update-cons-ready");

                output_store_req = UPDATE_CONS_READY_REQ;
                output_store_req_valid = true;

                output_state_req_dbg.write(UPDATE_CONS_READY_REQ);

                this->output_store_start_handshake();
                wait();
                output_store_req_valid = false;
                wait();
                this->output_store_done_handshake();
                wait();

                // Wait for all writes to be done and then issue fence
                output_store_req = STORE_FENCE;
                output_store_req_valid = true;

                output_state_req_dbg.write(STORE_FENCE);

                this->output_store_start_handshake();
                wait();
                output_store_req_valid = false;
                wait();
                this->output_store_done_handshake();
                wait();
            }
            // Store output data
            {
                HLS_PROTO("store-output-data");

                output_store_req = STORE_DATA_REQ;
                output_store_req_valid = true;

                output_state_req_dbg.write(STORE_DATA_REQ);

                this->output_store_start_handshake();
                wait();
                output_store_req_valid = false;
                wait();
                this->output_store_done_handshake();
                wait();

                // Wait for all writes to be done and then issue fence
                output_store_req = STORE_FENCE;
                output_store_req_valid = true;

                output_state_req_dbg.write(STORE_FENCE);

                this->output_store_start_handshake();
                wait();
                output_store_req_valid = false;
                wait();
                this->output_store_done_handshake();
                wait();
            }
            // update consumer's ready for new data available
            {
                HLS_PROTO("update-cons-ready");

                output_store_req = UPDATE_CONS_VALID_REQ;
                output_store_req_valid = true;

                output_state_req_dbg.write(UPDATE_CONS_VALID_REQ);

                this->output_store_start_handshake();
                wait();
                output_store_req_valid = false;
                wait();
                this->output_store_done_handshake();
                wait();

                // Wait for all writes to be done and then issue fence
                output_store_req = STORE_FENCE;
                output_store_req_valid = true;

                output_state_req_dbg.write(STORE_FENCE);

                this->output_store_start_handshake();
                wait();
                output_store_req_valid = false;
                wait();
                this->output_store_done_handshake();
                wait();
            }

            // End operation
            {
                HLS_PROTO("end-acc");

                if (end_acc == 1)
                {
                    output_store_req = ACC_DONE;
                    output_store_req_valid = true;

                    output_state_req_dbg.write(ACC_DONE);

                    this->output_store_start_handshake();
                    wait();
                    output_store_req_valid = false;
                    wait();
                    this->output_store_done_handshake();
                    wait();
                    this->process_done();
                }
            }
        }
    } // while (true)
} // Function : output_asi_kernel 