// Copyright (c) 2011-2019 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "fir.hpp"
#include "fir_directives.hpp"

// Functions

#include "fir_functions.hpp"

// Processes

void fir::load_input()
{
    // Reset
    {
        HLS_PROTO("load-reset");

        this->reset_load_input();
        this->reset_load_to_store();

        load_state_req_dbg.write(0);

        load_ready.ack.reset_ack();
        load_done.req.reset_req();

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        logn_samples = config.logn_samples;
        num_samples = 1 << logn_samples;
    }

    // Load
    while(true)
    {
        HLS_PROTO("load-dma");

        wait();

        this->load_compute_ready_handshake();

        load_state_req_dbg.write(load_state_req);

        switch (load_state_req)
        {
            case POLL_PREV_REQ:
            {
                dma_info_t dma_info(0, 1, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                int32_t new_task = 0;

                wait();

                // Wait for producer to send new data
                while (new_task != 1)
                {
                    HLS_UNROLL_LOOP(OFF);
                    this->dma_read_ctrl.put(dma_info);
                    dataBv = this->dma_read_chnl.get();
                    wait();
                    new_task = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
                }
            }
            break;
            case POLL_NEXT_REQ:
            {
                int32_t end_sync_offset = SYNC_VAR_SIZE + 2 * num_samples;
                dma_info_t dma_info(end_sync_offset / DMA_WORD_PER_BEAT, 1, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;
                int32_t new_task = 1;

                wait();

                // Wait for consumer to accept new data
                while (new_task == 1)
                {
                    HLS_UNROLL_LOOP(OFF);
                    this->dma_read_ctrl.put(dma_info);
                    dataBv = this->dma_read_chnl.get();
                    wait();
                    new_task = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
                }
            }
            break;
            case LOAD_DATA_REQ:
            // Load input data
            {
                dma_info_t dma_info(SYNC_VAR_SIZE / DMA_WORD_PER_BEAT, 2 * num_samples / DMA_WORD_PER_BEAT, DMA_SIZE);
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
                dma_info_t dma_info(4 * 2 * num_samples / DMA_WORD_PER_BEAT, 2 * num_samples / DMA_WORD_PER_BEAT, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                this->dma_read_ctrl.put(dma_info);

                for (int i = 0; i < 2 * num_samples; i += DMA_WORD_PER_BEAT)
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
                dma_info_t dma_info(5 * 2 * num_samples / DMA_WORD_PER_BEAT, num_samples / DMA_WORD_PER_BEAT, DMA_SIZE);
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

void fir::store_output()
{
    // Reset
    {
        HLS_PROTO("store-reset");

        this->reset_store_output();
        this->reset_store_to_load();

        store_state_req_dbg.write(0);

        store_ready.ack.reset_ack();
        store_done.req.reset_req();

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process

        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        logn_samples = config.logn_samples;
        num_samples = 1 << logn_samples;
    }

    // Store
    while(true)
    {
        HLS_PROTO("store-dma");

        wait();

        this->store_compute_ready_handshake();

        store_state_req_dbg.write(store_state_req);

        switch (store_state_req)
        {
            case UPDATE_PREV_REQ:
            {
                dma_info_t dma_info(0, 1, DMA_SIZE);
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
            case UPDATE_NEXT_REQ:
            {
                int32_t end_sync_offset = SYNC_VAR_SIZE + 2 * num_samples;
                dma_info_t dma_info(end_sync_offset / DMA_WORD_PER_BEAT, 1, DMA_SIZE);
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
            case STORE_DATA_REQ:
            {
                int32_t end_sync_offset = (2 * SYNC_VAR_SIZE) + (2 * num_samples);
                dma_info_t dma_info(end_sync_offset / DMA_WORD_PER_BEAT, 2 * num_samples / DMA_WORD_PER_BEAT, DMA_SIZE);
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

void fir::compute_kernel()
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

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        logn_samples = config.logn_samples;
        num_samples = 1 << logn_samples;
    }

    while(true)
    {
        // Poll lock for new task
        {
            HLS_PROTO("poll-for-new-task");

            load_state_req = POLL_PREV_REQ;

            compute_state_req_dbg.write(1);

            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();
        }

        // Load input data
        {
            HLS_PROTO("load-input-data");

            load_state_req = LOAD_DATA_REQ;

            compute_state_req_dbg.write(2);

            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();
        }

        // Post-process FFT output
        {
            HLS_PROTO("post-process-fft");

            compute_state_req_dbg.write(3);

            this->compute_post_proc_ready_handshake();
            wait();
            this->compute_post_proc_done_handshake();
            wait();
        }

        // Compute
        {
            compute_state_req_dbg.write(4);

            for (unsigned k = 0; k < 2 * num_samples; k+=2) {

                CompNum akj, akjm;
                CompNum bkj, bkjm;

                akj.re = int2fp<FPDATA, WORD_SIZE>(A0[k]);
                akj.im = int2fp<FPDATA, WORD_SIZE>(A0[k + 1]);
                akjm.re = int2fp<FPDATA, WORD_SIZE>(F0[k]);
                akjm.im = int2fp<FPDATA, WORD_SIZE>(F0[k + 1]);

                CompNum t;
                compMul(akj, akjm, t);

                {
                    HLS_PROTO("compute_write_A0");
                    HLS_BREAK_DEP(A0);
                    wait();
                    A0[k] = fp2int<FPDATA, WORD_SIZE>(t.re);
                    A0[k + 1] = fp2int<FPDATA, WORD_SIZE>(t.im);
                }
            } // for (k = 0 .. num_samples)
        } // Compute

        // Pre-process IFFT output
        {
            HLS_PROTO("pre-process-ifft");

            compute_state_req_dbg.write(5);

            this->compute_pre_proc_ready_handshake();
            wait();
            this->compute_pre_proc_done_handshake();
            wait();
        }

        // Poll lock for consumer's ready
        {
            HLS_PROTO("poll-for-cons-ready");

            load_state_req = POLL_NEXT_REQ;

            compute_state_req_dbg.write(6);

            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();
        }

        // Store output data
        {
            HLS_PROTO("store-output-data");

            store_state_req = STORE_DATA_REQ;

            this->compute_store_ready_handshake();

            compute_state_req_dbg.write(7);

            wait();
            this->compute_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_state_req_dbg.write(8);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();
        }

        // update consumer for data ready
        {
            HLS_PROTO("update-next-lock");

            store_state_req = UPDATE_NEXT_REQ;

            compute_state_req_dbg.write(9);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_state_req_dbg.write(10);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();
        }

        // update producer for ready to accept
        {
            HLS_PROTO("update-prev-lock");

            store_state_req = UPDATE_PREV_REQ;

            compute_state_req_dbg.write(11);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_state_req_dbg.write(12);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();
        }

        // End operation
        {
            HLS_PROTO("end-acc");

            store_state_req = ACC_DONE;

            compute_state_req_dbg.write(13);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();
            this->process_done();
        }
    } // while (true)
} // Function : compute_kernel

void fir::fft_post_proc()
{
    // Reset
    {
        HLS_PROTO("fft_post_proc-reset");

        post_proc_done.req.reset_req();
        post_proc_ready.ack.reset_ack();

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    {
        HLS_PROTO("fft_post_proc-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        logn_samples = config.logn_samples;
        num_samples = 1 << logn_samples;
    }

    while(true)
    {
        CompNum fpnk, fpk, f1k, f2k, tw, tdc;
        CompNum tf, f0, fn;

        {
            HLS_PROTO("wait-for-post-proc-ready");
            wait();
            this->post_proc_compute_ready_handshake();
        }

        // First and last element
        {
            tdc.re = int2fp<FPDATA, WORD_SIZE>(A0[0]);
            tdc.im = int2fp<FPDATA, WORD_SIZE>(A0[1]);

            tdc.re /= 2; tdc.im /= 2;

            f0.re = tdc.re + tdc.im;
            f0.im = 0; // not in original code
            fn.re = tdc.re - tdc.im;
            fn.im = 0;

            // Write back first and last element to memory
            {
                HLS_PROTO("write-back-post-proc-0");
                HLS_BREAK_DEP(A0);
                wait();
                A0[0] = fp2int<FPDATA, WORD_SIZE>(f0.re);
                A0[1] = fp2int<FPDATA, WORD_SIZE>(f0.im);
                wait();
                A0[(2 * num_samples)] = fp2int<FPDATA, WORD_SIZE>(fn.re);
                A0[(2 * num_samples) + 1] = fp2int<FPDATA, WORD_SIZE>(fn.im);
            }
        }

        // Remaining elements
        for (unsigned k = 2; k < num_samples; k+=2)
        {
            // Read FFT output
            fpk.re = int2fp<FPDATA, WORD_SIZE>(A0[k]);
            fpk.im = int2fp<FPDATA, WORD_SIZE>(A0[k + 1]);
            fpnk.re = int2fp<FPDATA, WORD_SIZE>(A0[(2 * num_samples) - k]);
            fpnk.im = - (int2fp<FPDATA, WORD_SIZE>(A0[(2 * num_samples) - k + 1]));

            // Read twiddle factors
            tf.re = int2fp<FPDATA, WORD_SIZE>(T0[k/2 - 1]);
            tf.im = int2fp<FPDATA, WORD_SIZE>(T0[k/2]);

            fpk.re /= 2; fpk.im /= 2;
            fpnk.re /= 2; fpnk.im /= 2;

            compAdd(fpk, fpnk, f1k);
            compSub(fpk, fpnk, f2k);
            compMul(f2k, tf, tw);

            // Computing freqdata's
            compAdd(f1k, tw, f0);
            f0.re /= 2; f0.im /= 2;

            compSub(f1k, tw, fn);
            fn.re /= 2; fn.im /= 2;

            {
                HLS_PROTO("write-back-post-proc-k");
                HLS_BREAK_DEP(A0);
                wait();
                A0[k] = fp2int<FPDATA, WORD_SIZE>(f0.re);
                A0[k + 1] = fp2int<FPDATA, WORD_SIZE>(f0.im);
                wait();
                A0[(2 * num_samples) - k] = fp2int<FPDATA, WORD_SIZE>(fn.re);
                A0[(2 * num_samples) - k + 1] = - (fp2int<FPDATA, WORD_SIZE>(fn.im));
            }
        } // for (k = 2 .. num_samples)

        {
            HLS_PROTO("send-for-post-proc-done");
            wait();
            this->post_proc_compute_done_handshake();
        }
    } // while (true)
} // Function : fft_post_proc

void fir::ifft_pre_proc()
{
    // Reset
    {
        HLS_PROTO("ifft_pre_proc-reset");

        pre_proc_done.req.reset_req();
        pre_proc_ready.ack.reset_ack();

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    {
        HLS_PROTO("ifft_pre_proc-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        logn_samples = config.logn_samples;
        num_samples = 1 << logn_samples;
    }

    while(true)
    {
        CompNum fk, fnkc, fek, fok, tmp;
        CompNum tmpbuf0, tmpbufn;
        CompNum tf, f0, fn, t0;

        {
            HLS_PROTO("wait-for-post-proc-ready");
            wait();
            this->pre_proc_compute_ready_handshake();
        }

        // First and last element
        {
            f0.re = int2fp<FPDATA, WORD_SIZE>(A0[0]);
            f0.im = int2fp<FPDATA, WORD_SIZE>(A0[1]);
            fn.re = int2fp<FPDATA, WORD_SIZE>(A0[(2 * num_samples)]);
            fn.im = int2fp<FPDATA, WORD_SIZE>(A0[(2 * num_samples) + 1]);

            t0.re = f0.re + fn.re;
            t0.im = f0.re - fn.re;

            t0.re /= 2; t0.im /= 2;

            // Write back first and last element to memory
            {
                HLS_PROTO("write-back-pre-proc-0");
                HLS_BREAK_DEP(A0);
                wait();
                A0[0] = fp2int<FPDATA, WORD_SIZE>(t0.re);
                A0[1] = fp2int<FPDATA, WORD_SIZE>(t0.im);
            }
        }

        // Remaining elements
        for (unsigned k = 2; k < num_samples; k+=2)
        {
            // Read FIR output
            fk.re = int2fp<FPDATA, WORD_SIZE>(A0[k]);
            fk.im = int2fp<FPDATA, WORD_SIZE>(A0[k + 1]);
            fnkc.re = int2fp<FPDATA, WORD_SIZE>(A0[(2 * num_samples) - k]);
            fnkc.im = - (int2fp<FPDATA, WORD_SIZE>(A0[(2 * num_samples) - k + 1]));

            // Read twiddle factors
            tf.re = int2fp<FPDATA, WORD_SIZE>(T0[k/2 - 1]);
            tf.im = int2fp<FPDATA, WORD_SIZE>(T0[k/2]);

            fk.re /= 2; fk.im /= 2;
            fnkc.re /= 2; fnkc.im /= 2;

            compAdd(fk, fnkc, fek);
            compSub(fk, fnkc, tmp);
            compMul(tmp, tf, fok);

            compAdd(fek, fok, tmpbuf0);
            compSub(fek, fok, tmpbufn);

            {
                HLS_PROTO("write-back-pre-proc-k");
                HLS_BREAK_DEP(A0);
                wait();
                A0[k] = fp2int<FPDATA, WORD_SIZE>(tmpbuf0.re);
                A0[k + 1] = fp2int<FPDATA, WORD_SIZE>(tmpbuf0.im);
                wait();
                A0[(2 * num_samples) - k] = fp2int<FPDATA, WORD_SIZE>(tmpbufn.re);
                A0[(2 * num_samples) - k + 1] = - (fp2int<FPDATA, WORD_SIZE>(tmpbufn.im));
            }
        } // for (k = 2 .. num_samples)

        {
            HLS_PROTO("send-for-post-proc-done");
            wait();
            this->pre_proc_compute_done_handshake();
        }
    } // while (true)
} // Function : ifft_pre_proc
