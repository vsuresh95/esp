// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __GEMM_SM_INFO_HPP__
#define __GEMM_SM_INFO_HPP__

#include <systemc.h>
#include "gemm_conf_info.hpp"

//
// Configuration parameters for the accelerator.
//
class sm_info_t
{
public:
    //
    // constructors
    //
    sm_info_t()
    {
        /* <<--ctor-->> */
        this->dim_m = 1;
        this->dim_n = 1;
        this->dim_k = 1;
        this->weight_base = 1;
        this->input_base = 0;
        this->output_base = 0;
    }

    sm_info_t(
        /* <<--ctor-args-->> */
        int32_t dim_m,
        int32_t dim_n,
        int32_t dim_k,
        int32_t weight_base,
        int32_t input_base,
        int32_t output_base
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
    inline bool operator==(const sm_info_t &rhs) const
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
    inline sm_info_t& operator=(const sm_info_t& other)
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
    friend void sc_trace(sc_trace_file *tf, const sm_info_t &v, const std::string &NAME)
    {}

    // redirection operator
    friend ostream& operator << (ostream& os, sm_info_t const &sm_info)
    {
        os << "{";
        /* <<--print-->> */
        os << "}";
        return os;
    }

    /* <<--params-->> */
    int32_t dim_m;
    int32_t dim_n;
    int32_t dim_k;
    int32_t weight_base;
    int32_t input_base;
    int32_t output_base;
};

#endif // __GEMM_SM_INFO_HPP__
