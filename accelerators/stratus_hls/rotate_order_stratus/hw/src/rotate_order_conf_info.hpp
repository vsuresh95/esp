// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __ROTATE_ORDER_CONF_INFO_HPP__
#define __ROTATE_ORDER_CONF_INFO_HPP__

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
        this->logn_samples = 1024;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t logn_samples
        )
    {
        /* <<--ctor-custom-->> */
        this->logn_samples = logn_samples;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (logn_samples != rhs.logn_samples) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        logn_samples = other.logn_samples;
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
        os << "logn_samples = " << conf_info.logn_samples << "";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t logn_samples;
};

#endif // __ROTATE_ORDER_CONF_INFO_HPP__
