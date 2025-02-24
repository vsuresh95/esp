// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __AUDIO_FIR_CONF_INFO_HPP__
#define __AUDIO_FIR_CONF_INFO_HPP__

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
        this->logn_samples = 11;
        this->input_queue_base = 0;
        this->output_queue_base = 0;
        this->filter_queue_base = 0;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t logn_samples, 
        int32_t input_queue_base,
        int32_t output_queue_base,
        int32_t filter_queue_base
        )
    {
        /* <<--ctor-custom-->> */
        this->logn_samples = logn_samples;
        this->input_queue_base = input_queue_base;
        this->output_queue_base = output_queue_base;
        this->filter_queue_base = filter_queue_base;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (logn_samples != rhs.logn_samples) return false;
        if (input_queue_base != rhs.input_queue_base) return false;
        if (output_queue_base != rhs.output_queue_base) return false;
        if (filter_queue_base != rhs.filter_queue_base) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        logn_samples = other.logn_samples;
        input_queue_base = other.input_queue_base;
        output_queue_base = other.output_queue_base;
        filter_queue_base = other.filter_queue_base;
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
        os << "logn_samples = " << conf_info.logn_samples << ", ";
        os << "input_queue_base = " << conf_info.input_queue_base << ", ";
        os << "output_queue_base = " << conf_info.output_queue_base << ", ";
        os << "filter_queue_base = " << conf_info.filter_queue_base << ", ";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t logn_samples;
        int32_t input_queue_base;
        int32_t output_queue_base;
        int32_t filter_queue_base;
};

#endif // __AUDIO_FIR_CONF_INFO_HPP__
