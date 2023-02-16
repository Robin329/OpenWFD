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

#ifndef OWF_MESSAGEQUEUE_H_
#define OWF_MESSAGEQUEUE_H_

#include "owftypes.h"
#include "owfsemaphore.h"
#include "owfmutex.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    OWFuint                 id;
    void*                   data;
} OWF_MESSAGE;

typedef struct _MSGQUE {
    OWFint                  read;
    OWFint                  write;
} OWF_MESSAGE_QUEUE;

/*
 *  Destroy message queue
 *
 *  \param queue Message queue to destroy
 */
OWF_API_CALL void
OWF_MessageQueue_Destroy(OWF_MESSAGE_QUEUE* queue);

/*
 *  Initialize message queue
 *
 *  \param queue Message queue to initialize
 *  \return 0 if initialization succeeded, < 0 otherwise
 *
 */
OWF_API_CALL OWFint
OWF_MessageQueue_Init(OWF_MESSAGE_QUEUE* queue);

/*
 *  Check whether the message queue is empty
 *
 *  \param queue Queue to check
 *  \return OWF_TRUE if queue is empty, OWF_FALSE if not.
 */
OWF_API_CALL OWFboolean
OWF_MessageQueue_Empty(OWF_MESSAGE_QUEUE* queue);

/*
 *  Insert message into message queue (send it to
 *  THE other side)
 *
 *  \param queue Message queue
 *  \param msg Message to send
 *  \param data Message contents
 *
 */
OWF_API_CALL void
OWF_Message_Send(OWF_MESSAGE_QUEUE* queue,
                 OWFuint msg,
                 void* data);

/*
 *  Poll message queue for incoming messages
 *
 *  \param queue Message queue
 *  \param msg Where to store the incoming message, if one is available
 *
 *  \return < 0 if error occurred, 0 if no message is available, > 0 otherwise;
 *  received message is stored into OWF_MESSAGE structure pointed by the msg
 *  param
 */
OWF_API_CALL OWFint
OWF_Message_Poll(OWF_MESSAGE_QUEUE* queue,
                 OWF_MESSAGE* msg);

/*
 *  Wait for message
 *
 *  \param queue Message queue
 *  \param msg Where to store the received message
 *  \param timeout Time to wait for the message (microseconds)
 *
 *  \return < 0 if error occurred, 0 if no message was received within
 *  given period of time, > 0 otherwise; received message is stored into
 *  OWF_MESSAGE structure pointed by the msg param
 *
 */
OWF_API_CALL OWFint
OWF_Message_Wait(OWF_MESSAGE_QUEUE* queue,
                 OWF_MESSAGE* msg,
                 OWFint timeout);


#ifdef __cplusplus
}
#endif


#endif
