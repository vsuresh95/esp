// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __AUDIO_FFI_CONF_INFO_HPP__
#define __AUDIO_FFI_CONF_INFO_HPP__

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
        this->logn_samples = 11;
        this->do_shift = 1;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t do_inverse, 
        int32_t logn_samples, 
        int32_t do_shift
        )
    {
        /* <<--ctor-custom-->> */
        this->do_inverse = do_inverse;
        this->logn_samples = logn_samples;
        this->do_shift = do_shift;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (do_inverse != rhs.do_inverse) return false;
        if (logn_samples != rhs.logn_samples) return false;
        if (do_shift != rhs.do_shift) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        do_inverse = other.do_inverse;
        logn_samples = other.logn_samples;
        do_shift = other.do_shift;
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
        os << "do_shift = " << conf_info.do_shift << "";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t do_inverse;
        int32_t logn_samples;
        int32_t do_shift;
};

#endif // __AUDIO_FFI_CONF_INFO_HPP__
