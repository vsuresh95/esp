#include <stdint.h>
#include <encoding.h>
#include <stdio.h>

#ifndef INIT_H
#define INIT_H

#define MTIME           (*(volatile long long *)(0x02000000 + 0xbff8))
#define MTIMECMP        ((volatile long long *)(0x02000000 + 0x4000))
#define NHARTS 2

#define csr_read(csr)                                   \
({                                                      \
    register unsigned long __v;                         \
    __asm__ __volatile__ ("csrr %0, " #csr              \
                  : "=r" (__v));                        \
    __v;                                                \
})

typedef void* (*trap_handler_t)(unsigned hartid, unsigned mcause, void *mepc,
        void *sp);
void set_trap_handler(trap_handler_t handler);
void enable_timer_interrupts();

#endif

trap_handler_t trap_handler[NHARTS] = {0};

void set_trap_handler(trap_handler_t handler)
{
    unsigned hartid = csr_read(mhartid);
    trap_handler[hartid] = handler;
}

void enable_timer_interrupts()
{
    set_csr(mie, MIP_MTIP);
    set_csr(mstatus, MSTATUS_MIE);
}

void handle_trap(unsigned int mcause, void *mepc, void *sp)
{
    unsigned hartid = csr_read(mhartid);
    // printf("%0d trap %0d mcause\n", hartid, mcause);
    if (trap_handler[hartid]) {
        trap_handler[hartid](hartid, mcause, mepc, sp);
        return;
    }

    while (1)
        ;
}

typedef struct {
    int counter;
} atomic_t;

static inline int atomic_xchg(atomic_t *v, int n)
{
    register int c;

    __asm__ __volatile__ (
            "amoswap.w.aqrl %0, %2, %1"
            : "=r" (c), "+A" (v->counter)
            : "r" (n));
    return c;
}

static inline void mb(void)
{
    __asm__ __volatile__ ("fence");
}

void get_lock(atomic_t *lock)
{
    while (atomic_xchg(lock, 1) == 1)
        ;
    mb();
}

void put_lock(atomic_t *lock)
{
    mb();
    atomic_xchg(lock, 0);
}

static atomic_t buf_lock = { .counter = 0 };
static char buf[32];
static int buf_initialized;
static int shared_count;
static unsigned hart_count[NHARTS];
static unsigned interrupt_count[NHARTS];

static unsigned delta = 0x100;
void *increment_count(unsigned hartid, unsigned mcause, void *mepc, void *sp)
{
    interrupt_count[hartid]++;
    MTIMECMP[hartid] = MTIME + delta;
    return mepc;
}

int riscv_multicore ()
{
    uint32_t hartid = csr_read(mhartid);
    hart_count[hartid] = 0;
    interrupt_count[hartid] = 0;
    buf_initialized = 0;
    set_trap_handler(increment_count);
    // Despite being memory-mapped, there appears to be one mtimecmp
    // register per hart. The spec does not address this.
    MTIMECMP[hartid] = MTIME + delta;
    enable_timer_interrupts();

    for (uint64_t i = 0; i < 100; i++) {
        __asm__ __volatile__ ("nop");
    }

    shared_count = 0;
    while (1) {
        get_lock(&buf_lock);
        printf("my hartid is %d\n", hartid);
        if (!buf_initialized) {
            for (unsigned i = 0; i < sizeof(buf); i++) {
                buf[i] = 'A' + (i % 26);
            }
            buf_initialized = 1;
        }
        char first = buf[0];
        int offset = (first & ~0x20) - 'A';
        for (unsigned i = 0; i < sizeof(buf); i++) {
            while (buf[i] != (first - offset + ((offset + i) % 26)))
                printf("buf[%d] = %d, exp %d\n", i, buf[i], (first - offset + ((offset + i) % 26)));
            if (hartid & 1)
                buf[i] = 'A' + ((i + hartid + hart_count[hartid]) % 26);
            else
                buf[i] = 'a' + ((i + hartid + hart_count[hartid]) % 26);
        }
        shared_count++;
        hart_count[hartid]++;
        int total = 0;
        for (int i = 0; i < NHARTS; i++) {
            total += hart_count[i];
        }
        if (total != shared_count){
            printf("ERR ERR ERR total: %d, shared: %d!\n", total, shared_count);
            break;
        }
        printf("count is %d\n", shared_count);
        put_lock(&buf_lock);
    }
}
