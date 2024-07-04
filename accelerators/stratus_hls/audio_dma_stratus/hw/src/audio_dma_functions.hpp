// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "audio_dma.hpp"

// Optional application-specific helper functions
inline void audio_dma::compute_load_ready_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-load-ready-handshake");

        load_ready.req.req();
    }
}

inline void audio_dma::load_compute_ready_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-compute-ready-handshake");

        load_ready.ack.ack();
    }
}

inline void audio_dma::compute_store_ready_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-store-ready-handshake");

        store_ready.req.req();
    }
}

inline void audio_dma::store_compute_ready_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-compute-ready-handshake");

        store_ready.ack.ack();
    }
}

inline void audio_dma::compute_load_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-load-done-handshake");

        load_done.ack.ack();
    }
}

inline void audio_dma::load_compute_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-compute-done-handshake");

        load_done.req.req();
    }
}

inline void audio_dma::compute_store_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-store-done-handshake");

        store_done.ack.ack();
    }
}

inline void audio_dma::store_compute_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-compute-done-handshake");

        store_done.req.req();
    }
}
