// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __AUDIO_FFI_CONF_INFO_HPP__
#define __AUDIO_FFI_CONF_INFO_HPP__

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
            this->do_shift[i] = 1;
            this->input_queue_base[i] = 0;
            this->output_queue_base[i] = 0;
            this->filter_queue_base[i] = 0;
        }
        this->context_quota = 0;
        this->valid_contexts = 0;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t logn_samples[MAX_CONTEXTS], 
        int32_t do_shift[MAX_CONTEXTS],
        int32_t input_queue_base[MAX_CONTEXTS],
        int32_t output_queue_base[MAX_CONTEXTS],
        int32_t filter_queue_base[MAX_CONTEXTS],
        int32_t context_quota,
        int32_t valid_contexts
        )
    {
        /* <<--ctor-custom-->> */
        for (int i = 0; i < MAX_CONTEXTS; i++) {
            this->logn_samples[i] = logn_samples[i];
            this->do_shift[i] = do_shift[i];
            this->input_queue_base[i] = input_queue_base[i];
            this->output_queue_base[i] = output_queue_base[i];
            this->filter_queue_base[i] = filter_queue_base[i];
        }
        this->context_quota = context_quota;
        this->valid_contexts = valid_contexts;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        for (int i = 0; i < MAX_CONTEXTS; i++) {
            if (logn_samples[i] != rhs.logn_samples[i]) return false;
            if (do_shift[i] != rhs.do_shift[i]) return false;
            if (input_queue_base[i] != rhs.input_queue_base[i]) return false;
            if (output_queue_base[i] != rhs.output_queue_base[i]) return false;
            if (filter_queue_base[i] != rhs.filter_queue_base[i]) return false;
        }
        if (context_quota != rhs.context_quota) return false;
        if (valid_contexts != rhs.valid_contexts) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        for (int i = 0; i < MAX_CONTEXTS; i++) {
            logn_samples[i] = other.logn_samples[i];
            do_shift[i] = other.do_shift[i];
            input_queue_base[i] = other.input_queue_base[i];
            output_queue_base[i] = other.output_queue_base[i];
            filter_queue_base[i] = other.filter_queue_base[i];
        }
        context_quota = other.context_quota;
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
        // os << "do_shift = " << conf_info.do_shift << "";
        // os << "input_queue_base = " << conf_info.input_queue_base << ", ";
        // os << "output_queue_base = " << conf_info.output_queue_base << ", ";
        // os << "filter_queue_base = " << conf_info.filter_queue_base << ", ";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t logn_samples[MAX_CONTEXTS];
        int32_t do_shift[MAX_CONTEXTS];
        int32_t input_queue_base[MAX_CONTEXTS];
        int32_t output_queue_base[MAX_CONTEXTS];
        int32_t filter_queue_base[MAX_CONTEXTS];
        int32_t context_quota;
        int32_t valid_contexts;
};

#endif // __AUDIO_FFI_CONF_INFO_HPP__
