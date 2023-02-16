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

/*! \ingroup wfd
 *  \file wfdevent.c
 *  \brief OpenWF Display SI, event and event container handling implementation.
 */


#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "wfdhandle.h"
#include "wfdevent.h"
#include "wfddebug.h"
#include "wfddevice.h"
#include "wfdutils.h"
#include "wfdport.h"

#include "owfobject.h"
#include "owfarray.h"
#include "owfmemory.h"


static WFDint
WFD_Event_BindQueueSize(WFD_DEVICE* device, const WFDint* attribList);

static WFDint
WFD_Event_ContainerQueueSize(WFD_DEVICE* device, WFDint bindQueueSize);

static void
WFD_EVENT_CONTAINER_Ctor(void* self);


static void
WFD_EVENT_CONTAINER_Ctor(void* self)
{
    self = self;
}

void WFD_EVENT_CONTAINER_Dtor(void* payload)
{
    WFD_EVENT_CONTAINER* pEventCont;

    OWF_ASSERT(payload);

    pEventCont = (WFD_EVENT_CONTAINER*) payload;

    REMREF(pEventCont->device);

    if (pEventCont->handle != WFD_INVALID_HANDLE)
    {
        WFD_Handle_Delete(pEventCont->handle);
        pEventCont->handle = WFD_INVALID_HANDLE;
    }

    /* start clearing */
    if (pEventCont->event)
    {
        OWF_Pool_PutObject(pEventCont->event);
    }

    OWF_List_Clear(pEventCont->eventQueue);
    pEventCont->eventQueue = NULL;

    OWF_Pool_Destroy(pEventCont->nodePool);
    pEventCont->nodePool = NULL;

    OWF_Pool_Destroy(pEventCont->eventPool);
    pEventCont->eventPool = NULL;

    WFD_Handle_Delete(pEventCont->handle);
    pEventCont->handle = WFD_INVALID_HANDLE;

    OWF_Cond_Destroy(&pEventCont->cond);
    pEventCont->cond = NULL;

    OWF_Mutex_Destroy(&pEventCont->mutex);
    pEventCont->mutex = NULL;

    /* Event container is freed by REMREF macro */
}

static WFDint
WFD_Event_BindQueueSize(WFD_DEVICE* device, const WFDint* attribList)
{
    WFDboolean found = WFD_FALSE;
    WFDint bqs = 0;

    OWF_ASSERT(device && device->config);

    if (attribList)
    {
        WFDint i;
        for (i=0; attribList[i] != WFD_NONE; i += 2)
        {
            if (attribList[i] == WFD_EVENT_PIPELINE_BIND_QUEUE_SIZE)
            {
                bqs = attribList[i+1];
                found = WFD_TRUE;
                break;
            }
        }
     }

    if (!found)
    {
        WFDint i;

        /* default bind event queue size calculation */
        for (i=0; i < device->config->portCount; i++)
        {
            bqs += WFD_Port_GetMaxRefreshRate(&device->config->ports[i]);
        }
        bqs *= device->config->pipelineCount;
    }

    return (bqs > 0) ? bqs : 0;
}

static WFDint
WFD_Event_ContainerQueueSize(WFD_DEVICE* device, WFDint bqs)
{
    WFDint i;
    WFDint eqs = 0;

    OWF_ASSERT(device && device->config);
    OWF_ASSERT(bqs >= 0);

    /* add attach/detach event count */
    for (i=0; i < device->config->portCount; i++)
    {
        if (device->config->ports[i].detachable)
        {
            eqs++;
        }
    }

    /* add port protection event count */
    eqs += device->config->portCount;

    if (eqs + bqs> 0)
    {
        eqs += bqs;
    }
    else
    {
        /* This can happen if device have no ports
         * Ensure that at least one slot in queue */
        eqs = 1;
    }

    return (eqs > bqs) ? eqs : bqs;
}
OWF_API_CALL WFDEvent OWF_APIENTRY
WFD_Event_CreateContainer(WFD_DEVICE* device, const WFDint* attribList) OWF_APIEXIT
{
    WFD_EVENT_CONTAINER* pEventCont = NULL;
    WFDEvent handle = WFD_INVALID_HANDLE;
    WFDint eqs = 0; /* event queue size */
    WFDint bqs = 0; /* bind queue size */
    WFDboolean ok = WFD_FALSE;

    OWF_ASSERT(device && device->config);

    DPRINT(("WFD_Event_Create(%p, %p)", device, attribList));

    pEventCont = CREATE(WFD_EVENT_CONTAINER);
    if (pEventCont)
    {
        DPRINT(("  event container %p", pEventCont));

        ADDREF(pEventCont->device,device);

        ok = (OWF_Mutex_Init(&pEventCont->mutex) == 0);
        if (ok)
        {
            ok = (OWF_Cond_Init(&pEventCont->cond, pEventCont->mutex) == 0);
        }

        bqs = WFD_Event_BindQueueSize(device, attribList);
        eqs = WFD_Event_ContainerQueueSize(device, bqs);

        DPRINT(("  event queue size %p == %i", pEventCont, eqs));

        /* allocate space for event queue, one extra pool object for
         * events that are unique in the queue
         */
        if (ok && eqs > 0)
        {
/*          Enchancement hint: use node as event queue header!
            pEventCont->nodePool =
                OWF_Pool_Create(sizeof(OWF_NODE)+sizeof(WFD_EVENT), eqs);
*/
            pEventCont->nodePool =
                OWF_Pool_Create(sizeof(OWF_NODE), eqs+1);
            ok = (pEventCont->nodePool != NULL);
        }

        if (ok && eqs > 0)
        {
            pEventCont->eventPool =
                OWF_Pool_Create(sizeof(WFD_EVENT), eqs+1);
            ok = (pEventCont->eventPool != NULL);
        }

        if (ok)
        {
            handle = WFD_Handle_Create(WFD_EVENT_HANDLE, pEventCont);
            ok = (handle != WFD_INVALID_HANDLE);
        }

        if (ok)
        {
            WFDint i;
            /* initialize event container data */
            for (i=0; i<WFD_EVENT_FILTER_SIZE; i++)
            {
                pEventCont->eventFilter[i] = WFD_TRUE;
            }

            pEventCont->handle = handle;
            pEventCont->pipelineBindQueueSize = bqs;
            pEventCont->eventQueue = NULL;
            pEventCont->queueLength = 0;

            /* append  to device's event containers */
            ok = OWF_Array_AppendItem(&device->eventConts, pEventCont);
        }

    }

    if (!ok && pEventCont)
    {
        /* out of memory exit */
        DESTROY(pEventCont);
        WFD_Device_SetError(device, WFD_ERROR_OUT_OF_MEMORY);
        return WFD_INVALID_HANDLE;
    }

    return handle;
}

OWF_API_CALL void OWF_APIENTRY
WFD_Event_DestroyContainer(WFD_DEVICE* pDevice,
                           WFD_EVENT_CONTAINER* pEventCont) OWF_APIEXIT
{
    WFD_EVENT event;

    OWF_ASSERT(pDevice && pEventCont);

    DPRINT(("WFD_Event_Destroy(%p, %p)", pDevice, pEventCont));
    DPRINT(("  - %d events still in queue", pEventCont->queueLength));

    /* invalidate handle first */
    WFD_Handle_Delete(pEventCont->handle);
    pEventCont->handle = WFD_INVALID_HANDLE;

    event.type = WFD_EVENT_DESTROYED;
    WFD_Event_Insert(pEventCont, &event);

    /* clear  container*/
    OWF_Array_RemoveItem(&pDevice->eventConts, pEventCont);
    DESTROY(pEventCont);
 }

OWF_API_CALL WFD_EVENT_CONTAINER* OWF_APIENTRY
WFD_Event_FindByHandle(WFD_DEVICE* pDevice, WFDEvent event) OWF_APIEXIT
{
    WFD_EVENT_CONTAINER* pEventCont;

    OWF_ASSERT(pDevice);

    pEventCont = (WFD_EVENT_CONTAINER*)WFD_Handle_GetObj(event,WFD_EVENT_HANDLE);

    /* paranoid double check - container should reside in the container array */
    if (pEventCont &&  pEventCont->device == pDevice)
    {
        WFDint i=0;
        OWF_ARRAY_ITEM item;
        while ( (item=OWF_Array_GetItemAt(&pDevice->eventConts, i++)) )
        {
            if ((WFD_EVENT_CONTAINER*)item == pEventCont)
            {
                return pEventCont;
            }
        }
    }

    return NULL;
}


OWF_API_CALL WFDint OWF_APIENTRY
WFD_Event_GetAttribi (WFD_EVENT_CONTAINER* pEventCont,
                      WFDEventAttrib attrib) OWF_APIEXIT
{
    WFDint value = 0;
    WFDEventType type;
    WFDboolean valid;

    OWF_ASSERT(pEventCont);

    OWF_Mutex_Lock(&pEventCont->mutex);

    type = (pEventCont->event) ? pEventCont->event->type : WFD_EVENT_NONE;
    valid = WFD_Util_ValidAttributeForEvent(type, attrib);

    if (!valid)
    {
        WFD_Device_SetError(pEventCont->device, WFD_ERROR_ILLEGAL_ARGUMENT);
        OWF_Mutex_Unlock(&pEventCont->mutex);
        return value;
    }

    if (attrib ==  WFD_EVENT_PIPELINE_BIND_QUEUE_SIZE)
    {
        value = pEventCont->pipelineBindQueueSize;
    }
    else if (attrib == WFD_EVENT_TYPE)
    {
        value = type;
    }
    else if (pEventCont->event)
    {
        switch (attrib)
        {
        case WFD_EVENT_PORT_ATTACH_PORT_ID:
            value = pEventCont->event->data.portAttachEvent.portId;
            break;

        case WFD_EVENT_PORT_ATTACH_STATE:
            value = pEventCont->event->data.portAttachEvent.attached;
            break;

        case WFD_EVENT_PIPELINE_BIND_PIPELINE_ID:
            value = pEventCont->event->data.pipelineBindEvent.pipelineId;
            break;

        case WFD_EVENT_PIPELINE_BIND_SOURCE:
            value = (WFDint)pEventCont->event->data.pipelineBindEvent.handle;
            break;

        case WFD_EVENT_PIPELINE_BIND_MASK:
            value = (WFDint) pEventCont->event->data.pipelineBindEvent.handle;
            break;

        case WFD_EVENT_PIPELINE_BIND_QUEUE_OVERFLOW:
            value = pEventCont->event->data.pipelineBindEvent.overflow;
            break;

        case WFD_EVENT_PORT_PROTECTION_PORT_ID:
            value = pEventCont->event->data.portProtectionEvent.portId;
            break;

        default:
            break;
        }
    }

    OWF_Mutex_Unlock(&pEventCont->mutex);
    return value;
}

#define FILTER_IND(type) (type-WFD_FIRST_FILTERED)

OWF_API_CALL void OWF_APIENTRY
WFD_Event_SetFilter (WFD_EVENT_CONTAINER* pEventCont,
                     const WFDEventType* filter) OWF_APIEXIT
{
    WFDint i;

    OWF_ASSERT(pEventCont);

    /* precondition: only valid types in filter array */

    OWF_Mutex_Lock(&pEventCont->mutex);

    if (!filter)
    {
        /* set filtering off */
        for (i=WFD_FIRST_FILTERED; i<WFD_LAST_FILTERED; i++)
        {
            pEventCont->eventFilter[FILTER_IND(i)] = WFD_TRUE;
        }
    }
    else
    {
        /* first set all events filtered */
        for (i=WFD_FIRST_FILTERED; i<=WFD_LAST_FILTERED; i++)
        {
            pEventCont->eventFilter[FILTER_IND(i)] = WFD_FALSE;
        }

        for (i=0; filter[i] != WFD_NONE; i++)
        {
            pEventCont->eventFilter[FILTER_IND(filter[i])] = WFD_TRUE;
        }
        
        /* ensure that WFD_EVENT_NONE & WFD_EVENT_DESTROYED are never filtered */
        pEventCont->eventFilter[FILTER_IND(WFD_EVENT_NONE)] = WFD_TRUE;
        pEventCont->eventFilter[FILTER_IND(WFD_EVENT_DESTROYED)] = WFD_TRUE;
    }

    OWF_Mutex_Unlock(&pEventCont->mutex);
}

OWF_API_CALL WFDEventType OWF_APIENTRY
WFD_Event_Wait(WFD_EVENT_CONTAINER* pEventCont,
               WFDtime timeout) OWF_APIEXIT
{
    WFDEventType result;

    OWF_ASSERT(pEventCont);

    DPRINT(("WFD_Event_Wait(%p, %d), enter", pEventCont, timeout));

    /* prevent container from disappearing while
     * someone is waiting
     */
    ADDREF(pEventCont, pEventCont);

    OWF_Mutex_Lock(&pEventCont->mutex);

    if (pEventCont->waiting == WFD_TRUE)
    {
        result =  WFD_EVENT_INVALID;
    }
    else
    {
        WFDboolean tmo = WFD_FALSE;

        pEventCont->waiting = WFD_TRUE;

        while (pEventCont->queueLength == 0 && !tmo)
        {
            DPRINT(("WFD_Event_Wait: going to wait"));

            tmo = OWF_Cond_Wait(pEventCont->cond, timeout);
        }

        pEventCont->waiting = WFD_FALSE;

        /* get queue head and return */
        if (pEventCont->queueLength > 0)
        {
            OWF_NODE* node;

            node = pEventCont->eventQueue; /* list head */
            pEventCont->eventQueue = OWF_List_Remove(node, node);
            pEventCont->queueLength--;
            if (pEventCont->event) /* return previous event to pool */
            {
                OWF_Pool_PutObject(pEventCont->event);
            }
            pEventCont->event = (WFD_EVENT*) node->data;
            OWF_Pool_PutObject(node);

            result = pEventCont->event->type;
            DPRINT(("WFD_Event_Wait: result %x, queue length now %d",
                    result, pEventCont->queueLength));
        }
        else
        {
            /* timeout or queue empty */
            result = WFD_EVENT_NONE;
            if (timeout != 0)
            {
                DPRINT(("WFD_Event_Wait: timeout", pEventCont, timeout));
            }
            else
            {
                DPRINT(("WFD_Event_Wait: queue empty"));
            }
        }
    }

    OWF_Mutex_Unlock(&pEventCont->mutex);

    REMREF(pEventCont);

    return result;
}

OWF_API_CALL void OWF_APIENTRY
WFD_Event_Async(WFD_EVENT_CONTAINER* pEventCont,
                WFDEGLDisplay display,
                WFDEGLSync sync) OWF_APIEXIT
{
    OWF_ASSERT(pEventCont);

    OWF_Mutex_Lock(&pEventCont->mutex);

    if (pEventCont->queueLength > 0)
    {
        pEventCont->sync = WFD_INVALID_SYNC;
        OWF_Mutex_Unlock(&pEventCont->mutex);
        /* immediate signal - do not store sync */
        eglSignalSyncKHR(display, sync, EGL_SIGNALED_KHR);

    }
    else
    {
        /* store sync for later use */
        pEventCont->sync = sync;
        pEventCont->display = display;
        OWF_Mutex_Unlock(&pEventCont->mutex);
    }
}

/* insert an event to all containers of a device */
OWF_API_CALL void OWF_APIENTRY
WFD_Event_InsertAll(WFD_DEVICE* pDevice,
                    const WFD_EVENT* pEvent) OWF_APIEXIT
{
    WFDint i = 0;
    OWF_ARRAY_ITEM arrayItem;

    OWF_ASSERT(pDevice && pEvent);

    while ((arrayItem=OWF_Array_GetItemAt(&pDevice->eventConts, i)))
    {
        WFD_EVENT_CONTAINER* pEventCont;

        pEventCont = (WFD_EVENT_CONTAINER*)arrayItem;
        WFD_Event_Insert(pEventCont, pEvent);
        i++;
    }
}

static void
WFD_Event_MarkOverflow(WFD_EVENT_CONTAINER* pEventCont)
{
    /* queue overflow - find last bind event and mark overflow */
    OWF_NODE*   curr    = NULL;
    OWF_NODE*   last    = NULL;
    WFD_EVENT*  event   = NULL;

    OWF_ASSERT(pEventCont);

    curr    = pEventCont->eventQueue;
    event   = (WFD_EVENT*)curr->data;

    while (curr)
    {
        if (event->type == WFD_EVENT_PIPELINE_BIND_SOURCE_COMPLETE ||
            event->type == WFD_EVENT_PIPELINE_BIND_MASK_COMPLETE)
        {
            last = curr;
        }

        curr = curr->next;
    }

    if (last)
    {
        event   = (WFD_EVENT*)last->data;
        event->data.pipelineBindEvent.overflow = WFD_TRUE;
    }
}


static WFDboolean
WFD_Event_CanInsert(WFD_EVENT_CONTAINER* pEventCont,
                 const WFD_EVENT* pEvent)
{
    OWF_ASSERT(pEventCont && pEvent);

    if (!pEventCont->eventFilter[FILTER_IND(pEvent->type)])
    {
        /* filtered event */
        DPRINT(("WFD_Event_Insert: filtered, event %x", pEvent->type));
        return WFD_FALSE;
    }

    if ((pEvent->type == WFD_EVENT_PIPELINE_BIND_SOURCE_COMPLETE ||
        pEvent->type == WFD_EVENT_PIPELINE_BIND_MASK_COMPLETE) &&
        pEventCont->pipelineBindQueueSize <= 0)
    {
        /* zero or negative bind queue size prevents bind events */
        DPRINT(("WFD_Event_Insert: bind events disabled,  %x", pEvent->type));
        return WFD_FALSE;
    }

    if ((pEvent->type == WFD_EVENT_PIPELINE_BIND_SOURCE_COMPLETE ||
        pEvent->type == WFD_EVENT_PIPELINE_BIND_MASK_COMPLETE) &&
        pEventCont->queueLength >= pEventCont->pipelineBindQueueSize)
    {
        /* bind queue overflow */
        WFD_Event_MarkOverflow(pEventCont);
        DPRINT(("WFD_Event_Insert: overflow, event %x", pEvent->type));
        return WFD_FALSE;
    }

    return WFD_TRUE;
}

static WFD_EVENT*
WFD_Event_FindPreviousEventByPortId(WFDEventType type,
                                    OWF_NODE* eventQueue,
                                    OWF_NODE* node)
{
    WFD_EVENT* newEvent = (WFD_EVENT*)node->data;

    while (eventQueue)
    {
        WFD_EVENT* oldEvent = (WFD_EVENT*)eventQueue->data;

        if (oldEvent->type == newEvent->type)
        {
            if ( type == WFD_EVENT_PORT_ATTACH_DETACH  &&
                 oldEvent->data.portAttachEvent.portId ==
                 newEvent->data.portAttachEvent.portId)
            {
                return oldEvent;
            }

            if ( type == WFD_EVENT_PORT_PROTECTION_FAILURE  &&
                 oldEvent->data.portProtectionEvent.portId ==
                 newEvent->data.portProtectionEvent.portId)
            {
                return oldEvent;
            }
        }

        eventQueue = eventQueue->next;
    }

    return NULL;
}

static OWF_NODE*
WFD_Event_InsertByEventType(WFDEventType type,
                            OWF_NODE* eventQueue,
                            OWF_NODE* node)
{
    OWF_NODE* newRoot = eventQueue;
    WFD_EVENT* oldEvent;

    OWF_ASSERT(node);

    switch (type)
    {
    case WFD_EVENT_DESTROYED:
        /* destroyed event is put to queue front */
        newRoot = OWF_List_Insert(eventQueue, node);
        break;

    case WFD_EVENT_PIPELINE_BIND_SOURCE_COMPLETE:
    case WFD_EVENT_PIPELINE_BIND_MASK_COMPLETE:
        /* bind event are put to queue tail */
        newRoot = OWF_List_Append(eventQueue, node);
        break;

    case WFD_EVENT_PORT_ATTACH_DETACH:
    case WFD_EVENT_PORT_PROTECTION_FAILURE:
        /* replace old event with new data */

        oldEvent = WFD_Event_FindPreviousEventByPortId(type, eventQueue, node);
        if (!oldEvent)
        {
            newRoot = OWF_List_Append(eventQueue, node);
        }
        else
        {
            /* replace existing with new data */
            WFD_EVENT* newEvent = (WFD_EVENT*)node->data;
            if ( type == WFD_EVENT_PORT_ATTACH_DETACH )
            {
                oldEvent->data.portAttachEvent.attached =
                    newEvent->data.portAttachEvent.attached;
            }
            else
            {
                /* no attributes to replace */
            }

            /* return new objects to pool */
            OWF_Pool_PutObject(newEvent);
            OWF_Pool_PutObject(node);
        }
        break;

    default:
        OWF_ASSERT(0); /* should newer be here */
        break;
    }

    return newRoot;
}

/* insert an event to event queue of a container */
OWF_API_CALL void OWF_APIENTRY
WFD_Event_Insert(WFD_EVENT_CONTAINER* pEventCont,
                 const WFD_EVENT* pEvent) OWF_APIEXIT
{
    OWF_NODE* node = NULL;
    WFD_EVENT* data = NULL;
    WFDEGLSync sync = WFD_INVALID_SYNC;
    WFDEGLDisplay display;

    OWF_ASSERT(pEventCont && pEvent);

    DPRINT(("WFD_Event_Insert(%p, %x, %d)",
            pEventCont, pEvent->type, pEventCont->queueLength));

    OWF_Mutex_Lock(&pEventCont->mutex);
    {
        if (!WFD_Event_CanInsert(pEventCont, pEvent))
        {
            OWF_Mutex_Unlock(&pEventCont->mutex);
            return;
        }

        node = OWF_Pool_GetObject(pEventCont->nodePool);
        if (!node)
        {
            OWF_ASSERT(node); /* should always succeed!! */
            OWF_Mutex_Unlock(&pEventCont->mutex);
            return;
        }
        data = OWF_Pool_GetObject(pEventCont->eventPool);
        if (!data)
        {
            OWF_ASSERT(data); /* should always succeed!! */
            OWF_Mutex_Unlock(&pEventCont->mutex);
            return;
        }

        memcpy(data, pEvent, sizeof(*data));
        node->data = data;

        pEventCont->eventQueue =
            WFD_Event_InsertByEventType(pEvent->type, pEventCont->eventQueue, node);

        pEventCont->queueLength++;

        DPRINT(("WFD_Event_Insert: queue length now %d", pEventCont->queueLength));

        OWF_Cond_Signal(pEventCont->cond);

        if (pEventCont->sync != WFD_INVALID_SYNC)
        {
            sync = pEventCont->sync;
            display = pEventCont->display;
        }
    }
    OWF_Mutex_Unlock(&pEventCont->mutex);

    if (sync != WFD_INVALID_SYNC)
    {
        eglSignalSyncKHR(display, sync, EGL_SIGNALED_KHR);
    }
}


#ifdef __cplusplus
}
#endif
