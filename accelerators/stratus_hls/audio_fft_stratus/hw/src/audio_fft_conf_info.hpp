// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __AUDIO_FFT_CONF_INFO_HPP__
#define __AUDIO_FFT_CONF_INFO_HPP__

#include <systemc.h>

//
// Configuration parameters for the accelerator.
//
class conf_info_t
{
public:

    //
    // constructors
    //
    conf_info_t()
    {
        /* <<--ctor-->> */
        this->do_inverse = 1;
        this->logn_samples = 1;
        this->do_shift = 1;
        this->prod_valid_offset = 0;
        this->prod_ready_offset = 0;
        this->cons_valid_offset = 0;
        this->cons_ready_offset = 0;
        this->input_offset = 0;
        this->output_offset = 0;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t do_inverse, 
        int32_t logn_samples, 
        int32_t do_shift,
        int32_t prod_valid_offset,
        int32_t prod_ready_offset,
        int32_t cons_valid_offset,
        int32_t cons_ready_offset,
        int32_t input_offset,
        int32_t output_offset
        )
    {
        /* <<--ctor-custom-->> */
        this->do_inverse = do_inverse;
        this->logn_samples = logn_samples;
        this->do_shift = do_shift;
        this->prod_valid_offset = prod_valid_offset;
        this->prod_ready_offset = prod_ready_offset;
        this->cons_valid_offset = cons_valid_offset;
        this->cons_ready_offset = cons_ready_offset;
        this->input_offset = input_offset;
        this->output_offset = output_offset;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (do_inverse != rhs.do_inverse) return false;
        if (logn_samples != rhs.logn_samples) return false;
        if (do_shift != rhs.do_shift) return false;
        if (prod_valid_offset != rhs.prod_valid_offset) return false;
        if (prod_ready_offset != rhs.prod_ready_offset) return false;
        if (cons_valid_offset != rhs.cons_valid_offset) return false;
        if (cons_ready_offset != rhs.cons_valid_offset) return false;
        if (input_offset != rhs.input_offset) return false;
        if (output_offset != rhs.output_offset) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        do_inverse = other.do_inverse;
        logn_samples = other.logn_samples;
        do_shift = other.do_shift;
        prod_valid_offset = other.prod_valid_offset;
        prod_ready_offset = other.prod_ready_offset;
        cons_valid_offset = other.cons_valid_offset;
        cons_ready_offset = other.cons_ready_offset;
        input_offset = other.input_offset;
        output_offset = other.output_offset;
        return *this;
    }

    // VCD dumping function
    friend void sc_trace(sc_trace_file *tf, const conf_info_t &v, const std::string &NAME)
    {}

    // redirection operator
    friend ostream& operator << (ostream& os, conf_info_t const &conf_info)
    {
        os << "{";
        /* <<--print-->> */
        os << "do_inverse = " << conf_info.do_inverse << ", ";
        os << "logn_samples = " << conf_info.logn_samples << ", ";
        os << "do_shift = " << conf_info.do_shift << ", ";
        os << "prod_valid_offset = " << conf_info.prod_valid_offset << ", ";
        os << "prod_ready_offset = " << conf_info.prod_ready_offset << ", ";
        os << "cons_valid_offset = " << conf_info.cons_valid_offset << ", ";
        os << "cons_ready_offset = " << conf_info.cons_ready_offset << ", ";
        os << "input_offset = " << conf_info.input_offset << ", ";
        os << "output_offset = " << conf_info.output_offset << "";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t do_inverse;
        int32_t logn_samples;
        int32_t do_shift;
        int32_t prod_valid_offset;
        int32_t prod_ready_offset;
        int32_t cons_valid_offset;
        int32_t cons_ready_offset;
        int32_t input_offset;
        int32_t output_offset;
};

#endif // __AUDIO_FFT_CONF_INFO_HPP__
