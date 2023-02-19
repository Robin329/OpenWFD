/* Copyright (c) 2009 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#include <pthread.h>

#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "owfdebug.h"
#include "owfhstore.h"
#include "owfmemory.h"

/***************************************************************
 * NOTE: Implementation of EGLSync is a cut-down prototype and
 * exists purely to allow the OpenWF Composition CT to exercise
 * the OpenWF Composition SI
 ***************************************************************/

#define EGLSYNC_TYPE 0xE5

/* sync object data type for POSIX thread implementation*/

struct CondVarSync_ {
    EGLint type;
    EGLint status;
    EGLBoolean autoReset;
    EGLint condition;
    EGLDisplay dpy;

    /* internal */
    pthread_mutex_t mutex;
    pthread_cond_t condVar;
    EGLint nWaiters;
    EGLBoolean destroyed;
};

typedef struct CondVarSync_ *NativeSyncType;

#ifdef __cplusplus
extern "C" {
#endif

EGLAPI EGLSyncKHR EGLAPIENTRY eglCreateSyncKHR(EGLDisplay dpy, EGLenum type,
                                               const EGLint *attrib_list) {
    EGLint defStatusValue = EGL_UNSIGNALED_KHR;
    EGLSyncKHR sync = EGL_NO_SYNC_KHR; /* sync handle */
    NativeSyncType syncObj;

    DPRINT(("eglCreateSynchKHR\n"));

    if (type != EGL_SYNC_REUSABLE_KHR) {
        DPRINT(("  Illegal sync type\n"));
        return EGL_NO_SYNC_KHR;
    }

    if (attrib_list != NULL && attrib_list[0] != EGL_NONE) {
        DPRINT(("  Illegal use of attributes for EGL_SYNC_REUSABLE_KHR\n"));
        return EGL_NO_SYNC_KHR;
    }

    /* Check that dpy is a valid EGLdisplay here.
     * In this implementation the check is not possible */

    /* create sync object */
    syncObj = xalloc(1, sizeof(*syncObj));
    if (syncObj != NULL) {
        DPRINT(("sync object %p\n", syncObj));
        syncObj->type = EGL_SYNC_REUSABLE_KHR;
        syncObj->status = defStatusValue;
        syncObj->dpy = dpy;

        pthread_mutex_init(&syncObj->mutex, NULL);
        pthread_cond_init(&syncObj->condVar, NULL);
        syncObj->nWaiters = 0;
        syncObj->destroyed = EGL_FALSE;
    }

    sync = (EGLSyncKHR)OWF_HStore_HandleCreate(EGLSYNC_TYPE, syncObj);
    if (sync) {
        DPRINT(("eglCreateSynchKHR %p\n", sync));
    } else {
        xfree(syncObj);
    }

    return sync;
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroySyncKHR(EGLDisplay dpy,
                                                EGLSyncKHR sync) {
    /*!
     *  EGLsync for OpenWF RI:
     *  sync object is flagged destroyed.
     *  If no one is waiting for signal, memory is freed immediately
     */

    EGLBoolean destroy = EGL_FALSE;
    NativeSyncType syncObj;

    DPRINT(("eglDestroySynchKHR %p\n", sync));

    if (sync == NULL) {
        return EGL_FALSE;
    }

    syncObj = (NativeSyncType)OWF_HStore_GetObj((OWFHandle)sync, EGLSYNC_TYPE);
    if (syncObj == EGL_NO_SYNC_KHR) {
        DPRINT(("  Illegal sync object\n"));
        return EGL_FALSE;
    }

    if (syncObj->dpy != dpy) {
        DPRINT(("  sync is not a valid sync for display %d\n", dpy));
        return EGL_FALSE; /* behaviour is undefined */
    }

    pthread_mutex_lock(&syncObj->mutex);
    {
        syncObj->destroyed = EGL_TRUE;

        if (syncObj->nWaiters <= 0) {
            destroy = EGL_TRUE; /* delete immediately */
        }
    }
    pthread_mutex_unlock(&syncObj->mutex);

    OWF_HStore_HandleDelete((OWFHandle)sync);

    if (destroy) {
        pthread_mutex_destroy(&syncObj->mutex);
        pthread_cond_destroy(&syncObj->condVar);
        xfree(syncObj);
    }

    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglFenceKHR(EGLDisplay dpy, EGLSyncKHR sync) {
    /*!
     * EGLsync for OpenWF SI: Routine is not implemented, composition
     * has a fence primitive of its own
     */
    sync = sync;
    dpy = dpy;
    return EGL_FALSE;
}

EGLAPI EGLint EGLAPIENTRY eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync,
                                               EGLint flags,
                                               EGLTimeKHR timeout) {
    EGLint result = EGL_FALSE;
    EGLBoolean destroyAfterWait = EGL_FALSE;
    NativeSyncType syncObj;

    DPRINT(("eglClientWaitSyncKHR %p\n", sync));

    /*!
     * EGLsync for OpenWF RI:
     * Flags are not implemented. Flags parameter is ignored
     * Timed wait is not implemented. Timeout parameter is ignored, FOREVER
     * assumed.
     */
    flags = flags;     /* flags unused */
    timeout = timeout; /* timeout unused */

    if (sync == NULL) {
        return EGL_FALSE;
    }

    syncObj = (NativeSyncType)OWF_HStore_GetObj((OWFHandle)sync, EGLSYNC_TYPE);

    if (syncObj == EGL_NO_SYNC_KHR) {
        DPRINT(("  Illegal sync object\n"));
        return EGL_FALSE;
    }

    if (syncObj->dpy != dpy) {
        DPRINT(("  sync is not a valid sync for display %d\n", dpy));
        return EGL_FALSE; /* behaviour is undefined */
    }

    pthread_mutex_lock(&syncObj->mutex);
    {
        if (syncObj->destroyed) /* no new waiters if already destroyed */
        {
            result = EGL_FALSE;
        } else if (syncObj->status == EGL_SIGNALED_KHR) {
            result = EGL_CONDITION_SATISFIED_KHR;
        } else if (syncObj->status == EGL_UNSIGNALED_KHR) {
            syncObj->nWaiters++;
            pthread_cond_wait(&syncObj->condVar, &syncObj->mutex);
            syncObj->nWaiters--;
            result = EGL_CONDITION_SATISFIED_KHR;
            if (syncObj->destroyed && syncObj->nWaiters <= 0) {
                destroyAfterWait = EGL_TRUE;
            }
        }
    }
    pthread_mutex_unlock(&syncObj->mutex);

    if (destroyAfterWait) {
        DPRINT(("  destroy after wait\n"));
        eglDestroySyncKHR(syncObj->dpy, sync);
    }

    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY eglSignalSyncKHR(EGLDisplay dpy, EGLSyncKHR sync,
                                               EGLenum mode) {
    EGLBoolean result = EGL_TRUE;
    NativeSyncType syncObj;

    DPRINT(("eglSignalSyncKHR %p\n", sync));

    if (mode != EGL_SIGNALED_KHR && mode != EGL_UNSIGNALED_KHR) {
        return EGL_FALSE;
    }

    if (sync == NULL) {
        return EGL_FALSE;
    }

    syncObj = (NativeSyncType)OWF_HStore_GetObj((OWFHandle)sync, EGLSYNC_TYPE);
    if (syncObj == EGL_NO_SYNC_KHR) {
        DPRINT(("  Illegal sync object\n"));
        return EGL_FALSE;
    }

    if (syncObj->type != EGL_SYNC_REUSABLE_KHR) {
        return EGL_FALSE;
    }

    if (syncObj->dpy != dpy) {
        DPRINT(("  sync is not a valid sync for display %d\n", dpy));
        return EGL_FALSE; /* behaviour is undefined */
    }

    pthread_mutex_lock(&syncObj->mutex);
    {
        if (syncObj->destroyed && syncObj->status == EGL_SIGNALED_KHR) {
            DPRINT(("  sync already destroyed\n"));
            result = EGL_FALSE;
        }

        else if (mode != (EGLenum)syncObj->status) {
            /* should be able to signal even if flagged destroyed */
            syncObj->status = mode;

            if (syncObj->status == EGL_SIGNALED_KHR) {
                if (pthread_cond_broadcast(&syncObj->condVar) != 0) {
                    DPRINT(("  signalling failed\n"));
                    result = EGL_FALSE;
                }
            }
        }
    }
    pthread_mutex_unlock(&syncObj->mutex);

    return result;
}

/*!
 * EGLsync for OpenWF SI.
 */
EGLAPI EGLBoolean EGLAPIENTRY eglGetSyncAttribKHR(EGLDisplay dpy,
                                                  EGLSyncKHR sync,
                                                  EGLint attribute,
                                                  EGLint *value) {
    EGLBoolean result = EGL_TRUE;
    NativeSyncType syncObj;

    DPRINT(("eglSyncAttribKHR %p\n", sync));

    if (value == NULL) {
        return EGL_FALSE;
    }

    if (attribute != EGL_SYNC_TYPE_KHR && attribute != EGL_SYNC_STATUS_KHR) {
        DPRINT(("  not a valid sync attribute %d\n", attribute));
        return EGL_FALSE;
    }

    if (sync == NULL) {
        return EGL_FALSE;
    }

    syncObj = (NativeSyncType)OWF_HStore_GetObj((OWFHandle)sync, EGLSYNC_TYPE);

    if (syncObj == EGL_NO_SYNC_KHR) {
        DPRINT(("  Illegal sync object\n"));
        return EGL_FALSE;
    }

    if (syncObj->dpy != dpy) {
        DPRINT(("  sync is not a valid sync for display %d\n", dpy));
        return EGL_FALSE; /* behaviour is undefined */
    }

    pthread_mutex_lock(&syncObj->mutex);
    {
        if (syncObj->destroyed) /* already destroyed */
        {
            result = EGL_FALSE;
        } else {
            switch (attribute) {
                case EGL_SYNC_TYPE_KHR:
                    *value = syncObj->type;
                    break;
                case EGL_SYNC_STATUS_KHR:
                    *value = syncObj->status;
                    break;
                default:
                    OWF_ASSERT(0); /* Invalid attribute value. */
                    break;
            }
        }
    }
    pthread_mutex_unlock(&syncObj->mutex);

    return result;
}

#ifdef __cplusplus
}
#endif
