// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __NERF_MLP_CONF_INFO_HPP__
#define __NERF_MLP_CONF_INFO_HPP__

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
        this->batch_size = 1;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t batch_size
        )
    {
        /* <<--ctor-custom-->> */
        this->batch_size = batch_size;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (batch_size != rhs.batch_size) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        batch_size = other.batch_size;
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
        os << "batch_size = " << conf_info.batch_size << "";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t batch_size;
};

#endif // __NERF_MLP_CONF_INFO_HPP__
