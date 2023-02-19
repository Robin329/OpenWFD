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

#include "owfmessagequeue.h"

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "owfdebug.h"
#include "owfmemory.h"
#include "owfmutex.h"
#include "owfsemaphore.h"
#include "owftypes.h"

#define LOCK_QUEUE(x) OWF_Mutex_Lock(&(x->mutex))
#define UNLOCK_QUEUE(x) OWF_Mutex_Unlock(&(x->mutex))

#define WAIT_TIL_THE_END_OF_TIME -1

OWF_API_CALL void OWF_MessageQueue_Destroy(OWF_MESSAGE_QUEUE *queue) {
    if (!queue) {
        return;
    }

    close(queue->read);
    close(queue->write);
}

OWF_API_CALL OWFint OWF_MessageQueue_Init(OWF_MESSAGE_QUEUE *queue) {
    OWFint err, handles[2];

    OWF_ASSERT(queue);

    queue->read = queue->write = -1;

    err = pipe(handles);
    if (err) {
        return err;
    }

    queue->read = handles[0];
    queue->write = handles[1];
    return 0;
}

OWF_API_CALL OWFboolean OWF_MessageQueue_Empty(OWF_MESSAGE_QUEUE *queue) {
    OWFint r = 0;
    struct pollfd s;

    OWF_ASSERT(queue);

    s.fd = queue->read;
    s.events = POLLIN;
    r = poll(&s, 1, 0);
    return (OWFboolean)r > 0;
}

OWF_API_CALL void OWF_Message_Send(OWF_MESSAGE_QUEUE *queue, OWFuint msg,
                                   void *data) {
    OWF_MESSAGE m;

    OWF_ASSERT(queue);

    m.id = msg;
    m.data = data;

    write(queue->write, &m, sizeof(OWF_MESSAGE));
}

static OWFint OWF_Message_DoFetch(OWF_MESSAGE_QUEUE *queue, OWF_MESSAGE *msg,
                                  OWFint timeout) {
    /*
    MUST BE CALLED FROM MUTEX
    */

    OWF_ASSERT(queue);
    OWF_ASSERT(msg);

    if (timeout > -1) {
        OWFint r = 0;
        fd_set s;
        struct timeval to;

        to.tv_sec = 0;
        to.tv_usec = (suseconds_t)timeout;

        FD_ZERO(&s);
        FD_SET(queue->read, &s);
        r = select(FD_SETSIZE, &s, NULL, NULL, &to);

        if (r < 1) {
            return -1;
        }
    }
    read(queue->read, msg, sizeof(OWF_MESSAGE));
    return 0;
}

OWF_API_CALL OWFint OWF_Message_Poll(OWF_MESSAGE_QUEUE *queue,
                                     OWF_MESSAGE *msg) {
    OWFint r = 0;
    struct pollfd s;

    OWF_ASSERT(queue);
    OWF_ASSERT(msg);

    s.fd = queue->read;
    s.events = POLLIN;
    r = poll(&s, 1, 0);

    if (r > 0) {
        (void)OWF_Message_DoFetch(queue, msg, -1);
    }

    return r;
}

OWF_API_CALL OWFint OWF_Message_Wait(OWF_MESSAGE_QUEUE *queue, OWF_MESSAGE *msg,
                                     OWFint timeout) {
    OWF_ASSERT(queue);
    OWF_ASSERT(msg);

    return OWF_Message_DoFetch(queue, msg, timeout);
}

#ifdef __cplusplus
}
#endif
