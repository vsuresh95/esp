#ifndef GEMM_AMU_H
#define GEMM_AMU_H

// Size and parameter defines
#define CONF_INFO_SIZE 14

// Context descriptors status
#define QUEUE_INVALID 0
#define QUEUE_AVAIL 1
#define QUEUE_BUSY 2

#define GEMM_QUEUE_SIZE 4

// Queue layout parameters (words, 32-bit)
#define QUEUE_ENTRY_SIZE 2
#define ENTRY_OFFSET 6
#define SM_QUEUE_WORDS (ENTRY_OFFSET + (QUEUE_ENTRY_SIZE * GEMM_QUEUE_SIZE))

#define N_THREADS 2

typedef struct {
    uint64_t stat;
    uint64_t head;
    uint64_t tail;
    uint64_t entry[GEMM_QUEUE_SIZE];
} sm_queue_t;

static inline void sm_queue_init(sm_queue_t *q) {
    __atomic_store_n(&(q->stat), QUEUE_AVAIL, __ATOMIC_SEQ_CST);
    __atomic_store_n(&(q->head), 0, __ATOMIC_SEQ_CST);
    __atomic_store_n(&(q->tail), 0, __ATOMIC_SEQ_CST);
    for (unsigned i = 0; i < GEMM_QUEUE_SIZE; i++) {
        q->entry[i] = 0;
    }
}

static inline bool sm_queue_empty(const sm_queue_t *q) {
    uint64_t head = __atomic_load_n(&(q->head), __ATOMIC_ACQUIRE);
    uint64_t tail = __atomic_load_n(&(q->tail), __ATOMIC_ACQUIRE);
    return (head == tail);
}

static inline bool sm_queue_full(const sm_queue_t *q) {
    uint64_t head = __atomic_load_n(&(q->head), __ATOMIC_ACQUIRE);
    uint64_t tail = __atomic_load_n(&(q->tail), __ATOMIC_ACQUIRE);
    return (head - tail) >= GEMM_QUEUE_SIZE;
}

static inline void sm_queue_push(sm_queue_t *q, uint64_t value) {
    uint64_t head = __atomic_load_n(&(q->head), __ATOMIC_ACQUIRE);

    q->entry[head % GEMM_QUEUE_SIZE] = value;
    __atomic_thread_fence(__ATOMIC_RELEASE);
    __atomic_store_n(&(q->head), head + 1, __ATOMIC_RELEASE);
}

static inline uint64_t sm_queue_pop(sm_queue_t *q) {
    uint64_t tail = __atomic_load_n(&(q->tail), __ATOMIC_ACQUIRE);

    uint64_t value = q->entry[tail % GEMM_QUEUE_SIZE];
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
    __atomic_store_n(&(q->tail), tail + 1, __ATOMIC_RELEASE);
    return value;
}

static inline uint64_t get_counter() {
	uint64_t t_current;
	asm volatile (
		"li t0, 0;"
		"csrr t0, mcycle;"
		"mv %0, t0"
		: "=r" (t_current)
		:
		: "t0"
	);
	return t_current;
}

static inline bool need_to_delay (uint64_t *start_cycles, uint64_t delay) {
	uint64_t curr_cycles = get_counter();
	if ((curr_cycles - *start_cycles) < delay) {
		return true;
	} else {
		return false;
	}
}

void gemm_amu() {
	printf("Starting gemm_amu...\n");
	int i, n;
	int ndev;
	struct esp_device *espdevs;
	struct esp_device *dev;
	unsigned done;
	unsigned spin_ct;
	unsigned **ptable[N_THREADS] = {NULL};
	unsigned *mem[N_THREADS] = {NULL};
	float *gold[N_THREADS] = {NULL};
	unsigned errors[N_THREADS] = {0};
	unsigned coherence;

	printf("dim_m %u dim_n %u dim_k %u\n", dim_m, dim_n, dim_k);
    unsigned mat_a_len = dim_m * dim_k;
    unsigned mat_b_len = dim_n * dim_k;
    unsigned mat_c_len = dim_m * dim_n;
    // Data offsets
    unsigned mat_a_offset = 0;
    unsigned mat_b_offset = mat_a_offset + mat_a_len;
    unsigned mat_c_offset = mat_b_offset + mat_b_len;
    // Queue and descriptor placement (in 32-bit words)
    unsigned input_queue_offset = mat_c_offset + mat_c_len;
    unsigned output_queue_offset = input_queue_offset + SM_QUEUE_WORDS;
    unsigned descriptor_offset = output_queue_offset + SM_QUEUE_WORDS;
    unsigned mem_words = descriptor_offset + CONF_INFO_SIZE;
    unsigned mem_size = N_THREADS * (mem_words * sizeof(unsigned));
	// SM queue pointers
	sm_queue_t *input_q[N_THREADS] = {NULL};
	sm_queue_t *output_q[N_THREADS] = {NULL};
	// Descriptor base pointer per thread
	unsigned *descriptor_base[N_THREADS] = {NULL};

	// Search for the device
	ndev = probe(&espdevs, VENDOR_SLD, SLD_GEMM, DEV_NAME);
	if (ndev == 0) {
		printf("%s not found\n", DEV_NAME);
		return;
	}

	printf("**************** %s.0 ****************\n", DEV_NAME);

	dev = &espdevs[0];

	// Check DMA capabilities
	if (ioread32(dev, PT_NCHUNK_MAX_REG) == 0) {
		printf("  -> scatter-gather DMA is disabled. Abort.\n");
		return;
	}

	if (ioread32(dev, PT_NCHUNK_MAX_REG) < NCHUNK(mem_size)) {
		printf("  -> Not enough TLB entries available. Abort.\n");
		return;
	}

	// Allocate memory
	for (i = 0; i < N_THREADS; i++) {
		gold[i] = aligned_malloc((mat_c_offset + mat_c_len) * sizeof(float));
		mem[i] = aligned_malloc(mem_size);

		// Allocate and populate page table
		ptable[i] = aligned_malloc(NCHUNK(mem_size) * sizeof(unsigned *));
		for (n = 0; n < NCHUNK(mem_size); n++)
			ptable[i][n] = (unsigned *) &mem[i][n * (CHUNK_SIZE / sizeof(unsigned))];
		
        input_q[i] = (sm_queue_t *) &mem[i][input_queue_offset];
        sm_queue_init(input_q[i]);
        output_q[i] = (sm_queue_t *) &mem[i][output_queue_offset];
        sm_queue_init(output_q[i]);
        descriptor_base[i] = &mem[i][descriptor_offset];
	}

	// Pass common configuration parameters
	coherence = ACC_COH_RECALL;
	iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	iowrite32(dev, COHERENCE_REG, coherence);

	iowrite32(dev, PT_ADDRESS_REG, (unsigned long long) ptable[0]);
	iowrite32(dev, PT_NCHUNK_REG, NCHUNK(mem_size));
	iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);

	// Use the following if input and output data are not allocated at the default offsets
	iowrite32(dev, SRC_OFFSET_REG, 0x0);
	iowrite32(dev, DST_OFFSET_REG, 0x0);

	// Flush (customize coherence model here)
	esp_flush(coherence);

	// Configure first context
	iowrite32(dev, AMU_INFO_QUEUE_PTR_REG_0, input_queue_offset);
	iowrite32(dev, PT_ADDRESS_REG_0, (unsigned long long) ptable[0]);
	iowrite32(dev, AMU_INFO_NPRIO_REG_0, 1);
	iowrite32(dev, AMU_INFO_VLD_CTXT_REG, 0x1);
	iowrite32(dev, AMU_INFO_SCHED_PERIOD_REG, 10000); // in cycles
	// Start accelerators
	iowrite32(dev, CMD_REG, CMD_MASK_START);
	printf("First context configured\n");

	// Configure second context
	iowrite32(dev, AMU_INFO_QUEUE_PTR_REG_1, input_queue_offset);
	iowrite32(dev, PT_ADDRESS_REG_1, (unsigned long long) ptable[1]);
	iowrite32(dev, AMU_INFO_NPRIO_REG_1, 2);
	iowrite32(dev, AMU_INFO_VLD_CTXT_REG, 0x3);
	printf("Second context configured\n");

	unsigned t_id = 0;
	unsigned iterations[N_THREADS];
	iterations[0] = 20;
	iterations[1] = 50;

	unsigned outputs_remaining[N_THREADS];
	unsigned inputs_remaining[N_THREADS];
	for (i = 0; i < N_THREADS; i++) {
		outputs_remaining[i] = iterations[i];
		inputs_remaining[i] = iterations[i];
	}

    unsigned thread_status[N_THREADS];
    for (unsigned i = 0; i < N_THREADS; i++) {
        thread_status[i] = 0;
    }
    unsigned threads_done = 0;
	uint64_t start_cycles[N_THREADS];
	for (i = 0; i < N_THREADS; i++) {
		start_cycles[i] = get_counter();
	}
	uint64_t period[N_THREADS];
	period[0] = 4 * 1000; // in cycles
	period[1] = 1000; // in cycles

	// Initialize descriptors
	for (i = 0; i < N_THREADS; i++) {
		unsigned *desc = descriptor_base[i];
		desc[0] = output_queue_offset;
		desc[1] = 0; // test does not do anything with descriptor pointer
		desc[2] = 1;
		desc[3] = dim_m;
		desc[4] = dim_k;
		desc[5] = dim_n;
		desc[6] = mat_a_offset;
		desc[7] = mat_b_offset;
		desc[8] = 0;
		desc[9] = mat_c_offset;
		desc[10] = 0;
		desc[11] = 0;
		desc[12] = 0;
		desc[13] = 0;
	}

	// Main processing loop
    while (threads_done < N_THREADS) {
		// Yield to other threads if applicable
		if (thread_status[t_id] == 1) {
			t_id = (t_id + 1) % N_THREADS;
			continue;
		}
		// Check if thread is done
		if (inputs_remaining[t_id] + outputs_remaining[t_id] == 0) {
			threads_done++;
			thread_status[t_id] = 1;
			printf("Thread %d done\n", t_id);
			t_id = (t_id + 1) % N_THREADS;
			continue;
		}
		// Check if input queue is full
		if (inputs_remaining[t_id] > 0) {
			if(!need_to_delay(&start_cycles[t_id], period[t_id])) {
				sm_queue_t *inq = input_q[t_id];
				if (!sm_queue_full(inq)) {
					uint64_t head = __atomic_load_n(&(inq->head), __ATOMIC_ACQUIRE);
					__atomic_thread_fence(__ATOMIC_RELEASE);
					sm_queue_push(inq, descriptor_offset);
					start_cycles[t_id] = get_counter();
					inputs_remaining[t_id]--;
				}
			}
		}
		// Check if output data is valid
		if (outputs_remaining[t_id] > 0) {
			sm_queue_t *out_q = output_q[t_id];
			if (!sm_queue_empty(out_q)) {
				uint64_t entry = sm_queue_pop(out_q);
				outputs_remaining[t_id]--;
			}
		}
		t_id = (t_id + 1) % N_THREADS;
	}

	for (i = 0; i < N_THREADS; i++) {
		printf("Freeing resources for thread %d\n", i);
		aligned_free(ptable[i]);
		aligned_free(mem[i]);
		aligned_free(gold[i]);
	}

	printf("  Errors = \n");
	for (i = 0; i < N_THREADS; i++)
		printf("%d, ", errors[i]);
	printf("\n");
}

#endif // GEMM_AMU_H
