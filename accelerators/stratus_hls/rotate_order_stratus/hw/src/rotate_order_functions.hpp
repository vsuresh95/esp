// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "rotate_order.hpp"

// Optional application-specific helper functions

inline void rotate_order::compute_load_ready_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-load-ready-handshake");

        load_ready.req.req();
    }
}

inline void rotate_order::load_compute_ready_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-compute-ready-handshake");

        load_ready.ack.ack();
    }
}

inline void rotate_order::compute_store_ready_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-store-ready-handshake");

        store_ready.req.req();
    }
}

inline void rotate_order::store_compute_ready_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-compute-ready-handshake");

        store_ready.ack.ack();
    }
}

inline void rotate_order::compute_load_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-load-done-handshake");

        load_done.ack.ack();
    }
}

inline void rotate_order::load_compute_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-compute-done-handshake");

        load_done.req.req();
    }
}

inline void rotate_order::compute_store_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-store-done-handshake");

        store_done.ack.ack();
    }
}

inline void rotate_order::store_compute_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-compute-done-handshake");

        store_done.req.req();
    }
}

inline void rotate_order::compute_rotate_1_ready_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-rotate-1-ready-handshake");

        rotate_1_ready.req.req();
    }
}

inline void rotate_order::rotate_1_compute_ready_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("rotate-1-compute-ready-handshake");

        rotate_1_ready.ack.ack();
    }
}

inline void rotate_order::compute_rotate_1_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-rotate-1-done-handshake");

        rotate_1_done.ack.ack();
    }
}

inline void rotate_order::rotate_1_compute_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("rotate-1-compute-done-handshake");

        rotate_1_done.req.req();
    }
}

inline void rotate_order::compute_rotate_2_ready_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-rotate-2-ready-handshake");

        rotate_2_ready.req.req();
    }
}

inline void rotate_order::rotate_2_compute_ready_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("rotate-2-compute-ready-handshake");

        rotate_2_ready.ack.ack();
    }
}

inline void rotate_order::compute_rotate_2_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-rotate-2-done-handshake");

        rotate_2_done.ack.ack();
    }
}

inline void rotate_order::rotate_2_compute_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("rotate-2-compute-done-handshake");

        rotate_2_done.req.req();
    }
}

inline void rotate_order::compute_rotate_3_ready_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-rotate-3-ready-handshake");

        rotate_3_ready.req.req();
    }
}

inline void rotate_order::rotate_3_compute_ready_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("rotate-3-compute-ready-handshake");

        rotate_3_ready.ack.ack();
    }
}

inline void rotate_order::compute_rotate_3_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-rotate-3-done-handshake");

        rotate_3_done.ack.ack();
    }
}

inline void rotate_order::rotate_3_compute_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("rotate-3-compute-done-handshake");

        rotate_3_done.req.req();
    }
}
