#ifndef __TILED_APP__
#define __TILED_APP__

#include "cfg.h"
#include "stdio.h"

uint64_t t_cpu_write=0;
uint64_t t_acc=0;
uint64_t t_cpu_read=0;
uint64_t t_sw=0;
uint64_t t_total=0;


/* For performance profiling */
// static inline
static inline uint64_t get_counter() {
    uint64_t t_end = 0;
#ifndef __linux__
	asm volatile (
		"li t0, 0;"
		"csrr t0, mcycle;"
		"mv %0, t0"
		: "=r" (t_end)
		:
		: "t0"
	);
#else
	asm volatile (
		"li t0, 0;"
		"csrr t0, cycle;"
		"mv %0, t0"
		: "=r" (t_end)
		:
		: "t0"
	);
#endif
	return t_end;
}


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

static inline void set_offsets(){

	int32_t rel_accel_prod_ready_offset = 0;
	int32_t rel_accel_prod_valid_offset = rel_accel_prod_ready_offset+2;
	int32_t rel_accel_prod_last_offset = rel_accel_prod_valid_offset+1;
	int32_t rel_input_buffer_offset = SYNC_VAR_SIZE;
	int32_t rel_accel_cons_ready_offset = tile_size + SYNC_VAR_SIZE;
	int32_t rel_accel_cons_valid_offset = rel_accel_cons_ready_offset+2;
	int32_t rel_accel_con_last_offset = rel_accel_cons_valid_offset+1;
	int32_t rel_output_buffer_offset = tile_size + 2*SYNC_VAR_SIZE;

    for(int dev_id = 0; dev_id < num_devices; dev_id++){
		accel_prod_valid_offset[dev_id] = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_prod_valid_offset;
		accel_cons_ready_offset[dev_id] = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_cons_ready_offset;
		accel_prod_ready_offset[dev_id] = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_prod_ready_offset;
		accel_cons_valid_offset[dev_id] = dev_id*(tile_size + SYNC_VAR_SIZE) + rel_accel_cons_valid_offset;
		input_buffer_offset[dev_id]  	= dev_id*(tile_size + SYNC_VAR_SIZE) + rel_input_buffer_offset;
		output_buffer_offset[dev_id] 	= dev_id*(tile_size + SYNC_VAR_SIZE) + rel_output_buffer_offset;
    }
	cpu_prod_ready_offset = &accel_cons_ready_offset[num_devices-1];
	cpu_prod_valid_offset = &accel_cons_valid_offset[num_devices-1];
}
static inline void print_flags(){
	for(int n = 0; n < num_devices; n++){
		printf("Device %d prod rdy [%d] = %d ", n,accel_prod_ready_offset[n], read_mem((void*)&buf[accel_prod_ready_offset[n]]) );
		printf(" cons rdy [%d] = %d", accel_cons_ready_offset[n], read_mem((void*)&buf[accel_cons_ready_offset[n]] ));
		printf(" prod vld [%d] = %d", accel_prod_valid_offset[n], read_mem((void*)&buf[accel_prod_valid_offset[n]] ));
		printf(" cons vld [%d] = %d\n", accel_cons_valid_offset[n], read_mem((void*)&buf[accel_cons_valid_offset[n]]) );
		//printf("accel %d pv[%d]=%d ",dev_id, accel_prod_valid_offset[dev_id], buf[accel_prod_valid_offset[dev_id]]);
		//printf("cr[%d]=%d " ,accel_cons_ready_offset[dev_id], buf[accel_cons_ready_offset[dev_id]]);
		//printf("pr[%d]=%d " ,accel_prod_ready_offset[dev_id], buf[accel_prod_ready_offset[dev_id]]);
		//printf("cv=[%d]%d\n",accel_cons_valid_offset[dev_id], buf[accel_cons_valid_offset[dev_id]]);
	}
}
static inline void reset_sync(){
	int n;
    // printf("Inside reset sync\n");
	for(n = 0; n< num_devices; n++){
		write_mem(((void*)(buf + accel_cons_valid_offset[n])), 0);
	//	write_mem(((void*)(buf + accel_cons_valid_offset[n]+1)), 0);
		asm volatile ("fence w, w");	
		write_mem(((void*)(buf + accel_prod_valid_offset[n])), 0);
		asm volatile ("fence w, w");	
		write_mem(((void*)(buf + accel_cons_ready_offset[n])), 1); 
		asm volatile ("fence w, w");	
		write_mem(((void*)(buf + accel_prod_ready_offset[n])), 1);//BM
		asm volatile ("fence w, w");	
		//printf("Device %d prod rdy [%d] = %d ", n,accel_prod_ready_offset[n], read_mem((void*)&buf[accel_prod_ready_offset[n]]) );
		//printf(" cons rdy [%d] = %d", accel_cons_ready_offset[n], read_mem((void*)&buf[accel_cons_ready_offset[n]] ));
		//printf(" prod vld [%d] = %d", accel_prod_valid_offset[n], read_mem((void*)&buf[accel_prod_valid_offset[n]] ));
		//printf(" cons vld [%d] = %d\n", accel_cons_valid_offset[n], read_mem((void*)&buf[accel_cons_valid_offset[n]]) );
	}
	asm volatile ("fence w, w");	
}

static inline uint32_t poll_cons_rdy(){
	int64_t value_64 = 0;
	void* dst = (void*)(buf + (*cpu_cons_ready_offset));
	value_64 = read_mem(dst);
	return (value_64 == 1);
}

static inline uint32_t poll_prod_valid(){
	void* dst = (void*)(buf+(*cpu_prod_valid_offset));
	int64_t value_64 = 0;
	value_64 = read_mem(dst);
	//if(value_64 == 2) printf("last from accel\n");
	return (value_64 == 1 || value_64 == 2);
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
	//#ifndef ESP
	//asm volatile ("fence w, w");	//release semantics
	//#endif
	//void* dst = (void*)(buf+(*cpu_cons_valid_offset)+1);
	//write_mem(dst, last);

	//#ifdef ESP
	asm volatile ("fence w, w");	//release semantics
	//#endif
	//printf("Update cpu cons valid (%d)\n", *cpu_cons_valid_offset);
	void* dst = (void*)(buf+(*cpu_cons_valid_offset));
	int64_t value_64 = 1+last;
	write_mem(dst, value_64);


//#if defined(ESP) && COH_MODE==1
//	int time_var = 0;
//	while(time_var<100) time_var++;
//#endif
	asm volatile ("fence w, w");	
}


static inline void update_cons_rdy(){
	asm volatile ("fence w, w");	//acquire semantics
	void* dst = (void*)(buf+(*cpu_cons_ready_offset));
	int64_t value_64 = 0;
	write_mem(dst, value_64);
//#if defined(ESP) && COH_MODE==1
//	int time_var = 0;
//	while(time_var<100) time_var++;
//#endif
	asm volatile ("fence w, w");	
}

static inline void update_prod_rdy(){
	asm volatile ("fence w, w");	//acquire semantics
	void* dst = (void*)(buf+(*cpu_prod_ready_offset));
	int64_t value_64 = 1; 
	write_mem(dst, value_64);
//#if defined(ESP) && COH_MODE==1
//	int time_var = 0;
//	while(time_var<100) time_var++;
//#endif
	asm volatile ("fence w, w");	
}

static inline void update_prod_valid(){
	asm volatile ("fence w, w");	//release semantics
	void* dst = (void*)(buf+(*cpu_prod_valid_offset));
	int64_t value_64 = 0; 
	write_mem(dst, value_64);
//#if defined(ESP) && COH_MODE==1
//	int time_var = 0;
//	while(time_var<100) time_var++;
//#endif
	asm volatile ("fence w, w");	//acquire semantics
}



static inline void update_con_last(){
	void* dst = (void*)(buf+(*cpu_cons_valid_offset)+1);
	int64_t value_64 = 1; 
	write_mem(dst, value_64);	
}
static inline void update_con_not_last(){
	void* dst = (void*)(buf+(*cpu_cons_valid_offset)+1);
	int64_t value_64 = 0; 
	write_mem(dst, value_64);
}

static inline void write_to_cons(){
	void *dst = (void*)(&buf[input_buffer_offset[0]]);
	int64_t val_64 = 123; 
	int local_tile_size = tile_size;
	for (int j = 0; j < local_tile_size; j++){
		write_mem(dst, val_64);
		dst += 8;
	}
}

static inline void read_from_prod(){
	void *src = (void*)(buf+output_buffer_offset[num_devices-1]);
	int64_t out_val;
	int local_tile_size = tile_size;
	for (int j = 0; j < local_tile_size; j++){
			out_val = read_mem(src);
			//printf("%ld\n", out_val);
			src += 8;
	}
}


// static inline 
// mode: 0: reg inv
// mode: 1: chaining
// mode: 2: pipelining
void synth_accel_asi(int mode){
	 t_cpu_write=0;
	 t_acc=0;
	 t_cpu_read=0;
	 t_sw=0;
	 t_total=0;
	int local_num_tiles = (mode==MODE_REG_INV)? 1 : num_tiles;
	int iterations = (mode==MODE_REG_INV)? num_tiles : 1;
#ifdef CFA
	int pipeline_depth = (mode==MODE_PIPE)?num_devices-1:0;
#else
	int pipeline_depth = (mode==MODE_PIPE)?comp_stages-1:0;
#endif
	if(mode == MODE_PIPE)
	if (pipeline_depth > local_num_tiles) {
		pipeline_depth = 0; //local_num_tiles;
		mode = MODE_CHAIN;
	}
	//printf(" Pipeline Depth: %d\n", pipeline_depth);
	read_from_prod(); //dummy read for ownership

	int64_t total_time_start = get_counter();
	int64_t temp, temp2, temp3;
	int n_dev = 0; 

	if(mode==MODE_REG_INV){
		for(int reg_iters = 0; reg_iters < iterations; reg_iters++){
			// printf("reg iter %d/%d\n", reg_iters, iterations);
			temp = get_counter();
			write_to_cons();
			temp2 = get_counter();
			//update_con_last();	
			//update_cons_valid(0);
			update_cons_valid(1);
			// update_prod_rdy();
			temp3 = get_counter();
			t_cpu_write += (temp2-temp);
			t_sw += (temp3-temp2);
			for(n_dev = 0; n_dev< num_devices; n_dev++){
				write_mem(((void*)(buf + accel_cons_ready_offset[n_dev])), 1);
			}
			asm volatile ("fence w, w");

			int64_t accel_time_start = get_counter();
			for (n_dev = 0; n_dev < num_devices; n_dev++){
#ifdef BAREMETAL_TEST
				struct esp_device *dev = &espdevs[n_dev];
				iowrite32(dev, CMD_REG, CMD_MASK_START);
				unsigned done = 0;
				while (!done) {
					done = ioread32(dev, STATUS_REG);
					done &= STATUS_MASK_DONE;
				}
				iowrite32(dev, CMD_REG, 0x0);
#else
				esp_run(cfg_000+n_dev, NACC);
#endif
			}
			temp2 = get_counter();
			t_acc += (temp2-accel_time_start);

			temp = get_counter();
			read_from_prod();
			temp2 = get_counter();
			t_cpu_read += (temp2-temp);
		}
	} // mode REG INV ends
	else{
		update_prod_rdy();
		for (n_dev = 0; n_dev < num_devices; n_dev++){
#ifdef BAREMETAL_TEST
//printf("Starting accels");
//print_flags();
			struct esp_device *dev = &espdevs[n_dev];
			iowrite32(dev, CMD_REG, CMD_MASK_START);
#else
			esp_run(cfg_000+n_dev, NACC);
#endif
		}


		total_time_start = get_counter();
//		if(mode == MODE_PIPE){
//			int input_tile = 0;
//			int output_tile = 0;
//			int accel_status = 0;
//			int64_t last_tile = 0;
//			update_prod_rdy();
//			poll_cons_rdy();
//			poll_prod_valid();
//			// while(!last_tile){
//			while(input_tile < local_num_tiles || output_tile< local_num_tiles){
//				accel_status = 0;
//				#ifdef TIMERS
//				int64_t accel_time_start = get_counter();
//				#endif
//				// // int toggle = 0;
//				// if(input_tile < MAX_DEVICES){
//				// 	while(!poll_cons_rdy());
//				// 	accel_status = 1;
//				// }
//				// else 
//				//printf("Poll for cons rdy or prod valid\n");
//				while(!accel_status){
//					// // accel_status = accel_rdy(toggle);
//
//					// if(input_tile<2 || ((input_tile < local_num_tiles) && toggle)){
//					// // if(((input_tile < local_num_tiles) && toggle)){
//					// 	if(poll_cons_rdy()) accel_status = 1;
//					// }
//					// else{
//					// 	if(poll_prod_valid()) accel_status = 2;
//					// }
//					// toggle = !toggle;
//
//					if(input_tile < local_num_tiles && poll_cons_rdy()) accel_status = 1;
//					else if(poll_prod_valid()) accel_status = 2;
//				}
//				#ifdef TIMERS
//				temp2 = get_counter();
//				t_acc += (temp2-accel_time_start);
//				#endif
//				if(accel_status == 1){ // consumer ready //&& input_tile < local_num_tiles
//					// #ifdef TIMERS
//					// temp3 = get_counter();
//					// #endif
//					// update_cons_rdy();
//					#ifdef TIMERS
//					temp = get_counter();
//					#endif
//					write_to_cons();
//
//					#ifdef TIMERS
//					temp2 = get_counter();
//					t_cpu_write += (temp2-temp);
//					// t_sw += (temp - temp3);
//
//					temp = get_counter();
//					#endif
//					// #ifdef TIMERS
//					// temp3 = get_counter();
//					// #endif
//					update_cons_rdy();
//					// #ifdef TIMERS
//					// temp = get_counter();
//					// #endif
//					//printf("Write to Consumer %d\n", input_tile);
//					int last = 0;
//					if(input_tile==local_num_tiles-1){
//						// printf("last\n");
//						last = 1;
//						// update_con_last();	
//					}
//					// else
//					// 	update_con_not_last();
//					update_cons_valid(last);
//					#ifdef TIMERS
//					temp2 = get_counter();
//					t_sw += (temp2 - temp);
//					#endif
//					input_tile++;
//				} else if(accel_status == 2){ // producer ready
//					void* dst = (void*)(buf + (*cpu_prod_valid_offset));
//					last_tile = (read_mem(dst)==2);
//					//last_tile = (poll_prod_valid()==2); //(output_tile == local_num_tiles-1);
//
//					#ifdef TIMERS
//					temp = get_counter();
//					#endif
//					update_prod_valid();
//					#ifdef TIMERS
//					temp2 = get_counter();
//					t_sw += (temp2 - temp);
//					temp = get_counter();
//					#endif
//					read_from_prod();
//					//printf("Read from Producer %d, last:%ld\n", output_tile, last_tile);
//					#ifdef TIMERS
//					temp2 = get_counter();
//					t_cpu_read += (temp2-temp);
//					temp3 = get_counter();
//					#endif
//					update_prod_rdy();
//					#ifdef TIMERS
//					temp = get_counter();
//					t_sw += (temp - temp3);
//					#endif
//					output_tile ++;
//				}
//				accel_status = 0;
//			}
//			// // Drain pipeline
//			// for(; output_tile < local_num_tiles; output_tile++){
//			// 	// asm volatile ("fence rw, w");
//			// 	int64_t accel_time_start = get_counter();
//			// 	asm volatile ("fence w, r");
//			// 	while(!poll_prod_valid()); //Wait for Output
//			// 	temp2 = get_counter();
//			// 	t_acc += (temp2-accel_time_start);
//			// 	update_prod_valid();
//			// 	temp = get_counter();
//			// 	read_from_prod();
//			// 	temp2 = get_counter();
//			// 	t_cpu_read += (temp2-temp);
//			// 	update_prod_rdy();
//			// }
//		}
//			else
		{
		// Fill pipeline first
		int tile = 0;
	//	if(mode==MODE_PIPE){
			//printf("Pipelining\n");
		 	for(; tile < pipeline_depth; tile++){
//#if defined(ESP) 
//				//&& (COH_MODE==1)
//		 		asm volatile ("fence rw, rw");
//#endif
		 		int64_t accel_time_start = get_counter();
		 		while(!poll_cons_rdy());
//printf("===tile:%d\n", tile);
//print_flags();
		 		temp2 = get_counter();
		 		t_acc += (temp2-accel_time_start);
		 		update_cons_rdy();

		 		temp = get_counter();
		 		write_to_cons();
#if defined(ESP)
			        //	&& (COH_MODE==1)
		 		asm volatile ("fence rw, rw");
#endif
		 		//printf("Write to Consumer %d\n", tile);

		 		temp2 = get_counter();
		 	       	int last = 0;
		 	       	if(tile==local_num_tiles-1)last = 1;
		 		t_cpu_write += (temp2-temp);
#if defined(ESP) 
				//&& (COH_MODE==1)
		 		asm volatile ("fence rw, w");
#endif
	
//print_flags();
				update_cons_valid(last);
//print_flags();
		//	//uncommented apr5
#if defined(ESP) //&& (COH_MODE==1) 
				//&& (COH_MODE==1)
		 		asm volatile ("fence rw, w");
#endif
		 		//printf("Update cons valid %d\n", tile);
		 	}
			//printf("filled pipeline\n");
		//	//added apr5
//#if defined(ESP) && (COH_MODE==1)
//			asm volatile ("fence rw, rw");
//#endif
//		}
//print_flags();
		// Steady State // Chain
		for(; tile < local_num_tiles; tile++){
			int64_t accel_time_start = get_counter();
//Commented apr5
//#if defined(ESP) && (COH_MODE==1)
//			asm volatile ("fence rw, r");
//#else
//			asm volatile ("fence rw, r");
//#endif
			//printf("wait for cons rdy\n");
			
			while(!poll_cons_rdy());//print_flags();
			//print_flags();
			temp2 = get_counter();
			t_acc += (temp2-accel_time_start);

//#if defined(ESP) && (COH_MODE==1)
//			asm volatile ("fence rw, w");
//#endif

			update_cons_rdy();

//#if defined(ESP) && (COH_MODE==1)
//			asm volatile ("fence rw, w");
//#endif

			temp = get_counter();
			write_to_cons();

			//printf("Write to Consumer %d\n", tile);

#if defined(ESP) 
//&& (COH_MODE==1)
			asm volatile ("fence rw, rw");
#endif

			temp2 = get_counter();
			t_cpu_write += (temp2-temp);
			int last = 0;
			if(tile==local_num_tiles-1)
				last = 1;

			// 	update_con_last();	
			// else
			// 	update_con_not_last();

//#if defined(ESP) && (COH_MODE==1)
//			asm volatile ("fence rw, rw");
//#endif

			update_cons_valid(last);
			//print_flags();

		//	printf("Produced input\n");

			//update_prod_rdy();
	
			accel_time_start = get_counter();
			//printf("wait for prod valid\n");

#if defined(ESP) && (COH_MODE==1)
			asm volatile ("fence rw, rw");
#endif
			while(!poll_prod_valid());//print_flags(); //Wait for Output
			//print_flags();
			temp2 = get_counter();
			t_acc += (temp2-accel_time_start);

#if defined(ESP) && (COH_MODE==1)
			asm volatile ("fence rw, rw");
#endif

			update_prod_valid();

#if defined(ESP) && (COH_MODE==1)
			asm volatile ("fence rw, rw");
#endif
			temp = get_counter();
			read_from_prod();
			temp2 = get_counter();
			t_cpu_read += (temp2-temp);

			//printf("Consumed output %d\n", tile);

//		 	asm volatile ("fence rw, w");
//uncommented apr5 //needed for cohdma
#if defined(ESP) && (COH_MODE==1)
			asm volatile ("fence rw, w");
#endif

			if(tile < local_num_tiles+pipeline_depth-1)
			update_prod_rdy();
//print_flags();
//uncommented apr5
//#if defined(ESP) && (COH_MODE==1)
//			asm volatile ("fence rw, rw");
//#endif

		}
		// Drain pipeline
//		if(mode==MODE_PIPE){
	//printf("drain pipeline\n");
//uncommented apr5
//#if defined(ESP) && (COH_MODE==1)
//			asm volatile ("fence rw, rw");
//#endif
		 	for(; tile < local_num_tiles+pipeline_depth; tile++){
		 	//	asm volatile ("fence rw, rw");
		 		//update_prod_rdy();
			//	printf("Drained output %d\n", tile);

		 		//asm volatile ("fence rw, r");
		 		int64_t accel_time_start = get_counter();
		 		//asm volatile ("fence w, r");
#if defined(ESP) && (COH_MODE==1)
			asm volatile ("fence rw, rw");
#endif
		 		while(!poll_prod_valid()); //Wait for Output
				
				//print_flags();
		 		temp2 = get_counter();
		 		t_acc += (temp2-accel_time_start);

#if defined(ESP) && (COH_MODE==1)
			asm volatile ("fence rw, rw");
#endif
				//printf("Drained output %d\n", tile-pipeline_depth);

		 		update_prod_valid();
		 		temp = get_counter();
		 		read_from_prod();
		 		temp2 = get_counter();
		 		t_cpu_read += (temp2-temp);
		 		asm volatile ("fence rw, w");
				if(tile < local_num_tiles+pipeline_depth-1){
		 			update_prod_rdy();
		 			asm volatile ("fence rw, w");
				}
		 	}
		}
	//		printf("drained pipeline\n");
//		}

	} // end of else (Reg Inv)

	temp2 = get_counter();
	t_total = temp2 - total_time_start;

	// for(n_dev = 0; n_dev < num_devices; n_dev++) printf("Last item for %d: %ld\n",n_dev,buf[accel_cons_valid_offset[n_dev]+1]);
}

#endif
