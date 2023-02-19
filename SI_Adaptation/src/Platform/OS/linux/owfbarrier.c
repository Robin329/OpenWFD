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

#ifdef __cplusplus
extern "C" {
#endif

#include "owfbarrier.h"

#include <pthread.h>

#include "owfmemory.h"
#include "owfmutex.h"

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} OWF_BARRIER_DATA;

#define BARRIER(x) ((OWF_BARRIER_DATA *)*x)

OWF_API_CALL void OWF_Barrier_Init(OWF_BARRIER *barrier) {
    if (!barrier) {
        return;
    }

    if (*barrier) {
        return;
    }

    *barrier = xalloc(1, sizeof(OWF_BARRIER_DATA));
    if (*barrier) {
        pthread_mutex_init(&BARRIER(barrier)->mutex, NULL);
        pthread_cond_init(&BARRIER(barrier)->condition, NULL);
    }
}

OWF_API_CALL void OWF_Barrier_Wait(OWF_BARRIER *barrier) {
    if (!barrier) {
        return;
    }
    pthread_mutex_lock(&BARRIER(barrier)->mutex);
    pthread_cond_wait(&BARRIER(barrier)->condition, &BARRIER(barrier)->mutex);
    pthread_mutex_unlock(&BARRIER(barrier)->mutex);
}

OWF_API_CALL void OWF_Barrier_Break(OWF_BARRIER *barrier) {
    if (!barrier) {
        return;
    }
    pthread_cond_broadcast(&BARRIER(barrier)->condition);
}

OWF_API_CALL void OWF_Barrier_Destroy(OWF_BARRIER *barrier) {
    if (!barrier) {
        return;
    }
    pthread_mutex_destroy(&BARRIER(barrier)->mutex);
    pthread_cond_destroy(&BARRIER(barrier)->condition);
    xfree(*barrier);
    *barrier = NULL;
}

#ifdef __cplusplus
}
#endif
