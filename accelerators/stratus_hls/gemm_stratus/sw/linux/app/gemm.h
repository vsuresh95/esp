#ifndef __GEMM__
#define __GEMM__



// static inline uint64_t get_counter() {
//     uint64_t t_end = 0;
// #ifndef __linux__
// 	asm volatile (
// 		"li t0, 0;"
// 		"csrr t0, mcycle;"
// 		"mv %0, t0"
// 		: "=r" (t_end)
// 		:
// 		: "t0"
// 	);
// #else
// 	asm volatile (
// 		"li t0, 0;"
// 		"csrr t0, cycle;"
// 		"mv %0, t0"
// 		: "=r" (t_end)
// 		:
// 		: "t0"
// 	);
// #endif
// 	return t_end;
// }


/* Write to the memory*/
static inline void write_mem (void* dst, int64_t value_64)
{
	asm volatile (
		"mv t0, %0;"
		"mv t1, %1;"
		".word " QU(WRITE_CODE)
		:
		: "r" (dst), "r" (value_64)
		: "t0", "t1", "memory"
	);
}

/* Read from the memory*/
static inline int64_t read_mem (void* dst)
{
	int64_t value_64;

	asm volatile (
		"mv t0, %1;"
		".word " QU(READ_CODE) ";"
		"mv %0, t1"
		: "=r" (value_64)
		: "r" (dst)
		: "t0", "t1", "memory"
	);

	return value_64;
}

static inline void reset_sync(){
	int n;
    printf("Inside reset sync\n");
	for(n = 0; n< num_devices; n++){
		write_mem(((void*)(mem + rel_accel_prod_ready_offset)), 1);//BM
		printf("reset accel_prod_ready mem[%d]: %d\n", rel_accel_prod_ready_offset, *(mem + rel_accel_prod_ready_offset));
		write_mem(((void*)(mem + rel_accel_cons_valid_offset)), 0);
		// DBG_PRINT("reset accel_cons_valid mem[%d]: %d\n", rel_accel_cons_valid_offset, *(mem + rel_accel_cons_valid_offset));
		write_mem(((void*)(mem + rel_accel_con_last_offset)), 0);
		// DBG_PRINT("reset accel_con_last mem[%d]: %d\n", rel_accel_con_last_offset, *(mem + rel_accel_con_last_offset));
		write_mem(((void*)(mem + rel_accel_prod_valid_offset)), 0);
		// DBG_PRINT("reset accel_prod_valid mem[%d]: %d\n", rel_accel_prod_valid_offset, *(mem + rel_accel_prod_valid_offset));
		write_mem(((void*)(mem + rel_accel_cons_ready_offset)), 0); 
		// DBG_PRINT("reset accel_cons_ready mem[%d]: %d\n", rel_accel_cons_ready_offset, *(mem + rel_accel_cons_ready_offset));
	}
	asm volatile ("fence w, w");	
}

static inline uint32_t poll_cons_rdy(){
	int64_t value_64 = 0;
	void* dst = (void*)(mem + (*cpu_cons_ready_offset));
	value_64 = read_mem(dst);
	// printf("Polling cons rdy mem[%d]: %d\n", *cpu_cons_ready_offset, value_64);
	return ((value_64&0x1) == 1);
}

static inline uint32_t poll_prod_valid(){
	void* dst = (void*)(mem+(*cpu_prod_valid_offset));
	int64_t value_64 = 0;
	value_64 = read_mem(dst);
	return ((value_64&0x1) == 1);
}

static inline uint32_t accel_rdy(int toggle){
	if(toggle){
		if(poll_cons_rdy()) return 1;
	}
	else{
		if(poll_prod_valid()) return 2;
	}
	return 0;
}

static inline void update_cons_valid(int64_t last){
	#ifndef ESP
	asm volatile ("fence w, w");	//release semantics
	#endif
	// void* dst = (void*)(mem+(*cpu_cons_valid_offset)+1);
	// write_mem(dst, last);

	// #ifdef ESP
	// asm volatile ("fence w, w");	//release semantics
	// #endif

	// dst = (void*)(mem+(*cpu_cons_valid_offset));
	// int64_t value_64 = 1;
	// write_mem(dst, value_64);


	// int time_var = 0;
	// while(time_var<100) time_var++;

	mem[*cpu_cons_valid_offset+1] = last;
	mem[*cpu_cons_valid_offset] = 1;
	asm volatile ("fence w, w");	
}


static inline void update_cons_rdy(){
	asm volatile ("fence w, w");	//acquire semantics
	// void* dst = (void*)(mem+(*cpu_cons_ready_offset));
	// int64_t value_64 = 0;
	// write_mem(dst, value_64);
	// int time_var = 0;
	// while(time_var<100) time_var++;
	mem[*cpu_cons_ready_offset] = 0;
	asm volatile ("fence w, w");	
}

static inline void update_prod_rdy(){
	asm volatile ("fence w, w");	//acquire semantics
	// void* dst = (void*)(mem+(*cpu_prod_ready_offset));
	// int64_t value_64 = 1; 
	// write_mem(dst, value_64);
	// int time_var = 0;
	// while(time_var<100) time_var++;

	mem[*cpu_prod_ready_offset] = 1;
	asm volatile ("fence w, w");	
}

static inline void update_prod_valid(){
	asm volatile ("fence w, w");	//release semantics
	// void* dst = (void*)(mem+(*cpu_prod_valid_offset));
	// int64_t value_64 = 0; 
	// write_mem(dst, value_64);
	// int time_var = 0;
	// while(time_var<100) time_var++;

	mem[*cpu_prod_valid_offset] = 0;
	asm volatile ("fence w, w");	//acquire semantics
}



static inline void update_con_last(){
	void* dst = (void*)(mem+(*cpu_cons_valid_offset)+1);
	int64_t value_64 = 1; 
	write_mem(dst, value_64);	
}
static inline void update_con_not_last(){
	void* dst = (void*)(mem+(*cpu_cons_valid_offset)+1);
	int64_t value_64 = 0; 
	write_mem(dst, value_64);
}

static inline void write_to_cons(){
	void *dst = (void*)(&mem[rel_input_buffer_offset]);
	int64_t val_64 = 123; 
	int local_tile_size = tile_size;
	for (int j = 0; j < local_tile_size; j++){
		write_mem(dst, val_64);
		dst += 8;
	}
}

static inline void read_from_prod(){
	void *src = (void*)(mem+rel_output_buffer_offset);
	int64_t out_val;
	int local_tile_size = tile_size;
	for (int j = 0; j < local_tile_size; j++){
			out_val = read_mem(src);
			src += 8;
	}
}

#endif // __GEMM__