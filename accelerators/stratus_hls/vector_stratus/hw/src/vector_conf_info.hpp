// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __VECTOR_CONF_INFO_HPP__
#define __VECTOR_CONF_INFO_HPP__

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
        this->total_len = 1;
        this->input1_offset = 1;
        this->input2_offset = 1;
        this->output_offset = 1;
        this->do_relu = 0;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t total_len, 
        int32_t input1_offset, 
        int32_t input2_offset,
        int32_t output_offset,
        int32_t do_relu
        )
    {
        /* <<--ctor-custom-->> */
        this->total_len = total_len;
        this->input1_offset = input1_offset;
        this->input2_offset = input2_offset;
        this->output_offset = output_offset;
        this->do_relu = do_relu;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (total_len != rhs.total_len) return false;
        if (input1_offset != rhs.input1_offset) return false;
        if (input2_offset != rhs.input2_offset) return false;
        if (output_offset != rhs.output_offset) return false;
        if (do_relu != rhs.do_relu) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        total_len = other.total_len;
        input1_offset = other.input1_offset;
        input2_offset = other.input2_offset;
        output_offset = other.output_offset;
        do_relu = other.do_relu;
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
        os << "total_len = " << conf_info.total_len << ", ";
        os << "input1_offset = " << conf_info.input1_offset << ", ";
        os << "input2_offset = " << conf_info.input2_offset << "";
        os << "output_offset = " << conf_info.output_offset << ", ";
        os << "do_relu = " << conf_info.do_relu << ", ";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t total_len;
        int32_t input1_offset;
        int32_t input2_offset;
        int32_t output_offset;
        int32_t do_relu;
};

#endif // __VECTOR_CONF_INFO_HPP__
