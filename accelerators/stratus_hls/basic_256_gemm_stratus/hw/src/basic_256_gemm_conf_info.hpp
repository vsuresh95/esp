// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __BASIC_256_GEMM_CONF_INFO_HPP__
#define __BASIC_256_GEMM_CONF_INFO_HPP__

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
        this->C_mat_offset = 0;
        this->B_mat_offset = 0;
        this->A_mat_offset = 0;
        this->K = 1;
        this->M = 1;
        this->N = 1;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t C_mat_offset, 
        int32_t B_mat_offset, 
        int32_t A_mat_offset, 
        int32_t K, 
        int32_t M, 
        int32_t N
        )
    {
        /* <<--ctor-custom-->> */
        this->C_mat_offset = C_mat_offset;
        this->B_mat_offset = B_mat_offset;
        this->A_mat_offset = A_mat_offset;
        this->K = K;
        this->M = M;
        this->N = N;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (C_mat_offset != rhs.C_mat_offset) return false;
        if (B_mat_offset != rhs.B_mat_offset) return false;
        if (A_mat_offset != rhs.A_mat_offset) return false;
        if (K != rhs.K) return false;
        if (M != rhs.M) return false;
        if (N != rhs.N) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        C_mat_offset = other.C_mat_offset;
        B_mat_offset = other.B_mat_offset;
        A_mat_offset = other.A_mat_offset;
        K = other.K;
        M = other.M;
        N = other.N;
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
        os << "C_mat_offset = " << conf_info.C_mat_offset << ", ";
        os << "B_mat_offset = " << conf_info.B_mat_offset << ", ";
        os << "A_mat_offset = " << conf_info.A_mat_offset << ", ";
        os << "K = " << conf_info.K << ", ";
        os << "M = " << conf_info.M << ", ";
        os << "N = " << conf_info.N << "";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t C_mat_offset;
        int32_t B_mat_offset;
        int32_t A_mat_offset;
        int32_t K;
        int32_t M;
        int32_t N;
};

#endif // __BASIC_256_GEMM_CONF_INFO_HPP__
