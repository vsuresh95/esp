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

        // explicit PLM ports reset if any

        // User-defined reset code

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    int32_t num_ffts;
    {
        HLS_PROTO("load-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        logn_samples = config.logn_samples;
        num_samples = 1 << logn_samples;
        num_ffts = config.num_ffts;
    }

    // Load
    {
        HLS_PROTO("load-dma");
        uint32_t offset = 0;

        wait();
#if (DMA_WORD_PER_BEAT == 0)
        uint32_t length = 2 * num_ffts * num_samples;
#else
        uint32_t length = round_up(2 * num_ffts * num_samples, DMA_WORD_PER_BEAT);
#endif
        // Configure DMA transaction
        uint32_t len = length;
#if (DMA_WORD_PER_BEAT == 0)
        // data word is wider than NoC links
        dma_info_t dma_info(offset * DMA_BEAT_PER_WORD, len * DMA_BEAT_PER_WORD, DMA_SIZE);
#else
        dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
#endif
        offset += len;

        this->dma_read_ctrl.put(dma_info);

#if (DMA_WORD_PER_BEAT == 0)
        // data word is wider than NoC links
        for (uint16_t i = 0; i < len; i++)
        {
            sc_dt::sc_bv<DATA_WIDTH> dataBv;

            for (uint16_t k = 0; k < DMA_BEAT_PER_WORD; k++)
            {
                dataBv.range((k+1) * DMA_WIDTH - 1, k * DMA_WIDTH) = this->dma_read_chnl.get();
                wait();
            }

            // Write to PLM
            A0[i] = dataBv.to_int64();
        }
#else
        for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
        {
            HLS_BREAK_DEP(A0);

            sc_dt::sc_bv<DMA_WIDTH> dataBv;

            dataBv = this->dma_read_chnl.get();
            wait();

            // Write to PLM (all DMA_WORD_PER_BEAT words in one cycle)
            for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
            {
                HLS_UNROLL_SIMPLE;
                A0[i + k] = dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH).to_int64();
            }
        }
#endif
        this->load_compute_handshake();
    } // Load scope

    // Conclude
    {
        this->process_done();
    }
}



void fft2::store_output()
{
    // Reset
    {
        HLS_PROTO("store-reset");

        this->reset_store_output();
        this->reset_store_to_load();

        // explicit PLM ports reset if any

        // User-defined reset code

        wait();
    }

    // Config
    /* <<--params-->> */
    int32_t logn_samples;
    int32_t num_samples;
    int32_t num_ffts;
    {
        HLS_PROTO("store-config");

        cfg.wait_for_config(); // config process
        conf_info_t config = this->conf_info.read();

        // User-defined config code
        /* <<--local-params-->> */
        logn_samples = config.logn_samples;
        num_samples = 1 << logn_samples;
        num_ffts = config.num_ffts;
    }

    // Store
    {
        HLS_PROTO("store-dma");
        wait();

        uint32_t offset = 0;

#if (DMA_WORD_PER_BEAT == 0)
        uint32_t length = 2 * num_ffts * num_samples;
#else
        uint32_t length = round_up(2 * num_ffts * num_samples, DMA_WORD_PER_BEAT);
#endif
        this->store_compute_handshake();

        // Configure DMA transaction
        uint32_t len = length;
#if (DMA_WORD_PER_BEAT == 0)
        // data word is wider than NoC links
        dma_info_t dma_info(offset * DMA_BEAT_PER_WORD, len * DMA_BEAT_PER_WORD, DMA_SIZE);
#else
        dma_info_t dma_info(offset / DMA_WORD_PER_BEAT, len / DMA_WORD_PER_BEAT, DMA_SIZE);
#endif
        offset += len;

        this->dma_write_ctrl.put(dma_info);

#if (DMA_WORD_PER_BEAT == 0)
        // data word is wider than NoC links
        for (uint16_t i = 0; i < len; i++)
        {
            // Read from PLM
            sc_dt::sc_int<DATA_WIDTH> data;
            wait();
            data = A0[i];
            sc_dt::sc_bv<DATA_WIDTH> dataBv(data);

            uint16_t k = 0;
            for (k = 0; k < DMA_BEAT_PER_WORD - 1; k++)
            {
                this->dma_write_chnl.put(dataBv.range((k+1) * DMA_WIDTH - 1, k * DMA_WIDTH));
                wait();
            }
            // Last beat on the bus does not require wait(), which is
            // placed before accessing the PLM
            this->dma_write_chnl.put(dataBv.range((k+1) * DMA_WIDTH - 1, k * DMA_WIDTH));
        }
#else
        for (uint16_t i = 0; i < len; i += DMA_WORD_PER_BEAT)
        {
            sc_dt::sc_bv<DMA_WIDTH> dataBv;

            // Read from PLM
            wait();
            for (uint16_t k = 0; k < DMA_WORD_PER_BEAT; k++)
            {
                HLS_UNROLL_SIMPLE;
                dataBv.range((k+1) * DATA_WIDTH - 1, k * DATA_WIDTH) = A0[i + k];
            }
            this->dma_write_chnl.put(dataBv);
        }
#endif
    } // Store scope

    // Conclude
    {
        this->accelerator_done();
        this->process_done();
    }
}


void fft2::compute_kernel()
{
    // Reset
    {
        HLS_PROTO("compute-reset");

        this->reset_compute_kernel();

        // explicit PLM ports reset if any

        // User-defined reset code

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
#ifndef STRATUS_HLS
        sc_assert(logn_samples < MAX_LOGN_SAMPLES);
#endif
        num_samples = 1 << logn_samples;
        do_inverse = config.do_inverse;
        do_shift = config.do_shift;
    }

    // Compute
    {
        // Compute FFT(s) in the PLM
        this->compute_load_handshake();

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

        this->compute_store_handshake();
    } // Compute scope

    // Conclude
    {
        this->process_done();
    }

} // Function : compute_kernel
