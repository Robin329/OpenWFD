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
 *  \file wfdimageprovider.c
 *  \brief OpenWF Display SI, image provider module implementation.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>

#include "wfddevice.h"
#include "wfdstructs.h"
#include "wfddebug.h"

#include "owfimage.h"
#include "owfarray.h"
#include "owfmemory.h"
#include "owfobject.h"
#include "owfstream.h"

void WFD_IMAGE_PROVIDER_Ctor(void* self)
{
    self = self;
}

void WFD_IMAGE_PROVIDER_Dtor(void* self)
{

    WFD_IMAGE_PROVIDER*     ip;

    ip = (WFD_IMAGE_PROVIDER*) self;
    OWF_ASSERT(ip);

    REMREF(ip->device);
    REMREF(ip->pipeline);

    DPRINT(("WFD_IMAGE_PROVIDER_Dtor"));
    switch (ip->sourceType)
    {
        case WFD_SOURCE_STREAM:
        {
            DPRINT(("  Releasing stream"));
            OWF_Stream_Destroy(ip->source.stream);
            break;
        }
        case WFD_SOURCE_IMAGE:
        {
            REMREF(ip->source.image);
            break;
        }
        default:
        {
            OWF_ASSERT(0);
        }
    }
}

static WFD_IMAGE_PROVIDER*
WFD_ImageProvider_DoCreate(WFD_DEVICE* device,
                           WFD_PIPELINE* pipeline,
                           void* sourceHandle,
                           WFD_IMAGE_PROVIDER_SOURCE_TYPE sourceType,
                           WFD_IMAGE_PROVIDER_TYPE providerType)
{
    WFD_IMAGE_PROVIDER*         object;

    object = CREATE(WFD_IMAGE_PROVIDER);

    if (!(NULL != object && WFD_INVALID_HANDLE != sourceHandle))
    {
        DESTROY(object);
        return NULL;
    }

    object->type        = providerType;
    object->sourceType  = sourceType;
    ADDREF(object->device, device);
    ADDREF(object->pipeline, pipeline);

    switch (sourceType)
    {
        case WFD_SOURCE_STREAM:
        {
            object->source.stream = (OWF_STREAM*) sourceHandle;
            OWF_Stream_AddReference(object->source.stream);
            break;
        }
        case WFD_SOURCE_IMAGE:
        {
            ADDREF(object->source.image, (OWF_IMAGE*)sourceHandle);
            break;
        }
        default:
        {
            OWF_ASSERT(0);
        }
    }

    return object;
}

OWF_API_CALL WFD_IMAGE_PROVIDER* OWF_APIENTRY
WFD_ImageProvider_Create(WFD_DEVICE* device,
                         WFD_PIPELINE* pipeline,
                         void* source,
                         WFD_IMAGE_PROVIDER_SOURCE_TYPE sourceType,
                         WFD_IMAGE_PROVIDER_TYPE providerType) OWF_APIEXIT
{
    WFD_IMAGE_PROVIDER*     object;

    object = WFD_ImageProvider_DoCreate(device,
                                        pipeline,
                                        source,
                                        sourceType,
                                        providerType);

    DPRINT(("WFD_ImageProvider_Create: object = %p (handle = %d)",
            object, object ? object->handle : 0));
    return object;
}

OWF_API_CALL OWF_IMAGE* OWF_APIENTRY
WFD_ImageProvider_LockForReading(WFD_IMAGE_PROVIDER* provider) OWF_APIEXIT
{
    OWF_IMAGE*              result = NULL;

    if (!provider)
    {
        DPRINT(("WFD_ImageProvider_LockForReading: provider = NULL"));
        return NULL;
    }

    switch (provider->sourceType)
    {
        case WFD_SOURCE_STREAM:
        {
            result = OWF_Stream_LockForReading(provider->source.stream);
            break;
        }
        case WFD_SOURCE_IMAGE:
        {
            result = (OWF_IMAGE*) provider->source.image;
            break;
        }
        default:
        {
            OWF_ASSERT(0);
        }
    }
    return result;
}

OWF_API_CALL void OWF_APIENTRY
WFD_ImageProvider_Unlock(WFD_IMAGE_PROVIDER* provider) OWF_APIEXIT
{
    if (!provider)
    {
        DPRINT(("WFD_ImageProvider_Unlock: provider = NULL"));
        return;
    }

    switch (provider->sourceType)
    {
        case WFD_SOURCE_STREAM:
        {
            OWF_Stream_Unlock(provider->source.stream);
            break;
        }
        case WFD_SOURCE_IMAGE:
        {
            /* no need to do anything */
            break;
        }
        default:
        {
            OWF_ASSERT(0);
        }
    }
}

OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_ImageProvider_IsRegionValid(WFD_IMAGE_PROVIDER* provider,
                                const WFDRect *region) OWF_APIEXIT
{
    WFD_SOURCE*  src;
    OWF_IMAGE*   img;

    if (!region)
    {
        return WFD_TRUE;
    }

    if (provider->type != WFD_IMAGE_SOURCE)
    {
        return WFD_FALSE;
    }
    if (provider->sourceType != WFD_SOURCE_IMAGE)
    {
        return WFD_FALSE;
    }

    src = (WFD_SOURCE*)provider;
    img = (OWF_IMAGE*)src->source.image;

    if (img)
    {
        if (region->offsetX + region->width > img->width)
        {
            return WFD_FALSE;
        }
        if (region->offsetY + region->height > img->height)
        {
            return WFD_FALSE;
        }
    }

    return WFD_TRUE;
}

OWF_API_CALL void OWF_APIENTRY
WFD_ImageProvider_GetDimensions(WFD_IMAGE_PROVIDER* provider,
                                WFDint* width,
                                WFDint* height) OWF_APIEXIT
{
    switch (provider->sourceType)
    {

    case WFD_SOURCE_STREAM:
    {
        OWF_STREAM* stream;

        stream = (OWF_STREAM*) provider->source.stream;
        owfNativeStreamGetHeader(stream->handle, width, height, NULL, NULL, NULL);
        break;
    }
    case WFD_SOURCE_IMAGE:
    {
        OWF_IMAGE*  image;

        image = (OWF_IMAGE*) provider->source.image;
        if (width)
        {
            *width = image->width;
        }
        if (height)
        {
            *height = image->height;
        }
        break;
    }
    default:
        OWF_ASSERT(0);
        break;
    }
}

#ifdef __cplusplus
}
#endif


