// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __ESP_FPDATA_HPP__
#define __ESP_FPDATA_HPP__

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <systemc.h>

#include "cynw_fixed.h"


#define WORD_SIZE 32
const unsigned int FPDATA_WL = WORD_SIZE;
const unsigned int FPDATA_IL = WORD_SIZE / 2;
const unsigned int FPDATA_FL = WORD_SIZE - FPDATA_IL;
typedef cynw_fixed<FPDATA_WL, FPDATA_IL> FPDATA;

// Helper functions

template<typename T, size_t N>
T int2fp(sc_dt::sc_int<N> data_in)
{
    T data_out;

    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "int2fp1");

        for (unsigned i = 0; i < N; i++)
        {
            HLS_UNROLL_LOOP(ON, "int2fp-loop");

            data_out[i] = data_in[i].to_bool();
        }
    }

    return data_out;
}

template<typename T, size_t N>
void int2fp(T &data_out, sc_dt::sc_int<N> data_in)
{
    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "int2fp2");

        for (unsigned i = 0; i < N; i++)
        {
            HLS_UNROLL_LOOP(ON, "int2fp-loop");

            data_out[i] = data_in[i].to_bool();
        }
    }
}

#define INT2FP(x) int2fp<FPDATA, WORD_SIZE>(x)

template<typename T, size_t N>
sc_dt::sc_int<N> fp2int(T data_in)
{
    sc_dt::sc_int<N> data_out;

    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "fp2int1");

        for (unsigned i = 0; i < N; i++)
        {
            HLS_UNROLL_LOOP(ON, "fp2int-loop");

            data_out[i] = (bool) data_in[i];
        }
    }

    return data_out;
}

template<typename T, size_t N>
void fp2int(sc_dt::sc_int<N> &data_out, T data_in)
{
    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "fp2int2");

        for (unsigned i = 0; i < N; i++)
        {
            HLS_UNROLL_LOOP(ON, "fp2int-loop");

            data_out[i] = (bool) data_in[i];
        }
    }
}

#define FP2INT(x) fp2int<FPDATA, WORD_SIZE>(x)

#endif // __ESP_FPDATA_HPP__