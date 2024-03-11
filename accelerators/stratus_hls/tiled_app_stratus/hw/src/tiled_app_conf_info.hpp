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
        this->ping_pong_en = 0;
        this->compute_over_data = 0;
        this->compute_iters = 0;
        this->output_tile_start_offset = 0;
        this->input_tile_start_offset  = 0;
        this->output_update_sync_offset  = 0;
        this->input_update_sync_offset   = 0;
        this->output_spin_sync_offset    = 0;
        this->input_spin_sync_offset     = 0;
        this->num_tiles = 12;
        this->tile_size = 1024;
        this->rd_wr_enable = 0;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t ping_pong_en,
        int32_t compute_over_data,
        int32_t compute_iters,
        int32_t output_tile_start_offset,
        int32_t input_tile_start_offset ,
        int32_t output_update_sync_offset,
        int32_t input_update_sync_offset,
        int32_t output_spin_sync_offset,  
        int32_t input_spin_sync_offset,           
        int32_t num_tiles, 
        int32_t tile_size, 
        int32_t rd_wr_enable
        )
    {
        /* <<--ctor-custom-->> */
        this->ping_pong_en = ping_pong_en;
        this->compute_over_data = compute_over_data;
        this->compute_iters = compute_iters;
        this->output_tile_start_offset = output_tile_start_offset;
        this->input_tile_start_offset  = input_tile_start_offset ;
        this->output_update_sync_offset = output_update_sync_offset;
        this->input_update_sync_offset  = input_update_sync_offset ;
        this->output_spin_sync_offset   = output_spin_sync_offset  ;
        this->input_spin_sync_offset    = input_spin_sync_offset   ;
        this->num_tiles = num_tiles;
        this->tile_size = tile_size;
        this->rd_wr_enable = rd_wr_enable;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if(ping_pong_en != rhs.ping_pong_en) return false;
        if (compute_over_data != rhs.compute_over_data) return false;
        if (compute_iters != rhs.compute_iters) return false;
        if (output_tile_start_offset != rhs.output_tile_start_offset ) return false;
        if (input_tile_start_offset  != rhs.input_tile_start_offset  ) return false;       
        if (output_update_sync_offset!= rhs.output_update_sync_offset) return false;
        if (input_update_sync_offset != rhs.input_update_sync_offset ) return false;
        if (output_spin_sync_offset  != rhs.output_spin_sync_offset  ) return false;
        if (input_spin_sync_offset   != rhs.input_spin_sync_offset   ) return false;
        if (num_tiles != rhs.num_tiles) return false;
        if (tile_size != rhs.tile_size) return false;
        if (rd_wr_enable != rhs.rd_wr_enable) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        ping_pong_en = other.ping_pong_en;
        compute_over_data = other.compute_over_data;
        compute_iters = other.compute_iters;
        output_tile_start_offset = other.output_tile_start_offset;
        input_tile_start_offset  = other.input_tile_start_offset ;
        output_update_sync_offset   = other.output_update_sync_offset;
        input_update_sync_offset    = other.input_update_sync_offset ;
        output_spin_sync_offset     = other.output_spin_sync_offset  ;
        input_spin_sync_offset      = other.input_spin_sync_offset   ;
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
        os << "ping_pong_en = "<< conf_info.ping_pong_en;
        os << "compute_over_data = " << conf_info.compute_over_data;
        os << "compute_iters = " << conf_info.compute_iters ;
        os << "output_tile_start_offset = " << conf_info.output_tile_start_offset;
        os << "input_tile_start_offset = " << conf_info.input_tile_start_offset ;
        os << "output_update_sync_offset = " << conf_info.output_update_sync_offset<< ", ";
        os << "input_update_sync_offset = " << conf_info.input_update_sync_offset<< ", ";
        os << "output_spin_sync_offset = " << conf_info.output_spin_sync_offset<< ", ";
        os << "input_spin_sync_offset = " << conf_info.input_spin_sync_offset<< ", ";
        os << "num_tiles = " << conf_info.num_tiles << ", ";
        os << "tile_size = " << conf_info.tile_size << ", ";
        os << "rd_wr_enable = " << conf_info.rd_wr_enable << "";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t ping_pong_en;
        int32_t compute_over_data;
        int32_t compute_iters;
        int32_t output_tile_start_offset;
        int32_t input_tile_start_offset;
        int32_t output_update_sync_offset;
        int32_t input_update_sync_offset ;
        int32_t output_spin_sync_offset  ;
        int32_t input_spin_sync_offset   ;
        int32_t num_tiles;
        int32_t tile_size;
        int32_t rd_wr_enable;
};

#endif // __TILED_APP_CONF_INFO_HPP__
