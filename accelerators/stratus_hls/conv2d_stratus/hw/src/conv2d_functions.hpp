// Copyright (c) 2011-2023 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "conv2d.hpp"

// Optional application-specific helper functions

inline uint16_t conv2d::conv2d_chunk_output_rows(const uint16_t effective_rows,
    const uint4_t filter_height, const uint4_t stride_log2)
{
    if (effective_rows < filter_height)
        return 0;

    return (((uint16_t) (effective_rows - filter_height)) >> stride_log2) + 1;
}

inline void conv2d::compute_dimensions(
    const uint16_t height, const uint16_t width, const uint16_t n_channels,
    const bool is_padded, const uint4_t stride, const uint4_t filter_height,
    const uint4_t filter_width, const uint16_t n_filters, const uint2_t pool_type, const uint16_t batch_size,
    uint16_t *output_w, uint4_t *pad_top, uint4_t *pad_bottom, uint4_t *pad_left,
    uint16_t *feature_size, uint16_t *filter_size, uint32_t *filters_size, 
    uint16_t *max_cacheable_rows, uint16_t *max_cacheable_rows_init,
    uint16_t *max_cacheable_size,  uint16_t *max_cacheable_size_init,
    uint16_t *max_cacheable_filters, uint16_t *max_cacheable_filters_size,
    uint16_t *max_cacheable_bias_chunks, uint16_t *max_cacheable_bias_size,
    uint16_t *total_input_chunks, uint16_t *total_filters_chunks,
    uint16_t *feature_offset_incr, uint16_t *feature_offset_incr_init,
    uint16_t *feature_row_incr, uint16_t *feature_row_incr_init,
    uint16_t *channel_offset_incr, uint16_t *out_channel_offset_incr,
    uint16_t *out_channel_pool_offset_incr,
    uint32_t *filters_offset_start_base, uint32_t *bias_offset_start_base,
    uint32_t *feature_offset_start_base,
    uint12_t *loadable_chan, uint12_t *chan_iters, uint12_t *chan_rem,
    uint16_t *loadable_chan_sz, uint16_t *chan_rem_sz)
{
    uint16_t filter_size_base = (uint16_t) filter_height * filter_width;
    uint16_t output_h;
    uint4_t stride_log2 = ilog2(stride);
    /* Spatial dimensions of the output activation map */
    if (is_padded) {
        uint16_t pad_h;
        uint16_t pad_w;
        output_h = ((((uint16_t)(height - 1)) >> stride_log2)) + 1;
        *output_w = (((uint16_t) (width - 1)) >> stride_log2) + 1;
        pad_h = max((int32_t) 0,
                (int32_t) (((output_h - 1) * stride) + filter_height - height));
        pad_w = max((int32_t) 0,
                (int32_t) (((*output_w - 1) * stride) + filter_width - width));
        *pad_top = pad_h >> 1;
        *pad_bottom = pad_h - *pad_top;
        *pad_left = pad_w >> 1;
    } else {
        output_h = ((((uint16_t)(height - filter_height)) >> stride_log2)) + 1;
        *pad_top = 0;
        *pad_bottom = 0;
        *pad_left = 0;
        *output_w = (((uint16_t) (width - filter_width)) >> stride_log2) + 1;
    }

    /* Size (in number of words) of an input */
    *feature_size = height * width;

    /* Size (in number of weights) of each filter */
    *filter_size = n_channels * filter_size_base;
    *filters_size = *filter_size * n_filters;

    /* Max number of input rows cacheable in the input PLM */
    uint16_t max_io_channels = max(n_channels, n_filters);
    uint16_t io_channel_size = max_io_channels * width;
    *max_cacheable_rows = min(((uint16_t) INPUT_PLM_SIZE) / ((uint16_t) io_channel_size), height);

    *chan_iters = 1;
    *chan_rem = n_channels;
    *loadable_chan = n_channels;
    if (*max_cacheable_rows < filter_height + 1) {
	*loadable_chan = ((uint16_t) INPUT_PLM_SIZE) / ((uint16_t) (width * (filter_height + 1)));
	*chan_iters = ((uint16_t) (n_channels - 1)) / ((uint12_t) *loadable_chan) + 1;
	*chan_rem = n_channels - (*loadable_chan * (*chan_iters - 1));
	*max_cacheable_rows = filter_height + 1;
    }
    *loadable_chan_sz = (uint16_t) *loadable_chan * filter_size_base;
    *chan_rem_sz = (uint16_t) *chan_rem * filter_size_base;

    *max_cacheable_rows_init = *max_cacheable_rows;

    /* Max number of input rows cacheable in the input PLM */
    // TODO optimize
    uint16_t cacheable_outputs = OUTPUT_PLM_SIZE / ((uint16_t) *output_w *
						    *max_cacheable_rows);
    uint16_t cacheable_filters = WEIGHTS_PLM_SIZE / (*filter_size);
    *max_cacheable_filters = (min(min(cacheable_filters, n_filters),
    				  min(cacheable_outputs, BIAS_PLM_SIZE)) >> 1) << 1;
    *max_cacheable_filters_size = *filter_size * *max_cacheable_filters;

    if (*max_cacheable_rows < height) {
    	if ((*max_cacheable_rows & 1) == 1) {
    	    *max_cacheable_rows -= 1;
    	    if (!is_padded)
    		*max_cacheable_rows_init -= 1;
    	} else {
    	    if (is_padded && (filter_height == 3))
    		*max_cacheable_rows_init -= 1;
    	}
    }

    *max_cacheable_size = *max_cacheable_rows * width;
    *max_cacheable_size_init = *max_cacheable_rows_init * width;

    /* Amount of input chunks to be loaded in the input PLM */
    uint16_t init_loaded_rows = min(height, *max_cacheable_rows_init);
    uint16_t init_top_padding = *pad_top;
    uint16_t init_bottom_padding = 0;
    if (*max_cacheable_rows_init >= height) {
	    init_bottom_padding = *pad_bottom;
    }

    uint16_t init_effective_rows = init_loaded_rows + init_top_padding +
	init_bottom_padding;
    uint16_t output_rows_per_chunk = conv2d_chunk_output_rows(
	    *max_cacheable_rows, filter_height, stride_log2);
    uint16_t output_rows_per_chunk_init = conv2d_chunk_output_rows(
	    init_effective_rows, filter_height, stride_log2);
    *feature_row_incr = output_rows_per_chunk * stride;
    *feature_row_incr_init = output_rows_per_chunk_init * stride - *pad_top;

    *total_input_chunks = 1;
    uint16_t first_row_to_load_i = *feature_row_incr_init;

    while (first_row_to_load_i < height) {
        uint16_t loadable_rows_i = min((uint16_t) (height - first_row_to_load_i),
                                       *max_cacheable_rows);
        uint16_t effective_rows_i = loadable_rows_i;

        bool is_last_chunk_i = ((uint16_t)(first_row_to_load_i + loadable_rows_i) >= height);
        if (is_last_chunk_i) {
            effective_rows_i += *pad_bottom;
        }

        if (!conv2d_chunk_output_rows(effective_rows_i, filter_height, stride_log2)) {
            break;
        }

        (*total_input_chunks)++;
        if (is_last_chunk_i) {
            break;
        }
        first_row_to_load_i += *feature_row_incr;
    }

    /* Amount of filter chunks to be loaded in the filter PLM */
    *total_filters_chunks = ((uint16_t) (n_filters - 1)) / (*max_cacheable_filters) + 1;

    *max_cacheable_bias_chunks = BIAS_PLM_SIZE / *max_cacheable_filters;
    if (*max_cacheable_bias_chunks >= *total_filters_chunks)
    	*max_cacheable_bias_size = n_filters;
    else
    	*max_cacheable_bias_size = *max_cacheable_bias_chunks * *max_cacheable_filters;

    /* Load offsets */
    *channel_offset_incr = round_up(*feature_size, DMA_WORD_PER_BEAT);
    *out_channel_offset_incr = round_up(output_h * *output_w, DMA_WORD_PER_BEAT);
    uint16_t output_pool_h = pool_type ? output_h >> 1 : output_h;
    uint16_t output_pool_w = pool_type ? *output_w >> 1 : *output_w;
    *out_channel_pool_offset_incr = round_up(output_pool_h * output_pool_w, DMA_WORD_PER_BEAT);

    *feature_offset_incr = *feature_row_incr * width;
    *feature_offset_incr_init = *feature_row_incr_init * width;
    *filters_offset_start_base =  *channel_offset_incr * n_channels * batch_size;
    *bias_offset_start_base = *filters_offset_start_base + *filters_size;
    *feature_offset_start_base = *filters_offset_start_base + *filters_size + n_filters;
}

inline void conv2d::load_compute_cfg_handshake()
{
    HLS_DEFINE_PROTOCOL("load-compute-cfg-handshake");

    load_compute_cfg_done.req.req();
}

inline void conv2d::compute_load_cfg_handshake()
{
    HLS_DEFINE_PROTOCOL("compute-load-cfg-handshake");

    load_compute_cfg_done.ack.ack();
}

inline void conv2d::load_store_cfg_handshake()
{
    HLS_DEFINE_PROTOCOL("load-store-cfg-handshake");

    load_store_cfg_done.req.req();
}

inline void conv2d::store_load_cfg_handshake()
{
    HLS_DEFINE_PROTOCOL("store-load-cfg-handshake");

    load_store_cfg_done.ack.ack();
}
