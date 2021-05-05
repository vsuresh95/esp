// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __GEMM_BLOCK_CONF_INFO_HPP__
#define __GEMM_BLOCK_CONF_INFO_HPP__

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
        this->offset_c = 0;
        this->gemm_batch = 1;
        this->offset_b = 0;
        this->offset_a = 0;
        this->block_size = 16;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t gemm_m, 
        int32_t gemm_n, 
        int32_t gemm_k, 
        int32_t offset_c, 
        int32_t gemm_batch, 
        int32_t offset_b, 
        int32_t offset_a, 
        int32_t block_size
        )
    {
        /* <<--ctor-custom-->> */
        this->gemm_m = gemm_m;
        this->gemm_n = gemm_n;
        this->gemm_k = gemm_k;
        this->offset_c = offset_c;
        this->gemm_batch = gemm_batch;
        this->offset_b = offset_b;
        this->offset_a = offset_a;
        this->block_size = block_size;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (gemm_m != rhs.gemm_m) return false;
        if (gemm_n != rhs.gemm_n) return false;
        if (gemm_k != rhs.gemm_k) return false;
        if (offset_c != rhs.offset_c) return false;
        if (gemm_batch != rhs.gemm_batch) return false;
        if (offset_b != rhs.offset_b) return false;
        if (offset_a != rhs.offset_a) return false;
        if (block_size != rhs.block_size) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        gemm_m = other.gemm_m;
        gemm_n = other.gemm_n;
        gemm_k = other.gemm_k;
        offset_c = other.offset_c;
        gemm_batch = other.gemm_batch;
        offset_b = other.offset_b;
        offset_a = other.offset_a;
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
        os << "gemm_m = " << conf_info.gemm_m << ", ";
        os << "gemm_n = " << conf_info.gemm_n << ", ";
        os << "gemm_k = " << conf_info.gemm_k << ", ";
        os << "offset_c = " << conf_info.offset_c << ", ";
        os << "gemm_batch = " << conf_info.gemm_batch << ", ";
        os << "offset_b = " << conf_info.offset_b << ", ";
        os << "offset_a = " << conf_info.offset_a << ", ";
        os << "block_size = " << conf_info.block_size << "";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t gemm_m;
        int32_t gemm_n;
        int32_t gemm_k;
        int32_t offset_c;
        int32_t gemm_batch;
        int32_t offset_b;
        int32_t offset_a;
        int32_t block_size;
};

#endif // __GEMM_BLOCK_CONF_INFO_HPP__
