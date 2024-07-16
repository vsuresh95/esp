// void WriteScratchReg(unsigned value) {
// 	asm volatile (
// 		"mv t0, %0;"
// 		"csrrw t0, mscratch, t0"
// 		:
// 		: "r" (value)
// 		: "t0", "t1", "memory"
// 	);
// }

static inline void write_mem_wtfwd (void* dst, int64_t value_64)
{
	asm volatile (
		"mv t0, %0;"
		"mv t1, %1;"
		".word " QU(WRITE_CODE_WTFWD)
		// ".word " QU(WRITE_CODE)
		:
		: "r" (dst), "r" (value_64)
		: "t0", "t1", "memory"
	);
}

static inline int64_t read_mem_reqv (void* dst)
{
	int64_t value_64;

	asm volatile (
		"mv t0, %1;"
		".word " QU(READ_CODE_REQV) ";"
		"mv %0, t1"
		: "=r" (value_64)
		: "r" (dst)
		: "t0", "t1", "memory"
	);

	return value_64;
}

static inline int64_t read_mem_reqodata (void* dst)
{
	int64_t value_64;

	asm volatile (
		"mv t0, %1;"
		".word " QU(READ_CODE_REQODATA) ";"
		// ".word " QU(READ_CODE) ";"
		"mv %0, t1"
		: "=r" (value_64)
		: "r" (dst)
		: "t0", "t1", "memory"
	);

	return value_64;
}

void UpdateSync(void* sync, int64_t UpdateValue) {
	asm volatile ("fence w, w");

	// Need to cast to void* for extended ASM code.
	write_mem_wtfwd((void *) sync, UpdateValue);

	asm volatile ("fence w, w");
}

void SpinSync(void* sync, int64_t SpinValue) {
	int64_t ExpectedValue = SpinValue;
	int64_t ActualValue = 0xcafedead;

	while (ActualValue != ExpectedValue) {
		// Need to cast to void* for extended ASM code.
		ActualValue = read_mem_reqodata((void *) sync);
	}
}

// bool TestSync(void* sync, int64_t TestValue) {
// 	int64_t ExpectedValue = TestValue;
// 	int64_t ActualValue = 0xcafedead;

// 	// Need to cast to void* for extended ASM code.
// 	ActualValue = read_mem_reqodata((void *) sync);

// 	if (ActualValue != ExpectedValue) return false;
// 	else return true;
// }

