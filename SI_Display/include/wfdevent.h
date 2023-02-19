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
 *  \file wfdevent.h
 *  \brief Event and event container handling interface
 *
 */

#ifndef WFDEVENT_H_
#define WFDEVENT_H_

#include "WF/wfd.h"
#include "wfdstructs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Create an event container
 *
 *  \param pDevice Pointer to device object
 *  \param attribList Attribute list. See OpenWF specification
 *
 *  \return Handle to the created container of WFD_INVALID_HANDLE
 *  if creation fails.
 */
OWF_API_CALL WFDEvent OWF_APIENTRY WFD_Event_CreateContainer(
    WFD_DEVICE* pDevice, const WFDint* attribList) OWF_APIEXIT;

/*! \brief Destroy an event container
 *
 *  \param pDevice Pointer to device object
 *  \param pEventCont Pointer to an event container
 */
OWF_API_CALL void OWF_APIENTRY WFD_Event_DestroyContainer(
    WFD_DEVICE* pDevice, WFD_EVENT_CONTAINER* pEventCont) OWF_APIEXIT;

/*! \brief Find an event container object by handle
 *
 *  \param pDevice Pointer to device object
 *  \param event Handle to event container
 *
 *  \return Pointer to the event container object
 */
OWF_API_CALL WFD_EVENT_CONTAINER* OWF_APIENTRY
WFD_Event_FindByHandle(WFD_DEVICE* pDevice, WFDEvent event) OWF_APIEXIT;

/*! \brief Read event container attributes
 *
 *  \param pEventCont Pointer to an event container
 *  \param attrib Attribute name.
 *
 *  \return Value of the attribute
 */
OWF_API_CALL WFDint OWF_APIENTRY WFD_Event_GetAttribi(
    WFD_EVENT_CONTAINER* pEventCont, WFDEventAttrib attrib) OWF_APIEXIT;

/*! \brief Set event filter
 *
 * The filter parameter defines the event type that will
 * be queued to event containers queue. The event types not
 * in filter list will be dropped. By default all
 * events are queue.
 *
 *  \param pEventCont Pointer to an event container
 *  \param filter An array of event types terminated by
 *  WFD_NONE.
 */
OWF_API_CALL void OWF_APIENTRY WFD_Event_SetFilter(
    WFD_EVENT_CONTAINER* pEventCont, const WFDEventType* filter) OWF_APIEXIT;

/*! \brief Set-up asynchronic notification
 *
 *  The routine stores a sync object which is signalled
 *  when next event is queued to event container's queue.
 *
 *  \param pEventCont Pointer to an event container
 *  \param display EGL Display defining the sync object
 *  \param sync EGL Sync object
 *
 *
 */
OWF_API_CALL void OWF_APIENTRY WFD_Event_Async(WFD_EVENT_CONTAINER* pEventCont,
                                               WFDEGLDisplay display,
                                               WFDEGLSync sync) OWF_APIEXIT;

/*! \brief Wait on event queue
 *
 *  Block the caller to wait for an event to be queue to
 *  event container. If containers event queue is not empty,
 *  the routine returns immediately without blocking.
 *
 *  \param pEventCont Pointer to an event container
 *  \param timeout Maximum time to wait. If timeout is
 *  zero, the routine never blocks. If timeout is WFD_FOREVER,
 *  the routine will wait until an even occurs.
 *
 *  \return Type of the event occurred. WFD_EVENT_INVALID is
 *  returned if the routine fails, e.g. timeout is detected or
 *  the container has already a waiting thread.
 *
 */
OWF_API_CALL WFDEventType OWF_APIENTRY
WFD_Event_Wait(WFD_EVENT_CONTAINER* pEventCont, WFDtime timeout) OWF_APIEXIT;

/*! \brief Append an event to event queues of all event containers created for
 * the device
 *
 *  This routine is called by a event generating routine and
 *  is used to record an event for all event listeners. For
 *  each container conditions for appending or rejecting are
 *  checked separately (see: WFC_Event_Insert())
 *
 *  \param pDevice Pointer to device object
 *  \param pEvent Pointer to an event object
 */
OWF_API_CALL void OWF_APIENTRY
WFD_Event_InsertAll(WFD_DEVICE* pDevice, const WFD_EVENT* pEvent) OWF_APIEXIT;

/*! \brief Append an event to the event queue of an event container
 *
 *  This routine appends an event to a specific event queue.
 *  Event is not appended if event type is specified filtered or event is
 *  an bind event or bind queue is full. In other cases, it
 *  is guaranteed that event will be placed to queue.
 *
 *  A waiting thread on event queue is signalled. Also, if there
 *  is an EGL sync object stored with event container, the sync
 *  object is signalled.
 *
 *  \param pEventCont Pointer to an event container
 *  \param pEvent Pointer to an event object
 *
 *  \return
 */
OWF_API_CALL void OWF_APIENTRY WFD_Event_Insert(
    WFD_EVENT_CONTAINER* pEventCont, const WFD_EVENT* pEvent) OWF_APIEXIT;

#ifdef __cplusplus
}
#endif

#endif /* WFDEVENT_H_ */
