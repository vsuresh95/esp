#include <stdio.h>
#include <stdbool.h>

#ifndef __SYSTEST_HELPER__
#define __SYSTEST_HELPER__

uint64_t read_hartid ();

void thread_entry (int cid, int nc);

uint64_t amo_swap (volatile uint64_t* handshake, uint64_t value);

void amo_add (volatile uint64_t* handshake, uint64_t value);

void self_inval (volatile uint64_t* handshake);

void wb_flush (volatile uint64_t* handshake);

void write_dword_fcs (volatile uint64_t* dst, uint64_t value, bool dcs_en, bool owner_pred);

volatile uint64_t read_dword_fcs (volatile uint64_t* dst, bool dcs_en, bool owner_pred);

uint64_t get_counter();

#endif // __SYSTEST_HELPER__