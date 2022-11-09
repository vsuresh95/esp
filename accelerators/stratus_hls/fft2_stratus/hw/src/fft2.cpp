// Copyright (c) 2011-2019 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "fft2.hpp"
#include "fft2_directives.hpp"

// Functions

#include "fft2_functions.hpp"

// Processes

void fft2::load_input()
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
                dma_info_t dma_info(0, SYNC_VAR_SIZE / DMA_WORD_PER_BEAT, DMA_SIZE);
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
                    dataBv = this->dma_read_chnl.get();
                    wait();
                    last_task = dataBv.range(DATA_WIDTH - 1, 0).to_int64();
                }
            }
            break;
            case CFG_REQ:
            {
                dma_info_t dma_info(SYNC_VAR_SIZE / DMA_WORD_PER_BEAT, 1, DMA_SIZE);
                sc_dt::sc_bv<DMA_WIDTH> dataBv;

                wait();

                this->dma_read_ctrl.put(dma_info);

                dataBv = this->dma_read_chnl.get();
                wait();
                int64_t opts = dataBv.range(DMA_WIDTH - 1, 0).to_int64();
                spandex_rd_opts.dcs_en          = (opts >>  0) & 0x01;
                spandex_rd_opts.use_owner_pred  = (opts >>  1) & 0x01;
                spandex_rd_opts.dcs             = (opts >>  2) & 0x03;
                spandex_rd_opts.pred_cid        = (opts >>  4) & 0x0F;
                spandex_wr_opts.dcs_en          = (opts >>  8) & 0x01;
                spandex_wr_opts.use_owner_pred  = (opts >>  9) & 0x01;
                spandex_wr_opts.dcs             = (opts >> 10) & 0x03;
                spandex_wr_opts.pred_cid        = (opts >> 12) & 0x0F;
            }
            break;
            case POLL_NEXT_REQ:
            {
                int32_t end_sync_offset = SYNC_VAR_SIZE + SPANDEX_CONFIG_VAR_SIZE + 2 * num_samples;
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
            {
                dma_info_t dma_info((SYNC_VAR_SIZE + SPANDEX_CONFIG_VAR_SIZE) / DMA_WORD_PER_BEAT, 
                                    2 * num_samples / DMA_WORD_PER_BEAT, 
                                    DMA_SIZE,
                                    spandex_rd_opts);
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

void fft2::store_output()
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
                int32_t end_sync_offset = SYNC_VAR_SIZE + SPANDEX_CONFIG_VAR_SIZE + 2 * num_samples;
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
                int32_t end_sync_offset = (2 * (SYNC_VAR_SIZE + SPANDEX_CONFIG_VAR_SIZE)) + (2 * num_samples);
                dma_info_t dma_info(end_sync_offset / DMA_WORD_PER_BEAT, 
                                    2 * num_samples / DMA_WORD_PER_BEAT, 
                                    DMA_SIZE,
                                    spandex_wr_opts);
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

void fft2::compute_kernel()
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
    int32_t do_inverse;
    int32_t do_shift;
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

        // Load config request
        {
            HLS_PROTO("cfg-req");

            load_state_req = CFG_REQ;

            compute_state_req_dbg.write(2);

            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();
        }

        // Load input data
        {
            HLS_PROTO("load-input-data");

            load_state_req = LOAD_DATA_REQ;

            compute_state_req_dbg.write(3);

            this->compute_load_ready_handshake();
            wait();
            this->compute_load_done_handshake();
            wait();

            compute_state_req_dbg.write(4);
        }

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

        // // Poll lock for consumer's ready
        // {
        //     HLS_PROTO("poll-for-cons-ready");

        //     load_state_req = POLL_NEXT_REQ;

        //     compute_state_req_dbg.write(4);

        //     this->compute_load_ready_handshake();
        //     wait();
        //     this->compute_load_done_handshake();
        //     wait();
        // }

        // Store output data
        {
            HLS_PROTO("store-output-data");

            store_state_req = STORE_DATA_REQ;

            this->compute_store_ready_handshake();

            compute_state_req_dbg.write(5);

            wait();
            this->compute_store_done_handshake();
            wait();

            // Wait for all writes to be done and then issue fence
            store_state_req = STORE_FENCE;

            compute_state_req_dbg.write(6);

            this->compute_store_ready_handshake();
            wait();
            this->compute_store_done_handshake();
            wait();
        }

        // update consumer for data ready
        {
            HLS_PROTO("update-next-lock");

            store_state_req = UPDATE_NEXT_REQ;

            compute_state_req_dbg.write(7);

            this->compute_store_ready_handshake();
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

        // update producer for ready to accept
        {
            HLS_PROTO("update-prev-lock");

            store_state_req = UPDATE_PREV_REQ;

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

        // End operation
        {
            HLS_PROTO("end-acc");

            if (last_task == 1)
            {
                store_state_req = ACC_DONE;

                compute_state_req_dbg.write(11);

                this->compute_store_ready_handshake();
                wait();
                this->compute_store_done_handshake();
                wait();
                this->process_done();
            }
        }
    } // while (true)
} // Function : compute_kernel
