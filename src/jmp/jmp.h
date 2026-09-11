#ifndef _GAME_JMP_H
#define _GAME_JMP_H

#include <dolphin/types.h>

#ifdef __MWERKS__

typedef struct gc_jmp_buf {
    u32 lr;
    u32 cr;
    u32 sp;
    u32 r2;
    u32 pad;
    u32 regs[19];
    double flt_regs[19];
} gc_jmp_buf;

#elif defined(_WIN32)

#include <windows.h>

typedef struct {
    CRITICAL_SECTION cs;
    CONDITION_VARIABLE cond;
    volatile LONG runnable;
    volatile LONG status;
    volatile LONG started;
    volatile LONG exiting;
    HANDLE thread;
} gc_jmp_buf;

#else

#include <pthread.h>

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    volatile int runnable;
    volatile int status;
    volatile int started;
    volatile int exiting;
    pthread_t thread;
} gc_jmp_buf;

#endif

s32 gcsetjmp(gc_jmp_buf *jump);
s32 gclongjmp(gc_jmp_buf *jump, s32 status);

#ifndef __MWERKS__
void gcsetcontext(
    gc_jmp_buf *jump,
    void (*func)(void),
    void *stack,
    u32 stack_size
);
void gcExitToScheduler(gc_jmp_buf *scheduler, s32 status);
void gcJoinContext(gc_jmp_buf *jump);
#endif

#endif
