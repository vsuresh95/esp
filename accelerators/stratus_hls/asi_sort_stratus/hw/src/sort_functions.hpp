// Copyright (c) 2011-2022 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

#include "sort.hpp"

/* TODO: handle exceptions and NaN */
inline bool lt_float(const unsigned in0, const unsigned in1)
{

	sc_uint<32> op0  = in0;
	sc_uint<32> op1  = in1;
	bool        s0   = op0[31] == 0 ? false : true;
	bool        s1   = op1[31] == 0 ? false : true;
	sc_uint<8>  exp0 = op0.range(30, 23);
	sc_uint<8>  exp1 = op1.range(30, 23);
	sc_uint<23> man0 = op0.range(22, 0);
	sc_uint<23> man1 = op1.range(22, 0);

	bool zero0 = (exp0 == 0 && man0 == 0);
	bool zero1 = (exp1 == 0 && man1 == 0);
	bool one_zero = (zero0 || zero1);
	bool both_zero = (zero0 && zero1);
	bool discordant = (s0 != s1);
	bool exp0_lt_exp1 = (exp0 < exp1);
	bool exp0_gt_exp1 = (exp0 > exp1);
	bool exp0_eq_exp1 = (exp0 == exp1);
	bool man0_lt_man1 = (man0 < man1);
	bool man0_gt_man1 = (man0 > man1);

	return ((both_zero) ||
		(zero0 && !s1) ||
		(zero1 && s0) ||
		(discordant && s0) ||
		((!discordant && exp0_lt_exp1) && !s0) ||
		((!discordant && exp0_gt_exp1) && s0) ||
		((!discordant && exp0_eq_exp1 && man0_lt_man1) && !s0) ||
		((!discordant && exp0_eq_exp1 && man0_gt_man1) && s0)
		);
}

inline void sort::compute_load_ready_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-load-ready-handshake");

        load_ready.req.req();
    }
}

inline void sort::load_compute_ready_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-compute-ready-handshake");

        load_ready.ack.ack();
    }
}

inline void sort::compute_2_store_ready_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-store-ready-handshake");

        store_ready.req.req();
    }
}

inline void sort::store_compute_2_ready_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-compute-ready-handshake");

        store_ready.ack.ack();
    }
}

inline void sort::compute_load_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-load-done-handshake");

        load_done.ack.ack();
    }
}

inline void sort::load_compute_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("load-compute-done-handshake");

        load_done.req.req();
    }
}

inline void sort::compute_2_store_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("compute-store-done-handshake");

        store_done.ack.ack();
    }
}

inline void sort::store_compute_2_done_handshake()
{
    {
        HLS_DEFINE_PROTOCOL("store-compute-done-handshake");

        store_done.req.req();
    }
}

