// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "tiled_app.hpp"

// Optional application-specific helper functions


inline void tiled_app::store_done_req()
{
    HLS_DEFINE_PROTOCOL("store-done-req-handshake");

    store_done.req.req();
}

inline void tiled_app::store_done_ack()
{
    HLS_DEFINE_PROTOCOL("store-done-ack-handshake");

    store_done.ack.ack();
}

inline void tiled_app::load_done_req()
{
    HLS_DEFINE_PROTOCOL("load-done-req-handshake");

    load_done.req.req();
}

inline void tiled_app::load_done_ack()
{
    HLS_DEFINE_PROTOCOL("load-done-ack-handshake");

    load_done.ack.ack();
}

inline void tiled_app::load_next_tile_req()
{
    HLS_DEFINE_PROTOCOL("load-next-tile-req-handshake");

    load_next_tile.req.req();
}

inline void tiled_app::load_next_tile_ack()
{
    HLS_DEFINE_PROTOCOL("load-next-tile-ack-handshake");

    load_next_tile.ack.ack();
}


inline void tiled_app::compute_done_req()
{
    HLS_DEFINE_PROTOCOL("compute-done-ack-handshake");

    compute_done.req.req();
}

inline void tiled_app::compute_done_ack()
{
    HLS_DEFINE_PROTOCOL("compute-done-req-handshake");

    compute_done.ack.ack();
}
