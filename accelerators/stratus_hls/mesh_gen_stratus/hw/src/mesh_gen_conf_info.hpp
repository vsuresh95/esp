// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __MESH_GEN_CONF_INFO_HPP__
#define __MESH_GEN_CONF_INFO_HPP__

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
        this->num_hash_table = 1;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t num_hash_table
        )
    {
        /* <<--ctor-custom-->> */
        this->num_hash_table = num_hash_table;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (num_hash_table != rhs.num_hash_table) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        num_hash_table = other.num_hash_table;
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
        os << "num_hash_table = " << conf_info.num_hash_table << "";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t num_hash_table;
};

#endif // __MESH_GEN_CONF_INFO_HPP__
