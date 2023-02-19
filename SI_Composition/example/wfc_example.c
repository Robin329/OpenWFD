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

/*!
 * \file wfc_example.c
 * \brief OpenWF Composition example application.
 * \author Lars Remes <lars.remes@symbio.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "WF/wfc.h"
#include "owfstream.h"
#include "wfc_logo.h"

#define FAIL_IF(c, m) \
    if (c) {          \
        printf(m);    \
        goto CLEANUP; \
    }
#define CHECK_ERROR(c, m)   \
    err = wfcGetError(dev); \
    if (err != c) {         \
        printf(m);          \
        goto CLEANUP;       \
    }

/*!
 * Creates a native image stream.
 * This function is WFC implementation specific.
 */
WFCNativeStreamType createNativeStream(WFCint width, WFCint height,
                                       WFCint numBuffers) {
    WFCNativeStreamType stream;
    OWF_IMAGE_FORMAT imgf;

    imgf.pixelFormat = OWF_IMAGE_ARGB8888;
    imgf.linear = OWF_FALSE;
    imgf.premultiplied = OWF_TRUE;
    imgf.rowPadding = OWF_Image_GetFormatPadding(imgf.pixelFormat);

    stream = owfNativeStreamCreateImageStream(width, height, &imgf, numBuffers);

    return stream;
}

/*!
 * Write raw image data to native stream.
 * Note that this function is WFC implementation specific.
 */
void writeImageToStream(WFCNativeStreamType stream, void* data,
                        WFCint numPixels) {
    OWFNativeStreamBuffer buffer;
    void* dataPtr;

    buffer = owfNativeStreamAcquireWriteBuffer(stream);
    dataPtr = owfNativeStreamGetBufferPtr(stream, buffer);

    memcpy(dataPtr, data, numPixels * 4);

    owfNativeStreamReleaseWriteBuffer(stream, buffer, EGL_DEFAULT_DISPLAY,
                                      EGL_NO_SYNC_KHR);
}

/*!
 * Destroys native stream.
 * Note that this function is WFC implementation specific.
 */
void destroyNativeStream(WFCNativeStreamType stream) {
    owfNativeStreamDestroy(stream);
}

int main(int argc, char** argv) {
    WFCint numDevs = 0;
    WFCint* devIds = NULL;
    WFCint size = 0;
    WFCContext ctx = WFC_INVALID_HANDLE;
    WFCint attribValue = 0;
    WFCDevice dev = WFC_INVALID_HANDLE;
    WFCErrorCode err = WFC_ERROR_NONE;
    WFCSource source = WFC_INVALID_HANDLE;
    WFCNativeStreamType sourceStream = WFC_INVALID_HANDLE;
    WFCElement element1 = WFC_INVALID_HANDLE;
    WFCElement element2 = WFC_INVALID_HANDLE;
    WFCint rect[4] = {0, 0, 0, 0};
    WFCfloat bgColor[4] = {0.7f, 0.7f, 1.0f, 1.0f};
    WFCint i = 0;
    WFCint ctxHeight = 0;
    WFCint ctxWidth = 0;

    printf("OpenWF Composition example application.\r\n");
    printf("Copyright (c) 2009 The Khronos Group Inc.\r\n");

    /* Get list of devices */
    numDevs = wfcEnumerateDevices(NULL, 0, NULL);
    FAIL_IF(numDevs <= 0, "Invalid number of devices");
    if (numDevs > 0) {
        devIds = malloc(sizeof(WFCint) * numDevs);
        size = wfcEnumerateDevices(devIds, numDevs, NULL);

        /* select correct device */
        for (i = 0; i < numDevs; ++i) {
            dev = wfcCreateDevice(devIds[i], NULL);
            attribValue = wfcGetDeviceAttribi(dev, WFC_DEVICE_CLASS);
            if (attribValue == WFC_DEVICE_CLASS_FULLY_CAPABLE) {
                break;
            }
            err = wfcDestroyDevice(dev);
        }
        free(devIds);

        FAIL_IF(dev == WFC_INVALID_HANDLE,
                "No on-screen capable device found.");
    }

    /* Read a device attribute */
    attribValue = wfcGetDeviceAttribi(dev, WFC_DEVICE_ID);
    CHECK_ERROR(WFC_ERROR_NONE, "Failed to get WFC_DEVICE_ID");
    printf("Device id [%d]\r\n", attribValue);

    ctx = wfcCreateOnScreenContext(dev, WFC_DEFAULT_SCREEN_NUMBER, NULL);
    CHECK_ERROR(WFC_ERROR_NONE,
                "Failed to get create context for default screen.");

    /* set a context attribute */
    wfcSetContextAttribfv(dev, ctx, WFC_CONTEXT_BG_COLOR, 4, bgColor);

    /* create single buffered input stream */
    sourceStream = createNativeStream(wfc_logo_width, wfc_logo_height, 2);

    source = wfcCreateSourceFromStream(dev, ctx, sourceStream, NULL);
    CHECK_ERROR(WFC_ERROR_NONE, "Failed to create source stream.");

    writeImageToStream(sourceStream, &wfc_logo_data,
                       wfc_logo_width * wfc_logo_height);

    /*
     * Create first element and insert it at the bottom of the element stack.
     */
    element1 = wfcCreateElement(dev, ctx, NULL);
    FAIL_IF(element1 == WFC_INVALID_HANDLE, "Failed to create element.");

    wfcInsertElement(dev, element1, WFC_INVALID_HANDLE);
    CHECK_ERROR(WFC_ERROR_NONE, "Failed to insert element.");

    /* set element attributes */
    wfcSetElementAttribi(dev, element1, WFC_ELEMENT_SOURCE, source);
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = wfc_logo_width;
    rect[3] = wfc_logo_height;
    wfcSetElementAttribiv(dev, element1, WFC_ELEMENT_SOURCE_RECTANGLE, 4, rect);

    rect[0] = 0;
    rect[1] = 0;
    ctxWidth = wfcGetContextAttribi(dev, ctx, WFC_CONTEXT_TARGET_WIDTH);
    ctxHeight = wfcGetContextAttribi(dev, ctx, WFC_CONTEXT_TARGET_HEIGHT);
    rect[2] = ctxWidth;
    rect[3] = (WFCint)(wfc_logo_height * (wfc_logo_width / ctxWidth));
    wfcSetElementAttribiv(dev, element1, WFC_ELEMENT_DESTINATION_RECTANGLE, 4,
                          rect);

    wfcSetElementAttribi(dev, element1, WFC_ELEMENT_TRANSPARENCY_TYPES,
                         WFC_TRANSPARENCY_SOURCE);
    CHECK_ERROR(WFC_ERROR_NONE, "Failed to set element transparency.");

    wfcSetElementAttribi(dev, element1, WFC_ELEMENT_SOURCE_SCALE_FILTER,
                         WFC_SCALE_FILTER_BETTER);
    CHECK_ERROR(WFC_ERROR_NONE, "Failed to set element transparency.");

    /*
     * Create second element and insert it at the bottom of the element stack.
     */
    element2 = wfcCreateElement(dev, ctx, NULL);
    FAIL_IF(element2 == WFC_INVALID_HANDLE, "Failed to create element.");

    wfcInsertElement(dev, element2, WFC_INVALID_HANDLE);
    CHECK_ERROR(WFC_ERROR_NONE, "Failed to insert element.");

    /* set element attributes */
    wfcSetElementAttribi(dev, element2, WFC_ELEMENT_SOURCE, source);
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = wfc_logo_width;
    rect[3] = wfc_logo_height;
    wfcSetElementAttribiv(dev, element2, WFC_ELEMENT_SOURCE_RECTANGLE, 4, rect);

    rect[0] = 0;
    rect[1] = wfc_logo_height;
    ctxWidth = wfcGetContextAttribi(dev, ctx, WFC_CONTEXT_TARGET_WIDTH);
    ctxHeight = wfcGetContextAttribi(dev, ctx, WFC_CONTEXT_TARGET_HEIGHT);
    rect[2] = ctxWidth;
    rect[3] = (WFCint)((wfc_logo_height * (wfc_logo_width / ctxWidth)) / 2);
    wfcSetElementAttribiv(dev, element2, WFC_ELEMENT_DESTINATION_RECTANGLE, 4,
                          rect);

    wfcSetElementAttribi(dev, element2, WFC_ELEMENT_TRANSPARENCY_TYPES,
                         WFC_TRANSPARENCY_SOURCE);
    CHECK_ERROR(WFC_ERROR_NONE, "Failed to set element transparency.");

    wfcSetElementAttribi(dev, element2, WFC_ELEMENT_SOURCE_FLIP, WFC_TRUE);
    CHECK_ERROR(WFC_ERROR_NONE, "Failed to set element transparency.");

    wfcSetElementAttribi(dev, element2, WFC_ELEMENT_SOURCE_SCALE_FILTER,
                         WFC_SCALE_FILTER_BETTER);
    CHECK_ERROR(WFC_ERROR_NONE, "Failed to set element transparency.");

    wfcCommit(dev, ctx, WFC_TRUE);
    CHECK_ERROR(WFC_ERROR_NONE, "Failed to commit changes.");

    wfcCompose(dev, ctx, WFC_TRUE);
    CHECK_ERROR(WFC_ERROR_NONE, "Failed to compose scene.");

    /*
     * Elements can be destroyed at any time, but resources will be freed only
     * when they are not used in the scene anymore.
     */
    wfcDestroyElement(dev, element2);

    /* Sleep for a while */
    sleep(5);

CLEANUP:
    if (dev != WFC_INVALID_HANDLE) {
        if (element1 != WFC_INVALID_HANDLE) {
            /* example how to reset element source and mask */
            wfcSetElementAttribi(dev, element1, WFC_ELEMENT_SOURCE,
                                 WFC_INVALID_HANDLE);
            wfcSetElementAttribi(dev, element1, WFC_ELEMENT_MASK,
                                 WFC_INVALID_HANDLE);
            wfcCommit(dev, ctx, WFC_TRUE);

            wfcDestroyElement(dev, element1);
        }

        if (element2 != WFC_INVALID_HANDLE) {
            /* example how to reset element source and mask */
            wfcSetElementAttribi(dev, element2, WFC_ELEMENT_SOURCE,
                                 WFC_INVALID_HANDLE);
            wfcSetElementAttribi(dev, element2, WFC_ELEMENT_MASK,
                                 WFC_INVALID_HANDLE);
            wfcCommit(dev, ctx, WFC_TRUE);

            wfcDestroyElement(dev, element2);
        }

        if (ctx != WFC_INVALID_HANDLE) {
            wfcDestroyContext(dev, ctx);
        }

        err = wfcDestroyDevice(dev);
        dev = WFC_INVALID_HANDLE;
    }

    if (sourceStream != WFC_INVALID_HANDLE) {
        destroyNativeStream(sourceStream);
    }

    return 0;
}
