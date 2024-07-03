// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __VITDODEC_CONF_INFO_HPP__
#define __VITDODEC_CONF_INFO_HPP__

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
        this->cbps = 48;
        this->ntraceback = 5;
        this->data_bits = 288;
        this->in_length = 24852;
        this->out_length = 18585;
        this->input_start_offset    = 0;
        this->output_start_offset   = 0;
        this->accel_cons_vld_offset = 0;
        this->accel_prod_rdy_offset = 0;
        this->accel_cons_rdy_offset = 0;
        this->accel_prod_vld_offset = 0;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t cbps, 
        int32_t ntraceback, 
        int32_t data_bits,
        int32_t in_length,
        int32_t out_length,
        int32_t input_start_offset    ,
        int32_t output_start_offset   ,
        int32_t accel_cons_vld_offset ,
        int32_t accel_prod_rdy_offset ,
        int32_t accel_cons_rdy_offset ,
        int32_t accel_prod_vld_offset
        )
    {
        /* <<--ctor-custom-->> */
        this->cbps = cbps;
        this->ntraceback = ntraceback;
        this->data_bits = data_bits;
        this->in_length = in_length;
        this->out_length = out_length;
        this->input_start_offset    = input_start_offset   ;
        this->output_start_offset   = output_start_offset  ;
        this->accel_cons_vld_offset = accel_cons_vld_offset;
        this->accel_prod_rdy_offset = accel_prod_rdy_offset;
        this->accel_cons_rdy_offset = accel_cons_rdy_offset;
        this->accel_prod_vld_offset = accel_prod_vld_offset;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (cbps != rhs.cbps) return false;
        if (ntraceback != rhs.ntraceback) return false;
        if (data_bits != rhs.data_bits) return false;
        if (in_length != rhs.in_length) return false;
        if (out_length != rhs.out_length) return false;
        if (input_start_offset    != input_start_offset   ) return false;
        if (output_start_offset   != output_start_offset  ) return false;
        if (accel_cons_vld_offset != accel_cons_vld_offset) return false;
        if (accel_prod_rdy_offset != accel_prod_rdy_offset) return false;
        if (accel_cons_rdy_offset != accel_cons_rdy_offset) return false;
        if (accel_prod_vld_offset != accel_prod_vld_offset) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        cbps = other.cbps;
        ntraceback = other.ntraceback;
        data_bits = other.data_bits;
        in_length = other.in_length;
        out_length = other.out_length;
        input_start_offset    = other.input_start_offset   ;
        output_start_offset   = other.output_start_offset  ;
        accel_cons_vld_offset = other.accel_cons_vld_offset;
        accel_prod_rdy_offset = other.accel_prod_rdy_offset;
        accel_cons_rdy_offset = other.accel_cons_rdy_offset;
        accel_prod_vld_offset = other.accel_prod_vld_offset;
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
        os << "cbps = " << conf_info.cbps << ", ";
        os << "ntraceback = " << conf_info.ntraceback << ", ";
        os << "data_bits = " << conf_info.data_bits << ", ";
        os << "in_length = " << conf_info.in_length << ", ";
        os << "out_length = " << conf_info.out_length << ", ";
        os << "input_start_offset   " << conf_info.input_start_offset    << ", ";
        os << "output_start_offset  " << conf_info.output_start_offset   << ", ";
        os << "accel_cons_vld_offset" << conf_info.accel_cons_vld_offset << ", ";
        os << "accel_prod_rdy_offset" << conf_info.accel_prod_rdy_offset << ", ";
        os << "accel_cons_rdy_offset" << conf_info.accel_cons_rdy_offset << ", ";
        os << "accel_prod_vld_offset" << conf_info.accel_prod_vld_offset << "";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t cbps;
        int32_t ntraceback;
        int32_t data_bits;
        int32_t in_length;
        int32_t out_length;
        int32_t input_start_offset    ;
        int32_t output_start_offset   ;
        int32_t accel_cons_vld_offset ;
        int32_t accel_prod_rdy_offset ;
        int32_t accel_cons_rdy_offset ;
        int32_t accel_prod_vld_offset ;
};

#endif // __VITDODEC_CONF_INFO_HPP__
