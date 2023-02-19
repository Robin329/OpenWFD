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
 *  \file wfdimageprovider.h
 *  \brief Interface to display image providers
 *
 */

#ifndef WFDIMAGEPROVIDER_H_
#define WFDIMAGEPROVIDER_H_

#include <stdio.h>
#include <stdlib.h>

#include "WF/wfd.h"
#include "owfstream.h"
#include "wfdstructs.h"

#ifdef __cplusplus
extern "C" {
#endif

OWF_API_CALL WFD_IMAGE_PROVIDER *WFD_ImageProvider_Create(
    WFD_DEVICE *device, WFD_PIPELINE *pipeline, void *source,
    WFD_IMAGE_PROVIDER_SOURCE_TYPE sourceType,
    WFD_IMAGE_PROVIDER_TYPE providerType) OWF_APIEXIT;

OWF_API_CALL OWF_IMAGE *WFD_ImageProvider_LockForReading(
    WFD_IMAGE_PROVIDER *provider) OWF_APIEXIT;

OWF_API_CALL void WFD_ImageProvider_Unlock(WFD_IMAGE_PROVIDER *provider)
    OWF_APIEXIT;

OWF_API_CALL WFDboolean WFD_ImageProvider_IsRegionValid(
    WFD_IMAGE_PROVIDER *provider, const WFDRect *region) OWF_APIEXIT;

OWF_API_CALL void WFD_ImageProvider_GetDimensions(WFD_IMAGE_PROVIDER *provider,
                                                  WFDint *width,
                                                  WFDint *height) OWF_APIEXIT;

#ifdef __cplusplus
}
#endif

#endif /* WFCIMAGEPROVIDER_H_ */
