// Size and parameter defines
#define VALID_OFFSET 0
#define PAYLOAD_OFFSET 8
#define SM_INFO_SIZE 8

// Context descriptors status
#define QUEUE_INVALID 0
#define QUEUE_AVAIL 1
#define QUEUE_BUSY 2

#define SM_ENTRY_SIZE 6

typedef struct {
} sm_queue_entry_t;

typedef struct {
    uint64_t stat;
    uint64_t head;
    uint64_t tail;
} sm_queue_t;

static inline void sm_queue_init(sm_queue_t *q) {
    __atomic_store_n(&(q->stat), QUEUE_AVAIL, __ATOMIC_SEQ_CST);
    __atomic_store_n(&(q->head), 0, __ATOMIC_SEQ_CST);
    __atomic_store_n(&(q->tail), 0, __ATOMIC_SEQ_CST);
}

#define GEMM_PARAM_SIZE 6

// Task parameters for GEMM
typedef struct {
    // Parameters
    unsigned dim_m;
    unsigned dim_n;
    unsigned dim_k;
    unsigned weight_base;
    unsigned input_base;
    unsigned output_base;
} gemm_params_t;

#define GEMM_QUEUE_SIZE 4
#define GEMM_ENTRY_SIZE GEMM_PARAM_SIZE

typedef struct {
    sm_queue_entry_t common;
    gemm_params_t gemm_params;
} gemm_queue_entry_t;

typedef struct {
    sm_queue_t info;
    gemm_queue_entry_t entry[GEMM_QUEUE_SIZE];
} gemm_queue_t;

static inline bool gemm_queue_push(gemm_queue_t *q, gemm_queue_entry_t *e) {
    unsigned head = __atomic_load_n(&(q->info.head), __ATOMIC_ACQUIRE);
    unsigned tail = __atomic_load_n(&(q->info.tail), __ATOMIC_ACQUIRE);

    // Full when advancing head would equal tail
    unsigned next = (head + 1) % GEMM_QUEUE_SIZE;
    if (next == tail) {
        return false;
    }

    // Copy params to head slot and advance head
    __atomic_thread_fence(__ATOMIC_ACQ_REL);
    gemm_queue_entry_t *slot = &(q->entry[head]);
    *slot = *e;
    __atomic_store_n(&(q->info.head), next, __ATOMIC_RELEASE);
    return true;
}
