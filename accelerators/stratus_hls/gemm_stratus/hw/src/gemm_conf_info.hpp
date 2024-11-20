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
        this->dim_m = 1;
        this->dim_n = 1;
        this->dim_k = 1;
        this->prod_valid_offset = 0;
        this->prod_ready_offset = 0;
        this->cons_valid_offset = 0;
        this->cons_ready_offset = 0;
        this->input_1_offset = 0;
        this->input_2_offset = 0;
        this->output_offset = 0;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t dim_m, 
        int32_t dim_n, 
        int32_t dim_k,
        int32_t prod_valid_offset,
        int32_t prod_ready_offset,
        int32_t cons_valid_offset,
        int32_t cons_ready_offset,
        int32_t input_1_offset,
        int32_t input_2_offset,
        int32_t output_offset
        )
    {
        /* <<--ctor-custom-->> */
        this->dim_m = dim_m;
        this->dim_n = dim_n;
        this->dim_k = dim_k;
        this->prod_valid_offset = prod_valid_offset;
        this->prod_ready_offset = prod_ready_offset;
        this->cons_valid_offset = cons_valid_offset;
        this->cons_ready_offset = cons_ready_offset;
        this->input_1_offset = input_1_offset;
        this->input_2_offset = input_2_offset;
        this->output_offset = output_offset;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (dim_m != rhs.dim_m) return false;
        if (dim_n != rhs.dim_n) return false;
        if (dim_k != rhs.dim_k) return false;
        if (prod_valid_offset != rhs.prod_valid_offset) return false;
        if (prod_ready_offset != rhs.prod_ready_offset) return false;
        if (cons_valid_offset != rhs.cons_valid_offset) return false;
        if (cons_ready_offset != rhs.cons_valid_offset) return false;
        if (input_1_offset != rhs.input_1_offset) return false;
        if (input_2_offset != rhs.input_2_offset) return false;
        if (output_offset != rhs.output_offset) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        dim_m = other.dim_m;
        dim_n = other.dim_n;
        dim_k = other.dim_k;
        prod_valid_offset = other.prod_valid_offset;
        prod_ready_offset = other.prod_ready_offset;
        cons_valid_offset = other.cons_valid_offset;
        cons_ready_offset = other.cons_ready_offset;
        input_1_offset = other.input_1_offset;
        input_2_offset = other.input_2_offset;
        output_offset = other.output_offset;
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
        os << "dim_m = " << conf_info.dim_m << ", ";
        os << "dim_n = " << conf_info.dim_n << ", ";
        os << "dim_k = " << conf_info.dim_k << ", ";
        os << "prod_valid_offset = " << conf_info.prod_valid_offset << ", ";
        os << "prod_ready_offset = " << conf_info.prod_ready_offset << ", ";
        os << "cons_valid_offset = " << conf_info.cons_valid_offset << ", ";
        os << "cons_ready_offset = " << conf_info.cons_ready_offset << ", ";
        os << "input_1_offset = " << conf_info.input_1_offset << ", ";
        os << "input_2_offset = " << conf_info.input_2_offset << ", ";
        os << "output_offset = " << conf_info.output_offset << "";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t dim_m;
        int32_t dim_n;
        int32_t dim_k;
        int32_t prod_valid_offset;
        int32_t prod_ready_offset;
        int32_t cons_valid_offset;
        int32_t cons_ready_offset;
        int32_t input_1_offset;
        int32_t input_2_offset;
        int32_t output_offset;
};

#endif // __GEMM_CONF_INFO_HPP__
