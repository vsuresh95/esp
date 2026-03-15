#ifndef CONV_AMU_H
#define CONV_AMU_H

// Size and parameter defines
#define CONF_INFO_SIZE 16

// Context descriptors status
#define QUEUE_INVALID 0
#define QUEUE_AVAIL 1
#define QUEUE_BUSY 2

#define SM_QUEUE_SIZE 4

// Queue layout parameters (words, 32-bit)
#define QUEUE_ENTRY_SIZE 2
#define ENTRY_OFFSET 6
#define SM_QUEUE_WORDS (ENTRY_OFFSET + (QUEUE_ENTRY_SIZE * SM_QUEUE_SIZE))

#define N_THREADS 2

typedef struct {
    uint64_t stat;
    uint64_t head;
    uint64_t tail;
    uint64_t entry[SM_QUEUE_SIZE];
} sm_queue_t;

static inline void sm_queue_init(sm_queue_t *q) {
    __atomic_store_n(&(q->stat), QUEUE_AVAIL, __ATOMIC_SEQ_CST);
    __atomic_store_n(&(q->head), 0, __ATOMIC_SEQ_CST);
    __atomic_store_n(&(q->tail), 0, __ATOMIC_SEQ_CST);
    for (unsigned i = 0; i < SM_QUEUE_SIZE; i++) {
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
    return (head - tail) >= SM_QUEUE_SIZE;
}

static inline void sm_queue_push(sm_queue_t *q, uint64_t value) {
    uint64_t head = __atomic_load_n(&(q->head), __ATOMIC_ACQUIRE);

    q->entry[head % SM_QUEUE_SIZE] = value;
    __atomic_thread_fence(__ATOMIC_RELEASE);
    __atomic_store_n(&(q->head), head + 1, __ATOMIC_RELEASE);
}

static inline uint64_t sm_queue_pop(sm_queue_t *q) {
    uint64_t tail = __atomic_load_n(&(q->tail), __ATOMIC_ACQUIRE);

    uint64_t value = q->entry[tail % SM_QUEUE_SIZE];
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

int conv2d_amu()
{
	int i;
	int n;
	int ndev;
	struct esp_device *espdevs;
	struct esp_device *dev;
	unsigned done;
	unsigned **ptable[N_THREADS] = {NULL};
	unsigned *mem[N_THREADS] = {NULL};
	unsigned errors[N_THREADS] = {0};
	unsigned coherence;

	// Input data and golden output (aligned to DMA_WIDTH makes your life easier)
	if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
	    in_words_adj = n_channels * feature_map_height * feature_map_width;
	    weights_words_adj = n_filters * n_channels * filter_height * filter_width;
	    bias_words_adj = n_filters;
	    out_words_adj = n_filters * feature_map_height * feature_map_width;
	} else {
	    in_words_adj = round_up(n_channels * feature_map_height * feature_map_width,
				    DMA_WORD_PER_BEAT(sizeof(token_t)));
	    weights_words_adj = round_up(n_filters * n_channels * filter_height * filter_width,
					 DMA_WORD_PER_BEAT(sizeof(token_t)));
	    bias_words_adj = round_up(n_filters, DMA_WORD_PER_BEAT(sizeof(token_t)));
	    out_words_adj = round_up(n_filters * feature_map_height * feature_map_width,
				     DMA_WORD_PER_BEAT(sizeof(token_t)));
	}

	in_len = in_words_adj * (1);
	weights_len = weights_words_adj * (1);
	bias_len = bias_words_adj * (1);
	out_len = out_words_adj * (1);
	weights_offset = in_len;
	bias_offset = in_len + weights_len;
	out_offset  = in_len + weights_len + bias_len;

    // Queue and descriptor placement (in 32-bit words)
    unsigned input_queue_offset = out_offset + out_len;
    unsigned output_queue_offset = input_queue_offset + SM_QUEUE_WORDS;
    unsigned descriptor_offset = output_queue_offset + SM_QUEUE_WORDS;
	// SM queue pointers
	sm_queue_t *input_q[N_THREADS] = {NULL};
	sm_queue_t *output_q[N_THREADS] = {NULL};
	// Descriptor base pointer per thread
	unsigned *descriptor_base[N_THREADS] = {NULL};
    unsigned mem_words = descriptor_offset + CONF_INFO_SIZE;
    unsigned mem_size = N_THREADS * (mem_words * sizeof(token_t));

	// Search for the device
	printf("Scanning device tree... \n");

	ndev = probe(&espdevs, VENDOR_SLD, SLD_CONV2D, DEV_NAME);
	if (ndev == 0) {
		printf("conv2d not found\n");
		return 0;
	}

	dev = &espdevs[0];

	// Check DMA capabilities
	if (ioread32(dev, PT_NCHUNK_MAX_REG) == 0) {
		printf("  -> scatter-gather DMA is disabled. Abort.\n");
		return 0;
	}

	if (ioread32(dev, PT_NCHUNK_MAX_REG) < NCHUNK(mem_size)) {
		printf("  -> Not enough TLB entries available. Abort.\n");
		return 0;
	}

	// Allocate memory
	for (i = 0; i < N_THREADS; i++) {
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

	coherence = ACC_COH_RECALL;

	// Pass common configuration parameters
	iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	iowrite32(dev, COHERENCE_REG, coherence);
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
		desc[2] = n_channels;
		desc[3] = n_filters;
		desc[4] = filter_height;
		desc[5] = stride_w;
		desc[6] = is_padded;
		desc[7] = feature_map_height;
		desc[8] = feature_map_width;
		desc[9] = do_relu;
		desc[10] = pool_type;
		desc[11] = batch_size;
		desc[12] = 0x0;
		desc[13] = in_len;
		desc[14] = in_len + weights_len;
		desc[15] = in_len + weights_len + bias_len;
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
	}

	printf("  Errors = \n");
	for (i = 0; i < N_THREADS; i++)
		printf("%d, ", errors[i]);
	printf("\n");

	return 0;
}

#endif // CONV_AMU_H