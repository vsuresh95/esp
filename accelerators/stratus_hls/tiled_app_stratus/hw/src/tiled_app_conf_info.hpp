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
        this->output_offset = 0;
        this->input_offset  = 0;
        this->cons_valid_offset  = 0;
        this->prod_ready_offset   = 0;
        this->cons_ready_offset    = 0;
        this->prod_valid_offset     = 0;
        this->num_tiles = 12;
        this->tile_size = 1024;
        this->rd_wr_enable = 0;
        this->num_comp_units = 3;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t ping_pong_en,
        int32_t compute_over_data,
        int32_t compute_iters,
        int32_t output_offset,
        int32_t input_offset ,
        int32_t cons_valid_offset,
        int32_t prod_ready_offset,
        int32_t cons_ready_offset,  
        int32_t prod_valid_offset,           
        int32_t num_tiles, 
        int32_t tile_size, 
        int32_t rd_wr_enable,
        int32_t num_comp_units
        )
    {
        /* <<--ctor-custom-->> */
        this->ping_pong_en = ping_pong_en;
        this->compute_over_data = compute_over_data;
        this->compute_iters = compute_iters;
        this->output_offset = output_offset;
        this->input_offset  = input_offset ;
        this->cons_valid_offset = cons_valid_offset;
        this->prod_ready_offset  = prod_ready_offset ;
        this->cons_ready_offset   = cons_ready_offset  ;
        this->prod_valid_offset    = prod_valid_offset   ;
        this->num_tiles = num_tiles;
        this->tile_size = tile_size;
        this->rd_wr_enable = rd_wr_enable;
        this->num_comp_units = num_comp_units;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if(ping_pong_en != rhs.ping_pong_en) return false;
        if (compute_over_data != rhs.compute_over_data) return false;
        if (compute_iters != rhs.compute_iters) return false;
        if (output_offset != rhs.output_offset ) return false;
        if (input_offset  != rhs.input_offset  ) return false;       
        if (cons_valid_offset!= rhs.cons_valid_offset) return false;
        if (prod_ready_offset != rhs.prod_ready_offset ) return false;
        if (cons_ready_offset  != rhs.cons_ready_offset  ) return false;
        if (prod_valid_offset   != rhs.prod_valid_offset   ) return false;
        if (num_tiles != rhs.num_tiles) return false;
        if (tile_size != rhs.tile_size) return false;
        if (rd_wr_enable != rhs.rd_wr_enable) return false;
        if (num_comp_units != rhs.num_comp_units) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        ping_pong_en = other.ping_pong_en;
        compute_over_data = other.compute_over_data;
        compute_iters = other.compute_iters;
        output_offset = other.output_offset;
        input_offset  = other.input_offset ;
        cons_valid_offset   = other.cons_valid_offset;
        prod_ready_offset    = other.prod_ready_offset ;
        cons_ready_offset     = other.cons_ready_offset  ;
        prod_valid_offset      = other.prod_valid_offset   ;
        num_tiles = other.num_tiles;
        tile_size = other.tile_size;
        rd_wr_enable = other.rd_wr_enable;
        num_comp_units = other.num_comp_units;
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
        os << "output_offset = " << conf_info.output_offset;
        os << "input_offset = " << conf_info.input_offset ;
        os << "cons_valid_offset = " << conf_info.cons_valid_offset<< ", ";
        os << "prod_ready_offset = " << conf_info.prod_ready_offset<< ", ";
        os << "cons_ready_offset = " << conf_info.cons_ready_offset<< ", ";
        os << "prod_valid_offset = " << conf_info.prod_valid_offset<< ", ";
        os << "num_tiles = " << conf_info.num_tiles << ", ";
        os << "tile_size = " << conf_info.tile_size << ", ";
        os << "rd_wr_enable = " << conf_info.rd_wr_enable << ", ";
        os << "num_comp_units = " << conf_info.num_comp_units << " ";
        os << "}";
        return os;
    }

        /* <<--params-->> */
        int32_t ping_pong_en;
        int32_t compute_over_data;
        int32_t compute_iters;
        int32_t output_offset;
        int32_t input_offset;
        int32_t cons_valid_offset;
        int32_t prod_ready_offset ;
        int32_t cons_ready_offset  ;
        int32_t prod_valid_offset   ;
        int32_t num_tiles;
        int32_t tile_size;
        int32_t rd_wr_enable;
        int32_t num_comp_units;
};

#endif // __TILED_APP_CONF_INFO_HPP__
