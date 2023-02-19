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
/*! \ingroup wfc
 *  \file wfcdevice.c
 *
 *  \brief SI Device handling
 */

#include "wfcdevice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "owfarray.h"
#include "owfdebug.h"
#include "owfmemory.h"
#include "owfmutex.h"
#include "owfobject.h"
#include "owfscreen.h"
#include "owfstream.h"
#include "owftypes.h"
#include "wfccontext.h"
#include "wfcelement.h"
#include "wfcimageprovider.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_NUM_DEVICE_IDS 3
#define MAX_ATTRIBUTES 32

#define FAIL_IF(c, e) \
    if (c) {          \
        LEAVE(0);     \
        return e;     \
    }

#define FIRST_DEVICE_HANDLE 1000
#define FIRST_DEVICEINSTANCE_HANDLE 3000

/*! Array of available devices */
DEVICE_INSTANCE_LIST gPhyDevice;

static void WFC_Device_RemoveUnusedStreams(WFC_DEVICE* device);

/*-------------------------------------------------------------------------*//*!
 *  \internal
 *
 *  Initialize devices list with default parameters.
 *//*-------------------------------------------------------------------------*/
OWF_API_CALL void WFC_Devices_Initialize() {
    static WFCboolean initialised = WFC_FALSE;
    if (!initialised) {
        DPRINT(("WFC_Devices_Initialize (Enter)"));
        /* Initialize video 'hardware' */
        OWF_Screen_Initialize();

        /* initialise the array of device instances */
        OWF_Array_Initialize(&(gPhyDevice.iDeviceInstanceArray));
        gPhyDevice.gDeviceHandleID = FIRST_DEVICEINSTANCE_HANDLE;
        initialised = WFC_TRUE;
    }
}

/*-------------------------------------------------------------------------*//*!
 *  \internal
 *
 *  Initialize new device with default parameters.
 *//*-------------------------------------------------------------------------*/
static void WFC_Device_Initialize(WFC_DEVICE* device, OWFint deviceid) {
    OWF_SCREEN screen;
    DPRINT(("WFC_Device_Initialize (Enter)"));
    memset(device, 0, sizeof(WFC_DEVICE));

    device->handle = WFC_INVALID_HANDLE;
    device->deviceId = deviceid;
    device->latestUnreadError = WFC_ERROR_NONE;
    /* There are many possible device:context mappings possible with OpenWF-C
     * API */
    /* This SI implementation assigns 1 screen-number per device ID */
    device->screenNumber = deviceid - FIRST_DEVICE_HANDLE;
    OWF_Array_Initialize(&device->contexts);
    OWF_Array_Initialize(&device->providers);
    OWF_Array_Initialize(&device->elements);
    if (!OWF_Screen_GetHeader(
            device->screenNumber,
            &screen)) { /* If the given screen number can't be opened then the
                           device must be off-screen only */
        device->screenNumber = OWF_RESERVED_BAD_SCREEN_NUMBER;
    }

    device->handle = ++gPhyDevice.gDeviceHandleID;
}

/*---------------------------------------------------------------------------
 *
 *
 *----------------------------------------------------------------------------*/

OWF_API_CALL WFCint WFC_Devices_GetIds(WFCint* idList, WFCint listCapacity,
                                       const WFCint* filterList) {
    /* #4584: initialized here */
    WFCint screenNumber = OWF_RESERVED_BAD_SCREEN_NUMBER /* -1 = all screens */,
           n = MAX_ATTRIBUTES, i = 0;
    OWF_SCREEN screen;
    WFCboolean hasFilter;

    WFC_Devices_Initialize();

    hasFilter =
        (filterList && (*filterList != WFC_NONE)) ? WFC_TRUE : WFC_FALSE;

    /* handle filter list, if given */
    if (hasFilter) {
        /* scan filter list (up to 32 attributes) */
        while (n > 0 && WFC_NONE != filterList[0]) {
            switch (filterList[0]) {
                case WFC_DEVICE_FILTER_SCREEN_NUMBER:
                    if (screenNumber != OWF_RESERVED_BAD_SCREEN_NUMBER) {
                        /* invalid repeated filter on screen number */
                        return 0;
                    }
                    screenNumber = filterList[1];
                    if (!OWF_Screen_GetHeader(screenNumber, &screen)) {
                        /* invalid screen number */
                        return 0;
                    }
                    break;

                case WFC_NONE:
                    n = 0;
                    break;

                default:
                    /* EARLY EXIT - invalid attribute */
                    return 0;
            }

            /* Move pointer to next key-value pair */
            filterList += 2;
            n--;
        }
    }

    /* There are many possible device:context mappings allowed with OpenWF-C API
     */
    /* This SI implementation assigns 1 screen-number per device ID */
    for (i = 0, n = 0; i < MAX_NUM_DEVICE_IDS && listCapacity >= 0; i++) {
        /* if screen number filter is specified, use it */
        if (screenNumber != OWF_RESERVED_BAD_SCREEN_NUMBER) {
            if (i != screenNumber) {
                DPRINT(("Continue (i != screenNumber) (i=%i n=%i)", i, n));
                continue;
            }
        } else { /* screenzero device id is only legal if zero screen number is
                    supported */
            if (i == 0 && !OWF_Screen_GetHeader(i, &screen)) {
                DPRINT(("Continue !OWF_Screen_GetHeader(i, (i=%i n=%i)", i, n));
                continue;
            }
        }

        /* idList might be NULL when querying all devices w/
         * screen number in filter list */
        if (idList && n < listCapacity) {
            idList[n] = FIRST_DEVICE_HANDLE + i;
        }
        DPRINT(("N++(i=%i n=%i)", i, n));

        n++;
    }

    return n;
}

/*---------------------------------------------------------------------------
 *  Create instance of a device whose ID
 *  matches deviceId
 *
 *  \param deviceId ID of the device to create. Must be WFC_DEFAULT_DEVICE_ID
 *  or some other valid device ID (returned by Devices_GetIds)
 *
 *  \return Handle to created device or WFC_INVALID_HANDLE
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCint WFC_Device_Create(WFCint deviceId) {
    OWF_SCREEN screen;

    WFC_DEVICE* device = NULL;

    WFCint checkScreenNum = deviceId - FIRST_DEVICE_HANDLE;

    WFC_Devices_Initialize();

    if (deviceId ==
        WFC_DEFAULT_DEVICE_ID) { /* So the default device is the one that
                                    supports the default screen number */
        checkScreenNum = OWF_Screen_GetDefaultNumber();
        DPRINT(("Default device= screen %i", checkScreenNum));
    }
    ENTER(WFC_Device_Create);
    /* There are many possible device:context mappings possible with OpenWF-C
     * API */
    /* This SI implementation assigns 1 screen-number per device ID */
    /* In-range high-numbered device IDs that can't show screenNumber are
     * considered to be off-screen-only */
    /* 0 is allowed to be a legal screen number, depending on OWF_SCREEN
     * adaptation */
    if (checkScreenNum < MAX_NUM_DEVICE_IDS && checkScreenNum >= 0 &&
        (checkScreenNum > 0 || OWF_Screen_GetHeader(checkScreenNum, &screen))) {
        device = DEVICE(malloc(sizeof(WFC_DEVICE)));
    } else {
        DPRINT(("Did not try to create device - id out of range D%i S%i",
                deviceId, checkScreenNum));
    }

    if (device) {
        if (!OWF_Array_AppendItem(&(gPhyDevice.iDeviceInstanceArray), device)) {
            free(device);
            device = NULL;
        } else {
            WFC_Device_Initialize(device, deviceId);
        }
    }
    LEAVE(WFC_Device_Create);
    if (device) {
        return device->handle;
    } else {
        return WFC_INVALID_HANDLE;
    }
}

/*---------------------------------------------------------------------------
 *  Set error code for device. Obs! In case the previous
 *  error code hasn't been read from the device, this function
 *  does nothing; the new error is set only if "current" error
 *  is none.
 *
 *  \param dev Device object
 *  \param code Error to set
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL void OWF_APIENTRY WFC_Device_SetError(WFCDevice dev,
                                                   WFCErrorCode code) {
    static char* const errmsg[] = {
        "WFC_ERROR_NONE",
        "WFC_ERROR_OUT_OF_MEMORY",
        "WFC_ERROR_ILLEGAL_ARGUMENT",
        "WFC_ERROR_UNSUPPORTED",
        "WFC_ERROR_BAD_ATTRIBUTE",
        "WFC_ERROR_IN_USE",
        "WFC_ERROR_BUSY",
        "WFC_ERROR_BAD_DEVICE",
        "WFC_ERROR_BAD_HANDLE",
        "WFC_ERROR_INCONSISTENCY",
    };
    WFC_DEVICE* device = WFC_Device_FindByHandle(dev);
    if (WFC_INVALID_HANDLE == device) {
        /* Invalid device handle. Nothing we can do about it so return. */
        return;
    }

    if (!device->mutex) {
        OWF_Mutex_Init(&device->mutex);
    }

    OWF_Mutex_Lock(&device->mutex);

    if (WFC_INVALID_HANDLE != device) {
        if (WFC_ERROR_NONE == device->latestUnreadError &&
            code != device->latestUnreadError) {
            char* const msg = errmsg[code > WFC_ERROR_NONE
                                         ? code - WFC_ERROR_OUT_OF_MEMORY + 1
                                         : 0];

            DPRINT(("setError(dev = %08x, err = %08x)", dev, code));
            DPRINT(("  error set to = %04x (%s)", (OWFuint16)code, msg));

            device->latestUnreadError = code;
        }
    }

    OWF_Mutex_Unlock(&device->mutex);
}

/*---------------------------------------------------------------------------
 *  Read and reset last error code from device.
 *
 *  \param device Device object
 *
 *  \return WFCErrorCode
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode WFC_Device_GetError(WFC_DEVICE* device) {
    WFCErrorCode err;

    OWF_ASSERT(device);

    err = device->latestUnreadError;
    device->latestUnreadError = WFC_ERROR_NONE;
    return err;
}

/*---------------------------------------------------------------------------
 *  Find device object by handle
 *
 *  \param dev Device handle
 *
 *  \return Matching device object or NULL
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFC_DEVICE* WFC_Device_FindByHandle(WFCDevice dev) {
    WFCint i = 0, length = 0;
    WFC_DEVICE* pDevice = NULL;

    if (dev == WFC_INVALID_HANDLE) {
        return NULL;
    }

    WFC_Devices_Initialize();

    length = gPhyDevice.iDeviceInstanceArray.length;
    for (i = 0; i < length; i++) {
        pDevice =
            DEVICE(OWF_Array_GetItemAt(&(gPhyDevice.iDeviceInstanceArray), i));
        if (pDevice && pDevice->handle == dev) {
            return pDevice;
        }
    }

    return NULL;
}

/*---------------------------------------------------------------------------
 *  Get device attribute
 *
 *  \param device Device
 *  \param attrib Attribute name
 *  \param value Pointer to where the value should be saved
 *
 *  \return WFCErrorCode
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode WFC_Device_GetAttribi(WFC_DEVICE* device,
                                                WFCDeviceAttrib attrib,
                                                WFCint* value) {
    WFCErrorCode result = WFC_ERROR_NONE;

    OWF_ASSERT(device);

    switch (attrib) {
        case WFC_DEVICE_CLASS: {
            if (device->screenNumber == OWF_RESERVED_BAD_SCREEN_NUMBER) {
                *value = WFC_DEVICE_CLASS_OFF_SCREEN_ONLY;
            } else {
                *value = WFC_DEVICE_CLASS_FULLY_CAPABLE;
            }
            break;
        }
        case WFC_DEVICE_ID: {
            *value = device->deviceId;
            break;
        }
        default: {
            result = WFC_ERROR_BAD_ATTRIBUTE;
        }
    }
    return result;
}

/*[][][][] CONTEXTS  [][][][][][][][][][][][][][][][][][][][][][][][][][][][]*/

/*---------------------------------------------------------------------------
 *  Create context on device
 *
 *  \param device Device
 *  \param stream Target stream for context
 *  \param type Context type
 *
 *  \return New context
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFC_CONTEXT* WFC_Device_CreateContext(WFC_DEVICE* device,
                                                   WFCNativeStreamType stream,
                                                   WFCContextType type,
                                                   WFCint screenNum) {
    WFC_CONTEXT* context;

    ENTER(WFC_Device_CreateContext);

    OWF_ASSERT(device);

    context = WFC_Context_Create(device, stream, type, screenNum);
    if (context) {
        if (!OWF_Array_AppendItem(&device->contexts, context)) {
            DESTROY(context);
        }
    }
    LEAVE(WFC_Device_CreateContext);

    return context;
}

/*---------------------------------------------------------------------------
 *  Destroy context from device
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode WFC_Device_DestroyContext(WFC_DEVICE* device,
                                                    WFCContext context) {
    WFCint i;
    WFCErrorCode result = WFC_ERROR_BAD_HANDLE;
    ENTER(WFC_Device_DestroyContext);

    OWF_ASSERT(device);

    DPRINT(("WFC_Device_DestroyContext(context = %d)", context));

    for (i = 0; i < device->contexts.length; i++) {
        WFC_CONTEXT* ctmp;

        ctmp = (WFC_CONTEXT*)OWF_Array_GetItemAt(&device->contexts, i);
        if (ctmp->handle == context) {
            WFC_CONTEXT* context;

            context = CONTEXT(OWF_Array_RemoveItemAt(&device->contexts, i));
            DPRINT(("  Shutting down context %d", context->handle));
            WFC_Context_Shutdown(context);
            DESTROY(context);
            result = WFC_ERROR_NONE;
            break;
        }
    }

    DPRINT(("Removing ununsed streams"));
    WFC_Device_RemoveUnusedStreams(device);
    DPRINT((""));

    DPRINT(("-------------------------------------------------------"));
    DPRINT(("Device statistics after context destruction:"));
    DPRINT(("  Contexts: %d", device->contexts.length));
    DPRINT(("  Elements: %d", device->elements.length));
    DPRINT(("  Image providers: %d", device->providers.length));
    DPRINT(("  Streams: %d", device->streams.length));
    DPRINT(("-------------------------------------------------------"));
    LEAVE(WFC_Device_DestroyContext);

    return result;
}

/*---------------------------------------------------------------------------
 *  Destroy all device's contexts
 *
 *  \param device Device object
 *----------------------------------------------------------------------------*/
OWF_API_CALL void WFC_Device_DestroyContexts(WFC_DEVICE* device) {
    WFCint i;

    ENTER(WFC_Device_DestroyContexts);

    for (i = 0; i < device->contexts.length; i++) {
        WFC_CONTEXT* context;

        context = CONTEXT(OWF_Array_GetItemAt(&device->contexts, i));
        WFC_Context_Shutdown(context);

        DESTROY(context);
    }
    OWF_Array_Destroy(&device->contexts);

    LEAVE(WFC_Device_DestroyContexts);
}

/*---------------------------------------------------------------------------
 *  Find context context object by handle
 *
 *  \param device Device
 *  \param context Context handle
 *
 *  \return Corresponding context object or NULL
 *  if handle is invalid.
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFC_CONTEXT* WFC_Device_FindContext(WFC_DEVICE* device,
                                                 WFCContext context) {
    WFCint i;
    WFC_CONTEXT* result = NULL;

    ENTER(WFC_Device_FindContext);

    FAIL_IF(NULL == device, NULL);

    for (i = 0; i < device->contexts.length; i++) {
        WFC_CONTEXT* ctmp;

        ctmp = CONTEXT(OWF_Array_GetItemAt(&device->contexts, i));
        if (ctmp->handle == context) {
            result = ctmp;
            break;
        }
    }
    LEAVE(WFC_Device_FindContext);

    return result;
}
/*[][][][] ELEMENTS  [][][][][][][][][][][][][][][][][][][][][][][][][][][][]*/

/*---------------------------------------------------------------------------
 *  Create new element
 *
 *  \param device
 *  \param context
 *
 *  \return New element or NULL
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFC_ELEMENT* WFC_Device_CreateElement(WFC_DEVICE* device,
                                                   WFC_CONTEXT* context) {
    WFC_ELEMENT* element;

    ENTER(WFC_Device_CreateElement);

    FAIL_IF(NULL == device || NULL == context, NULL);

    element = WFC_Element_Create(context);
    if (!OWF_Array_AppendItem(&device->elements, element)) {
        DPRINT(("WFC_Device_CreateElement: couldn't create element"));
        WFC_Element_Destroy(element);
        element = NULL;
    } else {
        /* #4585: statement moved to else block */
        DPRINT(("  Created element; handle = %d", element->handle));
    }
    LEAVE(WFC_Device_CreateElement);

    return element;
}

/*---------------------------------------------------------------------------
 *  Destroy element
 *
 *  \param device
 *  \param element
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode WFC_Device_DestroyElement(WFC_DEVICE* device,
                                                    WFCElement element) {
    WFCint i;
    WFCErrorCode result = WFC_ERROR_BAD_HANDLE;

    ENTER(WFC_Device_DestroyElement);

    FAIL_IF(NULL == device, WFC_ERROR_BAD_HANDLE);
    DPRINT(("destroying element %d", element));

    for (i = 0; i < device->elements.length; i++) {
        WFC_ELEMENT* object;

        object = ELEMENT(OWF_Array_GetItemAt(&device->elements, i));
        DPRINT(("  element %d = %d", i, object->handle));
        if (object->handle == element) {
            WFC_Context_RemoveElement(CONTEXT(object->context), element);

            OWF_Array_RemoveItemAt(&device->elements, i);
            WFC_Element_Destroy(object);
            result = WFC_ERROR_NONE;
            break;
        }
    }
    LEAVE(WFC_Device_DestroyElement);
    return result;
}

/*---------------------------------------------------------------------------
 *  Destroy all elements from device
 *
 *  \param device Device
 *----------------------------------------------------------------------------*/
OWF_API_CALL void WFC_Device_DestroyElements(WFC_DEVICE* device) {
    WFCint i;

    ENTER(WFC_Device_DestroyElements);

    OWF_ASSERT(device);

    for (i = 0; i < device->elements.length; i++) {
        WFC_ELEMENT* etemp;

        etemp = ELEMENT(OWF_Array_GetItemAt(&device->elements, i));
        WFC_Element_Destroy(etemp);
    }
    OWF_Array_Destroy(&device->elements);

    LEAVE(WFC_Device_DestroyElements);
}

/*---------------------------------------------------------------------------
 *  Find element by handle
 *
 *  \param device Device
 *  \param el Element handle
 *
 *  \return Element object
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFC_ELEMENT* WFC_Device_FindElement(WFC_DEVICE* device,
                                                 WFCElement el) {
    WFC_ELEMENT* result = WFC_INVALID_HANDLE;
    WFCint i;

    FAIL_IF(NULL == device, NULL);

    for (i = 0; i < device->elements.length; i++) {
        WFC_ELEMENT* element;
        element = ELEMENT(OWF_Array_GetItemAt(&device->elements, i));

        if (element->handle == el) {
            result = element;
            break;
        }
    }
    return result;
}

/*---------------------------------------------------------------------------
 *  Set element integer vector attribute
 *
 *  \param device Device
 *  \param element Element handle
 *  \param attrib Attribute name
 *  \param count Attribute value vector length (1 for scalar attribute)
 *  \param values Pointer to values
 *
 *  \return WFCErrorCode: WFC_ERROR_BAD_ATTRIBUTE if attribute name is invalid;
 *  WFC_ERROR_INVALID_ARGUMENT if values is NULL or if the count doesn't match
 *  the attribute's size; WFC_ERROR_BAD_HANDLE if element handle is invalid.
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode WFC_Device_SetElementAttribiv(WFC_DEVICE* device,
                                                        WFCElement element,
                                                        WFCElementAttrib attrib,
                                                        WFCint count,
                                                        const WFCint* values) {
    WFC_ELEMENT* object;

    OWF_ASSERT(device);

    object = WFC_Device_FindElement(device, element);
    FAIL_IF(NULL == object, WFC_ERROR_BAD_HANDLE);
    return WFC_Element_SetAttribiv(object, attrib, count, values);
}

/*---------------------------------------------------------------------------
 *
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode WFC_Device_SetElementAttribfv(
    WFC_DEVICE* device, WFCElement element, WFCElementAttrib attrib,
    WFCint count, const WFCfloat* values) {
    WFC_ELEMENT* object;

    OWF_ASSERT(device);

    object = WFC_Device_FindElement(device, element);
    FAIL_IF(NULL == object, WFC_ERROR_BAD_HANDLE);
    return WFC_Element_SetAttribfv(object, attrib, count, values);
}

/*---------------------------------------------------------------------------
 *
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode WFC_Device_GetElementAttribiv(WFC_DEVICE* device,
                                                        WFCElement element,
                                                        WFCElementAttrib attrib,
                                                        WFCint count,
                                                        WFCint* values) {
    WFC_ELEMENT* object;

    OWF_ASSERT(device);

    object = WFC_Device_FindElement(device, element);
    FAIL_IF(NULL == object, WFC_ERROR_BAD_HANDLE);
    return WFC_Element_GetAttribiv(object, attrib, count, values);
}

/*---------------------------------------------------------------------------
 *
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode WFC_Device_GetElementAttribfv(WFC_DEVICE* device,
                                                        WFCElement element,
                                                        WFCElementAttrib attrib,
                                                        WFCint count,
                                                        WFCfloat* values) {
    WFC_ELEMENT* object;

    OWF_ASSERT(device);

    object = WFC_Device_FindElement(device, element);
    FAIL_IF(NULL == object, WFC_ERROR_BAD_HANDLE);
    return WFC_Element_GetAttribfv(object, attrib, count, values);
}

/*[][][][] STREAMS [][][][][][][][][][][][][][][][][][][][][][][][][][][][][]*/

/*---------------------------------------------------------------------------
 *
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL OWF_STREAM* WFC_Device_CreateStream(WFC_DEVICE* device,
                                                 WFC_CONTEXT* context,
                                                 WFCNativeStreamType stream,
                                                 WFCboolean write) {
    WFCint i;
    OWF_STREAM* newStream = NULL;

    /* unused parameters, get rid of compiler warning */
    context = context;

    ENTER(WFC_Device_CreateStream);

    OWF_ASSERT(device);
    OWF_ASSERT(context);

    /* first try to look up a stream object that is associated
     * to given native stream
     */
    for (i = 0; i < device->streams.length; i++) {
        OWF_STREAM* stemp;

        stemp = STREAM(OWF_Array_GetItemAt(&device->streams, i));
        if (stemp->handle == stream) {
            /* found it */
            newStream = stemp;
            break;
        }
    }

    if (!newStream) {
        /* create new */
        newStream = OWF_Stream_Create(stream, write ? OWF_TRUE : OWF_FALSE);
        if (newStream) {
            if (!OWF_Array_AppendItem(&device->streams, newStream)) {
                OWF_Stream_Destroy(newStream);
                newStream = NULL;
            }
        }
    }

    LEAVE(WFC_Device_CreateStream);

    return newStream;
}

/*---------------------------------------------------------------------------
 *
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode WFC_Device_DestroyStream(WFC_DEVICE* device,
                                                   OWF_STREAM* stream) {
    WFCint i;
    WFCErrorCode result = WFC_ERROR_BAD_HANDLE;

    ENTER(WFC_Device_DestroyStream);

    OWF_ASSERT(device);

    FAIL_IF(NULL == device || stream == NULL, WFC_ERROR_BAD_HANDLE);

    for (i = 0; i < device->streams.length; i++) {
        OWF_STREAM* object;

        object = STREAM(OWF_Array_GetItemAt(&device->streams, i));
        if (object == stream) {
            if (OWF_Stream_Destroy(object)) {
                OWF_Array_RemoveItemAt(&device->streams, i);
            }
            result = WFC_ERROR_NONE;
            break;
        }
    }

    LEAVE(WFC_Device_DestroyStream);

    return result;
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL void WFC_Device_DestroyStreams(WFC_DEVICE* device) {
    WFCint i;

    ENTER(WFC_Device_DestroyStreams);

    OWF_ASSERT(device);

    for (i = 0; i < device->streams.length; i++) {
        OWF_STREAM* stream;

        stream = STREAM(OWF_Array_GetItemAt(&device->streams, i));
        DPRINT(("  Destroying stream %p", stream));

        OWF_Stream_Destroy(stream);
    }
    OWF_Array_Destroy(&device->streams);

    LEAVE(WFC_Device_DestroyStreams);
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static void WFC_Device_RemoveStreamIfUnused(WFC_DEVICE* device,
                                            OWF_STREAM* stream) {
    /* ref.count value 1 means that the only reference there is to
     * the stream is the reference from the device to stream. */
    if (1 == stream->useCount) {
        WFCint count = device->streams.length;

        /* stream get gone and be gone */
        WFC_Device_DestroyStream(device, stream);

        /* rrrr! ensure it be gone. */
        OWF_ASSERT(device->streams.length == count - 1);
    }
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static void WFC_Device_RemoveUnusedStreams(WFC_DEVICE* device) {
    WFCint i;

    ENTER(WFC_Device_RemoveUnusedStreams);

    OWF_ASSERT(device);

    i = device->streams.length - 1;
    while (i >= 0) {
        OWF_STREAM* stream;

        stream = STREAM(OWF_Array_GetItemAt(&device->streams, i));

        /* ref.count value 1 means that the only reference there is to
         * the stream is the reference from the device to stream. */
        WFC_Device_RemoveStreamIfUnused(device, stream);
        i--;
    }

    LEAVE(WFC_Device_RemoveUnusedStreams);
}

/*[][][][] IMAGE PROVIDERS [][][][][][][][][][][][][][][][][][][][][][][][][]*/

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL void WFC_Device_EnableContentNotifications(WFC_DEVICE* device,
                                                        WFC_CONTEXT* context,
                                                        WFCboolean enable) {
    WFCint i;

    OWF_ASSERT(device);
    OWF_ASSERT(context);

    context = context;

    for (i = 0; i < device->providers.length; i++) {
        WFC_IMAGE_PROVIDER* prov;
        OWF_STREAM* stream;

        prov = IMAGE_PROVIDER(OWF_Array_GetItemAt(&device->providers, i));
        stream = STREAM(prov->stream);

        owfNativeStreamEnableUpdateNotifications(stream->handle,
                                                 enable ? OWF_TRUE : OWF_FALSE);
    }
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static WFC_IMAGE_PROVIDER* WFC_Device_CreateImageProvider(
    WFC_DEVICE* device, WFC_CONTEXT* context, OWFNativeStreamType stream,
    WFC_IMAGE_PROVIDER_TYPE type) {
    WFC_IMAGE_PROVIDER* provider;
    OWF_STREAM* strm;
    WFCint success;

    OWF_ASSERT(device);
    OWF_ASSERT(context);

    /* create/fetch stream-wrapper for native stream.  */
    strm = WFC_Device_CreateStream(device, context, stream, WFC_FALSE);
    if (!strm) {
        return NULL;
    }

    /* create new image provider object associated to
     * native stream wrapper previously fetched */
    provider = WFC_ImageProvider_Create(context, strm, type);
    if (!provider) {
        WFC_Device_DestroyStream(device, strm);
        return NULL;
    }

    /* take advantage of short-circuiting in && evaluation; only add the
       observer if the appendition is successful */
    success =
        (OWF_Array_AppendItem(&device->providers, provider) == OWF_TRUE) &&
        (0 == owfNativeStreamAddObserver(
                  stream, WFC_Context_SourceStreamUpdated, context));
    if (!success) {
        WFC_Device_DestroyStream(device, strm);
        DESTROY(provider);
        provider = NULL;
    }

    return provider;
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static WFCErrorCode WFC_Device_DestroyImageProvider(WFC_DEVICE* device,
                                                    WFCHandle handle) {
    WFCint i;
    WFCErrorCode result = WFC_ERROR_BAD_HANDLE;
    void* owner = NULL;
    OWFNativeStreamType stream = OWF_INVALID_HANDLE;

    ENTER(WFC_Device_DestroyImageProvider);

    OWF_ASSERT(device);

    DPRINT(("  number of providers = %d", device->providers.length));

    for (i = 0; i < device->providers.length; i++) {
        WFC_IMAGE_PROVIDER* object;

        object = (OWF_Array_GetItemAt(&device->providers, i));
        if (object->handle == handle) {
            DPRINT(("  Destroying image provider %d", handle));
            owner = object->owner;
            stream = object->stream->handle;

            OWF_Array_RemoveItemAt(&device->providers, i);
            DESTROY(object);

            result = WFC_ERROR_NONE;
            break;
        }
    }

    /*
     *  Image provider's source content observer must be removed here.
     *  If the context is destroyed and then the stream is updated,
     *  then the observer data pointer will be accessed badly.
     *  Relying on owner being context.
     */
    if (stream) {
        owfNativeStreamRemoveObserver(stream, WFC_Context_SourceStreamUpdated,
                                      owner);
    }

    LEAVE(WFC_Device_DestroyImageProvider);
    return result;
}

/*---------------------------------------------------------------------------
 *  Destroy all image providers from device
 *
 *  \param device Device
 *----------------------------------------------------------------------------*/
OWF_API_CALL void WFC_Device_DestroyImageProviders(WFC_DEVICE* device) {
    WFCint i;

    ENTER(WFC_Device_DestroyImageProviders);

    OWF_ASSERT(device);

    DPRINT(("number of providers = %d", device->providers.length));

    for (i = 0; i < device->providers.length; i++) {
        WFC_IMAGE_PROVIDER* itemp;
        void* owner = NULL;
        OWFNativeStreamType stream = OWF_INVALID_HANDLE;
        owner = itemp->owner;
        stream = itemp->stream->handle;

        itemp = IMAGE_PROVIDER(OWF_Array_GetItemAt(&device->providers, i));

        /*
         *  Image provider's source content observer must be removed here.
         *  If the context is destroyed and then the stream is updated,
         *  then the observer data pointer will be accessed badly.
         *  Relying on owner being context.
         */
        if (stream) {
            owfNativeStreamRemoveObserver(
                stream, WFC_Context_SourceStreamUpdated, owner);
        }

        DESTROY(itemp);
    }
    OWF_Array_Destroy(&device->providers);

    LEAVE(WFC_Device_DestroyImageProviders);
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFC_IMAGE_PROVIDER* WFC_Device_FindImageProvider(
    WFC_DEVICE* device, WFCHandle handle, WFC_IMAGE_PROVIDER_TYPE type) {
    WFCint i;
    WFC_IMAGE_PROVIDER* result = WFC_INVALID_HANDLE;

    ENTER(WFC_Device_FindImageProvider);

    OWF_ASSERT(device);
    DPRINT(("number of providers = %d", device->providers.length));

    for (i = 0; i < device->providers.length; i++) {
        WFC_IMAGE_PROVIDER* object;

        object = IMAGE_PROVIDER(OWF_Array_GetItemAt(&device->providers, i));
        if (object->handle == handle && object->type == type) {
            result = object;
            break;
        }
    }

    LEAVE(WFC_Device_FindImageProvider);

    return result;
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFC_IMAGE_PROVIDER* WFC_Device_CreateSource(
    WFC_DEVICE* device, WFC_CONTEXT* context, WFCNativeStreamType stream) {
    OWF_ASSERT(device);
    OWF_ASSERT(context);
    return WFC_Device_CreateImageProvider(device, context, stream,
                                          WFC_IMAGE_SOURCE);
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFC_IMAGE_PROVIDER* WFC_Device_CreateMask(
    WFC_DEVICE* device, WFC_CONTEXT* context, WFCNativeStreamType stream) {
    OWF_ASSERT(device);
    OWF_ASSERT(context);
    return WFC_Device_CreateImageProvider(device, context, stream,
                                          WFC_IMAGE_MASK);
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode WFC_Device_DestroySource(WFC_DEVICE* device,
                                                   WFCSource source) {
    OWF_ASSERT(device);
    return WFC_Device_DestroyImageProvider(device, source);
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode WFC_Device_DestroyMask(WFC_DEVICE* device,
                                                 WFCMask mask) {
    OWF_ASSERT(device);
    return WFC_Device_DestroyImageProvider(device, mask);
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFC_IMAGE_PROVIDER* WFC_Device_FindSource(WFC_DEVICE* device,
                                                       WFCSource source) {
    OWF_ASSERT(device);
    return WFC_Device_FindImageProvider(device, source, WFC_IMAGE_SOURCE);
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFC_IMAGE_PROVIDER* WFC_Device_FindMask(WFC_DEVICE* device,
                                                     WFCMask mask) {
    OWF_ASSERT(device);
    return WFC_Device_FindImageProvider(device, mask, WFC_IMAGE_MASK);
}

/*---------------------------------------------------------------------------
 *  Destroy device, or rather queue it for destruction.
 *
 *  \param device Device
 *----------------------------------------------------------------------------*/
OWF_API_CALL void WFC_Device_Destroy(WFC_DEVICE* device) {
    ENTER(WFC_Device_Destroy);

    OWF_ASSERT(device);

    /* release resources */
    WFC_Device_DestroyElements(device);
    WFC_Device_DestroyImageProviders(device);
    WFC_Device_DestroyStreams(device);
    WFC_Device_DestroyContexts(device);

    OWF_Mutex_Destroy(&device->mutex);
    device->mutex = NULL;

    device->latestUnreadError = WFC_ERROR_NONE;
    device->handle = WFC_INVALID_HANDLE;

    OWF_Array_RemoveItem(&(gPhyDevice.iDeviceInstanceArray), device);
    free(device);
    if (gPhyDevice.iDeviceInstanceArray.length == 0) {
        OWF_Array_Destroy(&(gPhyDevice.iDeviceInstanceArray));
    }
    LEAVE(WFC_Device_Destroy);
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCboolean WFC_Device_StreamIsTarget(WFC_DEVICE* device,
                                                  WFCNativeStreamType stream) {
    WFCint i;
    WFCboolean result = WFC_FALSE;

    OWF_ASSERT(device);

    for (i = 0; i < device->contexts.length; i++) {
        WFC_CONTEXT* ctmp;

        ctmp = CONTEXT(OWF_Array_GetItemAt(&device->contexts, i));
        if (ctmp->stream == stream) {
            result = WFC_TRUE;
            break;
        }
    }
    return result;
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFC_CONTEXT* WFC_Device_FindContextByScreen(WFC_DEVICE* device,
                                                         WFCint screenNumber) {
    WFCint i;
    WFC_CONTEXT* result = NULL;

    OWF_ASSERT(device);

    for (i = 0; i < device->contexts.length; i++) {
        WFC_CONTEXT* ctmp;

        ctmp = CONTEXT(OWF_Array_GetItemAt(&device->contexts, i));
        if (ctmp->screenNumber == screenNumber) {
            result = ctmp;
            break;
        }
    }
    return result;
}

/*---------------------------------------------------------------------------
 *  Called from context's destructor to clean up any elements that
 *  weren't added to any scene at all i.e. they only reside in the
 *  device's element list. These elements must not stay alive after
 *  the context has been deleted.
 *----------------------------------------------------------------------------*/
OWF_API_CALL void WFC_Device_DestroyContextElements(WFC_DEVICE* device,
                                                    WFC_CONTEXT* context) {
    WFCint i;

    DPRINT(("WFC_Device_DestroyContextElements(device=%d, context=%d",
            device ? device->handle : 0, context ? context->handle : 0));

    if (!device || !context) {
        return;
    }

    for (i = device->elements.length; i > 0; i--) {
        WFC_ELEMENT* element;

        element = ELEMENT(OWF_Array_GetItemAt(&device->elements, i - 1));
        if (element->context == context) {
            DPRINT(("  Destroying element %d (%p)", element->handle, element));

            /* Improvement idea: This code is partially same as in
             * WFC_Device_RemoveElement. Maybe the common part should
             * be isolated into some DoRemoveElement function which then
             * would be called from here and RemoveElement.
             */
            WFC_Context_RemoveElement(CONTEXT(element->context),
                                      element->handle);
            OWF_Array_RemoveItemAt(&device->elements, i - 1);
            WFC_Element_Destroy(element);
        }
    }
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL void WFC_Device_DestroyContextImageProviders(
    WFC_DEVICE* device, WFC_CONTEXT* context) {
    WFCint i;

    DPRINT(("WFC_Device_DestroyContextImageProviders(device=%d, context=%d",
            device ? device->handle : 0, context ? context->handle : 0));

    if (!device || !context) {
        return;
    }

    for (i = device->providers.length; i > 0; i--) {
        WFC_IMAGE_PROVIDER* provider;

        provider =
            IMAGE_PROVIDER(OWF_Array_GetItemAt(&device->providers, i - 1));
        if (provider->owner == context) {
            DPRINT(("  Destroying image provider %d (%p)", provider->handle,
                    provider));

            WFC_Device_DestroyImageProvider(device, provider->handle);
        }
    }
}

#ifdef __cplusplus
}
#endif
