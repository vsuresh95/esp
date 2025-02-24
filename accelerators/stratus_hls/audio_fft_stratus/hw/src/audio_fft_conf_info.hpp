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
        this->input_queue_base = 0;
        this->output_queue_base = 0;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t do_inverse, 
        int32_t logn_samples, 
        int32_t do_shift,
        int32_t input_queue_base,
        int32_t output_queue_base
        )
    {
        /* <<--ctor-custom-->> */
        this->do_inverse = do_inverse;
        this->logn_samples = logn_samples;
        this->do_shift = do_shift;
        this->input_queue_base = input_queue_base;
        this->output_queue_base = output_queue_base;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (do_inverse != rhs.do_inverse) return false;
        if (logn_samples != rhs.logn_samples) return false;
        if (do_shift != rhs.do_shift) return false;
        if (input_queue_base != rhs.input_queue_base) return false;
        if (output_queue_base != rhs.output_queue_base) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        do_inverse = other.do_inverse;
        logn_samples = other.logn_samples;
        do_shift = other.do_shift;
        input_queue_base = other.input_queue_base;
        output_queue_base = other.output_queue_base;
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
        os << "input_queue_base = " << conf_info.input_queue_base << ", ";
        os << "output_queue_base = " << conf_info.output_queue_base << ", ";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t do_inverse;
        int32_t logn_samples;
        int32_t do_shift;
        int32_t input_queue_base;
        int32_t output_queue_base;
};

#endif // __AUDIO_FFT_CONF_INFO_HPP__
