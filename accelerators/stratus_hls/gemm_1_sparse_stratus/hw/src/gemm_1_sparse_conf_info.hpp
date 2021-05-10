// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __GEMM_1_SPARSE_CONF_INFO_HPP__
#define __GEMM_1_SPARSE_CONF_INFO_HPP__

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
        this->h_order_size = 64;
        this->gemm_m = 64;
        this->gemm_n = 64;
        this->gemm_k = 64;
        this->gemm_batch = 1;
        this->var_size = 128;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t h_order_size, 
        int32_t gemm_m, 
        int32_t gemm_n, 
        int32_t gemm_k, 
        int32_t gemm_batch, 
        int32_t var_size
        )
    {
        /* <<--ctor-custom-->> */
        this->h_order_size = h_order_size;
        this->gemm_m = gemm_m;
        this->gemm_n = gemm_n;
        this->gemm_k = gemm_k;
        this->gemm_batch = gemm_batch;
        this->var_size = var_size;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (h_order_size != rhs.h_order_size) return false;
        if (gemm_m != rhs.gemm_m) return false;
        if (gemm_n != rhs.gemm_n) return false;
        if (gemm_k != rhs.gemm_k) return false;
        if (gemm_batch != rhs.gemm_batch) return false;
        if (var_size != rhs.var_size) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        h_order_size = other.h_order_size;
        gemm_m = other.gemm_m;
        gemm_n = other.gemm_n;
        gemm_k = other.gemm_k;
        gemm_batch = other.gemm_batch;
        var_size = other.var_size;
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
        os << "h_order_size = " << conf_info.h_order_size << ", ";
        os << "gemm_m = " << conf_info.gemm_m << ", ";
        os << "gemm_n = " << conf_info.gemm_n << ", ";
        os << "gemm_k = " << conf_info.gemm_k << ", ";
        os << "gemm_batch = " << conf_info.gemm_batch << ", ";
        os << "var_size = " << conf_info.var_size << "";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t h_order_size;
        int32_t gemm_m;
        int32_t gemm_n;
        int32_t gemm_k;
        int32_t gemm_batch;
        int32_t var_size;
};

#endif // __GEMM_1_SPARSE_CONF_INFO_HPP__
