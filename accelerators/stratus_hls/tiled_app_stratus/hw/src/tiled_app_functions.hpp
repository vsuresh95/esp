// Copyright (c) 2011-2021 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "tiled_app.hpp"

// Optional application-specific helper functions


inline void tiled_app::store_sync_done_req()
{
    HLS_DEFINE_PROTOCOL("store-sync-done-req-handshake");

    store_sync_done.req.req();
}

inline void tiled_app::store_sync_done_ack()
{
    HLS_DEFINE_PROTOCOL("store-sync-done-ack-handshake");

    store_sync_done.ack.ack();
}

inline void tiled_app::load_sync_done_req()
{
    HLS_DEFINE_PROTOCOL("load-sync-done-ack-handshake");

    load_sync_done.req.req();
}

inline void tiled_app::load_sync_done_ack()
{
    HLS_DEFINE_PROTOCOL("store-sync-done-ack-handshake");

    load_sync_done.ack.ack();
}

inline void tiled_app::load_next_tile_req()
{
    HLS_DEFINE_PROTOCOL("load-next-tile-req-handshake");

    load_next_tile.req.req();
}

inline void tiled_app::load_next_tile_ack()
{
    HLS_DEFINE_PROTOCOL("store-next-tile-ack-handshake");

    load_next_tile.ack.ack();
}
