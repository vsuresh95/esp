// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#ifndef __CONV2D_CONF_INFO_HPP__
#define __CONV2D_CONF_INFO_HPP__

#include <systemc.h>

#define CONF_INFO_SIZE 14+2

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
        this->n_channels = 1;
        this->n_filters = 1;
        this->filter_dim = 1;
        this->stride = 1;
        this->is_padded = 1;
        this->feature_map_height = 1;
        this->feature_map_width = 1;
        this->do_relu = 1;
        this->pool_type = 1;
        this->batch_size = 1;
        this->input_offset = 0;
        this->filters_offset = 0;
        this->bias_offset = 0;
        this->output_offset = 0;
    }

    conf_info_t(
        /* <<--ctor-args-->> */
        int32_t n_channels, 
        int32_t n_filters, 
        int32_t filter_dim, 
        int32_t stride, 
        int32_t is_padded, 
        int32_t feature_map_height, 
        int32_t feature_map_width,
        int32_t do_relu,
        int32_t pool_type,
        int32_t batch_size,
        int32_t input_offset,
        int32_t filters_offset,
        int32_t bias_offset,
        int32_t output_offset
        )
    {
        /* <<--ctor-custom-->> */
        this->n_channels = n_channels;
        this->n_filters = n_filters;
        this->filter_dim = filter_dim;
        this->stride = stride;
        this->is_padded = is_padded;
        this->feature_map_height = feature_map_height;
        this->feature_map_width = feature_map_width;
        this->do_relu = do_relu;
        this->pool_type = pool_type;
        this->batch_size = batch_size;
        this->input_offset = input_offset;
        this->filters_offset = filters_offset;
        this->bias_offset = bias_offset;
        this->output_offset = output_offset;
    }

    // equals operator
    inline bool operator==(const conf_info_t &rhs) const
    {
        /* <<--eq-->> */
        if (n_channels != rhs.n_channels) return false;
        if (n_filters != rhs.n_filters) return false;
        if (filter_dim != rhs.filter_dim) return false;
        if (stride != rhs.stride) return false;
        if (is_padded != rhs.is_padded) return false;
        if (feature_map_height != rhs.feature_map_height) return false;
        if (feature_map_width != rhs.feature_map_width) return false;
        if (do_relu != rhs.do_relu) return false;
        if (pool_type != rhs.pool_type) return false;
        if (batch_size != rhs.batch_size) return false;
        if (input_offset != rhs.input_offset) return false;
        if (filters_offset != rhs.filters_offset) return false;
        if (bias_offset != rhs.bias_offset) return false;
        if (output_offset != rhs.output_offset) return false;
        return true;
    }

    // assignment operator
    inline conf_info_t& operator=(const conf_info_t& other)
    {
        /* <<--assign-->> */
        n_channels = other.n_channels;
        n_filters = other.n_filters;
        filter_dim = other.filter_dim;
        stride = other.stride;
        is_padded = other.is_padded;
        feature_map_height = other.feature_map_height;
        feature_map_width = other.feature_map_width;
        do_relu = other.do_relu;
        pool_type = other.pool_type;
        batch_size = other.batch_size;
        input_offset = other.input_offset;
        filters_offset = other.filters_offset;
        bias_offset = other.bias_offset;
        output_offset = other.output_offset;
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
        os << "n_channels = " << conf_info.n_channels << ", ";
        os << "n_filters = " << conf_info.n_filters << ", ";
        os << "filter_dim = " << conf_info.filter_dim << ", ";
        os << "stride = " << conf_info.stride << ", ";
        os << "is_padded = " << conf_info.is_padded << ", ";
        os << "feature_map_height = " << conf_info.feature_map_height << ", ";
        os << "feature_map_width = " << conf_info.feature_map_width << "";
        os << "do_relu = " << conf_info.do_relu << "";
        os << "pool_type = " << conf_info.pool_type << "";
        os << "batch_size = " << conf_info.batch_size << "";
        os << "input_offset = " << conf_info.input_offset << "";
        os << "filters_offset = " << conf_info.filters_offset << "";
        os << "bias_offset = " << conf_info.bias_offset << "";
        os << "output_offset = " << conf_info.output_offset << "";
        os << "}";
        return os;
    }

    // index assignment operator
    inline int32_t& operator[](int index)
    {
        /* <<--index assign-->> */
        switch (index) {
            case 0: return n_channels;
            case 1: return n_filters;
            case 2: return filter_dim;
            case 3: return stride;
            case 4: return is_padded;
            case 5: return feature_map_height;
            case 6: return feature_map_width;
            case 7: return do_relu;
            case 8: return pool_type;
            case 9: return batch_size;
            case 10: return input_offset;
            case 11: return filters_offset;
            case 12: return bias_offset;
            case 13: return output_offset;
            default: return do_relu;
        }
    }

    // index read operator
    inline const int32_t& operator[](int index) const
    {
        /* <<--index read-->> */
        switch (index) {
            case 0: return n_channels;
            case 1: return n_filters;
            case 2: return filter_dim;
            case 3: return stride;
            case 4: return is_padded;
            case 5: return feature_map_height;
            case 6: return feature_map_width;
            case 7: return do_relu;
            case 8: return pool_type;
            case 9: return batch_size;
            case 10: return input_offset;
            case 11: return filters_offset;
            case 12: return bias_offset;
            case 13: return output_offset;
            default: return do_relu;
        }
	}

        /* <<--params-->> */
        int32_t n_channels;
        int32_t n_filters;
        int32_t filter_dim;
        int32_t stride;
        int32_t is_padded;
        int32_t feature_map_height;
        int32_t feature_map_width;
        int32_t do_relu;
        int32_t pool_type; // 0: no pooling, 1: 2x2 max pooling, 2: 2x2 average pooling
        int32_t batch_size;
        int32_t input_offset;
        int32_t filters_offset;
        int32_t bias_offset;
        int32_t output_offset;
};

#endif // __CONV2D_CONF_INFO_HPP__
