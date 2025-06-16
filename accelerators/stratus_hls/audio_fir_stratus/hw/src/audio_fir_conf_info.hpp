// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __AUDIO_FIR_CONF_INFO_HPP__
#define __AUDIO_FIR_CONF_INFO_HPP__

#include <systemc.h>

#define MAX_CONTEXTS 4
#define MAX_CONTEXTS_BITS 2

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
        for (int i = 0; i < MAX_CONTEXTS; i++) {
            this->logn_samples[i] = 1;
            this->input_queue_base[i] = 0;
            this->output_queue_base[i] = 0;
            this->filter_queue_base[i] = 0;
            this->context_quota[i] = 0;
        }
        this->valid_contexts = 0;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t logn_samples[MAX_CONTEXTS], 
        int32_t input_queue_base[MAX_CONTEXTS],
        int32_t output_queue_base[MAX_CONTEXTS],
        int32_t filter_queue_base[MAX_CONTEXTS],
        int32_t context_quota[MAX_CONTEXTS],
        int32_t valid_contexts
        )
    {
        /* <<--ctor-custom-->> */
        for (int i = 0; i < MAX_CONTEXTS; i++) {
            this->logn_samples[i] = logn_samples[i];
            this->input_queue_base[i] = input_queue_base[i];
            this->output_queue_base[i] = output_queue_base[i];
            this->filter_queue_base[i] = filter_queue_base[i];
            this->context_quota[i] = context_quota[i];
        }
        this->valid_contexts = valid_contexts;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        for (int i = 0; i < MAX_CONTEXTS; i++) {
            if (logn_samples[i] != rhs.logn_samples[i]) return false;
            if (input_queue_base[i] != rhs.input_queue_base[i]) return false;
            if (output_queue_base[i] != rhs.output_queue_base[i]) return false;
            if (filter_queue_base[i] != rhs.filter_queue_base[i]) return false;
            if (context_quota[i] != rhs.context_quota[i]) return false;
        }
        if (valid_contexts != rhs.valid_contexts) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        for (int i = 0; i < MAX_CONTEXTS; i++) {
            logn_samples[i] = other.logn_samples[i];
            input_queue_base[i] = other.input_queue_base[i];
            output_queue_base[i] = other.output_queue_base[i];
            filter_queue_base[i] = other.filter_queue_base[i];
            context_quota[i] = other.context_quota[i];
        }
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
        // os << "logn_samples = " << conf_info.logn_samples << ", ";
        // os << "input_queue_base = " << conf_info.input_queue_base << ", ";
        // os << "output_queue_base = " << conf_info.output_queue_base << ", ";
        // os << "filter_queue_base = " << conf_info.filter_queue_base << ", ";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t logn_samples[MAX_CONTEXTS];
        int32_t input_queue_base[MAX_CONTEXTS];
        int32_t output_queue_base[MAX_CONTEXTS];
        int32_t filter_queue_base[MAX_CONTEXTS];
        int32_t context_quota[MAX_CONTEXTS];
        int32_t valid_contexts;
};

#endif // __AUDIO_FIR_CONF_INFO_HPP__
