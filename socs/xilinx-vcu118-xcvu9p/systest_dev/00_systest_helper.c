#include <00_systest_helper.h>

void thread_entry (int cid, int nc) {
	return;
}

uint64_t read_hartid () {
	uint64_t hartid;

	asm volatile (
		"csrr %0, mhartid"
		: "=r" (hartid)
	);

	return hartid;
}	

uint64_t amo_swap (volatile uint64_t* handshake, uint64_t value) {
	uint64_t old_val;

	asm volatile (
		"mv t0, %2;"
		"mv t2, %1;"
		"amoswap.d.aqrl t1, t0, (t2);"
		"mv %0, t1"
		: "=r" (old_val)
		: "r" (handshake), "r" (value)
		: "t0", "t1", "t2", "memory"
	);
	return old_val;
}

void spin_for_lock (volatile uint64_t* handshake) {
	uint64_t old_val;

	// acquire the lock
	while (1) {
		// check if lock is set
		if (read_dword_fcs(handshake, false, false) != 1) {
			// try to set lock
			old_val = amo_swap (handshake, 1); 

			// check if lock was set
			if (old_val != 1){
				break;
			}
		}
	}
}

void release_lock (volatile uint64_t* handshake) {
    // while (amo_swap (handshake, 0) != 1);
	write_dword_fcs(handshake, 0, false, false);
	asm volatile ("fence w, w");
}

void amo_add (volatile uint64_t* handshake, uint64_t value) {
	asm volatile (
		"mv t0, %1;"
		"mv t2, %0;"
		"amoadd.d.aqrl t1, t0, (t2);"
		:
		: "r" (handshake), "r" (value)
		: "t0", "t1", "t2", "memory"
	);
}

void self_inval (volatile uint64_t* handshake) {
	// dummy lr to generate self-invalidate
	asm volatile (
		"li t0, 0;"
		"mv t1, %0;"
		"lr.d.aq t0, (t1)"
		:
		: "r" (handshake)
	);
}

void wb_flush (volatile uint64_t* handshake) {
	// dummy sc to generate WB flush
	asm volatile (
		"li t0, 0;"
		"li t1, 1;"
		"mv t2, %0;"
		"sc.d.rl t0, t1, (t2)"
		:
		: "r" (handshake)
	);
}

void write_dword_fcs (volatile uint64_t* dst, uint64_t value, bool dcs_en, bool owner_pred) {
	if (dcs_en) {
		if (owner_pred) {
			// OP enabled, FwdWTFwd & Cache ID 1
			asm volatile (
				"mv t0, %0;"
				"mv t1, %1;"
				".word (0x2062B82B | (1 << 25))"
				: 
				: "r" (dst), "r" (value)
				: "t0", "t1", "memory"
			);
		} else {
			// DCS enabled & ReqWT
			asm volatile (
				"mv t0, %0;"
				"mv t1, %1;"
				".word 0x2062B02B"
				: 
				: "r" (dst), "r" (value)
				: "t0", "t1", "memory"
			);
		}
	} else {
		*dst = value;
	}
}

volatile uint64_t read_dword_fcs (volatile uint64_t* dst, bool dcs_en, bool owner_pred) {
	volatile uint64_t read_value;

	if (dcs_en) {
		if (owner_pred) {
			// OP enabled, ReqV & Cache ID 1
			asm volatile (
				"mv t0, %1;"
				".word (0x2102B30B | (1 << 25));"
				"mv %0, t1"
				: "=r" (read_value)
				: "r" (dst)
				: "t0", "t1", "memory"
			);
		} else {
			// DCS enabled & ReqV
			asm volatile (
				"mv t0, %1;"
				".word 0x2002B30B;"
				"mv %0, t1"
				: "=r" (read_value)
				: "r" (dst)
				: "t0", "t1", "memory"
			);
		}
	} else {
		read_value = *dst;
	}
	return read_value;
}

uint64_t get_counter() {
	uint64_t counter;
	asm volatile (
		"li t0, 0;"
		"csrr t0, mcycle;"
		"mv %0, t0"
		: "=r" ( counter )
		:
		: "t0"
	);

	return counter;
}

void delay (int cycles) {
	for (int i = 0; i < cycles; i++) {
		asm volatile ("nop");
	}
}