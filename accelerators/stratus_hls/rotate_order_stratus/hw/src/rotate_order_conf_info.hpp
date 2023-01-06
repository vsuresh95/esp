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
        this->num_channel = 16;
        this->block_size = 1024;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t num_channel, 
        int32_t block_size
        )
    {
        /* <<--ctor-custom-->> */
        this->num_channel = num_channel;
        this->block_size = block_size;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (num_channel != rhs.num_channel) return false;
        if (block_size != rhs.block_size) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        num_channel = other.num_channel;
        block_size = other.block_size;
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
        os << "num_channel = " << conf_info.num_channel << ", ";
        os << "block_size = " << conf_info.block_size << "";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t num_channel;
        int32_t block_size;
};

#endif // __ROTATE_ORDER_CONF_INFO_HPP__
