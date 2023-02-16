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
 *  \file wfdapi.c
 *  \brief OpenWF Display SI, API implementation.
 *
 *  For function documentations, see OpenWF Display specification 1.0
 *
 *  The general layout of an API function is:
 *  - grab API mutex (lock API)
 *  - check parameter validity
 *  - invoke implementation function (WFD_...)
 *  - unlock API
 *  - return
 *
 */

#ifdef __cplusplus
extern "C" {
#endif


#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <WF/wfd.h>
#include <WF/wfdext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "wfdstructs.h"
#include "wfdhandle.h"
#include "wfddevice.h"
#include "wfdevent.h"
#include "wfdport.h"
#include "wfdpipeline.h"
#include "wfdimageprovider.h"
#include "wfdutils.h"
#include "wfddebug.h"

#include "owfimage.h"
#include "owfmemory.h"
#include "owfscreen.h"

#define GET_DEVICE(d, h, retval) \
    WFD_Lock(); \
    d = WFD_Device_FindByHandle(h); \
    COND_FAIL(NULL != d, WFD_ERROR_BAD_DEVICE, retval)

#define GET_DEVICE_NR(d, h) \
    WFD_Lock(); \
    d = WFD_Device_FindByHandle(h); \
    COND_FAIL_NR(NULL != d, WFD_ERROR_BAD_DEVICE)

#define GET_EVENT(d, p, h, retval) \
    p = WFD_Event_FindByHandle(d, h); \
    COND_FAIL(p != NULL, WFD_ERROR_BAD_HANDLE, retval);

#define GET_EVENT_NR(d, p, h) \
    p = WFD_Event_FindByHandle(d, h); \
    COND_FAIL_NR(p != NULL, WFD_ERROR_BAD_HANDLE);

#define GET_PORT(d, p, h, retval) \
    p = WFD_Port_FindByHandle(d, h); \
    COND_FAIL(NULL != p, WFD_ERROR_BAD_HANDLE, retval);

#define GET_PORT_NR(d, p, h) \
    p = WFD_Port_FindByHandle(d, h); \
    COND_FAIL_NR(NULL != p, WFD_ERROR_BAD_HANDLE);

#define GET_PORT_MODE(p, m, h, retVal) \
    m = WFD_Port_FindMode(p, h); \
    COND_FAIL(NULL != m, WFD_ERROR_ILLEGAL_ARGUMENT, retVal);

#define GET_PIPELINE(d, p, h, retval) \
    p = WFD_Pipeline_FindByHandle(d, h); \
    COND_FAIL(NULL != p, WFD_ERROR_BAD_HANDLE, retval);

#define GET_PIPELINE_NR(d, p, h) \
    p = WFD_Pipeline_FindByHandle(d, h); \
    COND_FAIL_NR(NULL != p, WFD_ERROR_BAD_HANDLE);

#define SET_ERROR(h, e) \
    {\
        WFD_DEVICE* pDevice; \
        pDevice = (WFD_DEVICE*)WFD_Handle_GetObj(h, WFD_DEVICE_HANDLE); \
        if (pDevice) \
        { \
            WFD_Device_SetError(pDevice, e); \
        } \
    }\

#define SUCCEED(retval)    \
    SET_ERROR(device, WFD_ERROR_NONE); \
    WFD_Unlock(); \
    return retval

#define SUCCEED_NR(d) \
    SET_ERROR(device, WFD_ERROR_NONE); \
    WFD_Unlock(); \
    return

#define FAIL(ec,retval) \
    SET_ERROR(device, ec); \
    WFD_Unlock(); \
    return retval

#define FAIL_NR(ec) \
    SET_ERROR(device, ec); \
    WFD_Unlock(); \
    return

#define COND_FAIL(cond,ec,retval) \
    if (!(cond)) { \
        FAIL(ec,retval); \
    }

#define COND_FAIL_NR(cond,ec) \
    if (!(cond)) { \
        FAIL_NR(ec);    \
    }

#define CHECK_ACCESSOR(d, a, f, r) \
{ \
    COND_FAIL(WFD_Util_ValidAccessorForAttrib((WFDint)a,(ATTR_ACCESSOR)f), \
            WFD_ERROR_BAD_ATTRIBUTE, r) \
}

#define CHECK_ACCESSOR_NR(d, a, f) \
{ \
    COND_FAIL_NR(WFD_Util_ValidAccessorForAttrib((WFDint)a,(ATTR_ACCESSOR)f), \
            WFD_ERROR_BAD_ATTRIBUTE) \
}

#define PORT_MODE_IS_SET(p) \
    (WFD_INVALID_HANDLE != WFD_Port_GetCurrentMode(p)) \


static OWF_MUTEX           mutex;

static void WFD_Cleanup()
{
    OWF_Mutex_Destroy(&mutex);
}

static void WFD_Lock()
{
    if (!mutex)
    {
        OWF_Mutex_Init(&mutex);
        atexit(WFD_Cleanup);
    }
    OWF_Mutex_Lock(&mutex);
}

static void WFD_Unlock()
{
    OWF_Mutex_Unlock(&mutex);
}

/* =================================================================== */
/*                     2.11.  E r r o r s                              */
/* =================================================================== */


WFD_API_CALL WFDErrorCode WFD_APIENTRY
wfdGetError(WFDDevice device) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFDErrorCode            ec;

    /* check preconditions */
    GET_DEVICE(pDevice, device, WFD_ERROR_BAD_DEVICE);
    ec = WFD_Device_GetError(pDevice);
    WFD_Unlock();
    return ec;
}


/* =================================================================== */
/*                         3.  D e v i c e s                           */
/* =================================================================== */


WFD_API_CALL WFDint WFD_APIENTRY
wfdEnumerateDevices(WFDint *deviceIds,
                    WFDint deviceIdsCount,
                    const WFDint *filterList) WFD_APIEXIT
{
    WFDint i = 0;

    DPRINT(("wfdEnumerateDevices(%p,%d, %p)", deviceIds, deviceIdsCount, filterList));

    if ((NULL != deviceIds) && (deviceIdsCount <= 0))
    {
        return 0; /* failure - zero or negative device count */
    }

    if (NULL == filterList || filterList[0] == WFD_NONE)
    {
        /* nothing to filter */
        return WFD_Device_GetIds(deviceIds, deviceIdsCount);
    }

    /* check the filter list, only WFD_DEVICE_FILTER_PORT_ID accepted */
    for (i = 0; filterList[i] != WFD_NONE; i += 2)
    {
        if (filterList[i] != WFD_DEVICE_FILTER_PORT_ID)
            return 0;

        /* return all ids if port id has value WFD_INVALID_HANDLE */
        if (filterList[i+1] == WFD_INVALID_HANDLE)
            return WFD_Device_GetIds(deviceIds, deviceIdsCount);
    }

    return WFD_Device_FilterIds(deviceIds, deviceIdsCount, filterList);
}


WFD_API_CALL WFDDevice WFD_APIENTRY
wfdCreateDevice (WFDint deviceId, const WFDint *attribList)
{
    WFD_DEVICE_CONFIG* pDevConfig;
    WFDDevice device;

    DPRINT(("wfdCreateDevice(%d,%p)", deviceId, attribList));

    /* check preconditions
     * 1. OpenWF display 1.0 allows no attributes
     * 2. a device matching the id should exist
     * 3. if device is outstanding, an attempt to create should fail
     */

    if (attribList != NULL && *attribList != WFD_NONE){
        DPRINT(("  no attributes allowed\n"));
        return WFD_ERROR_BAD_ATTRIBUTE;
    }

    pDevConfig = WFD_Device_FindById(deviceId);
    if (NULL == pDevConfig)
    {
        DPRINT(("  couldn't find device with id %d\n", deviceId));
        return WFD_INVALID_HANDLE;
    }

    WFD_Lock();
    if (WFD_Device_IsAllocated(deviceId))
    {
        DPRINT(("  device already created %d\n", deviceId));
        WFD_Unlock();
        return WFD_INVALID_HANDLE;
    }

    device = WFD_Device_Allocate(deviceId);

    DPRINT(("  Device creation done\n"));

    SUCCEED(device); /* unlock and return */
}

WFD_API_CALL WFDErrorCode WFD_APIENTRY
wfdDestroyDevice(WFDDevice device) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;

    DPRINT(("wfdDestroyDevice(0x%08x)", device));

    GET_DEVICE(pDevice, device, WFD_ERROR_BAD_DEVICE);

    WFD_Device_Release(pDevice);

    WFD_Unlock(); /* don't use SUCCEED-macro, because device is gone */
    return WFD_ERROR_NONE;
}

WFD_API_CALL WFDint WFD_APIENTRY
wfdGetDeviceAttribi(WFDDevice device,
                    WFDDeviceAttrib attrib) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFDint                  value = 0;
    WFDErrorCode            ec;

    DPRINT(("wfdGetDeviceAttribi(%d,%d,%d)", device, attrib, value));

    GET_DEVICE(pDevice, device, 0);

    CHECK_ACCESSOR(device, attrib, wfdGetDeviceAttribi, value);

    ec = WFD_Device_GetAttribi(pDevice, attrib, &value);

    FAIL(ec, value); /* note ec can be WFD_ERROR_NONE */
}

WFD_API_CALL void WFD_APIENTRY
wfdSetDeviceAttribi(WFDDevice device,
                    WFDDeviceAttrib attrib,
                    WFDint value)
{
    WFD_DEVICE*     pDevice;
    WFDErrorCode    ec;

    DPRINT(("wfdSetDeviceAttribi(%d,%d,%d)", device, attrib, value));

    GET_DEVICE_NR(pDevice, device);

    CHECK_ACCESSOR_NR(device, attrib, wfdSetDeviceAttribi);

    ec = WFD_Device_SetAttribi(pDevice, attrib, value);

    FAIL_NR(ec); /* note ec can be WFD_ERROR_NONE */
}

WFD_API_CALL void WFD_APIENTRY
 wfdDeviceCommit(WFDDevice device,
        WFDCommitType type,
        WFDHandle handle) WFD_APIEXIT
{
    WFD_DEVICE* pDevice;
    WFD_PORT* pPort = NULL;
    WFD_PIPELINE* pPipeline = NULL;
    WFDErrorCode ec;

    DPRINT(("wfdDeviceCommit(%08x,%08x,%08x)\n", device, type, handle));

    /* check preconditions */
    GET_DEVICE_NR(pDevice, device);

    if (type == WFD_COMMIT_ENTIRE_PORT)
    {
        GET_PORT_NR(pDevice, pPort, handle);
    }
    else if (type == WFD_COMMIT_PIPELINE)
    {
        GET_PIPELINE_NR(pDevice, pPipeline, handle);
    }
    else if (type == WFD_COMMIT_ENTIRE_DEVICE)
    {
        COND_FAIL_NR(handle == WFD_INVALID_HANDLE, WFD_ERROR_BAD_HANDLE);
    }
    else
    {
        FAIL_NR(WFD_ERROR_ILLEGAL_ARGUMENT);
    }

    ec = WFD_Device_Commit(pDevice, pPort, pPipeline);
    FAIL_NR(ec); /* note: ec can be WFD_ERROR_NONE */
}


/* ================================================================== */
/* 3.6. A s y n c h r o n o u s   E v e n t   N o t i f i c a t i o n */
/* ================================================================== */


WFD_API_CALL WFDEvent WFD_APIENTRY
wfdCreateEvent(WFDDevice device, const WFDint *attribList)WFD_APIEXIT
{
    WFD_DEVICE* pDevice;
    WFDEvent handle = WFD_INVALID_HANDLE;

    GET_DEVICE(pDevice, device, WFD_INVALID_HANDLE);

    if (attribList && attribList[0] != WFD_NONE)
    {
        COND_FAIL (attribList[0] ==  WFD_EVENT_PIPELINE_BIND_QUEUE_SIZE,
                WFD_ERROR_BAD_ATTRIBUTE,
                WFD_INVALID_HANDLE);
        COND_FAIL (attribList[2] ==  WFD_NONE,
                WFD_ERROR_BAD_ATTRIBUTE,
                WFD_INVALID_HANDLE);
    }

    handle = WFD_Event_CreateContainer(pDevice, attribList);
    COND_FAIL(WFD_INVALID_HANDLE != handle, WFD_ERROR_OUT_OF_MEMORY,
            WFD_INVALID_HANDLE);

    SUCCEED(handle);
}


WFD_API_CALL void WFD_APIENTRY
wfdDestroyEvent(WFDDevice device,
                WFDEvent event) WFD_APIEXIT
{
    WFD_DEVICE* pDevice;
    WFD_EVENT_CONTAINER* pEventCont;

    GET_DEVICE_NR(pDevice, device);
    GET_EVENT_NR(pDevice, pEventCont, event);

    WFD_Event_DestroyContainer(pDevice, pEventCont);

    SUCCEED_NR();
}

WFD_API_CALL WFDint WFD_APIENTRY
wfdGetEventAttribi(WFDDevice device,
                   WFDEvent event,
                   WFDEventAttrib attrib) WFD_APIEXIT
{
    WFD_DEVICE*            pDevice;
    WFD_EVENT_CONTAINER*   pEventCont;
    WFDint                 value = 0;

    GET_DEVICE(pDevice, device, 0);
    GET_EVENT(pDevice, pEventCont, event, 0);

    CHECK_ACCESSOR(device, attrib, wfdGetEventAttribi, value);

    value = WFD_Event_GetAttribi(pEventCont, attrib);

    SUCCEED(value);
}


WFD_API_CALL void WFD_APIENTRY
wfdDeviceEventAsync(WFDDevice device,
                    WFDEvent event,
                    WFDEGLDisplay dpy,
                    WFDEGLSync sync) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_EVENT_CONTAINER*    pEventCont;
    EGLint                  attribValue = 0;
    EGLBoolean              ret;

    GET_DEVICE_NR(pDevice, device);
    GET_EVENT_NR(pDevice, pEventCont, event);

    /* display parameter check can be refined with real egl implementation */
    COND_FAIL_NR(sync != WFD_INVALID_SYNC, WFD_ERROR_ILLEGAL_ARGUMENT);

    ret = eglGetSyncAttribKHR(dpy, sync, EGL_SYNC_TYPE_KHR, &attribValue);
    COND_FAIL_NR(ret == EGL_TRUE && attribValue == EGL_SYNC_REUSABLE_KHR,
                 WFD_ERROR_ILLEGAL_ARGUMENT)

    WFD_Event_Async(pEventCont, dpy, sync);

    SUCCEED_NR();
}

WFD_API_CALL WFDEventType WFD_APIENTRY
wfdDeviceEventWait(WFDDevice device,
                   WFDEvent event,
                   WFDtime timeout) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_EVENT_CONTAINER*    pEventCont;
    WFDEventType            eventType;

    GET_DEVICE(pDevice, device, WFD_EVENT_INVALID);
    GET_EVENT(pDevice, pEventCont, event, WFD_EVENT_INVALID);

    /* should not call WFD_Event_Wait when holding API-lock */
    WFD_Unlock();
    eventType = WFD_Event_Wait(pEventCont, timeout);
    WFD_Lock();
    COND_FAIL(eventType!=WFD_EVENT_INVALID, WFD_ERROR_NOT_SUPPORTED,
            WFD_EVENT_INVALID);

    SUCCEED(eventType);
}

WFD_API_CALL void WFD_APIENTRY
wfdDeviceEventFilter(WFDDevice device,
                     WFDEvent event,
                     const WFDEventType *filter) WFD_APIEXIT
{
    WFD_DEVICE*            pDevice;
    WFD_EVENT_CONTAINER*   pEventCont;
    WFDint                 i;

    GET_DEVICE_NR(pDevice, device);
    GET_EVENT_NR(pDevice, pEventCont, event);

    if (filter)
    {
        for (i=0; filter[i] != WFD_NONE; i++)
        {
            COND_FAIL_NR(filter[i]>= WFD_FIRST_FILTERED &&
                         filter[i]<= WFD_LAST_FILTERED,
                         WFD_ERROR_ILLEGAL_ARGUMENT);
        }
    }

    WFD_Event_SetFilter(pEventCont, filter);

    SUCCEED_NR();
}

/* ============================================================================= */
/*                             4. P o r t s                                      */
/* ============================================================================= */

WFD_API_CALL WFDint WFD_APIENTRY
wfdEnumeratePorts(WFDDevice device,
                  WFDint *portIds,
                  WFDint idsCount,
                  const WFDint *filterList)
{
    WFD_DEVICE*                pDevice;
    WFDint                    count = 0;

    DPRINT(("wfdEnumeratePorts(%08x,%p,%d,%p)", device, portIds, idsCount, filterList));

    GET_DEVICE(pDevice, device, 0);

    COND_FAIL( ((portIds !=  NULL && idsCount > 0) || (portIds == NULL)),
            WFD_ERROR_ILLEGAL_ARGUMENT, 0);

    if (NULL == filterList || filterList[0] == WFD_NONE)
    {
        /* nothing to filter */
        count = WFD_Port_GetIds(pDevice, portIds, idsCount);
    }
    else
    {
        /* Check the filter list.  Currently no valid filtering attributes are
          are defined for filterList.
          
          Nothing to do since count is defaulted to zero. */
    }

    SUCCEED(count);
}

WFD_API_CALL WFDPort WFD_APIENTRY
wfdCreatePort(WFDDevice device,
              WFDint portId,
              const WFDint *attribList) WFD_APIEXIT
{
    WFD_DEVICE*         pDevice;
    WFDErrorCode        ec;
    WFDPort             handle;

    DPRINT(("wfdCreatePort(%08x,%d,%d)", device, portId, attribList));

    GET_DEVICE(pDevice, device, WFD_INVALID_HANDLE);

    COND_FAIL (!(attribList != NULL && attribList[0] != WFD_NONE),
                WFD_ERROR_BAD_ATTRIBUTE,
                WFD_INVALID_HANDLE);

    ec = WFD_Port_IsAllocated(pDevice, portId);
    COND_FAIL(WFD_ERROR_NONE == ec, ec, WFD_INVALID_HANDLE);

    handle = WFD_Port_Allocate(pDevice, portId);
    COND_FAIL(WFD_INVALID_HANDLE != handle,
            WFD_ERROR_OUT_OF_MEMORY,
            WFD_INVALID_HANDLE);

    SUCCEED(handle);
}


WFD_API_CALL void WFD_APIENTRY
wfdDestroyPort(WFDDevice device, WFDPort port) WFD_APIEXIT
{
    WFD_DEVICE*                pDevice;
    WFD_PORT*                pPort;

    GET_DEVICE_NR(pDevice, device);
    GET_PORT_NR(pDevice, pPort, port);

    WFD_Port_Release(pDevice, pPort);

    SUCCEED_NR();
}


WFD_API_CALL WFDint WFD_APIENTRY
wfdGetPortModes(WFDDevice device,
                WFDPort port,
                WFDPortMode *modes,
                WFDint modesCount) WFD_APIEXIT
{
    WFD_DEVICE*    pDevice;
    WFD_PORT*      pPort;
    WFDint         count;

    GET_DEVICE(pDevice, device, 0);
    GET_PORT(pDevice, pPort, port, 0);

    COND_FAIL(pPort->config->attached, WFD_ERROR_NOT_SUPPORTED, 0)

    COND_FAIL((modesCount > 0 && modes) || (!modes), WFD_ERROR_ILLEGAL_ARGUMENT, 0);

    count = WFD_Port_GetModes(pPort, modes, modesCount);
    SUCCEED(count);
}


WFD_API_CALL WFDint WFD_APIENTRY
wfdGetPortModeAttribi(WFDDevice device,
                      WFDPort port,
                      WFDPortMode mode,
                      WFDPortModeAttrib attrib) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PORT*               pPort;
    WFD_PORT_MODE*          pPortMode;
    WFDint                  value = 0;
    WFDErrorCode            ec;

    GET_DEVICE(pDevice, device, 0);
    GET_PORT(pDevice, pPort, port, 0);
    GET_PORT_MODE(pPort, pPortMode, mode, 0);

    CHECK_ACCESSOR(device, attrib, wfdGetPortModeAttribi, value);

    ec = WFD_PortMode_GetAttribi(pPortMode, attrib, &value);

    FAIL(ec, value);
}


WFD_API_CALL WFDfloat WFD_APIENTRY
wfdGetPortModeAttribf(WFDDevice device,
                      WFDPort port,
                      WFDPortMode mode,
                      WFDPortModeAttrib attrib) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PORT*               pPort;
    WFD_PORT_MODE*          pPortMode;
    WFDfloat                value = 0.0f;
    WFDErrorCode            ec;

    GET_DEVICE(pDevice, device, 0.0);
    GET_PORT(pDevice, pPort, port, 0.0);
    GET_PORT_MODE(pPort, pPortMode, mode, 0);

    CHECK_ACCESSOR(device, attrib, wfdGetPortModeAttribf, value);

    ec = WFD_PortMode_GetAttribf(pPortMode, attrib, &value);

    FAIL(ec, value);
}


WFD_API_CALL void WFD_APIENTRY
wfdSetPortMode(WFDDevice device,
               WFDPort port,
               WFDPortMode mode) WFD_APIEXIT
{
    WFD_DEVICE*         pDevice;
    WFD_PORT*           pPort;
    WFDboolean          succeed;


    GET_DEVICE_NR(pDevice, device);
    GET_PORT_NR(pDevice, pPort, port);

    COND_FAIL_NR(WFD_TRUE == pPort->config->attached, WFD_ERROR_NOT_SUPPORTED);

    succeed = WFD_Port_SetMode(pPort, mode);

    COND_FAIL_NR(succeed == WFD_TRUE, WFD_ERROR_ILLEGAL_ARGUMENT);

    SUCCEED_NR();
}


WFD_API_CALL WFDPortMode WFD_APIENTRY
wfdGetCurrentPortMode(WFDDevice device, WFDPort port) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PORT*               pPort;
    WFDPortMode             currentMode;

    GET_DEVICE(pDevice, device, WFD_INVALID_HANDLE);
    GET_PORT(pDevice, pPort, port, WFD_INVALID_HANDLE);

    currentMode = WFD_Port_GetCurrentMode(pPort);

    COND_FAIL(WFD_INVALID_HANDLE != currentMode,
            WFD_ERROR_NOT_SUPPORTED,
            WFD_INVALID_HANDLE);

    SUCCEED(currentMode);
}


WFD_API_CALL WFDint WFD_APIENTRY
wfdGetPortAttribi(WFDDevice device,
                  WFDPort port,
                  WFDPortConfigAttrib attrib) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PORT*               pPort;
    WFDint                  value;
    WFDErrorCode            ec;

    GET_DEVICE(pDevice, device, 0);
    GET_PORT(pDevice, pPort, port, 0);

    CHECK_ACCESSOR(device, attrib, wfdGetPortAttribi, value);

    ec = WFD_Port_GetAttribi(pPort, attrib, &value);
    FAIL(ec, value);
}


WFD_API_CALL WFDfloat WFD_APIENTRY
wfdGetPortAttribf(WFDDevice device,
                  WFDPort port,
                  WFDPortConfigAttrib attrib) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PORT*               pPort;
    WFDfloat                value;
    WFDErrorCode            ec;

    GET_DEVICE(pDevice, device, 0.0);
    GET_PORT(pDevice, pPort, port, 0.0);

    CHECK_ACCESSOR(device, attrib, wfdGetPortAttribf, value);

    ec = WFD_Port_GetAttribf(pPort, attrib, &value);
    FAIL(ec, value);
}


WFD_API_CALL void WFD_APIENTRY
wfdGetPortAttribiv(WFDDevice device,
                   WFDPort port,
                   WFDPortConfigAttrib attrib,
                   WFDint count,
                   WFDint *value) WFD_APIEXIT
{
    WFD_DEVICE*         pDevice;
    WFD_PORT*           pPort;
    WFDErrorCode        ec;

    GET_DEVICE_NR(pDevice, device);
    GET_PORT_NR(pDevice, pPort, port);

    COND_FAIL_NR(count > 0, WFD_ERROR_ILLEGAL_ARGUMENT);
    COND_FAIL_NR(value != NULL, WFD_ERROR_ILLEGAL_ARGUMENT);

    CHECK_ACCESSOR_NR(device, attrib, wfdGetPortAttribiv);

    ec = WFD_Port_GetAttribiv(pPort, attrib, count, value);
    FAIL_NR(ec);
}


WFD_API_CALL void WFD_APIENTRY
wfdGetPortAttribfv(WFDDevice device,
                   WFDPort port,
                   WFDPortConfigAttrib attrib,
                   WFDint count,
                   WFDfloat *value) WFD_APIEXIT
{
    WFD_DEVICE*            pDevice;
    WFD_PORT*              pPort;
    WFDErrorCode           ec;

    GET_DEVICE_NR(pDevice, device);
    GET_PORT_NR(pDevice, pPort, port);

    COND_FAIL_NR(count > 0, WFD_ERROR_ILLEGAL_ARGUMENT);
    COND_FAIL_NR(value != NULL, WFD_ERROR_ILLEGAL_ARGUMENT);

    CHECK_ACCESSOR_NR(device, attrib, wfdGetPortAttribfv);

    ec = WFD_Port_GetAttribfv(pPort, attrib, count, value);
    FAIL_NR(ec);
}


WFD_API_CALL void WFD_APIENTRY
wfdSetPortAttribi(WFDDevice device,
                  WFDPort port,
                  WFDPortConfigAttrib attrib,
                  WFDint value) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PORT*               pPort;
    WFDErrorCode            ec;

    GET_DEVICE_NR(pDevice, device);
    GET_PORT_NR(pDevice, pPort, port);

    COND_FAIL_NR(PORT_MODE_IS_SET(pPort), WFD_ERROR_NOT_SUPPORTED);

    CHECK_ACCESSOR_NR(device, attrib, wfdSetPortAttribi);

    ec = WFD_Port_SetAttribi(pPort, attrib, value);
    FAIL_NR(ec);
}


WFD_API_CALL void WFD_APIENTRY
wfdSetPortAttribf(WFDDevice device,
                  WFDPort port,
                  WFDPortConfigAttrib attrib,
                  WFDfloat value) WFD_APIEXIT
{
    WFD_DEVICE*                pDevice;
    WFD_PORT*                pPort;
    WFDErrorCode            ec;

    GET_DEVICE_NR(pDevice, device);
    GET_PORT_NR(pDevice, pPort, port);

    COND_FAIL_NR(PORT_MODE_IS_SET(pPort), WFD_ERROR_NOT_SUPPORTED);

    CHECK_ACCESSOR_NR(device, attrib, wfdSetPortAttribf);

    ec = WFD_Port_SetAttribf(pPort, attrib, value);
    FAIL_NR(ec);
}


WFD_API_CALL void WFD_APIENTRY
wfdSetPortAttribiv(WFDDevice device,
                   WFDPort port,
                   WFDPortConfigAttrib attrib,
                   WFDint count,
                   const WFDint *value) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PORT*               pPort;
    WFDErrorCode            ec;

    GET_DEVICE_NR(pDevice, device);
    GET_PORT_NR(pDevice, pPort, port);

    COND_FAIL_NR(PORT_MODE_IS_SET(pPort), WFD_ERROR_NOT_SUPPORTED);
    COND_FAIL_NR(count > 0, WFD_ERROR_ILLEGAL_ARGUMENT);
    COND_FAIL_NR(value != NULL, WFD_ERROR_ILLEGAL_ARGUMENT);

    CHECK_ACCESSOR_NR(device, attrib, wfdSetPortAttribiv);

    ec = WFD_Port_SetAttribiv(pPort, attrib, count, value);
    FAIL_NR(ec);
}


WFD_API_CALL void WFD_APIENTRY
wfdSetPortAttribfv(WFDDevice device,
                   WFDPort port,
                   WFDPortConfigAttrib attrib,
                   WFDint count,
                   const WFDfloat *value) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PORT*               pPort;
    WFDErrorCode            ec;

    GET_DEVICE_NR(pDevice, device);
    GET_PORT_NR(pDevice, pPort, port);

    COND_FAIL_NR(PORT_MODE_IS_SET(pPort), WFD_ERROR_NOT_SUPPORTED);
    COND_FAIL_NR(count > 0, WFD_ERROR_ILLEGAL_ARGUMENT);
    COND_FAIL_NR(value != NULL, WFD_ERROR_ILLEGAL_ARGUMENT);

    CHECK_ACCESSOR_NR(device, attrib, wfdSetPortAttribfv);

    ec = WFD_Port_SetAttribfv(pPort, attrib, count, value);
    FAIL_NR(ec);
}


WFD_API_CALL void WFD_APIENTRY
wfdBindPipelineToPort(WFDDevice device,
                      WFDPort port,
                      WFDPipeline pipeline) WFD_APIEXIT
{
    WFD_DEVICE*              pDevice;
    WFD_PORT*                pPort;
    WFD_PIPELINE*            pPipeline;

    GET_DEVICE_NR(pDevice, device);
    GET_PORT_NR(pDevice, pPort, port);
    GET_PIPELINE_NR(pDevice, pPipeline, pipeline);

    if (WFD_Port_PipelineBindable(pPort, pPipeline->config->id))
    {
        WFD_Port_PipelineCacheBinding(pPort, pPipeline);
        SUCCEED_NR();
    }
    else
    {
        FAIL_NR(WFD_ERROR_BAD_HANDLE);
    }

}


WFD_API_CALL WFDint WFD_APIENTRY
wfdGetDisplayDataFormats(WFDDevice device,
                         WFDPort port,
                         WFDDisplayDataFormat *format,
                         WFDint formatCount) WFD_APIEXIT
{
    WFD_DEVICE*         pDevice;
    WFD_PORT*           pPort;
    WFDint              retVal = 0;

    GET_DEVICE(pDevice, device, 0);
    GET_PORT(pDevice, pPort, port, 0);

    COND_FAIL((format && formatCount>0) || !format, WFD_ERROR_ILLEGAL_ARGUMENT, 0);

    retVal = WFD_Port_GetDisplayDataFormats(pPort, format, formatCount);

    SUCCEED(retVal);
}


WFD_API_CALL WFDint WFD_APIENTRY
wfdGetDisplayData(WFDDevice device,
                  WFDPort port,
                  WFDDisplayDataFormat format,
                  WFDuint8 *data,
                  WFDint dataCount) WFD_APIEXIT
{
    WFD_DEVICE*         pDevice;
    WFD_PORT*           pPort;
    WFDint              retVal = 0;

    GET_DEVICE(pDevice, device, 0);
    GET_PORT(pDevice, pPort, port, 0);

    COND_FAIL(WFD_Port_HasDisplayData(pPort, format),
              WFD_ERROR_ILLEGAL_ARGUMENT, 0);

    COND_FAIL(dataCount > 0, WFD_ERROR_ILLEGAL_ARGUMENT, 0);

    retVal =
        WFD_Port_GetDisplayData(pPort, format, data, dataCount);

    SUCCEED(retVal);
}

/* ============================================================================= */
/*                         5.  P i p e l i n e                                   */
/* ============================================================================= */

WFD_API_CALL WFDint WFD_APIENTRY
wfdEnumeratePipelines(WFDDevice device,
                      WFDint *pipelineIds,
                      WFDint idsCount,
                      const WFDint *filterList) WFD_APIEXIT
{
    WFD_DEVICE*                pDevice;
    WFDint                    count = 0;

    DPRINT(("wfdEnumeratePipelines(%p,%p,%d,%p)", device, pipelineIds, idsCount, filterList));

    GET_DEVICE(pDevice, device, 0);

    COND_FAIL( ((pipelineIds !=  NULL && idsCount > 0) ||
                (pipelineIds == NULL)),
               WFD_ERROR_ILLEGAL_ARGUMENT,
               0);

    if (NULL == filterList || filterList[0] == WFD_NONE)
    {
        /* nothing to filter */
        count = WFD_Pipeline_GetIds(pDevice, pipelineIds, idsCount);
    }
    else
    {
        /* Check the filter list.  Currently no valid filtering attributes are
          are defined for filterList.
          
          Nothing to do since count is defaulted to zero. */
    }

    SUCCEED(count);
}

WFD_API_CALL WFDPipeline WFD_APIENTRY
wfdCreatePipeline(WFDDevice device,
                  WFDint pipelineId,
                  const WFDint *attribList) WFD_APIEXIT
{
    WFD_DEVICE*         pDevice;
    WFDErrorCode        ec;
    WFDPipeline         handle;

    attribList = attribList;

    GET_DEVICE(pDevice, device, WFD_INVALID_HANDLE);

    COND_FAIL (!(attribList != NULL && attribList[0] != WFD_NONE),
                WFD_ERROR_BAD_ATTRIBUTE,
                WFD_INVALID_HANDLE);

    ec = WFD_Pipeline_IsAllocated(pDevice, pipelineId);
    COND_FAIL(WFD_ERROR_NONE == ec, ec, WFD_INVALID_HANDLE);

    handle = WFD_Pipeline_Allocate(pDevice, pipelineId);
    COND_FAIL(handle != WFD_INVALID_HANDLE,
            WFD_ERROR_OUT_OF_MEMORY,
            WFD_INVALID_HANDLE);

    SUCCEED(handle);
}


WFD_API_CALL void WFD_APIENTRY
wfdDestroyPipeline(WFDDevice device,
                   WFDPipeline pipeline) WFD_APIEXIT
{
    WFD_DEVICE*                pDevice;
    WFD_PIPELINE*            pPipeline;

    GET_DEVICE_NR(pDevice, device);
    GET_PIPELINE_NR(pDevice, pPipeline, pipeline);

    WFD_Pipeline_Release(pDevice, pPipeline);

    SUCCEED_NR();
}


WFD_API_CALL WFDSource WFD_APIENTRY
wfdCreateSourceFromImage(WFDDevice device,
                         WFDPipeline pipeline,
                         WFDEGLImage image,
                         const WFDint *attribList) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PIPELINE*           pPipeline;
    WFD_SOURCE*             pSource;
    WFDErrorCode            ec = WFD_ERROR_OUT_OF_MEMORY;

    GET_DEVICE(pDevice, device, WFD_INVALID_HANDLE);
    GET_PIPELINE(pDevice, pPipeline, pipeline, WFD_INVALID_HANDLE);

    COND_FAIL( (attribList == NULL || attribList[0] == WFD_NONE),
               WFD_ERROR_BAD_ATTRIBUTE, WFD_INVALID_HANDLE );

    ec = WFD_Pipeline_IsImageValidSource(pPipeline, image);
    COND_FAIL(ec == WFD_ERROR_NONE, ec, WFD_INVALID_HANDLE );

    pSource = WFD_Device_CreateImageProvider(pDevice,
                                             pPipeline,
                                             image,
                                             WFD_IMAGE_SOURCE);
    if (pSource)
    {
        SUCCEED(pSource->handle);
    }
    else
    {
        FAIL(WFD_ERROR_OUT_OF_MEMORY, WFD_INVALID_HANDLE);
    }
}


WFD_API_CALL WFDSource WFD_APIENTRY
wfdCreateSourceFromStream(WFDDevice device,
                          WFDPipeline pipeline,
                          WFDNativeStreamType stream,
                          const WFDint *attribList) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PIPELINE*           pPipeline;
    WFD_IMAGE_PROVIDER*     pSource;
    WFDErrorCode            ec = WFD_ERROR_OUT_OF_MEMORY;

    GET_DEVICE(pDevice, device, WFD_INVALID_HANDLE);
    GET_PIPELINE(pDevice, pPipeline, pipeline, WFD_INVALID_HANDLE);

    COND_FAIL( (attribList == NULL || attribList[0] == WFD_NONE),
               WFD_ERROR_BAD_ATTRIBUTE, WFD_INVALID_HANDLE );

    ec = WFD_Pipeline_IsStreamValidSource(pPipeline, stream);
    COND_FAIL(ec == WFD_ERROR_NONE, ec, WFD_INVALID_HANDLE );

    pSource = WFD_Device_CreateStreamProvider(pDevice,
                                              pPipeline,
                                              stream,
                                              WFD_IMAGE_SOURCE);
    if (pSource)
    {
        SUCCEED(pSource->handle);
    }
    else
    {
        FAIL(WFD_ERROR_OUT_OF_MEMORY, WFD_INVALID_HANDLE);
    }
}


WFD_API_CALL void WFD_APIENTRY
wfdDestroySource(WFDDevice device,
                 WFDSource source) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFDErrorCode            ec;

    GET_DEVICE_NR(pDevice, device);

    ec = WFD_Device_DestroyImageProvider(pDevice, source);
    FAIL_NR(ec);
}


WFD_API_CALL WFDMask WFD_APIENTRY
wfdCreateMaskFromImage(WFDDevice device,
                       WFDPipeline pipeline,
                       WFDEGLImage image,
                       const WFDint *attribList) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PIPELINE*           pPipeline;
    WFD_IMAGE_PROVIDER*     pMask;
    WFDErrorCode            ec = WFD_ERROR_OUT_OF_MEMORY;

    GET_DEVICE(pDevice, device, WFD_INVALID_HANDLE);
    GET_PIPELINE(pDevice, pPipeline, pipeline, WFD_INVALID_HANDLE);

    COND_FAIL( (attribList == NULL || attribList[0] == WFD_NONE),
               WFD_ERROR_BAD_ATTRIBUTE, WFD_INVALID_HANDLE );

    ec = WFD_Pipeline_IsImageValidMask(pPipeline, image);
    COND_FAIL(ec == WFD_ERROR_NONE, ec, WFD_INVALID_HANDLE );

    pMask = WFD_Device_CreateImageProvider(pDevice,
                                           pPipeline,
                                           image,
                                           WFD_IMAGE_MASK);
    if (pMask)
    {
        SUCCEED(pMask->handle);
    }
    else
    {
        FAIL(WFD_ERROR_OUT_OF_MEMORY, WFD_INVALID_HANDLE);
    }
}


WFD_API_CALL WFDMask WFD_APIENTRY
wfdCreateMaskFromStream(WFDDevice device,
                        WFDPipeline pipeline,
                        WFDNativeStreamType stream,
                        const WFDint *attribList) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PIPELINE*           pPipeline;
    WFD_MASK*               pMask;
    WFDErrorCode            ec = WFD_ERROR_OUT_OF_MEMORY;

    GET_DEVICE(pDevice, device, WFD_INVALID_HANDLE);
    GET_PIPELINE(pDevice, pPipeline, pipeline, WFD_INVALID_HANDLE);

    COND_FAIL( (attribList == NULL || attribList[0] == WFD_NONE),
               WFD_ERROR_BAD_ATTRIBUTE, WFD_INVALID_HANDLE );

    ec = WFD_Pipeline_IsStreamValidMask(pPipeline, stream);
    COND_FAIL(ec == WFD_ERROR_NONE, ec, WFD_INVALID_HANDLE );

    pMask = WFD_Device_CreateStreamProvider(pDevice,
                                            pPipeline,
                                            stream,
                                            WFD_IMAGE_MASK);

    if (pMask)
    {
        SUCCEED(pMask->handle);
    }
    else
    {
        FAIL(WFD_ERROR_OUT_OF_MEMORY, WFD_INVALID_HANDLE);
    }
}


WFD_API_CALL void WFD_APIENTRY
wfdDestroyMask(WFDDevice device,
               WFDMask mask) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFDErrorCode            ec;

    GET_DEVICE_NR(pDevice, device);

    ec = WFD_Device_DestroyImageProvider(pDevice, mask);
    FAIL_NR(ec);
}

WFD_API_CALL void WFD_APIENTRY
wfdBindSourceToPipeline(WFDDevice device,
                        WFDPipeline pipeline,
                        WFDSource source,
                        WFDTransition transition,
                        const WFDRect *region) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PIPELINE*           pPipeline;
    WFD_SOURCE*             pSource = NULL;

    GET_DEVICE_NR(pDevice, device);
    GET_PIPELINE_NR(pDevice, pPipeline, pipeline);

    if (source != WFD_INVALID_HANDLE)
    {
        pSource = (WFD_SOURCE*)WFD_Handle_GetObj(source, WFD_SOURCE_HANDLE);
        COND_FAIL_NR((pSource !=  NULL), WFD_ERROR_ILLEGAL_ARGUMENT);

        COND_FAIL_NR((pSource->pipeline ==  pPipeline), WFD_ERROR_ILLEGAL_ARGUMENT);
        COND_FAIL_NR((pSource->device ==  pDevice), WFD_ERROR_ILLEGAL_ARGUMENT);
        COND_FAIL_NR(WFD_ImageProvider_IsRegionValid(pSource, region),
                     WFD_ERROR_ILLEGAL_ARGUMENT );
    }

    COND_FAIL_NR( (transition == WFD_TRANSITION_IMMEDIATE ||
                   transition == WFD_TRANSITION_AT_VSYNC),
                  WFD_ERROR_ILLEGAL_ARGUMENT );

    WFD_Pipeline_SourceCacheBinding(pPipeline, pSource, transition, region);

    SUCCEED_NR();
}

WFD_API_CALL void WFD_APIENTRY
wfdBindMaskToPipeline(WFDDevice device,
                      WFDPipeline pipeline,
                      WFDMask mask,
                      WFDTransition transition) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PIPELINE*           pPipeline;
    WFD_MASK*               pMask = NULL;

    GET_DEVICE_NR(pDevice, device);
    GET_PIPELINE_NR(pDevice, pPipeline, pipeline);

    if (mask != WFD_INVALID_HANDLE)
    {
        pMask = (WFD_MASK*) WFD_Handle_GetObj(mask, WFD_MASK_HANDLE);
        COND_FAIL_NR((pMask !=  NULL), WFD_ERROR_ILLEGAL_ARGUMENT);

        COND_FAIL_NR((pMask->pipeline ==  pPipeline), WFD_ERROR_ILLEGAL_ARGUMENT);
        COND_FAIL_NR((pMask->device ==  pDevice), WFD_ERROR_ILLEGAL_ARGUMENT);
    }

    COND_FAIL_NR( (transition == WFD_TRANSITION_IMMEDIATE ||
                   transition == WFD_TRANSITION_AT_VSYNC),
                  WFD_ERROR_ILLEGAL_ARGUMENT );

    WFD_Pipeline_MaskCacheBinding(pPipeline, pMask, transition);

    SUCCEED_NR();
}


WFD_API_CALL WFDint WFD_APIENTRY
wfdGetPipelineAttribi(WFDDevice device,
                      WFDPipeline pipeline,
                      WFDPipelineConfigAttrib attrib) WFD_APIEXIT
{
    WFD_DEVICE*         pDevice;
    WFD_PIPELINE*       pPipeline;
    WFDint              value = 0;
    WFDErrorCode        ec;

    GET_DEVICE(pDevice, device, 0);
    GET_PIPELINE(pDevice, pPipeline, pipeline, 0);

    CHECK_ACCESSOR(device, attrib, wfdGetPipelineAttribi, value);

    ec = WFD_Pipeline_GetAttribi(pPipeline, attrib, &value);
    FAIL(ec, value);
}


WFD_API_CALL WFDfloat WFD_APIENTRY
wfdGetPipelineAttribf(WFDDevice device,
                      WFDPipeline pipeline,
                      WFDPipelineConfigAttrib attrib) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PIPELINE*           pPipeline;
    WFDfloat                value = 0;
    WFDErrorCode            ec = WFD_ERROR_NONE;

    GET_DEVICE(pDevice, device, 0.0);
    GET_PIPELINE(pDevice, pPipeline, pipeline, 0.0);

    CHECK_ACCESSOR(device, attrib, wfdGetPipelineAttribf, value);

    ec = WFD_Pipeline_GetAttribf(pPipeline, attrib, &value);
    FAIL(ec, value);
}


WFD_API_CALL void WFD_APIENTRY
wfdGetPipelineAttribiv(WFDDevice device,
                       WFDPipeline pipeline,
                       WFDPipelineConfigAttrib attrib,
                       WFDint count,
                       WFDint *value) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PIPELINE*           pPipeline;
    WFDErrorCode            ec;

    GET_DEVICE_NR(pDevice, device);
    GET_PIPELINE_NR(pDevice, pPipeline, pipeline);

    COND_FAIL_NR(count > 0, WFD_ERROR_ILLEGAL_ARGUMENT);
    COND_FAIL_NR(value != NULL, WFD_ERROR_ILLEGAL_ARGUMENT);

    CHECK_ACCESSOR_NR(device, attrib, wfdGetPipelineAttribiv);

    ec = WFD_Pipeline_GetAttribiv(pPipeline, attrib, count, value);
    FAIL_NR(ec);
}


WFD_API_CALL void WFD_APIENTRY
wfdGetPipelineAttribfv(WFDDevice device,
                       WFDPipeline pipeline,
                       WFDPipelineConfigAttrib attrib,
                       WFDint count,
                       WFDfloat *value) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PIPELINE*           pPipeline;
    WFDErrorCode            ec;

    GET_DEVICE_NR(pDevice, device);
    GET_PIPELINE_NR(pDevice, pPipeline, pipeline);

    COND_FAIL_NR(count > 0, WFD_ERROR_ILLEGAL_ARGUMENT);
    COND_FAIL_NR(value != NULL, WFD_ERROR_ILLEGAL_ARGUMENT);

    CHECK_ACCESSOR_NR(device, attrib, wfdGetPipelineAttribfv);

    ec = WFD_Pipeline_GetAttribfv(pPipeline, attrib, count, value);
    FAIL_NR(ec);
}


WFD_API_CALL void WFD_APIENTRY
wfdSetPipelineAttribi(WFDDevice device,
                      WFDPipeline pipeline,
                      WFDPipelineConfigAttrib attrib,
                      WFDint value) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PIPELINE*           pPipeline;
    WFDErrorCode            ec;

    GET_DEVICE_NR(pDevice, device);
    GET_PIPELINE_NR(pDevice, pPipeline, pipeline);

    CHECK_ACCESSOR_NR(device, attrib, wfdSetPipelineAttribi);

    ec = WFD_Pipeline_SetAttribi(pPipeline, attrib, value);
    FAIL_NR(ec);
}


WFD_API_CALL void WFD_APIENTRY
wfdSetPipelineAttribf(WFDDevice device,
                      WFDPipeline pipeline,
                      WFDPipelineConfigAttrib attrib,
                      WFDfloat value) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PIPELINE*           pPipeline;
    WFDErrorCode            ec;

    GET_DEVICE_NR(pDevice, device);
    GET_PIPELINE_NR(pDevice, pPipeline, pipeline);

    CHECK_ACCESSOR_NR(device, attrib, wfdSetPipelineAttribf);

    ec = WFD_Pipeline_SetAttribf(pPipeline, attrib, value);
    FAIL_NR(ec);
}


WFD_API_CALL void WFD_APIENTRY
wfdSetPipelineAttribiv(WFDDevice device,
                       WFDPipeline pipeline,
                       WFDPipelineConfigAttrib attrib,
                       WFDint count,
                       const WFDint *value) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PIPELINE*           pPipeline;
    WFDErrorCode            ec;

    GET_DEVICE_NR(pDevice, device);
    GET_PIPELINE_NR(pDevice, pPipeline, pipeline);

    COND_FAIL_NR(count > 0, WFD_ERROR_ILLEGAL_ARGUMENT);
    COND_FAIL_NR(value != NULL, WFD_ERROR_ILLEGAL_ARGUMENT);

    CHECK_ACCESSOR_NR(device, attrib, wfdSetPipelineAttribiv);

    ec = WFD_Pipeline_SetAttribiv(pPipeline, attrib, count, value);
    FAIL_NR(ec);
}


WFD_API_CALL void WFD_APIENTRY
wfdSetPipelineAttribfv(WFDDevice device,
                       WFDPipeline pipeline,
                       WFDPipelineConfigAttrib attrib,
                       WFDint count,
                       const WFDfloat *value) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PIPELINE*           pPipeline;
    WFDErrorCode            ec;

    GET_DEVICE_NR(pDevice, device);
    GET_PIPELINE_NR(pDevice, pPipeline, pipeline);

    COND_FAIL_NR(count > 0, WFD_ERROR_ILLEGAL_ARGUMENT);
    COND_FAIL_NR(value != NULL, WFD_ERROR_ILLEGAL_ARGUMENT);

    CHECK_ACCESSOR_NR(device, attrib, wfdSetPipelineAttribfv);

    ec = WFD_Pipeline_SetAttribfv(pPipeline, attrib, count, value);
    FAIL_NR(ec);
}


WFD_API_CALL WFDint WFD_APIENTRY
wfdGetPipelineTransparency(WFDDevice device,
                           WFDPipeline pipeline,
                           WFDbitfield *trans,
                           WFDint transCount) WFD_APIEXIT
{
    WFD_DEVICE*         pDevice;
    WFD_PIPELINE*       pPipeline;
    WFDint              retVal = 0;

    GET_DEVICE(pDevice, device, 0);
    GET_PIPELINE(pDevice, pPipeline, pipeline, 0);

    COND_FAIL(((transCount > 0 && trans != NULL) || trans == NULL),
               WFD_ERROR_ILLEGAL_ARGUMENT,
               0);
    retVal =
        WFD_Pipeline_GetTransparencyFeatures(pPipeline, trans, transCount);

    SUCCEED(retVal);
}


WFD_API_CALL void WFD_APIENTRY
wfdSetPipelineTSColor(WFDDevice device,
                      WFDPipeline pipeline,
                      WFDTSColorFormat colorFormat,
                      WFDint count,
                      const void *color) WFD_APIEXIT
{
    WFD_DEVICE*         pDevice;
    WFD_PIPELINE*       pPipeline;

    GET_DEVICE_NR(pDevice, device);
    GET_PIPELINE_NR(pDevice, pPipeline, pipeline);

    COND_FAIL_NR(color != NULL, WFD_ERROR_ILLEGAL_ARGUMENT);

    COND_FAIL_NR(WFD_Util_IsValidTSColor(colorFormat, count, color),
                 WFD_ERROR_ILLEGAL_ARGUMENT);

    COND_FAIL_NR(WFD_Pipeline_IsTransparencySupported(pPipeline,
                                 WFD_TRANSPARENCY_SOURCE_COLOR),
                 WFD_ERROR_NOT_SUPPORTED);

    WFD_Pipeline_SetTSColor(pPipeline, colorFormat, count, color);

    SUCCEED();
}



WFD_API_CALL WFDint WFD_APIENTRY
wfdGetPipelineLayerOrder(WFDDevice device,
                         WFDPort port,
                         WFDPipeline pipeline) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    WFD_PORT*               pPort;
    WFD_PIPELINE*           pPipeline;
    WFDint                  layer;

    GET_DEVICE(pDevice, device, 0);
    GET_PORT(pDevice, pPort, port, 0);
    GET_PIPELINE(pDevice, pPipeline, pipeline, 0);

    COND_FAIL(WFD_Port_PipelineBindable(pPort, pPipeline->config->id),
              WFD_ERROR_ILLEGAL_ARGUMENT,
              WFD_INVALID_PIPELINE_LAYER);

    layer = WFD_Port_QueryPipelineLayerOrder(pPort, pPipeline);

    SUCCEED(layer);
}

/* ============================================================================= */
/*                       6.  E x t e n s i o n s                                 */
/* ============================================================================= */

WFD_API_CALL WFDint WFD_APIENTRY
wfdGetStrings(WFDDevice device,
              WFDStringID name,
              const char **strings,
              WFDint stringsCount) WFD_APIEXIT
{
    WFD_DEVICE*             pDevice;
    const char**            tmp;
    WFDint                  retVal;

    GET_DEVICE(pDevice, device, 0);
    COND_FAIL(stringsCount >= 0, WFD_ERROR_ILLEGAL_ARGUMENT, 0);

    switch (name)
    {
    case WFD_VENDOR:
        retVal = 1;
        tmp = &wfd_strings[WFD_VENDOR_INDEX];
        break;

    case WFD_RENDERER:
        retVal = 1;
        tmp = &wfd_strings[WFD_RENDERER_INDEX];
        break;

    case WFD_VERSION:
        retVal = 1;
        tmp = &wfd_strings[WFD_VERSION_INDEX];
        break;

    case WFD_EXTENSIONS:
        tmp = &wfd_extensions[0];
        for (retVal=0; tmp[retVal] != NULL; retVal++)
        {
            /* get extensions array size */
        }
        break;

    default:
        FAIL(WFD_ERROR_ILLEGAL_ARGUMENT, 0);
    }

    if (strings != NULL)
    {
        WFDint i;

        if (stringsCount < retVal)
        {
            retVal = stringsCount;
        }
        for (i=0; i<retVal; i++)
        {
            strings[i] = tmp[i];
        }
    }

    SUCCEED(retVal);
}

WFD_API_CALL WFDboolean WFD_APIENTRY
wfdIsExtensionSupported(WFDDevice device,
                        const char *string) WFD_APIEXIT
{
    WFD_DEVICE* pDevice;
    WFDint i;
    WFDboolean retVal = WFD_FALSE;

    GET_DEVICE(pDevice, device, WFD_FALSE);

    for (i=0; wfd_extensions[i] != NULL; i++)
    {
        if (strcmp(string,wfd_extensions[i])==0)
        {
            retVal = WFD_TRUE;
            break;
        }
    }

    SUCCEED(retVal);
}


/* ============================================================================= */
/*                           T E S T   O N L Y                                   */
/* ============================================================================= */


WFD_API_CALL WFDEGLImage WFD_APIENTRY
wfdGetPortImage(WFDDevice device, WFDPort port) WFD_APIEXIT
{
    WFD_DEVICE*     pDevice;
    WFD_PORT*       pPort;
    WFDEGLImage     img;

    GET_DEVICE(pDevice, device, WFD_INVALID_HANDLE);
    GET_PORT(pDevice, pPort, port, WFD_INVALID_HANDLE);

    img = WFD_Port_AcquireCurrentImage(pPort);

    SUCCEED(img);
}


#ifdef __cplusplus
}
#endif
