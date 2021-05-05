// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __GEMM_CONF_INFO_HPP__
#define __GEMM_CONF_INFO_HPP__

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
        this->gemm_m = 64;
        this->gemm_n = 64;
        this->gemm_k = 64;
        this->gemm_batch = 1;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t gemm_m, 
        int32_t gemm_n, 
        int32_t gemm_k, 
        int32_t gemm_batch
        )
    {
        /* <<--ctor-custom-->> */
        this->gemm_m = gemm_m;
        this->gemm_n = gemm_n;
        this->gemm_k = gemm_k;
        this->gemm_batch = gemm_batch;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (gemm_m != rhs.gemm_m) return false;
        if (gemm_n != rhs.gemm_n) return false;
        if (gemm_k != rhs.gemm_k) return false;
        if (gemm_batch != rhs.gemm_batch) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        gemm_m = other.gemm_m;
        gemm_n = other.gemm_n;
        gemm_k = other.gemm_k;
        gemm_batch = other.gemm_batch;
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
        os << "gemm_m = " << conf_info.gemm_m << ", ";
        os << "gemm_n = " << conf_info.gemm_n << ", ";
        os << "gemm_k = " << conf_info.gemm_k << ", ";
        os << "gemm_batch = " << conf_info.gemm_batch << "";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t gemm_m;
        int32_t gemm_n;
        int32_t gemm_k;
        int32_t gemm_batch;
};

#endif // __GEMM_CONF_INFO_HPP__
