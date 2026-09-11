#include "game/jmp.h"
#include <limits.h>
#include <stdlib.h>

#ifdef _WIN32

static DWORD s_armedBufKey;
static INIT_ONCE s_keyOnce = INIT_ONCE_STATIC_INIT;

static BOOL CALLBACK makeKey(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *Context)
{
    s_armedBufKey = TlsAlloc();
    return TRUE;
}

static void jmpBufInit(gc_jmp_buf *jump) {
    InitializeCriticalSection(&jump->cs);
    InitializeConditionVariable(&jump->cond);
    jump->runnable = 0;
    jump->status = 0;
    jump->exiting = 0;
}

static void jmpBufDestroy(gc_jmp_buf *jump) {
    DeleteCriticalSection(&jump->cs);
}

static s32 jmpBufWaitUntilRunnable(gc_jmp_buf *jump) {
    s32 status;
    EnterCriticalSection(&jump->cs);
    while (!jump->runnable) {
        SleepConditionVariableCS(&jump->cond, &jump->cs, INFINITE);
    }
    jump->runnable = 0;
    status = jump->status;
    LeaveCriticalSection(&jump->cs);
    return status;
}

static void jmpBufWake(gc_jmp_buf *jump, s32 status) {
    EnterCriticalSection(&jump->cs);
    jump->status = status;
    jump->runnable = 1;
    WakeConditionVariable(&jump->cond);
    LeaveCriticalSection(&jump->cs);
}

s32 gcsetjmp(gc_jmp_buf *jump)
{
    InitOnceExecuteOnce(&s_keyOnce, makeKey, NULL, NULL);
    if (!jump->started) {
        jmpBufInit(jump);
        jump->started = 1;
    }
    TlsSetValue(s_armedBufKey, jump);
    return 0;
}

s32 gclongjmp(gc_jmp_buf *jump, s32 status)
{
    gc_jmp_buf *mine;

    InitOnceExecuteOnce(&s_keyOnce, makeKey, NULL, NULL);
    jmpBufWake(jump, status);

    mine = (gc_jmp_buf *)TlsGetValue(s_armedBufKey);
    if (!mine) {
        return 0;
    }
    status = jmpBufWaitUntilRunnable(mine);
    if (mine->exiting) {
        ExitThread(0);
    }
    return status;
}

void gcExitToScheduler(gc_jmp_buf *scheduler, s32 status)
{
    InitOnceExecuteOnce(&s_keyOnce, makeKey, NULL, NULL);
    jmpBufWake(scheduler, status);
    ExitThread(0);
}

void gcJoinContext(gc_jmp_buf *jump)
{
    if (jump->started) {
        WaitForSingleObject(jump->thread, INFINITE);
        CloseHandle(jump->thread);
        jmpBufDestroy(jump);
        jump->started = 0;
        jump->exiting = 0;
    }
}

typedef struct {
    void (*func)(void);
    gc_jmp_buf *jump;
} BootstrapArgs;

static DWORD WINAPI gcThreadEntry(LPVOID arg)
{
    BootstrapArgs *args = (BootstrapArgs *)arg;
    void (*func)(void) = args->func;
    gc_jmp_buf *jump = args->jump;
    free(args);

    InitOnceExecuteOnce(&s_keyOnce, makeKey, NULL, NULL);
    TlsSetValue(s_armedBufKey, jump);

    jmpBufWaitUntilRunnable(jump);
    if (jump->exiting) {
        return 0;
    }

    func();

    for (;;) {
        jmpBufWaitUntilRunnable(jump);
        if (jump->exiting) {
            return 0;
        }
    }

    return 0;
}

void gcsetcontext(
    gc_jmp_buf *jump,
    void (*func)(void),
    void *stack,
    u32 stack_size
)
{
    BootstrapArgs *args;

    (void)stack;

    InitOnceExecuteOnce(&s_keyOnce, makeKey, NULL, NULL);

    if (jump->started) {
        jump->exiting = 1;
        jmpBufWake(jump, 0);
        WaitForSingleObject(jump->thread, INFINITE);
        CloseHandle(jump->thread);
        jmpBufDestroy(jump);
        jump->started = 0;
        jump->exiting = 0;
    } else {
        jump->exiting = 0;
        jump->runnable = 0;
        jump->status = 0;
    }

    jmpBufInit(jump);
    jump->started = 1;

    args = (BootstrapArgs *)malloc(sizeof(BootstrapArgs));
    if (!args) {
        abort();
    }
    args->func = func;
    args->jump = jump;

    {
        SIZE_T minStack = (SIZE_T)stack_size;
        const SIZE_T kPcMinStack = 256u * 1024u;
        if (minStack < kPcMinStack) {
            minStack = kPcMinStack;
        }

        jump->thread = CreateThread(NULL, minStack, gcThreadEntry, args, 0, NULL);
        if (!jump->thread) {
            abort();
        }
    }
}

#else

#include <pthread.h>

#ifndef PTHREAD_STACK_MIN
#define PTHREAD_STACK_MIN 16384
#endif

static pthread_key_t s_armedBufKey;
static pthread_once_t s_keyOnce = PTHREAD_ONCE_INIT;

static void makeKey(void) {
    pthread_key_create(&s_armedBufKey, NULL);
}

static void jmpBufInit(gc_jmp_buf *jump) {
    pthread_mutex_init(&jump->mutex, NULL);
    pthread_cond_init(&jump->cond, NULL);
    jump->runnable = 0;
    jump->status = 0;
    jump->exiting = 0;
}

static void jmpBufDestroy(gc_jmp_buf *jump) {
    pthread_mutex_destroy(&jump->mutex);
    pthread_cond_destroy(&jump->cond);
}

static s32 jmpBufWaitUntilRunnable(gc_jmp_buf *jump) {
    s32 status;
    pthread_mutex_lock(&jump->mutex);
    while (!jump->runnable) {
        pthread_cond_wait(&jump->cond, &jump->mutex);
    }
    jump->runnable = 0;
    status = jump->status;
    pthread_mutex_unlock(&jump->mutex);
    return status;
}

static void jmpBufWake(gc_jmp_buf *jump, s32 status) {
    pthread_mutex_lock(&jump->mutex);
    jump->status = status;
    jump->runnable = 1;
    pthread_cond_signal(&jump->cond);
    pthread_mutex_unlock(&jump->mutex);
}

s32 gcsetjmp(gc_jmp_buf *jump)
{
    pthread_once(&s_keyOnce, makeKey);
    if (!jump->started) {
        jmpBufInit(jump);
        jump->started = 1;
    }
    pthread_setspecific(s_armedBufKey, jump);
    return 0;
}

s32 gclongjmp(gc_jmp_buf *jump, s32 status)
{
    gc_jmp_buf *mine;

    pthread_once(&s_keyOnce, makeKey);
    jmpBufWake(jump, status);

    mine = (gc_jmp_buf *)pthread_getspecific(s_armedBufKey);
    if (!mine) {
        return 0;
    }
    status = jmpBufWaitUntilRunnable(mine);
    if (mine->exiting) {
        pthread_exit(NULL);
    }
    return status;
}

void gcExitToScheduler(gc_jmp_buf *scheduler, s32 status)
{
    pthread_once(&s_keyOnce, makeKey);
    jmpBufWake(scheduler, status);
    pthread_exit(NULL);
}

void gcJoinContext(gc_jmp_buf *jump)
{
    if (jump->started) {
        pthread_join(jump->thread, NULL);
        jmpBufDestroy(jump);
        jump->started = 0;
        jump->exiting = 0;
    }
}

typedef struct {
    void (*func)(void);
    gc_jmp_buf *jump;
} BootstrapArgs;

static void *gcThreadEntry(void *arg)
{
    BootstrapArgs *args = (BootstrapArgs *)arg;
    void (*func)(void) = args->func;
    gc_jmp_buf *jump = args->jump;
    free(args);

    pthread_once(&s_keyOnce, makeKey);
    pthread_setspecific(s_armedBufKey, jump);

    jmpBufWaitUntilRunnable(jump);
    if (jump->exiting) {
        return NULL;
    }

    func();

    for (;;) {
        jmpBufWaitUntilRunnable(jump);
        if (jump->exiting) {
            return NULL;
        }
    }

    return NULL;
}

void gcsetcontext(
    gc_jmp_buf *jump,
    void (*func)(void),
    void *stack,
    u32 stack_size
)
{
    BootstrapArgs *args;
    pthread_attr_t attr;

    (void)stack;

    pthread_once(&s_keyOnce, makeKey);

    if (jump->started) {
        jump->exiting = 1;
        jmpBufWake(jump, 0);
        pthread_join(jump->thread, NULL);
        jmpBufDestroy(jump);
        jump->started = 0;
        jump->exiting = 0;
    } else {
        jump->exiting = 0;
        jump->runnable = 0;
        jump->status = 0;
    }

    jmpBufInit(jump);
    jump->started = 1;

    args = (BootstrapArgs *)malloc(sizeof(BootstrapArgs));
    if (!args) {
        abort();
    }
    args->func = func;
    args->jump = jump;

    pthread_attr_init(&attr);
    if (stack_size > 0) {
        size_t minStack = (size_t)stack_size;
        size_t pthreadMin = (size_t)PTHREAD_STACK_MIN;
        const size_t kPcMinStack = 256u * 1024u;
        if (minStack < pthreadMin) {
            minStack = pthreadMin;
        }
        if (minStack < kPcMinStack) {
            minStack = kPcMinStack;
        }
        pthread_attr_setstacksize(&attr, minStack);
    }

    if (pthread_create(&jump->thread, &attr, gcThreadEntry, args) != 0) {
        abort();
    }
    pthread_attr_destroy(&attr);
}

#endif
