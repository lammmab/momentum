#include "devcom.h"
#include <stdint.h>

#include "debug.h"
#include "devcom.static.h"
#include "synth.h"

#if PLATFORM_PC
typedef struct {
    HSD_DevComCallback callback;
    int dcReq;
    int args;
    void* buf;
    bool cancelflag;
} DeferredCallback;

static DeferredCallback s_deferredCallbacks[32];
static int s_numDeferredCallbacks = 0;
static bool s_inDevComProcessing = false;

void ProcessDeferredCallbacks(void) {
    while (s_numDeferredCallbacks > 0) {
        DeferredCallback cb = s_deferredCallbacks[0];
        for (int i = 1; i < s_numDeferredCallbacks; i++) {
            s_deferredCallbacks[i-1] = s_deferredCallbacks[i];
        }
        s_numDeferredCallbacks--;

        if (cb.callback) {
            cb.callback(cb.dcReq, cb.args, cb.buf, cb.cancelflag);
        }
    }
}
#endif

bool HSD_DevComIsBusy(int idx)
{
    return (bool) devComStatus[idx];
}

static void HSD_DevComUnlink(HSD_DevCom* dc)
{
    HSD_DevCom* curr;
    bool enabled = OSDisableInterrupts();
    int i = dc->dcReq & 3;

    if (devComStatus[i] == dc) {
        devComStatus[i] = dc->next;
        if (HSD_DevCom_804C6330[i] == dc) {
            HSD_DevCom_804C6330[i] = 0;
        }
        goto cleanup;
    }

    for (curr = devComStatus[i]; curr->next != NULL; curr = curr->next) {
        if (curr->next == dc) {
            curr->next = dc->next;
            if (HSD_DevCom_804C6330[i] == dc) {
                HSD_DevCom_804C6330[i] = curr;
            }
            OSRestoreInterrupts(enabled);
            return;
        }
    }
    HSD_ASSERT(0x6E, 0);

cleanup:
    OSRestoreInterrupts(enabled);
}

static void HSD_DevComStdCallback(uintptr_t arg)
{
    ARQRequest* request = (ARQRequest*) arg;
    int i;

    if (request == &devComARQR[0][0]) {
        i = 0;
    } else if (request == &devComARQR[1][0]) {
        i = 1;
    } else {
        HSD_ASSERT(0xA5, 0);
    }
    aramstate = 0;
    devComRelayBufFlag[i] = false;
#if !PLATFORM_PC
    // On PC, skip recursive wakeup calls to avoid deep synchronous recursion
    // The explicit wakeup calls in lb_800195D0() and HSD_DevComDVDWakeUp() handle processing
    HSD_DevComDVDWakeUp();
    HSD_DevComARAMWakeUp(0);
#endif
}

static inline void HSD_DevComARAMCallback_inline(HSD_DevCom* devcom)
{
    bool enabled = OSDisableInterrupts();
    devcom->next = HSD_DevCom_804D77F0;
    HSD_DevCom_804D77F0 = devcom;
    OSRestoreInterrupts(enabled);
}

static void HSD_DevComARAMCallback(uintptr_t arg)
{
    ARQRequest* request = (ARQRequest*) arg;
    int i;
    void* buf;

    if (aramDC->type == 0x1A) {
        if (request == &devComARQR[0][0]) {
            i = 0;
        } else {
            i = 1;
        }
        buf = HSD_DevCom_804C6330_bufs[i];
    } else {
        buf = NULL;
    }

#if PLATFORM_PC
    // Queue callback to avoid recursion with synchronous ARQ
    if (aramDC->callback != NULL && s_numDeferredCallbacks < 32) {
        s_deferredCallbacks[s_numDeferredCallbacks].callback = aramDC->callback;
        s_deferredCallbacks[s_numDeferredCallbacks].dcReq = aramDC->dcReq;
        s_deferredCallbacks[s_numDeferredCallbacks].args = (int)aramDC->args;
        s_deferredCallbacks[s_numDeferredCallbacks].buf = buf;
        s_deferredCallbacks[s_numDeferredCallbacks].cancelflag = aramDC->cancelflag;
        s_numDeferredCallbacks++;
    }
#else
    if (aramDC->callback != NULL) {
        aramDC->callback(aramDC->dcReq, (int) aramDC->args, buf,
                         aramDC->cancelflag);
    }
#endif

    HSD_DevComUnlink(aramDC);
    HSD_DevComARAMCallback_inline(aramDC);
    HSD_DevComStdCallback((uintptr_t)request);
}

static inline int getRelayBufIdx(void)
{
    int i;
    for (i = 0; i < 2; i++) {
        if (!devComRelayBufFlag[i]) {
            devComRelayBufFlag[i] = true;
            return i;
        }
    }
    return -1;
}

void HSD_DevComARAMWakeUp(uintptr_t arg)
{
    bool enabled;
    int req_idx;
    u32 xfer_size2;
    void (*arq_callback)(uintptr_t);
    void (*arq_callback2)(uintptr_t);

    enabled = OSDisableInterrupts();
    if (aramstate != 0) {
        OSRestoreInterrupts(enabled);
        return;
    }
    aramDC = devComStatus[3];
    if (devComStatus[3] != NULL) {
        if (aramDC->cancelflag) {
            if (aramDC->callback != NULL) {
                aramDC->callback(aramDC->dcReq, (s32) aramDC->args, NULL,
                                 true);
            }
            HSD_DevComUnlink(aramDC);
            OSRestoreInterrupts(enabled);
            HSD_DevComARAMWakeUp(0);
            return;
        }
        req_idx = getRelayBufIdx();
        if (req_idx >= 0) {
            if (aramDC->type == 3) {
                u32 xfer_size;
#if PLATFORM_PC
                u32 dest, size;
#endif
                if (aramDC->size > DEVCOM_BUF_SIZE) {
                    arq_callback = HSD_DevComStdCallback;
                    xfer_size = DEVCOM_BUF_SIZE;
                } else {
                    arq_callback = HSD_DevComARAMCallback;
                    xfer_size = aramDC->size;
                }
#if PLATFORM_PC
                // ARQPostRequest processes callbacks synchronously, which can clear aramDC
                dest = aramDC->dest;
                size = aramDC->size;
#endif
                {
                    int* p = HSD_DevCom_804C6330_bufs[req_idx];
                    int i;
                    for (i = 0x1000; i > 0; i--) {
                        *p++ = 0;
                    }
                }
                DCStoreRange(HSD_DevCom_804C6330_bufs[req_idx],
                             DEVCOM_BUF_SIZE);
                aramstate = 1;
                ARQPostRequest(devComARQR[req_idx], 0, 0, 1,
                               (uintptr_t) HSD_DevCom_804C6330_bufs[req_idx],
#if PLATFORM_PC
                               dest, xfer_size, arq_callback);
                if (aramDC != NULL) {
                    aramDC->dest = dest + xfer_size;
                    aramDC->size = size - xfer_size;
                }
#else
                               aramDC->dest, xfer_size, arq_callback);
                aramDC->dest += xfer_size;
                aramDC->size -= xfer_size;
#endif
            } else if (aramDC->type == 0xB) {
                DCStoreRange((void*) aramDC->src, aramDC->size);
                aramstate = 1;
                ARQPostRequest(devComARQR[req_idx], 0, 0, 1, aramDC->src,
                               aramDC->dest, aramDC->size,
                               HSD_DevComARAMCallback);
            } else if (aramDC->type == 0x19) {
                DCInvalidateRange((void*) aramDC->dest, aramDC->size);
                aramstate = 1;
                ARQPostRequest(devComARQR[req_idx], 0, 1, 1, aramDC->src,
                               aramDC->dest, aramDC->size,
                               HSD_DevComARAMCallback);
            } else if (aramDC->type == 0x1A) {
                DCInvalidateRange(HSD_DevCom_804C6330_bufs[req_idx],
                                  DEVCOM_BUF_SIZE);
                ARQPostRequest(devComARQR[req_idx], 0, 1, 1, aramDC->src,
                               (uintptr_t) HSD_DevCom_804C6330_bufs[req_idx],
                               aramDC->size, HSD_DevComARAMCallback);
                aramstate = 1;
            } else if (aramDC->type == 0x1B) {
                DCInvalidateRange(HSD_DevCom_804C6330_bufs[req_idx],
                                  DEVCOM_BUF_SIZE);
                if (aramDC->size > DEVCOM_BUF_SIZE) {
                    arq_callback2 = HSD_DevComStdCallback;
                    xfer_size2 = DEVCOM_BUF_SIZE;
                } else {
                    arq_callback2 = HSD_DevComARAMCallback;
                    xfer_size2 = aramDC->size;
                }
                ARQPostRequest(&devComARQR[req_idx][1], 0, 1, 1, aramDC->src,
                               (uintptr_t) HSD_DevCom_804C6330_bufs[req_idx],
                               xfer_size2, NULL);
                ARQPostRequest(&devComARQR[req_idx][0], 0, 0, 1,
                               (uintptr_t) HSD_DevCom_804C6330_bufs[req_idx],
                               aramDC->dest, xfer_size2, arq_callback2);
                aramDC->src += xfer_size2;
                aramDC->dest += xfer_size2;
                aramDC->size -= xfer_size2;
                aramstate = 1;
            }
        }
    }
    OSRestoreInterrupts(enabled);
}

static void HSD_DevComDVDStdCallback(uintptr_t arg)
{
    ARQRequest* request = (ARQRequest*) arg;
    int i;
    if (request == &devComARQR[0][0]) {
        i = 0;
    } else if (request == &devComARQR[1][0]) {
        i = 1;
    } else {
        HSD_ASSERT(0x158, 0);
    }
    devComRelayBufFlag[i] = false;
    HSD_DevComDVDWakeUp();
    HSD_DevComARAMWakeUp(0);
}

static void HSD_DevComDVDARAMEndCallback(uintptr_t arg)
{
    ARQRequest* request = (ARQRequest*)arg;
    int i;
    HSD_DevCom* dc;

    if (request == &devComARQR[0][0]) {
        i = 0;
    } else {
        i = 1;
    }

    dc = HSD_DevCom_804D77FC[i];
    HSD_DevCom_804D77FC[i] = NULL;
    devComRelayBufFlag[i] = false;

    if (dc != NULL) {
#if PLATFORM_PC
        // Queue callback to avoid recursion
        if (dc->callback != NULL && HSD_DevCom_804D7804 == 0 && s_numDeferredCallbacks < 32) {
            s_deferredCallbacks[s_numDeferredCallbacks].callback = dc->callback;
            s_deferredCallbacks[s_numDeferredCallbacks].dcReq = dc->dcReq;
            s_deferredCallbacks[s_numDeferredCallbacks].args = (int)dc->args;
            s_deferredCallbacks[s_numDeferredCallbacks].buf = NULL;
            s_deferredCallbacks[s_numDeferredCallbacks].cancelflag = dc->cancelflag;
            s_numDeferredCallbacks++;
        }
#else
        if (dc->callback != NULL && HSD_DevCom_804D7804 == 0) {
            dc->callback(dc->dcReq, (int) dc->args, NULL, dc->cancelflag);
        }
#endif
        HSD_DevComARAMCallback_inline(dc);
    }
}

static void HSD_DevComDVDMemCallback(s32 result, DVDFileInfo* unused)
{
    HSD_DevCom* dc;
    bool enabled;

    if (result == -1) {
        HSD_DevCom_804D7804 = 1;
    }
    if (dvdDC->size > 0x80000) {
        dvdDC->src += 0x80000;
        dvdDC->dest += 0x80000;
        dvdDC->size -= 0x80000;
        HSD_DevCom_804D77F5 = 0;
#if !PLATFORM_PC
        HSD_DevComDVDWakeUp();
#endif
        return;
    }
    if (dvdDC->callback != NULL && HSD_DevCom_804D7804 == 0) {
        dvdDC->callback(dvdDC->dcReq, (int) dvdDC->args, NULL,
                        dvdDC->cancelflag);
    }
    HSD_DevComUnlink(dvdDC);
    dc = dvdDC;
    enabled = OSDisableInterrupts();
    dc->next = HSD_DevCom_804D77F0;
    HSD_DevCom_804D77F0 = dc;
    OSRestoreInterrupts(enabled);
    HSD_DevCom_804D77F5 = 0;
#if !PLATFORM_PC
    HSD_DevComDVDWakeUp();
#endif
}

static void HSD_DevComDVDCallback(s32 result, DVDFileInfo* unused)
{
    HSD_DevCom* dc;
    s32 enabled;
    u16 type;

    PAD_STACK(8);

    if (result == -1) {
        HSD_DevCom_804D7804 = 1;
    }
    type = dvdDC->type;
    if (type == 0x22) {
        HSD_ASSERT(0x18C, dvdDC->size <= DEVCOM_BUF_SIZE);
        HSD_ASSERT(0x18D, dvdDC->callback);
        if (HSD_DevCom_804D7804 == 0) {
            dvdDC->callback(dvdDC->dcReq, (s32) dvdDC->args,
                            HSD_DevCom_804C6330_bufs[HSD_DevCom_804D77F6],
                            dvdDC->cancelflag);
        }
        HSD_DevComUnlink(dvdDC);
        dc = dvdDC;
        enabled = OSDisableInterrupts();
        dc->next = HSD_DevCom_804D77F0;
        HSD_DevCom_804D77F0 = dc;
        OSRestoreInterrupts(enabled);
        HSD_DevCom_804D77F5 = 0;
        devComRelayBufFlag[HSD_DevCom_804D77F6] = false;
#if !PLATFORM_PC
        HSD_DevComDVDWakeUp();
        HSD_DevComARAMWakeUp(0);
#endif
    } else if (type == 0x23) {
        HSD_DevCom_804D77F7 = HSD_DevCom_804D77F6;
        if (dvdDC->size > DEVCOM_BUF_SIZE) {
            ARQPostRequest(
                devComARQR[HSD_DevCom_804D77F7], 0, 0, 1,
                (uintptr_t) HSD_DevCom_804C6330_bufs[HSD_DevCom_804D77F7],
                dvdDC->dest, DEVCOM_BUF_SIZE, HSD_DevComDVDStdCallback);
            dvdDC->src += DEVCOM_BUF_SIZE;
            dvdDC->dest += DEVCOM_BUF_SIZE;
            dvdDC->size -= DEVCOM_BUF_SIZE;
#ifndef PLATFORM_PC
            HSD_DevCom_804D77F5 = 0;
            HSD_DevComDVDWakeUp();
#endif
        } else {
            HSD_DevCom_804D77FC[HSD_DevCom_804D77F7] = dvdDC;
            ARQPostRequest(
                devComARQR[HSD_DevCom_804D77F7], 0, 0, 1,
                (uintptr_t) HSD_DevCom_804C6330_bufs[HSD_DevCom_804D77F7],
                dvdDC->dest, dvdDC->size, HSD_DevComDVDARAMEndCallback);
            HSD_DevComUnlink(dvdDC);
#ifndef PLATFORM_PC
            HSD_DevCom_804D77F5 = 0;
            HSD_DevComDVDWakeUp();
#endif
        }
    }
}

void HSD_DevComDVDWakeUp(void)
{
    bool enabled = OSDisableInterrupts();
    int i;
    int buf_idx;

    if (HSD_DevCom_804D77F5 != 0) {
        OSRestoreInterrupts(enabled);
        return;
    }
#ifdef PLATFORM_PC
    while (1) {
        bool work_done = false;
        for (i = 0; i < 3; i++) {
            if ((dvdDC = devComStatus[i])) {
                if (dvdDC->cancelflag) {
                    if (dvdDC->callback != NULL) {
                        dvdDC->callback(dvdDC->dcReq, (s32) dvdDC->args, NULL,
                                        true);
                    }
                    HSD_DevComUnlink(dvdDC);
                    work_done = true;
                    break;
                }

                if (!DVDFastOpen(dvdDC->file, &fileinfo)) {
                    devComStatus[i] = dvdDC->next;
                    if (HSD_DevCom_804C6330[i] == dvdDC) {
                        HSD_DevCom_804C6330[i] = 0;
                    }
                    work_done = true;
                    break;
                }

                if (dvdDC->type == 0x21) {
                    HSD_DevCom_804D77F5 = 1;
                    DVDReadAsyncPrio(&fileinfo, (void*) dvdDC->dest,
                                     MIN(dvdDC->size, 0x80000), (s32) dvdDC->src,
                                     HSD_DevComDVDMemCallback, 2);
                    DVDClose(&fileinfo);
                    HSD_DevCom_804D77F5 = 0;
                    work_done = true;
                    break;
                }

                buf_idx = getRelayBufIdx();
                if (buf_idx >= 0) {
                    HSD_DevCom_804D77F6 = buf_idx;
                    HSD_DevCom_804D77F5 = 1;
                    DVDReadAsyncPrio(&fileinfo, HSD_DevCom_804C6330_bufs[buf_idx],
                                     MIN(dvdDC->size, DEVCOM_BUF_SIZE), dvdDC->src,
                                     HSD_DevComDVDCallback, 2);
                    DVDClose(&fileinfo);
                    HSD_DevCom_804D77F5 = 0;
                    work_done = true;
                    break;
                } else {
                    devComStatus[i] = dvdDC->next;
                    if (HSD_DevCom_804C6330[i] == dvdDC) {
                        HSD_DevCom_804C6330[i] = 0;
                    }
                    work_done = true;
                    break;
                }
            }
        }

        if (!work_done) {
            bool has_queued_work = false;
            for (int j = 0; j < 3; j++) {
                if (devComStatus[j] != NULL) {
                    has_queued_work = true;
                    break;
                }
            }
            if (has_queued_work) {
                HSD_DevCom_804D77F5 = 0;
                continue;
            }
            break;
        }

        HSD_DevCom_804D77F5 = 0;
    }
    OSRestoreInterrupts(enabled);
    HSD_DevComARAMWakeUp(0);
    ProcessDeferredCallbacks();
#else
    for (i = 0; i < 3; i++) {
        if ((dvdDC = devComStatus[i])) {
            if (dvdDC->cancelflag) {
                if (dvdDC->callback != NULL) {
                    dvdDC->callback(dvdDC->dcReq, (s32) dvdDC->args, NULL,
                                    true);
                }
                HSD_DevComUnlink(dvdDC);
                OSRestoreInterrupts(enabled);
                HSD_DevComDVDWakeUp();
                return;
            }
            DVDFastOpen(dvdDC->file, &fileinfo);
            if (dvdDC->type == 0x21) {
                DVDReadAsyncPrio(&fileinfo, (void*) dvdDC->dest,
                                 MIN(dvdDC->size, 0x80000), (s32) dvdDC->src,
                                 HSD_DevComDVDMemCallback, 2);
                HSD_DevCom_804D77F5 = 1;
                OSRestoreInterrupts(enabled);
                return;
            }
            buf_idx = getRelayBufIdx();
            if (buf_idx >= 0) {
                HSD_DevCom_804D77F6 = buf_idx;
                DVDReadAsyncPrio(&fileinfo, HSD_DevCom_804C6330_bufs[buf_idx],
                                 MIN(dvdDC->size, DEVCOM_BUF_SIZE), dvdDC->src,
                                 HSD_DevComDVDCallback, 2);
                HSD_DevCom_804D77F5 = 1;
                OSRestoreInterrupts(enabled);
                return;
            }
        }
    }
    OSRestoreInterrupts(enabled);
#endif
}

static inline int HSD_DevComGetDestType(int type)
{
    return type & 7;
}

#define INIT_N_DEVCOMS 16

static inline void DevComLinkNext(HSD_DevCom* dc)
{
    int i;
    for (i = 1; i < INIT_N_DEVCOMS - 1; i++) {
        dc[i].next = &dc[i] + 1;
    }
    dc[i].next = NULL;
}

int HSD_DevComRequest(int file, uintptr_t src, uintptr_t dest, size_t size,
                      int type, int pri, HSD_DevComCallback cb, void* args)
{
    bool enabled;
    HSD_DevCom* dc;
    int result;

    enabled = OSDisableInterrupts();

    if ((dc = HSD_DevCom_804D77F0)) {
        HSD_DevCom_804D77F0 = dc->next;
        OSRestoreInterrupts(enabled);
    } else {
        dc = HSD_AudioMalloc(sizeof(HSD_DevCom) * INIT_N_DEVCOMS);
        DevComLinkNext(dc);

        HSD_DevCom_804D77F0 = &dc[1];
        OSRestoreInterrupts(enabled);
    }

    HSD_ASSERT(0x1ED, dc);
    HSD_ASSERT(0x1EE,
        !(HSD_DevComGetDestType(type) == DEVCOMDEST_SBUF
            && size > DEVCOM_BUF_SIZE));

    HSD_ASSERT(0x1EF, src % 32 == 0);
    HSD_ASSERT(0x1F0, dest % 32 == 0);
    HSD_ASSERT(0x1F1, size % 32 == 0);
    HSD_ASSERT(0x1F2, size != 0);

    pri = (type & 0x38) == 0x20 ? pri : 3;

    dc->file = file;
    dc->src = src;
    dc->dest = dest;
    dc->size = size;
    dc->type = type;
    dc->cancelflag = false;
    dc->callback = cb;
    dc->args = args;

    enabled = OSDisableInterrupts();
    result = HSD_DevCom_804D6050 + pri;
    dc->dcReq = result;
    HSD_DevCom_804D6050 += 4;
    if (HSD_DevCom_804C6330[pri] != NULL) {
        HSD_DevCom_804C6330[pri]->next = dc;
    }
    HSD_DevCom_804C6330[pri] = dc;
    dc->next = NULL;
    if (devComStatus[pri] == NULL) {
        devComStatus[pri] = dc;
        HSD_DevComDVDWakeUp();
        HSD_DevComARAMWakeUp(0);
    }
    OSRestoreInterrupts(enabled);

    return result;
}

static inline HSD_DevCom* HSD_DevComCancelEx_inline(int dcReq)
{
    HSD_DevCom* cur = devComStatus[dcReq & 3];
    while (cur != NULL) {
        if (cur->dcReq == dcReq) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

int HSD_DevComCancelEx(int dcReq, u32 flags, HSD_DevComCallback cb, void* args)
{
    HSD_DevCom* dc;
    bool enabled = OSDisableInterrupts();

    if ((dc = HSD_DevComCancelEx_inline(dcReq))) {
        int tmp = dcReq & 3;
        if (flags & 1) {
            dc->callback = cb;
        }
        if (flags & 2) {
            dc->args = args;
        }
        if (devComStatus[tmp] == dc) {
            dc->cancelflag = true;
        } else {
            if (dc->callback != NULL) {
                dc->callback(dc->dcReq, (int) dc->args, NULL, true);
            }
            HSD_DevComUnlink(dc);
        }
    } else {
        int i;
        for (i = 0; i < 2; i++) {
            HSD_DevCom* dc = HSD_DevCom_804D77FC[i];
            if (dc != NULL && dc->dcReq == dcReq) {
                dc->callback = cb;
                dc->args = args;
                HSD_DevCom_804D77FC[i]->cancelflag = true;
            }
        }
    }
    OSRestoreInterrupts(enabled);
    return 0;
}
