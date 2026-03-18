// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __VECTOR_CONF_INFO_HPP__
#define __VECTOR_CONF_INFO_HPP__

#include <systemc.h>

#define VECTOR_OP_ADD 0
#define VECTOR_OP_AVG_POOL 1

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
        this->vector_op = 1;
        this->n_channel = 1;
        this->input_len = 1;
        this->stride = 1;
        this->input1_offset = 1;
        this->input2_offset = 1;
        this->output_offset = 1;
        this->do_relu = 0;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t vector_op,
        int32_t n_channel,
        int32_t input_len, 
        int32_t stride,
        int32_t input1_offset, 
        int32_t input2_offset,
        int32_t output_offset,
        int32_t do_relu
        )
    {
        /* <<--ctor-custom-->> */
        this->vector_op = vector_op;
        this->n_channel = n_channel;
        this->input_len = input_len;
        this->stride = stride;
        this->input1_offset = input1_offset;
        this->input2_offset = input2_offset;
        this->output_offset = output_offset;
        this->do_relu = do_relu;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (vector_op != rhs.vector_op) return false;
        if (n_channel != rhs.n_channel) return false;
        if (input_len != rhs.input_len) return false;
        if (stride != rhs.stride) return false;
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
        vector_op = other.vector_op;
        n_channel = other.n_channel;
        input_len = other.input_len;
        stride = other.stride;
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
        os << "vector_op = " << conf_info.vector_op << ", ";
        os << "n_channel = " << conf_info.n_channel << ", ";
        os << "input_len = " << conf_info.input_len << ", ";
        os << "stride = " << conf_info.stride << ", ";
        os << "input1_offset = " << conf_info.input1_offset << ", ";
        os << "input2_offset = " << conf_info.input2_offset << ", ";
        os << "output_offset = " << conf_info.output_offset << ", ";
        os << "do_relu = " << conf_info.do_relu << ", ";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t vector_op;
        int32_t n_channel; // only used for avg pool
        int32_t input_len; // total vector length for add, input width for avg pool
        int32_t stride; // only used for avg pool
        int32_t input1_offset;
        int32_t input2_offset;
        int32_t output_offset;
        int32_t do_relu;
};

#endif // __VECTOR_CONF_INFO_HPP__
