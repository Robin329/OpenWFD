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
 * \file wfd_example.c
 * \brief OpenWF Display example application.
 * \author Lars Remes <lars.remes@symbio.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "WF/wfd.h"
#include "owfimage.h"
#include "wfd_logo.h"

#define FAIL_IF(c, m) \
    if (c) { \
        printf(m); printf("\r\n"); \
        goto CLEANUP; \
    }
#define CHECK_ERROR(c, m) \
    err = wfdGetError(dev); \
    if (err != c) { \
        printf(m); printf("\r\n"); \
        goto CLEANUP; \
    }

/*!
 * Creates a native image from pixel data.
 * This function is WFD implementation specific.
 */
WFDEGLImage createNativeImage(int width,
                              int height,
                              void* data)
{
    OWF_IMAGE_FORMAT imgf;
    OWF_IMAGE* image = NULL;
    WFDint stride, numBytes;

    imgf.pixelFormat   = OWF_IMAGE_ARGB8888;
    imgf.linear        = OWF_FALSE;
    imgf.premultiplied = OWF_TRUE;
    imgf.rowPadding    = OWF_Image_GetFormatPadding(imgf.pixelFormat);

    image = OWF_Image_Create(width, height, &imgf, NULL, 0);
    assert(image != OWF_INVALID_HANDLE);

    /* overflow if size2 < size1 */
    stride = OWF_Image_GetStride(width, &imgf, 0);
    numBytes = stride * height;

    if (!image || !(image->data) || numBytes < stride) {
        OWF_Image_Destroy(image);
        image->data = NULL;
        free(image);
        image = NULL;
    } else {
        if (data) {
            memcpy(image->data, data, numBytes);
        }
    }

    return image;
}

void destroyNativeImage(WFDEGLImage image)
{
    if (image != NULL)
    {
        OWF_Image_Destroy((OWF_IMAGE*)image);
        image = NULL;
    }
}

#define PRINT_ATTRIB(attrib) \
    printf("%s: %d\r\n", "" # attrib, wfdGetPortModeAttribi(dev, port, mode, attrib));

void printPortModeInfo(WFDDevice dev, WFDPort port, WFDPortMode mode)
{
    PRINT_ATTRIB(WFD_PORT_MODE_WIDTH);
    PRINT_ATTRIB(WFD_PORT_MODE_HEIGHT);
    PRINT_ATTRIB(WFD_PORT_MODE_FLIP_MIRROR_SUPPORT);
    PRINT_ATTRIB(WFD_PORT_MODE_REFRESH_RATE);
    PRINT_ATTRIB(WFD_PORT_MODE_ROTATION_SUPPORT);
    PRINT_ATTRIB(WFD_PORT_MODE_INTERLACED);
}


int main(int argc, char **argv)
{
    WFDint numDevs = 0;
    WFDint* devIds = NULL;
    WFDint size = 0;
    WFDint numPorts = 0;
    WFDint* portIds = NULL;
    WFDint numPipelines = 0;
    WFDint* pipelineIds = NULL;
    WFDint numPortModes = 0;
    WFDPortMode* portModes = NULL;
    WFDint attribValue = 0;
    WFDDevice dev = WFD_INVALID_HANDLE;
    WFDErrorCode err = WFD_ERROR_NONE;
    WFDPort port = WFD_INVALID_HANDLE;
    WFDPipeline pipeline = WFD_INVALID_HANDLE;
    WFDSource source = WFD_INVALID_HANDLE;
    WFDEGLImage image = NULL;
    WFDint rect[4] = { 0, 0, 0, 0 };
    WFDfloat clearColor[3] = { 1.0f, 1.0f, 1.0f };

    printf("OpenWF Display example application.\r\n");
    printf("Copyright (c) 2009 The Khronos Group Inc.\r\n");

    /* Get list of devices */
    numDevs = wfdEnumerateDevices(NULL, 0, NULL);
    FAIL_IF(numDevs <= 0, "Invalid number of devices.");
    if (numDevs > 0) {
        devIds = malloc(sizeof(WFDint) * numDevs);
        size = wfdEnumerateDevices(devIds, numDevs, NULL);
        /*
         * Select correct device and create it:
         * dev = wfdCreateDevice(devIds[i], NULL);
         * FAIL_IF(dev == WFD_INVALID_HANDLE, "Failed to create default device.");
         */
        free(devIds);
        devIds = NULL;
    }

    /* Create default device */
    dev = wfdCreateDevice(WFD_DEFAULT_DEVICE_ID, NULL);
    FAIL_IF(dev == WFD_INVALID_HANDLE, "Failed to create default device.");

    /* Read a device attribute */
    attribValue = wfdGetDeviceAttribi(dev, WFD_DEVICE_ID);
    CHECK_ERROR(WFD_ERROR_NONE, "Failed to get WFD_DEVICE_ID.");

    /* Get list of available ports */
    numPorts = wfdEnumeratePorts(dev, NULL, 0, NULL);
    portIds = malloc(sizeof(WFDint) * numPorts);
    size = wfdEnumeratePorts(dev, portIds, numPorts, NULL);

    /* Create first port */
    port = wfdCreatePort(dev, portIds[0], NULL);
    CHECK_ERROR(WFD_ERROR_NONE, "Failed to create port.")
    free(portIds);
    portIds = NULL;

    /* Get port modes */
    numPortModes = wfdGetPortModes(dev, port, NULL, 0);
    portModes = malloc(sizeof(WFDint) * numPortModes);
    size = wfdGetPortModes(dev, port, portModes, numPortModes);

    /* Print port mode info */
    printPortModeInfo(dev, port, portModes[0]);

    /* Set port mode and port power mode */
    wfdSetPortMode(dev, port, portModes[0]);
    CHECK_ERROR(WFD_ERROR_NONE, "Failed to set port mode");
    wfdSetPortAttribi(dev, port, WFD_PORT_POWER_MODE, WFD_POWER_MODE_ON);
    CHECK_ERROR(WFD_ERROR_NONE, "Failed to set port power mode ON.");

    wfdSetPortAttribfv(dev, port, WFD_PORT_BACKGROUND_COLOR, 3, clearColor);
    CHECK_ERROR(WFD_ERROR_NONE, "Failed to set port background color.");

    free(portModes);
    portModes = NULL;

    /* Get list of pipelines */
    numPipelines = wfdEnumeratePipelines(dev, NULL, 0, NULL);
    pipelineIds = malloc(sizeof(WFDint) * numPipelines);
    size = wfdEnumeratePipelines(dev, pipelineIds, numPipelines, NULL);

    pipeline = wfdCreatePipeline(dev, pipelineIds[0], NULL);
    CHECK_ERROR(WFD_ERROR_NONE, "Failed to create pipeline.");

    /* use native interface to create native image */
    image = createNativeImage(wfd_logo_width, wfd_logo_height, &wfd_logo_data);

    source = wfdCreateSourceFromImage(dev, pipeline, image, NULL);
    CHECK_ERROR(WFD_ERROR_NONE, "Failed to create source from image.");

    wfdSetPipelineAttribi(dev, pipeline, WFD_PIPELINE_TRANSPARENCY_ENABLE, WFD_TRANSPARENCY_SOURCE_COLOR);
    CHECK_ERROR(WFD_ERROR_NONE, "Failed to enable transparency.");

    wfdBindSourceToPipeline(dev, pipeline, source, WFD_TRANSITION_IMMEDIATE, NULL);
    CHECK_ERROR(WFD_ERROR_NONE, "Failed to bind image.");

    wfdBindPipelineToPort(dev, port, pipeline);
    CHECK_ERROR(WFD_ERROR_NONE, "Failed to bind pipeline to port.");

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = 250;
    rect[3] = 91;
    wfdSetPipelineAttribiv(dev, pipeline, WFD_PIPELINE_SOURCE_RECTANGLE, 4, rect);
    CHECK_ERROR(WFD_ERROR_NONE, "Failed to set source rectangle.");

    wfdSetPipelineAttribiv(dev, pipeline, WFD_PIPELINE_DESTINATION_RECTANGLE, 4, rect);
    CHECK_ERROR(WFD_ERROR_NONE, "Failed to set destination rectangle.");

    free(pipelineIds);
    pipelineIds = NULL;

    wfdDeviceCommit(dev, WFD_COMMIT_ENTIRE_DEVICE, WFD_INVALID_HANDLE);
    CHECK_ERROR(WFD_ERROR_NONE, "Failed to commit changes.");

    /* Sleep for a while */
    sleep(5);

CLEANUP:
    if (pipeline != WFD_INVALID_HANDLE) {
        /* Example of how to release a pipeline */
        wfdBindMaskToPipeline(dev, pipeline, WFD_INVALID_HANDLE, WFD_TRANSITION_IMMEDIATE);
        wfdBindSourceToPipeline(dev, pipeline, WFD_INVALID_HANDLE, WFD_TRANSITION_IMMEDIATE, NULL);

        /* Changes can also be applied to pipeline only */
        wfdDeviceCommit(dev, WFD_COMMIT_PIPELINE, pipeline);
        if (wfdGetError(dev) != WFD_ERROR_NONE)
        {
            printf("Failed to commit changes");
        }

        wfdDestroyPipeline(dev, pipeline);
        pipeline = WFD_INVALID_HANDLE;
    }

    if (port != WFD_INVALID_HANDLE) {
        /* Example of how to release a port */
        /* turn port off */
        wfdSetPortAttribi(dev, port, WFD_PORT_POWER_MODE, WFD_POWER_MODE_OFF);
        if (wfdGetError(dev) != WFD_ERROR_NONE)
        {
            printf("Failed to set port power mode OFF.\r\n");
        }
        wfdDeviceCommit(dev, WFD_COMMIT_ENTIRE_PORT, port);
        if (wfdGetError(dev) != WFD_ERROR_NONE)
        {
            printf("Failed to commit port changes.\r\n");
        }

        wfdDestroyPort(dev, port);
        port = WFD_INVALID_HANDLE;
    }

    if (source != WFD_INVALID_HANDLE)
    {
        wfdDestroySource(dev, source);
        source = WFD_INVALID_HANDLE;
    }

    if (dev != WFD_INVALID_HANDLE) {
        err = wfdDestroyDevice(dev);
        dev = WFD_INVALID_HANDLE;
    }

    if (portModes != NULL)
    {
        free(portModes);
        portModes = NULL;
    }

    if (devIds != NULL)
    {
        free(devIds);
        devIds = NULL;
    }

    if (pipelineIds != NULL)
    {
        free(pipelineIds);
        pipelineIds = NULL;
    }

    if (portIds != NULL)
    {
        free(portIds);
        portIds = NULL;
    }

    destroyNativeImage(image);

    return 0;
}
