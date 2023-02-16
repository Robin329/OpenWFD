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
 *  \file wfddevice.c
 *  \brief OpenWF Display SI, device handling module implementation.
 */


#include "wfdhandle.h"
#include "wfdconfig.h"
#include "wfddevice.h"
#include "wfdevent.h"
#include "wfdport.h"
#include "wfdpipeline.h"
#include "wfdimageprovider.h"
#include "wfddebug.h"

#include "owfarray.h"
#include "owfmemory.h"
#include "owfobject.h"

#ifdef __cplusplus
extern "C"
{
#endif


#define NONE                        0
#define DEFAULT_DEVICE_IND          0

#define STREAMS_HASH_TABLE_SIZE     256
#define IMPROVIDER_HASH_TABLE_SIZE  256


/*! \brief Create a wrapper object for a native stream
 *
 *
 *
 * \param device device object
 * \param stream native stream
 * \param write
 *
 * \return pointer to wrapper object
 */
static OWF_STREAM*
WFD_Device_CreateStream(WFD_DEVICE* device,
                        WFDNativeStreamType stream,
                        WFDboolean write) OWF_APIEXIT;

static void
WFD_Device_DestroyImageProviders(WFD_DEVICE* device);

static void
WFD_Device_DestroyStreams(WFD_DEVICE* device);

/*
 * debugging function: check if a device pointer points to valid location
 */
static WFDboolean device_exists(WFD_DEVICE* device)
{
    OWFint i = 0;
    WFDboolean isValidPointer = WFD_FALSE;

    WFD_DEVICE_CONFIG* devConfigs;
    WFDint devCount;

    devCount = WFD_Config_GetDevices(&devConfigs);
    if (devConfigs == NULL || devCount == 0)
    {
       return WFD_FALSE;
    }

    if (device != NULL)
    {
        for (i = 0; i < devCount; i++)
        {
            if (device->config == &devConfigs[i])
            {
                isValidPointer = WFD_TRUE;
                break;
            }
        }
    }

    return isValidPointer;
}

/*------------------------------------------------------------------------*/
void WFD_DEVICE_Ctor(void* self)
{
    self = self;
}


void WFD_DEVICE_Dtor(void* payload)
{
    WFD_DEVICE* pDevice;

    OWF_ASSERT(payload);

    pDevice = (WFD_DEVICE*) payload;

    DPRINT(("WFD_DEVICE_Dtor(%p)", pDevice));

    WFD_Device_DestroyImageProviders(pDevice);
    WFD_Device_DestroyStreams(pDevice);

    OWF_Mutex_Destroy(&pDevice->commitMutex);

    pDevice->config->inUse = NULL;
}


OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Device_IsAllocated(WFDint id)  OWF_APIEXIT
{
    WFD_DEVICE_CONFIG* devConfig =  WFD_Device_FindById(id);

    if (devConfig && devConfig->inUse)
    {
        return WFD_TRUE;
    }

    return WFD_FALSE;
}


OWF_API_CALL WFDDevice OWF_APIENTRY
WFD_Device_Allocate(WFDint deviceId) OWF_APIEXIT
{
    WFD_DEVICE_CONFIG* devConfig;
    WFD_DEVICE* pDevice = NULL;
    WFDboolean ok = WFD_FALSE;
    WFDDevice handle = WFD_INVALID_HANDLE;

    OWF_ASSERT(!WFD_Device_IsAllocated(deviceId));

    /* locate config area */
    devConfig = WFD_Device_FindById(deviceId);
    if (!devConfig)
    {
        /* device does not exist */
        return WFD_INVALID_HANDLE;
    }

    pDevice = CREATE(WFD_DEVICE);
    if (pDevice)
    {
        pDevice->config = devConfig;
        pDevice->config->inUse = pDevice;
        pDevice->lastUnreadError = WFD_ERROR_NONE;

        ok = (OWF_Mutex_Init(&pDevice->commitMutex) == 0);
        pDevice->busyFlag = WFD_FALSE;

        if (ok)
        {
            pDevice->streams =
                OWF_Hash_TableCreate(STREAMS_HASH_TABLE_SIZE,
                                     OWF_Hash_BitMaskHash);
            ok = (pDevice->streams != NULL);
        }

        if (ok)
        {
            pDevice->imageProviders =
                OWF_Hash_TableCreate(IMPROVIDER_HASH_TABLE_SIZE,
                                     OWF_Hash_BitMaskHash);
            ok = (pDevice->imageProviders != NULL);
        }

        OWF_Array_Initialize(&pDevice->ports);
        OWF_Array_Initialize(&pDevice->pipelines);
        OWF_Array_Initialize(&pDevice->eventConts);

        if (ok)
        {
            pDevice->handle = WFD_Handle_Create(WFD_DEVICE_HANDLE, pDevice);
            handle = pDevice->handle;
        }

        ok = ok && (handle != WFD_INVALID_HANDLE);

    }

    if (!ok && pDevice) /* error exit, clean-up resources */
    {
        DESTROY(pDevice);
    }

    DPRINT(("WFD_Device_Allocate: object = %p (handle = %d)", pDevice, handle));

    return handle;
}

OWF_API_CALL void OWF_APIENTRY
WFD_Device_Release(WFD_DEVICE* pDevice) OWF_APIEXIT
{
    OWF_ARRAY_ITEM obj;

    DPRINT(("WFD_Device_Release(%p)", pDevice));

    OWF_ASSERT(device_exists(pDevice));

    WFD_Handle_Delete(pDevice->handle);
    pDevice->handle = WFD_INVALID_HANDLE;

    /* release ports  */
    while ((obj = OWF_Array_GetItemAt(&pDevice->ports, 0)))
    {
        WFD_Port_Release(pDevice, (WFD_PORT*)obj);
    }
    /* release pipelines */
    while ((obj = OWF_Array_GetItemAt(&pDevice->pipelines, 0)))
    {
        WFD_Pipeline_Release(pDevice, (WFD_PIPELINE*)obj);
    }

    /* destroy event containers */
    while ((obj = OWF_Array_GetItemAt(&pDevice->eventConts, 0)))
    {
        WFD_Event_DestroyContainer(pDevice, (WFD_EVENT_CONTAINER*)obj);
    }

    DESTROY(pDevice);
}

/* ================================================================== */
/*                D E V I C E   L O O K U P                           */
/* ================================================================== */
OWF_API_CALL WFDint OWF_APIENTRY
WFD_Device_GetIds(WFDint *idsList, WFDint listCapacity)  OWF_APIEXIT
{
    WFDint devCount;
    WFD_DEVICE_CONFIG* devConfigs;
    WFDint count = 0;

    devCount = WFD_Config_GetDevices(&devConfigs);
    if (devConfigs == NULL || devCount == 0)
    {
       return 0;
    }

    /* No idsList given. Return number of available devices */
    if (NULL == idsList)
    {
        return devCount;
    }
    else
    {
        WFDint i = 0;

        count = (listCapacity < devCount) ? listCapacity : devCount;

        /* Populate list */
        for (i = 0; i < count; i++)
        {
            idsList[i] = devConfigs[i].id;
        }
    }

    return count;
}

OWF_API_CALL WFDint OWF_APIENTRY
WFD_Device_FilterIds(WFDint *idsList, WFDint listCapacity,
        const WFDint *filterlist) OWF_APIEXIT
{
    WFDint i, j, k;
    WFDint count = 0;
    WFDint devCount = 0;
    WFD_DEVICE_CONFIG* devConfigs;
    WFDboolean noFiltering = WFD_FALSE;

    if (!idsList && listCapacity < 1)
    {
        return 0;
    }

    devCount = WFD_Config_GetDevices(&devConfigs);
    if (devConfigs == NULL || devCount == 0)
    {
       return 0;
    }

    /* check first that filter list contains only valid attributes */
    for (i = 0; filterlist[i] != WFD_NONE; i += 2)
    {
        if (filterlist[i] != WFD_DEVICE_FILTER_PORT_ID)
        {
            return 0;
        }

        if (filterlist[i+1] == WFD_INVALID_HANDLE)
        {
            noFiltering = WFD_TRUE;
        }
    }

    /* No port ID filtering is performed if the
     * given port ID is WFD_INVALID_HANDLE. */
    if (noFiltering)
    {
        return WFD_Device_GetIds(idsList, listCapacity);
    }

    for (i = 0; filterlist[i] != WFD_NONE && count < listCapacity; i += 2)
    {
        WFDint searchedId = filterlist[i+1];

        for (j = 0; j < devCount; j++)
        {
            WFDboolean found = WFD_FALSE;

            for (k = 0; k < devConfigs[j].portCount; k++)
            {
                WFD_PORT_CONFIG* port = &devConfigs[j].ports[k];

                if (port->id == searchedId)
                {
                    /* port found - put the device id into the idsList*/
                    idsList[count++] = devConfigs[i].id;
                    found = WFD_TRUE;
                    break;
                }
            }

            if (found)
            {
                break; /* next port id */
            }
        }
    }

    return count;
}


OWF_API_CALL WFD_DEVICE* OWF_APIENTRY
WFD_Device_FindByHandle(WFDDevice devHandle) OWF_APIEXIT
{
    return (WFD_DEVICE*) WFD_Handle_GetObj(devHandle, WFD_DEVICE_HANDLE);
}

OWF_API_CALL WFD_DEVICE_CONFIG* OWF_APIENTRY
WFD_Device_FindById(WFDint id) OWF_APIEXIT
{
    WFDint i = 0;
    WFD_DEVICE_CONFIG* device = NULL;
    WFDint devCount = 0;
    WFD_DEVICE_CONFIG* devConfigs;

    DPRINT(("WFD_Device_FindById(%d)", id));

    devCount = WFD_Config_GetDevices(&devConfigs);
    if (devConfigs == NULL || devCount == 0)
    {
       return 0;
    }

    if (id == WFD_DEFAULT_DEVICE_ID)
    {
        device = &devConfigs[DEFAULT_DEVICE_IND];
    }
    else
    {
        for (i = 0; i < devCount; i++)
        {
            if (devConfigs[i].id == id)
            {
                device = &devConfigs[i];
                break;
            }
        }
    }
    return device;
}


OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Device_GetAttribi(WFD_DEVICE* pDevice,
                      WFDDeviceAttrib attrib,
                      WFDint* attrValue)  OWF_APIEXIT
{
    WFDErrorCode ec = WFD_ERROR_NONE;

    OWF_ASSERT(pDevice && pDevice->config);
    OWF_ASSERT(attrValue);

    switch (attrib) {
        case WFD_DEVICE_ID: {
            *attrValue = pDevice->config->id;
            break;
        }

        default: {
            ec = WFD_ERROR_BAD_ATTRIBUTE;
            break;
        }
    }

    return ec;
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Device_SetAttribi(WFD_DEVICE* pDevice,
                      WFDDeviceAttrib attrib,
                      WFDint attrValue) OWF_APIEXIT
{
    WFDErrorCode ec = WFD_ERROR_NONE;

    OWF_ASSERT(pDevice && pDevice->config);

    switch (attrib)
    {
    default:
        /* no valid attributes to be set */
        attrValue = attrValue;
        ec = WFD_ERROR_BAD_ATTRIBUTE;
        break;
    }

    return ec;
}


/* ================================================================== */
/*                D E V I C E   E V E N T S                           */
/* ================================================================== */

/* see wfdevent.c */


/* ================================================================== */
/*                D E V I C E   E R R O R S                           */
/* ================================================================== */

static char*                errorStrings[] =
                            {
                                "WFD_ERROR_NONE",
                                "WFD_ERROR_OUT_OF_MEMORY",
                                "WFD_ERROR_ILLEGAL_ARGUMENT",
                                "WFD_ERROR_NOT_SUPPORTED",
                                "WFD_ERROR_BAD_ATTRIBUTE",
                                "WFD_ERROR_IN_USE",
                                "WFD_ERROR_BUSY",
                                "WFD_ERROR_BAD_DEVICE",
                                "WFD_ERROR_BAD_HANDLE",
                                "WFD_ERROR_INCONSISTENCY",
                                "WFD_ERROR_FORCE_32BIT"
                            };

static const char* errorString(WFDErrorCode err)
{
    err = (err > WFD_ERROR_NONE) ? err - WFD_ERROR_OUT_OF_MEMORY + 1 : err;

    return (err <= sizeof(errorStrings) / sizeof(char*)) ?
            errorStrings[err] : "<Unknown error>";
}
OWF_API_CALL void OWF_APIENTRY
WFD_Device_SetError(WFD_DEVICE* device, WFDErrorCode error) OWF_APIEXIT
{
    OWF_ASSERT(device_exists(device));

    OWF_ASSERT(    error == WFD_ERROR_NONE ||
            (error >= WFD_ERROR_OUT_OF_MEMORY &&
             error <= WFD_ERROR_INCONSISTENCY)
          );

    if (device->lastUnreadError == WFD_ERROR_NONE && error != device->lastUnreadError)
    {
        DPRINT(("Device 0x%08x error set to %s (0x%04x)",
                device->handle, errorString(error), error));
        device->lastUnreadError = error;
    }
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Device_GetError(WFD_DEVICE* device) OWF_APIEXIT
{
    WFDErrorCode result;

    OWF_ASSERT(device_exists(device));

    result = device->lastUnreadError;
    device->lastUnreadError = WFD_ERROR_NONE;

    OWF_ASSERT(    result == WFD_ERROR_NONE ||
            (result >= WFD_ERROR_OUT_OF_MEMORY &&
            result <= WFD_ERROR_INCONSISTENCY)
          );

    return result;
}

/* ================================================================== */
/*                D E V I C E   C O M M I T                           */
/* ================================================================== */


static WFDboolean
WFD_Device_IsCommitConsistent(WFD_DEVICE* device,
                              WFD_PORT* port,
                              WFD_PIPELINE* pipeline)
{
    WFDboolean ok = WFD_TRUE;

    OWF_ASSERT(device);

    if (pipeline)
    {
        ok = WFD_Pipeline_IsCommitConsistent(pipeline, WFD_COMMIT_PIPELINE);
    }
    else if (port)
    {
        ok = WFD_Port_IsCommitConsistent(port, WFD_COMMIT_ENTIRE_PORT);
    }
    else
    {
        WFDint i=0;

        do {
            port = (WFD_PORT*)OWF_Array_GetItemAt(&device->ports, i++);

            if (port)
            {
                ok = WFD_Port_IsCommitConsistent(port, WFD_COMMIT_ENTIRE_DEVICE);

            }
        } while (port && ok);
    }

    return ok;
}

/* \brief Do device commit for one or all ports
 *
 * 1) Set commit lock
 * 2) For every port check commit consistency
 * 3) If committing a pipeline, check pipeline for consistency
 * 4) For evey port
 *    - acquire port lock
 *    - commit attribute cache
 *    - commit all pipeline attributes
 *    - resolve all source/mask bindings and
 *      update data structures for these
 *    - kick port to start working
 *    - release port lock
 * 5) If committing a pipeline
 *    - commit attribute cache
 *    - commit pipeline image provider binding:s
 *    If there also is a cached port binding
 *    - aqcuire port lock
 *    - commit pipeline binding
 *    - release port lock
 *
 * \param device pointer to device data
 * \param port pointer to port data. If port and
 * pipeline are null, all ports are commited.
 * \param pipeline pointer to pipeline data. If
 * pipeline is not null, only that pipeline is
 * committed.
 *
 * \return
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Device_Commit(WFD_DEVICE* device, WFD_PORT* port, WFD_PIPELINE* pipeline) OWF_APIEXIT
{
    WFDboolean busy = WFD_FALSE;

    OWF_ASSERT((NULL == port) || (NULL == pipeline));

    OWF_Mutex_Lock(device->commitMutex);
    busy = device->busyFlag;
    if (!busy)
    {
        device->busyFlag = WFD_TRUE;
    }
    OWF_Mutex_Unlock(device->commitMutex);

    if (busy)
    {
        return WFD_ERROR_BUSY;
    }

    /* check first for inconsistencies */
    if (!WFD_Device_IsCommitConsistent(device, port, pipeline))
    {
        OWF_Mutex_Lock(device->commitMutex);
        device->busyFlag = WFD_FALSE;
        OWF_Mutex_Unlock(device->commitMutex);
        return WFD_ERROR_INCONSISTENCY;
    }

    /* start rolling */
    if (pipeline && WFD_Pipeline_IsAllocated(device, pipeline->config->id))
    {
        WFD_Pipeline_Commit(pipeline, port);
    }
    else if (port && WFD_Port_IsAllocated(device, port->config->id))
    {
        WFD_Port_Commit(port);
    }
    else
    {
        WFDint i=0;

        do {
            port = (WFD_PORT*)OWF_Array_GetItemAt(&device->ports, i++);

            if (port)
            {
                WFD_Port_Commit(port);
            }
        } while (port);
    }

    OWF_Mutex_Lock(device->commitMutex);
    device->busyFlag = WFD_FALSE;
    OWF_Mutex_Unlock(device->commitMutex);

    return WFD_ERROR_NONE;
}


/*============================================================================*/
/*                              S T R E A M S                                 */
/*============================================================================*/

static  OWF_STREAM*
WFD_Device_CreateStream(WFD_DEVICE* device,
                        WFDNativeStreamType stream,
                        WFDboolean write)
{
    OWF_STREAM*             wrapper;

    wrapper = OWF_Hash_Lookup(device->streams, (OWF_HASHKEY) stream);
    if (wrapper)
    {
        /* wrapper for stream found; increase its use count */
    }
    else
    {
        /* no wrapper yet for this stream, create one */
        wrapper = OWF_Stream_Create(stream, write);
        OWF_Hash_Insert(device->streams, (OWF_HASHKEY) stream, wrapper);
    }
    return wrapper;
}

static void
WFD_Device_DestroyStreams(WFD_DEVICE* device)
{
    OWF_HASHKEY             keys[5];
    void*                   values[5];
    OWFuint32                count;

    OWF_ASSERT(device);

    DPRINT(("OWF_Device_DestroyStreams"));
    count = OWF_Hash_ToArray(device->streams, keys, values, 5);
    while (count > 0)
    {
        OWFuint32            i;

        DPRINT(("  destroying %d stream(s)", count));
        for (i = 0; i < count; i++)
        {
            OWF_Stream_Destroy(values[i]);
            OWF_Hash_Delete(device->streams, keys[i]);
        }

        count = OWF_Hash_ToArray(device->streams, keys, values, 5);
    }

    OWF_Hash_TableDelete(device->streams);
}


/*============================================================================*/
/*                      I M A G E   P R O V I D E R S                         */
/*============================================================================*/

OWF_API_CALL WFD_IMAGE_PROVIDER* OWF_APIENTRY
WFD_Device_CreateStreamProvider(WFD_DEVICE* device,
                                WFD_PIPELINE* pipeline,
                                WFDNativeStreamType source,
                                WFD_IMAGE_PROVIDER_TYPE type) OWF_APIEXIT
{
    OWF_STREAM*             wrapper;
    WFD_IMAGE_PROVIDER*     provider;
    WFDHandle               handle;

    wrapper = WFD_Device_CreateStream(device, source, OWF_FALSE);
    if (!wrapper)
    {
        return NULL;
    }

    provider = WFD_ImageProvider_Create(device,
                                        pipeline,
                                        wrapper,
                                        WFD_SOURCE_STREAM,
                                        type);

    handle = WFD_Handle_Create(WFD_IMAGE_SOURCE == type ?
                                   WFD_SOURCE_HANDLE : WFD_MASK_HANDLE,
                               provider);
    if (WFD_INVALID_HANDLE == handle)
    {
        DESTROY(provider);
        return NULL;
    }

    /* double-entry bookkeeping */
    if (!OWF_Hash_Insert(device->imageProviders, handle, provider))
    {
        DESTROY(provider);
        return NULL;
    }

    provider->handle = handle;

    return provider;
}

OWF_API_CALL WFD_IMAGE_PROVIDER* OWF_APIENTRY
WFD_Device_CreateImageProvider(WFD_DEVICE* device,
                               WFD_PIPELINE* pipeline,
                               WFDEGLImage source,
                               WFD_IMAGE_PROVIDER_TYPE type) OWF_APIEXIT
{
    WFD_IMAGE_PROVIDER*     provider;
    WFDHandle               handle;

    provider = WFD_ImageProvider_Create(device,
                                        pipeline,
                                        source,
                                        WFD_SOURCE_IMAGE,
                                        type);

    handle = WFD_Handle_Create(WFD_IMAGE_SOURCE == type ?
                                   WFD_SOURCE_HANDLE : WFD_MASK_HANDLE,
                               provider);
    if (WFD_INVALID_HANDLE == handle)
    {
        DESTROY(provider);
        return NULL;
    }

    /* double-entry bookkeeping */
    if (!OWF_Hash_Insert(device->imageProviders, handle, provider))
    {
        DESTROY(provider);
        return NULL;
    }

    provider->handle = handle;

    return provider;
}


OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Device_DestroyImageProvider(WFD_DEVICE* device,
                                WFDHandle handle) OWF_APIEXIT
{
    WFD_SOURCE*             provider;
    WFDErrorCode            result = WFD_ERROR_BAD_HANDLE;

    provider = WFD_Handle_GetObj(handle, WFD_SOURCE_HANDLE);
    if (!provider)
    {
        provider = WFD_Handle_GetObj(handle, WFD_MASK_HANDLE);
    }

    if (provider && provider->device == device)
    {
        OWF_Hash_Delete(device->imageProviders, handle);
        WFD_Handle_Delete(handle);
        provider->handle = WFD_INVALID_HANDLE;
        DESTROY(provider);
        result = WFD_ERROR_NONE;
    }
    return result;
}


static void
WFD_Device_DestroyImageProviders(WFD_DEVICE* device)
{
    OWF_HASHKEY             keys[5];
    void*                   values[5];
    OWFuint32               count;

    OWF_ASSERT(device);

    DPRINT(("OWF_Device_DestroyImageProviders"));
    count = OWF_Hash_ToArray(device->imageProviders, keys, values, 5);
    while (count > 0)
    {
        OWFuint32            i;

        DPRINT(("  destroying %d image provider(s)", count));
        for (i = 0; i < count; i++)
        {
            DESTROY(values[i]);
            OWF_Hash_Delete(device->imageProviders, keys[i]);
        }

        count = OWF_Hash_ToArray(device->imageProviders, keys, values, 5);
    }

    OWF_Hash_TableDelete(device->imageProviders);
}

#ifdef __cplusplus
}
#endif
