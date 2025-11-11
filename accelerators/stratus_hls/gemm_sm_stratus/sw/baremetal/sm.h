// Size and parameter defines
#define SM_INFO_SIZE 8

// Context descriptors status
#define QUEUE_INVALID 0
#define QUEUE_AVAIL 1
#define QUEUE_BUSY 2

#define GEMM_QUEUE_SIZE 4

// Queue layout parameters (words, 32-bit)
#define QUEUE_ENTRY_SIZE 2
#define ENTRY_OFFSET 6
#define SM_QUEUE_WORDS (ENTRY_OFFSET + (QUEUE_ENTRY_SIZE * GEMM_QUEUE_SIZE))

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
