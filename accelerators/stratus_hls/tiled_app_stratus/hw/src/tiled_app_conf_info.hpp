// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __TILED_APP_CONF_INFO_HPP__
#define __TILED_APP_CONF_INFO_HPP__

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
        this->num_tiles = 12;
        this->tile_size = 1024;
        this->rd_wr_enable = 0;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t num_tiles, 
        int32_t tile_size, 
        int32_t rd_wr_enable
        )
    {
        /* <<--ctor-custom-->> */
        this->num_tiles = num_tiles;
        this->tile_size = tile_size;
        this->rd_wr_enable = rd_wr_enable;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (num_tiles != rhs.num_tiles) return false;
        if (tile_size != rhs.tile_size) return false;
        if (rd_wr_enable != rhs.rd_wr_enable) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        num_tiles = other.num_tiles;
        tile_size = other.tile_size;
        rd_wr_enable = other.rd_wr_enable;
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
        os << "num_tiles = " << conf_info.num_tiles << ", ";
        os << "tile_size = " << conf_info.tile_size << ", ";
        os << "rd_wr_enable = " << conf_info.rd_wr_enable << "";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t num_tiles;
        int32_t tile_size;
        int32_t rd_wr_enable;
};

#endif // __TILED_APP_CONF_INFO_HPP__
