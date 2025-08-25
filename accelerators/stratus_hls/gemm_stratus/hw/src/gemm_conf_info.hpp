// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __GEMM_CONF_INFO_HPP__
#define __GEMM_CONF_INFO_HPP__

#include <systemc.h>

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
            this->context_base_ptr[i] = 0;
            this->context_nprio[i] = 0;
        }
        this->valid_contexts = 0;
        this->sched_period = 0;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t context_base_ptr[N_CONTEXTS],
        int32_t context_nprio[N_CONTEXTS],
        int32_t valid_contexts,
        int32_t sched_period
        )
    {
        /* <<--ctor-custom-->> */
        for (int i = 0; i < N_CONTEXTS; i++) {
            this->context_base_ptr[i] = context_base_ptr[i];
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
            if (context_base_ptr[i] != rhs.context_base_ptr[i]) return false;
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
            context_base_ptr[i] = other.context_base_ptr[i];
            context_nprio[i] = other.context_nprio[i];
        }
        valid_contexts = other.valid_contexts;
        sched_period = other.sched_period;
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
    int32_t context_base_ptr[N_CONTEXTS];
    int32_t context_nprio[N_CONTEXTS];
    int32_t valid_contexts;
    int32_t sched_period;
};

#endif // __GEMM_CONF_INFO_HPP__
