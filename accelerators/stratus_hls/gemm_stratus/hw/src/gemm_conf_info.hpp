// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __GEMM_CONF_INFO_HPP__
#define __GEMM_CONF_INFO_HPP__

#include <systemc.h>

#define N_INPUTS 1
#define N_OUTPUTS 1
#define N_CONTEXTS 4
#define N_CONTEXTS_BITS 2

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
        for (int i = 0; i < N_CONTEXTS; i++) {
            this->dim_m[i] = 1;
            this->dim_n[i] = 1;
            this->dim_k[i] = 1;
            this->weight_base[i] = 1;
            for (int j = 0; j < N_INPUTS; j++) {
                this->input_base[i][j] = 0;
            }
            for (int j = 0; j < N_OUTPUTS; j++) {
                this->output_base[i][j] = 0;
            }
            this->context_nprio[i] = 0;
        }
        this->valid_contexts = 0;
        this->sched_period = 0;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t dim_m[N_CONTEXTS], 
        int32_t dim_n[N_CONTEXTS], 
        int32_t dim_k[N_CONTEXTS],
        int32_t weight_base[N_CONTEXTS],
        int32_t input_base[N_CONTEXTS][N_INPUTS],
        int32_t output_base[N_CONTEXTS][N_OUTPUTS],
        int32_t context_nprio[N_CONTEXTS],
        int32_t valid_contexts,
        int32_t sched_period
        )
    {
        /* <<--ctor-custom-->> */
        for (int i = 0; i < N_CONTEXTS; i++) {
            this->dim_m[i] = dim_m[i];
            this->dim_n[i] = dim_n[i];
            this->dim_k[i] = dim_k[i];
            this->weight_base[i] = weight_base[i];
            for (int j = 0; j < N_INPUTS; j++) {
                this->input_base[i][j] = input_base[i][j];
            }
            for (int j = 0; j < N_OUTPUTS; j++) {
                this->output_base[i][j] = output_base[i][j];
            }
            this->context_nprio[i] = context_nprio[i];
        }
        this->valid_contexts = valid_contexts;
        this->sched_period = sched_period;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        for (int i = 0; i < N_CONTEXTS; i++) {
            if (dim_m[i] != rhs.dim_m[i]) return false;
            if (dim_n[i] != rhs.dim_n[i]) return false;
            if (dim_k[i] != rhs.dim_k[i]) return false;
            if (weight_base[i] != rhs.weight_base[i]) return false;
            for (int j = 0; j < N_INPUTS; j++) {
                if (input_base[i][j] != rhs.input_base[i][j]) return false;
            }
            for (int j = 0; j < N_OUTPUTS; j++) {
                if (output_base[i][j] != rhs.output_base[i][j]) return false;
            }
            if (context_nprio[i] != rhs.context_nprio[i]) return false;
        }
        if (valid_contexts != rhs.valid_contexts) return false;
        if (sched_period != rhs.sched_period) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        for (int i = 0; i < N_CONTEXTS; i++) {
            dim_m[i] = other.dim_m[i];
            dim_n[i] = other.dim_n[i];
            dim_k[i] = other.dim_k[i];
            weight_base[i] = other.weight_base[i];
            for (int j = 0; j < N_INPUTS; j++) {
                input_base[i][j] = other.input_base[i][j];
            }
            for (int j = 0; j < N_OUTPUTS; j++) {
                output_base[i][j] = other.output_base[i][j];
            }
            context_nprio[i] = other.context_nprio[i];
        }
        valid_contexts = other.valid_contexts;
        valid_contexts = other.valid_contexts;
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
        int32_t dim_m[N_CONTEXTS];
        int32_t dim_n[N_CONTEXTS];
        int32_t dim_k[N_CONTEXTS];
        int32_t weight_base[N_CONTEXTS];
        int32_t input_base[N_CONTEXTS][N_INPUTS];
        int32_t output_base[N_CONTEXTS][N_OUTPUTS];
        int32_t context_nprio[N_CONTEXTS];
        int32_t valid_contexts;
        int32_t sched_period;
};

#endif // __GEMM_CONF_INFO_HPP__
