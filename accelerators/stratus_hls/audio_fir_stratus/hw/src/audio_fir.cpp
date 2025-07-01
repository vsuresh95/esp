// Copyright (c) 2011-2019 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef ENABLE_PP

#include "audio_fir.hpp"
#include "audio_fir_directives.hpp"

// Functions

#include "audio_fir_functions.hpp"

// Processes

void audio_fir::load_input()
{
    // Reset
    {
        HLS_PROTO("load-reset");

        this->reset_load_input();

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
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
        HLS_PROTO("load-loop");

        wait();

        this->load_avu_ready_handshake();

        // Read config information for current context
        {
            HLS_PROTO("read-load-config");

            conf_info_t config = this->conf_info.read();        
            HLS_FLATTEN_ARRAY(config.logn_samples);
            HLS_FLATTEN_ARRAY(config.input_queue_base[current_context_int]);

            // User-defined config code
            /* <<--local-params-->> */
            logn_samples = config.logn_samples[current_context_int];
            num_samples = 1 << logn_samples;

            // Configured shared memory base addresses for input queues
            input_payload_offset = config.input_queue_base[current_context_int][0] + PAYLOAD_OFFSET;
            filter_payload_offset = config.input_queue_base[current_context_int][1] + PAYLOAD_OFFSET;
            twiddle_payload_offset = filter_payload_offset + 2 * (num_samples + 1);

            wait();
        }

        // Load input data
        {
            HLS_PROTO("load-data");

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
            HLS_PROTO("load-filters");

            dma_info_t dma_info(filter_payload_offset / DMA_WORD_PER_BEAT, 2 * (num_samples + 1) / DMA_WORD_PER_BEAT, DMA_SIZE);
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
            HLS_PROTO("load-twiddle");

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

        wait();

        this->load_avu_done_handshake();
    }
} // Function : load_input

void audio_fir::store_output()
{
    // Reset
    {
        HLS_PROTO("store-reset");

        this->reset_store_output();

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    int32_t output_payload_offset;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process

        wait();
    }

    // Store
    while(true)
    {
        HLS_PROTO("store-loop");

        wait();

        this->store_avu_ready_handshake();

        // Read config information for current context
        {
            HLS_PROTO("read-store-config");

            conf_info_t config = this->conf_info.read();        
            HLS_FLATTEN_ARRAY(config.logn_samples);
            HLS_FLATTEN_ARRAY(config.output_queue_base);

            // User-defined config code
            /* <<--local-params-->> */
            logn_samples = config.logn_samples[current_context_int];
            num_samples = 1 << logn_samples;

            // Configured shared memory base addresses for output queue
            output_payload_offset = config.output_queue_base[current_context_int][0] + PAYLOAD_OFFSET;

            wait();
        }

        {
            HLS_PROTO("store-data");

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

        wait();

        this->store_avu_done_handshake();
    }
} // Function : store_output

void audio_fir::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-reset");

        this->reset_compute_kernel();

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process

        wait();
    }

    // Compute
    while(true)
    {
        // Read config information for current context
        {
            HLS_PROTO("read-compute-config");

            this->compute_avu_ready_handshake();

            wait();

            conf_info_t config = this->conf_info.read();        
            HLS_FLATTEN_ARRAY(config.logn_samples);
            
            logn_samples = config.logn_samples[current_context_int];
            num_samples = 1 << logn_samples;

            wait();
        }

        // Compute FIR
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
        } // Compute

        {
            HLS_PROTO("compute-done");

            this->compute_avu_done_handshake();

            wait();
        }
    } // while (true)
} // Function : compute_kernel

#else // ENABLE_PP

#include "audio_fir_pipelined.cpp"

#endif // ENABLE_PP
