// Copyright (c) 2011-2019 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __FFT_HPP__
#define __FFT_HPP__

#include "fft_data.hpp"
#include "fpdata.hpp"
#include "fft_conf_info.hpp"
#include "fft_debug_info.hpp"

#include "esp_templates.hpp"

#include "fft_directives.hpp"

#define __round_mask(x, y) ((y)-1)
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
/* <<--defines-->> */
#define LOG_LEN_MAX 15
#define LEN_MAX (1 << LOG_LEN_MAX)
#define DATA_WIDTH FX_WIDTH
#if (FX_WIDTH == 64)
#define DMA_SIZE SIZE_DWORD
#elif (FX_WIDTH == 32)
#define DMA_SIZE SIZE_WORD
#endif // FX_WIDTH
#define PLM_IN_WORD (LEN_MAX << 1)

class fft : public esp_accelerator_3P<DMA_WIDTH>
{
public:
    // Constructor
    SC_HAS_PROCESS(fft);
    fft(const sc_module_name& name)
    : esp_accelerator_3P<DMA_WIDTH>(name)
        , cfg("config")
    {
        // Signal binding
        cfg.bind_with(*this);

        // Map arrays to memories
        /* <<--plm-bind-->> */
        HLS_MAP_plm(A0, PLM_IN_NAME);
    }

    // Processes

    // Load the input data
    void load_input();

    // Computation
    void compute_kernel();

    // Store the output data
    void store_output();

    // Configure fft
    esp_config_proc cfg;

    // Functions
    void fft_bit_reverse(unsigned int n, unsigned int bits);

    // Private local memories
    FPDATA_WORD A0[PLM_IN_WORD];

};

class CompNum { // a complex number
public:
    FPDATA re;   // the real part
    FPDATA im;   // the imaginary part

    CompNum() {
        re = 0;
        im = 0;
    }

    CompNum(FPDATA real, FPDATA imaginary) {
        re = real;
        im = imaginary;
    }
};

#endif /* __FFT_HPP__ */
