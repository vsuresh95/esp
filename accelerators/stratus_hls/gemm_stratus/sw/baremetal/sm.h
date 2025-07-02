// Size and parameter defines
#define VALID_OFFSET 0
#define PAYLOAD_OFFSET 8

// Custom implementation of atomic flag for performance
typedef struct {
    volatile uint64_t *flag;
} atomic_flag_t;

uint64_t atomic_flag_load(atomic_flag_t *atom) {
	uint64_t val;

	asm volatile (
		"mv t0, %1;"
		"lr.d.aq t1, (t0);"
		"mv %0, t1;"
		: "=r" (val)
		: "r" (atom->flag)
		: "t0", "t1", "memory"
		);

	return val;
}

void atomic_flag_store(atomic_flag_t *atom, uint64_t val) {
	uint64_t old_val;

	asm volatile (
		"mv t0, %2;"
		"mv t2, %1;"
		"amoswap.d.aqrl t1, t0, (t2);"
		"mv %0, t1;"
		: "=r" (old_val)
		: "r" (atom->flag), "r" (val)
		: "t0", "t1", "t2", "memory"
		);
}

void atomic_flag_init(atomic_flag_t *atom, volatile uint64_t *f) {
	atom->flag = f;
	printf("flag = %p\n", atom->flag);
	atomic_flag_store(atom, 0);
}

