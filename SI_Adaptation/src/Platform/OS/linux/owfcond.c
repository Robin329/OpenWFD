
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

#define ONE_SEC 1000000000


#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "owfmemory.h"
#include "owfcond.h"
#include "owfdebug.h"



typedef struct OWF_COND_ {
    pthread_cond_t  cond;
    OWF_MUTEX      mutex;
} _OWF_COND_;


OWF_API_CALL OWFboolean
OWF_Cond_Init(OWF_COND* pCond, OWF_MUTEX mutex)
{
    _OWF_COND_* tmp;
    OWFint res;

    if (!pCond)
    {
        return EINVAL;
    }

    tmp = xalloc(1, sizeof(*tmp));
    if (!tmp)
    {
        return ENOMEM;
    }

    tmp->mutex = mutex;
    res = pthread_cond_init(&tmp->cond, NULL);
    *pCond = tmp;

    return res;
}

OWF_API_CALL void
OWF_Cond_Destroy(OWF_COND* pCond)
{
    OWFint             err = EINVAL;

    if (!pCond)
    {
        return;
    }

    if (*pCond)
    {
        _OWF_COND_* tmp = *((_OWF_COND_**) pCond);
        err = pthread_cond_destroy(&tmp->cond);
        xfree(tmp);
        *pCond = NULL;
    }
    return;
}

OWF_API_CALL OWFboolean
OWF_Cond_Wait(OWF_COND cond, OWFtime timeout)
{
    OWFint ret;
    _OWF_COND_* tmp = (_OWF_COND_*)cond;

    if (!tmp && tmp->mutex)
    {
        DPRINT(("COND WAIT FAILED!"));
        return OWF_FALSE;
    }
    else if (timeout == 0)
    {
        return OWF_TRUE; /* 'timeout' */
    }
    else if (timeout == (OWFtime) OWF_FOREVER)
    {
        pthread_mutex_t* mutex = (pthread_mutex_t*)tmp->mutex;

        ret = pthread_cond_wait(&tmp->cond, mutex);
        return (ret != 0);
    }
    else
    {
        struct timespec abstime;
        struct timeval  now;

        pthread_mutex_t* mutex = (pthread_mutex_t*)tmp->mutex;

        gettimeofday(&now, NULL);

        abstime.tv_sec  = now.tv_sec + timeout / ONE_SEC;
        abstime.tv_nsec = now.tv_usec * 1000 + timeout % ONE_SEC;

        if (abstime.tv_nsec > ONE_SEC)
        {
            abstime.tv_nsec -= ONE_SEC;
            abstime.tv_sec++;
        }

        ret =  pthread_cond_timedwait(&tmp->cond, mutex, &abstime);

        return (ret != 0);
    }
}

OWF_API_CALL OWFboolean
OWF_Cond_Signal(OWF_COND cond)
{
    _OWF_COND_* tmp = (_OWF_COND_*)cond;

    if (!(tmp && tmp->mutex))
    {
        return EINVAL;
    }

    return pthread_cond_signal(&tmp->cond);
}

OWF_API_CALL OWFboolean
OWF_Cond_SignalAll(OWF_COND cond)
{
    _OWF_COND_* tmp = (_OWF_COND_*)cond;

    if (!(tmp && tmp->mutex))
    {
        return EINVAL;
    }
    return pthread_cond_broadcast(&tmp->cond);
}



#ifdef __cplusplus
}
#endif

