// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __MAC_CONF_INFO_HPP__
#define __MAC_CONF_INFO_HPP__

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
        this->mac_n = 1;
        this->mac_vec = 100;
        this->mac_len = 64;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t mac_n, 
        int32_t mac_vec, 
        int32_t mac_len
        )
    {
        /* <<--ctor-custom-->> */
        this->mac_n = mac_n;
        this->mac_vec = mac_vec;
        this->mac_len = mac_len;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (mac_n != rhs.mac_n) return false;
        if (mac_vec != rhs.mac_vec) return false;
        if (mac_len != rhs.mac_len) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        mac_n = other.mac_n;
        mac_vec = other.mac_vec;
        mac_len = other.mac_len;
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
        os << "mac_n = " << conf_info.mac_n << ", ";
        os << "mac_vec = " << conf_info.mac_vec << ", ";
        os << "mac_len = " << conf_info.mac_len << "";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t mac_n;
        int32_t mac_vec;
        int32_t mac_len;
};

#endif // __MAC_CONF_INFO_HPP__
