// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __ISCA_SYNTH_CONF_INFO_HPP__
#define __ISCA_SYNTH_CONF_INFO_HPP__

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
        this->compute_ratio = 1;
        this->size = 1;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t compute_ratio, 
        int32_t size
        )
    {
        /* <<--ctor-custom-->> */
        this->compute_ratio = compute_ratio;
        this->size = size;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (compute_ratio != rhs.compute_ratio) return false;
        if (size != rhs.size) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        compute_ratio = other.compute_ratio;
        size = other.size;
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
        os << "compute_ratio = " << conf_info.compute_ratio << ", ";
        os << "size = " << conf_info.size << "";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t compute_ratio;
        int32_t size;
};

#endif // __ISCA_SYNTH_CONF_INFO_HPP__
