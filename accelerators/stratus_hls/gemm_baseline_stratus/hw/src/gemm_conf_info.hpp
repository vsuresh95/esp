// Copyright (c) 2011-2022 Columbia University, System Level Design Group
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
        this->dim_m = 0;
        this->dim_n = 0;
        this->dim_k = 0;
        this->weight_base = 0;
        this->input_base = 0;
        this->output_base = 0;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        uint32_t dim_m,
        uint32_t dim_n,
        uint32_t dim_k,
        uint32_t weight_base,
        uint32_t input_base,
        uint32_t output_base
        )
    {
        /* <<--ctor-custom-->> */
        this->dim_m = dim_m;
        this->dim_n = dim_n;
        this->dim_k = dim_k;
        this->weight_base = weight_base;
        this->input_base = input_base;
        this->output_base = output_base;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (dim_m != rhs.dim_m) return false;
        if (dim_n != rhs.dim_n) return false;
        if (dim_k != rhs.dim_k) return false;
        if (weight_base != rhs.weight_base) return false;
        if (input_base != rhs.input_base) return false;
        if (output_base != rhs.output_base) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        dim_m = other.dim_m;
        dim_n = other.dim_n;
        dim_k = other.dim_k;
        weight_base = other.weight_base;
        input_base = other.input_base;
        output_base = other.output_base;
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
        os << "}";
        return os;
    }

    /* <<--params-->> */
    uint32_t dim_m;
    uint32_t dim_n;
    uint32_t dim_k;
    uint32_t weight_base;
    uint32_t input_base;
    uint32_t output_base;
};

#endif // __GEMM_CONF_INFO_HPP__
