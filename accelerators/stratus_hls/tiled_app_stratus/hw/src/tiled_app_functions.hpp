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


// inline void tiled_app::_req(){
//     {
//         HLS_DEFINE_PROTOCOL("-req-handshake");
//         .req.req();
//     }
// }
// inline void tiled_app::_ack(){
//     {
//         HLS_DEFINE_PROTOCOL("-ack-handshake");
//         .ack.ack();
//     }
// }

//////////////////////////////////////////////////////
// OUTPUT -> LOAD/STORE START HANDSHAKES
//////////////////////////////////////////////////////

inline void tiled_app::output_load_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("output-load-start-handshake");

        output_load_start.req.req();
    }
}

inline void tiled_app::load_output_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-output-start-handshake");

        output_load_start.ack.ack();
    }
}

inline void tiled_app::output_store_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("output-store-start-handshake");

        output_store_start.req.req();
    }
}

inline void tiled_app::store_output_start_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-output-start-handshake");

        output_store_start.ack.ack();
    }
}

//////////////////////////////////////////////////////
// LOAD/STORE -> OUTPUT DONE HANDSHAKES
//////////////////////////////////////////////////////

inline void tiled_app::output_load_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("output-load-done-handshake");

        load_output_done.ack.ack();
    }
}

inline void tiled_app::load_output_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-output-done-handshake");

        load_output_done.req.req();
    }
}

inline void tiled_app::output_store_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("output-store-done-handshake");

        store_output_done.ack.ack();
    }
}

inline void tiled_app::store_output_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-output-done-handshake");

        store_output_done.req.req();
    }
}


#if 0
//ifndef SYNTH_APP_CFA
inline void tiled_app::compute_0_done_req(){
    {
        HLS_DEFINE_PROTOCOL("compute_0_done-req-handshake");
        compute_0_done.req.req();
    }
}
inline void tiled_app::compute_0_done_ack(){
    {
        HLS_DEFINE_PROTOCOL("compute_0_done-ack-handshake");
        compute_0_done.ack.ack();
    }
}

inline void tiled_app::compute_1_done_req(){
    {
        HLS_DEFINE_PROTOCOL("compute_1_done-req-handshake");
        compute_1_done.req.req();
    }
}
inline void tiled_app::compute_1_done_ack(){
    {
        HLS_DEFINE_PROTOCOL("compute_1_done-ack-handshake");
        compute_1_done.ack.ack();
    }
}

#endif
