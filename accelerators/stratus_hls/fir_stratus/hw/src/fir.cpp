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
                dma_info_t dma_info(4 * (2 * num_samples + SYNC_VAR_SIZE) / DMA_WORD_PER_BEAT, 2 * (num_samples + 1)/ DMA_WORD_PER_BEAT, DMA_SIZE);
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
                dma_info_t dma_info(6 * (2 * num_samples + SYNC_VAR_SIZE) / DMA_WORD_PER_BEAT, num_samples / DMA_WORD_PER_BEAT, DMA_SIZE);
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

    // Load input data
    {
        HLS_PROTO("load-input-data");

        load_state_req = LOAD_DATA_REQ;

        compute_state_req_dbg.write(2);

        this->compute_load_ready_handshake();
        wait();
        this->compute_load_done_handshake();
        wait();

        compute_state_req_dbg.write(3);
    }

    // Compute
    {
        CompNum fpnk, fpk, f1k, f2k, tw, tdc;
        CompNum tf, if0, ifn, of0, flt0, fltn, t0, tn;
        CompNum fk, fnkc, fek, fok, tmp;
        CompNum tmpbuf0, tmpbufn;

        // First and last element
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

            // FIR
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

            // FIR
            compMul(if0, flt0, t0);
            compMul(ifn, fltn, tn);

            if (k == num_samples) {
                fk = tn;
            } else {
                fk = t0;
            }

            fnkc.re = tn.re;
            fnkc.im = - (tn.im);

            tf.re *= -1; tf.im *= -1;

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
    } // Compute

    // Store output data
    {
        HLS_PROTO("store-output-data");

        store_state_req = STORE_DATA_REQ;

        this->compute_store_ready_handshake();

        compute_state_req_dbg.write(5);

        wait();
        this->compute_store_done_handshake();
        wait();
    }

    // End operation
    {
        HLS_PROTO("end-acc");

        store_state_req = ACC_DONE;

        compute_state_req_dbg.write(11);

        this->compute_store_ready_handshake();
        wait();
        this->compute_store_done_handshake();
        wait();
        this->process_done();
    }
} // Function : compute_kernel
