// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __FUSION_UNOPT_CONF_INFO_HPP__
#define __FUSION_UNOPT_CONF_INFO_HPP__

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
        this->veno = 10;
        this->imgwidth = 640;
        this->htdim = 512;
        this->imgheight = 480;
        this->sdf_block_size = 8;
        this->sdf_block_size3 = 512;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t veno, 
        int32_t imgwidth, 
        int32_t htdim, 
        int32_t imgheight, 
        int32_t sdf_block_size, 
        int32_t sdf_block_size3
        )
    {
        /* <<--ctor-custom-->> */
        this->veno = veno;
        this->imgwidth = imgwidth;
        this->htdim = htdim;
        this->imgheight = imgheight;
        this->sdf_block_size = sdf_block_size;
        this->sdf_block_size3 = sdf_block_size3;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (veno != rhs.veno) return false;
        if (imgwidth != rhs.imgwidth) return false;
        if (htdim != rhs.htdim) return false;
        if (imgheight != rhs.imgheight) return false;
        if (sdf_block_size != rhs.sdf_block_size) return false;
        if (sdf_block_size3 != rhs.sdf_block_size3) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        veno = other.veno;
        imgwidth = other.imgwidth;
        htdim = other.htdim;
        imgheight = other.imgheight;
        sdf_block_size = other.sdf_block_size;
        sdf_block_size3 = other.sdf_block_size3;
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
        os << "veno = " << conf_info.veno << ", ";
        os << "imgwidth = " << conf_info.imgwidth << ", ";
        os << "htdim = " << conf_info.htdim << ", ";
        os << "imgheight = " << conf_info.imgheight << ", ";
        os << "sdf_block_size = " << conf_info.sdf_block_size << ", ";
        os << "sdf_block_size3 = " << conf_info.sdf_block_size3 << "";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t veno;
        int32_t imgwidth;
        int32_t htdim;
        int32_t imgheight;
        int32_t sdf_block_size;
        int32_t sdf_block_size3;
};

#endif // __FUSION_UNOPT_CONF_INFO_HPP__
