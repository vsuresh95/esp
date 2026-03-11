// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __ADD_CONF_INFO_HPP__
#define __ADD_CONF_INFO_HPP__

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
            this->do_inverse[i] = 1;
            this->logn_samples[i] = 1;
            this->do_shift[i] = 1;
            for (int j = 0; j < N_INPUTS; j++) {
                this->input_queue_base[i][j] = 0;
            }
            for (int j = 0; j < N_OUTPUTS; j++) {
                this->output_queue_base[i][j] = 0;
            }
            this->context_nprio[i] = 0;
        }
        this->valid_contexts = 0;
        this->sched_period = 0;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t do_inverse[N_CONTEXTS], 
        int32_t logn_samples[N_CONTEXTS], 
        int32_t do_shift[N_CONTEXTS],
        int32_t input_queue_base[N_CONTEXTS][N_INPUTS],
        int32_t output_queue_base[N_CONTEXTS][N_OUTPUTS],
        int32_t context_nprio[N_CONTEXTS],
        int32_t valid_contexts,
        int32_t sched_period
        )
    {
        /* <<--ctor-custom-->> */
        for (int i = 0; i < N_CONTEXTS; i++) {
            this->do_inverse[i] = do_inverse[i];
            this->logn_samples[i] = logn_samples[i];
            this->do_shift[i] = do_shift[i];
            for (int j = 0; j < N_INPUTS; j++) {
                this->input_queue_base[i][j] = input_queue_base[i][j];
            }
            for (int j = 0; j < N_OUTPUTS; j++) {
                this->output_queue_base[i][j] = output_queue_base[i][j];
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
            if (do_inverse[i] != rhs.do_inverse[i]) return false;
            if (logn_samples[i] != rhs.logn_samples[i]) return false;
            if (do_shift[i] != rhs.do_shift[i]) return false;
            for (int j = 0; j < N_INPUTS; j++) {
                if (input_queue_base[i][j] != rhs.input_queue_base[i][j]) return false;
            }
            for (int j = 0; j < N_OUTPUTS; j++) {
                if (output_queue_base[i][j] != rhs.output_queue_base[i][j]) return false;
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
            do_inverse[i] = other.do_inverse[i];
            logn_samples[i] = other.logn_samples[i];
            do_shift[i] = other.do_shift[i];
            for (int j = 0; j < N_INPUTS; j++) {
                input_queue_base[i][j] = other.input_queue_base[i][j];
            }
            for (int j = 0; j < N_OUTPUTS; j++) {
                output_queue_base[i][j] = other.output_queue_base[i][j];
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
        // os << "do_inverse = " << conf_info.do_inverse << ", ";
        // os << "logn_samples = " << conf_info.logn_samples << ", ";
        // os << "do_shift = " << conf_info.do_shift << ", ";
        // os << "input_queue_base = " << conf_info.input_queue_base << ", ";
        // os << "output_queue_base = " << conf_info.output_queue_base << ", ";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t do_inverse[N_CONTEXTS];
        int32_t logn_samples[N_CONTEXTS];
        int32_t do_shift[N_CONTEXTS];
        int32_t input_queue_base[N_CONTEXTS][N_INPUTS];
        int32_t output_queue_base[N_CONTEXTS][N_OUTPUTS];
        int32_t context_nprio[N_CONTEXTS];
        int32_t valid_contexts;
        int32_t sched_period;
};

#endif // __ADD_CONF_INFO_HPP__
