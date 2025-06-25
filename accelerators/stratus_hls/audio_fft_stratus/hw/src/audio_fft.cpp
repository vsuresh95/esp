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

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    int32_t input_payload_offset;
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
            HLS_FLATTEN_ARRAY(config.input_queue_base);

            // User-defined config code
            /* <<--local-params-->> */
            logn_samples = config.logn_samples[current_context_int];
            num_samples = 1 << logn_samples;

            // Configured shared memory base addresses for input queue
            input_payload_offset = config.input_queue_base[current_context_int] + PAYLOAD_OFFSET;

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

        wait();

        this->load_avu_done_handshake();
    }
} // Function : load_input

void audio_fft::store_output()
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
    int32_t num_samples;;
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
            output_payload_offset = config.output_queue_base[current_context_int] + PAYLOAD_OFFSET;

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

void audio_fft::compute_kernel()
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
    int32_t do_inverse;
    int32_t do_shift;
    {
        HLS_PROTO("compute-config");

        cfg.wait_for_config(); // config process
        
        wait();
    }

    // Store
    while(true)
    {
        // Read config information for current context
        {
            HLS_PROTO("read-compute-config");

            this->compute_avu_ready_handshake();

            wait();

            conf_info_t config = this->conf_info.read();        
            HLS_FLATTEN_ARRAY(config.logn_samples);
            HLS_FLATTEN_ARRAY(config.do_shift);
            HLS_FLATTEN_ARRAY(config.do_inverse);
            
            logn_samples = config.logn_samples[current_context_int];
            num_samples = 1 << logn_samples;
            do_inverse = config.do_inverse[current_context_int];
            do_shift = config.do_shift[current_context_int];

            wait();
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

        {
            HLS_PROTO("compute-done");

            this->compute_avu_done_handshake();

            wait();
        }


    }
}

#else // ENABLE_PP

#include "audio_fft_pipelined.cpp"

#endif // ENABLE_PP
