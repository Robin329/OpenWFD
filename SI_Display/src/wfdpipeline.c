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
 *  \file wfdpipeline.c
 *  \brief OpenWF Display SI, pipeline implementation.
 */


#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "wfdhandle.h"
#include "wfdpipeline.h"
#include "wfdport.h"
#include "wfddebug.h"
#include "wfdevent.h"
#include "wfdutils.h"
#include "wfdimageprovider.h"

#include "owfhandle.h"
#include "owfimage.h"
#include "owfthread.h"
#include "owfmemory.h"
#include "owfobject.h"

#define ID(x)      (x->config->id)


static WFDErrorCode
WFD_Pipeline_ValidateAttribi(WFD_PIPELINE* pipeline,
                             WFDPipelineConfigAttrib attrib,
                             WFDint value);
static WFDErrorCode
WFD_Pipeline_ValidateAttribf(WFD_PIPELINE* pipeline,
                             WFDPipelineConfigAttrib attrib,
                             WFDfloat value);
static WFDErrorCode
WFD_Pipeline_ValidateAttribiv(WFD_PIPELINE* pipeline,
                              WFDPipelineConfigAttrib attrib,
                              WFDint count,
                              const WFDint* values);

static WFDErrorCode
WFD_Pipeline_ValidateAttribfv(WFD_PIPELINE* pipeline,
                              WFDPipelineConfigAttrib attrib,
                              WFDint count,
                              const WFDfloat* values);

static void
WFD_Pipeline_SourceStreamUpdated(OWFNativeStreamType stream,
                                 OWFNativeStreamEvent event,
                                 void* data);


static WFDboolean
WFD_Pipeline_InitScratchBuffers(WFD_PIPELINE* pPipeline);

static WFDboolean
WFD_Pipeline_InitBindings(WFD_PIPELINE* pPipeline);

static void
WFD_Pipeline_Preconfiguration(WFD_PIPELINE* pPipeline);

/* \brief
 *
 * \param pPipeline Pointer to pipeline object
 *
 * \return
 */
static WFDboolean
WFD_Pipeline_CommitImageProviders(WFD_PIPELINE* pPipeline) OWF_APIEXIT;



OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Pipeline_InitAttributes(WFD_PIPELINE* pPipeline) OWF_APIEXIT
{
    WFDint              ec;
    WFD_PIPELINE_CONFIG* config;

    OWF_ASSERT(pPipeline && pPipeline->config);

    config = pPipeline->config;

    /* preconditions: default values already exist */

    OWF_AttributeList_Create(&pPipeline->attributes,
                             WFD_PIPELINE_ID,
                             WFD_PIPELINE_GLOBAL_ALPHA);

    ec = OWF_AttributeList_GetError(&pPipeline->attributes);
    if (ec != ATTR_ERROR_NONE)
    {
        DPRINT(("Error at pipeline attribute list creation (%d)", ec));
        return WFD_FALSE;
    }

    OWF_Attribute_Initi(&pPipeline->attributes,
                        WFD_PIPELINE_ID,
                        (OWFint*) &config->id,
                        OWF_TRUE);

    OWF_Attribute_Initi(&pPipeline->attributes,
                         WFD_PIPELINE_PORTID,
                         (OWFint*) &config->portId,
                         OWF_TRUE);

    OWF_Attribute_Initi(&pPipeline->attributes,
                        WFD_PIPELINE_LAYER,
                        (OWFint*) &config->layer,
                        OWF_TRUE);

    OWF_Attribute_Initb(&pPipeline->attributes,
                        WFD_PIPELINE_SHAREABLE,
                        (OWFboolean*) &config->shareable,
                        OWF_TRUE);

    OWF_Attribute_Initb(&pPipeline->attributes,
                        WFD_PIPELINE_DIRECT_REFRESH,
                        (OWFboolean*) &config->directRefresh,
                        OWF_TRUE);

    OWF_Attribute_Initiv(&pPipeline->attributes,
                         WFD_PIPELINE_MAX_SOURCE_SIZE,
                         2,
                         (OWFint*) config->maxSourceSize,
                         OWF_TRUE);

    OWF_Attribute_Initiv(&pPipeline->attributes,
                         WFD_PIPELINE_SOURCE_RECTANGLE,
                         RECT_SIZE,
                         (OWFint*) config->sourceRectangle,
                         OWF_FALSE);

    OWF_Attribute_Initb(&pPipeline->attributes,
                        WFD_PIPELINE_FLIP,
                        (OWFboolean*) &config->flip,
                        OWF_FALSE);

    OWF_Attribute_Initb(&pPipeline->attributes,
                        WFD_PIPELINE_MIRROR,
                        (OWFboolean*) &config->mirror,
                        OWF_FALSE);

    OWF_Attribute_Initi(&pPipeline->attributes,
                        WFD_PIPELINE_ROTATION_SUPPORT,
                        (OWFint*) &config->rotationSupport,
                        OWF_TRUE);

    OWF_Attribute_Initi(&pPipeline->attributes,
                        WFD_PIPELINE_ROTATION,
                        (OWFint*) &config->rotation,
                        OWF_FALSE);

    OWF_Attribute_Initfv(&pPipeline->attributes,
                         WFD_PIPELINE_SCALE_RANGE,
                         2,
                         (OWFfloat*) config->scaleRange,
                         OWF_TRUE);

    OWF_Attribute_Initi(&pPipeline->attributes,
                        WFD_PIPELINE_SCALE_FILTER,
                        (OWFint*) &config->scaleFilter,
                        OWF_FALSE);

    OWF_Attribute_Initiv(&pPipeline->attributes,
                         WFD_PIPELINE_DESTINATION_RECTANGLE,
                         4,
                         (OWFint*) config->destinationRectangle,
                         OWF_FALSE);

    OWF_Attribute_Initi(&pPipeline->attributes,
                        WFD_PIPELINE_TRANSPARENCY_ENABLE,
                        (OWFint*) &config->transparencyEnable,
                        OWF_FALSE);

    OWF_Attribute_Initf(&pPipeline->attributes,
                        WFD_PIPELINE_GLOBAL_ALPHA,
                        (OWFfloat*) &config->globalAlpha,
                        OWF_FALSE);


    ec = OWF_AttributeList_GetError(&pPipeline->attributes);
    if (ec != ATTR_ERROR_NONE)
    {
        DPRINT(("Error at pipeline attribute list initialization (%d)", ec));
        return WFD_FALSE;
    }

    return WFD_TRUE;
}

OWF_API_CALL WFDint OWF_APIENTRY
WFD_Pipeline_GetIds(WFD_DEVICE* device,
                    WFDint* idsList,
                    WFDint listCapacity) OWF_APIEXIT
{
    WFDint count=0;
    WFD_DEVICE_CONFIG* devConfig;

    OWF_ASSERT(device && device->config);

    devConfig = device->config;

    if (!idsList)
    {
        count = devConfig->pipelineCount;
    }
    else
    {
        WFDint i;

        for (i = 0; i < devConfig->pipelineCount && count < listCapacity; i++)
        {
            if (devConfig->pipelines[i].id != WFD_INVALID_PIPELINE_ID)
            {
                idsList[count] = devConfig->pipelines[i].id;
                ++count;
            }
        }

        for (i = count; i < listCapacity; i++)
        {
            idsList[i] = WFD_INVALID_PIPELINE_ID;
        }

    }

    return count;
}

void WFD_PIPELINE_Ctor(void* self)
{
    self = self;
}


void WFD_PIPELINE_Dtor(void* payload)
{
    WFD_PIPELINE* pPipeline;
    WFD_DEVICE* pDevice = NULL;
    WFD_PIPELINE_CONFIG* plConfig = NULL;
    WFDint pipelineId;
    WFDint i;

    pPipeline = (WFD_PIPELINE*) payload;

    OWF_ASSERT(pPipeline->config);

    pipelineId = pPipeline->config->id;
    pDevice = pPipeline->device;

    for (i=0; i<WFD_PIPELINE_SCRATCH_COUNT; i++)
    {
        OWF_Image_Destroy(pPipeline->scratch[i]);
    }

    xfree(pPipeline->bindings);
    pPipeline->bindings = NULL;
    pPipeline->config->inUse = NULL;

    REMREF(pPipeline->device);

    OWF_AttributeList_Destroy(&pPipeline->attributes);

    xfree(pPipeline->config);
    pPipeline->config = NULL;

    /* locate static config area and mark port free */
    plConfig = WFD_Pipeline_FindById(pDevice, pipelineId);
    if (plConfig)
    {
        plConfig->inUse = NULL;
    }
}


static WFDboolean
WFD_Pipeline_InitScratchBuffers(WFD_PIPELINE* pPipeline)
{
    WFDboolean ret = WFD_TRUE;

    OWF_ASSERT(pPipeline && pPipeline->config);

    ret = WFD_Util_InitScratchBuffer(pPipeline->scratch,
                                     WFD_PIPELINE_SCRATCH_COUNT,
                                     pPipeline->config->maxSourceSize[0],
                                     pPipeline->config->maxSourceSize[1]);

    return ret;
}

static WFDboolean
WFD_Pipeline_InitBindings(WFD_PIPELINE* pPipeline)
{
    OWF_ASSERT(pPipeline);

    pPipeline->bindings = NEW0(WFD_PIPELINE_BINDINGS);

    if (pPipeline->bindings)
    {
        pPipeline->bindings->pipeline = pPipeline;

        pPipeline->bindings->boundMaskTransition =
            pPipeline->bindings->boundSrcTransition =
                pPipeline->bindings->cachedMaskTransition =
                    pPipeline->bindings->cachedSrcTransition = WFD_TRANSITION_INVALID;
        return WFD_TRUE;
    }

    return WFD_FALSE;
}

static void
WFD_Pipeline_Preconfiguration(WFD_PIPELINE* pPipeline)
{
    OWF_ASSERT(pPipeline && pPipeline->config && pPipeline->bindings);

    /* pre-configured port-pipeline binding */
    if (pPipeline->config->portId != WFD_INVALID_PORT_ID)
    {
        WFD_PORT_CONFIG* portConfig = NULL;
        WFD_PORT* pPort = NULL;

        portConfig = WFD_Port_FindById(pPipeline->device,
                pPipeline->config->portId);

        if (portConfig)
        {
            pPort = (WFD_PORT*)portConfig->inUse;
        }

        if (pPort)
        {
            WFDint pipelineInd;

            WFD_Port_AcquireLock(pPort);
            pipelineInd = WFD_Port_PipelineNbr(pPort, pPipeline);
            if (pipelineInd >= 0)
            {
                ADDREF(pPort->bindings[pipelineInd].boundPipeline,
                        pPipeline);
                ADDREF(pPipeline->bindings->boundPort, pPort);
                pPipeline->config->layer =
                    WFD_Port_QueryPipelineLayerOrder(pPort, pPipeline);
                pPipeline->config->portId =  pPort->config->id;
            }
            WFD_Port_ReleaseLock(pPort);

            DPRINT(("WFD_Pipeline_InitPreconfiguredBindings: port %d -> pipeline %d",
                    pPipeline->config->portId , pPipeline->config->id ));
        }
    }
}

OWF_API_CALL WFDPipeline OWF_APIENTRY
WFD_Pipeline_Allocate(WFD_DEVICE* pDevice,
                      WFDint pipelineId) OWF_APIEXIT
{
    WFD_PIPELINE_CONFIG* plConfig;
    WFD_PIPELINE* pPipeline = NULL;
    WFDboolean ok = WFD_FALSE;
    WFDPipeline handle = WFD_INVALID_HANDLE;

    /* locate the static config area */
    plConfig = WFD_Pipeline_FindById(pDevice, pipelineId);
    if (!plConfig)
    {
        /* pipeline does not exist */
        return WFD_INVALID_HANDLE;
    }

    pPipeline = CREATE(WFD_PIPELINE);
    if (pPipeline)
    {
        ADDREF(pPipeline->device,pDevice);
        OWF_Array_AppendItem(&pDevice->pipelines, pPipeline);

        /* mark the port allocated */
        plConfig->inUse = pPipeline;

        /* make copy of the static config are. this holds
         * committed pipeline attributes during pipeline's life-time
         */
        pPipeline->config = NEW0(WFD_PIPELINE_CONFIG);
        ok = (pPipeline->config != NULL);

        if (ok)
        {
            memcpy(pPipeline->config, plConfig, sizeof(WFD_PIPELINE_CONFIG));
        }

        ok = WFD_Pipeline_InitAttributes(pPipeline);
        if (ok)
        {
            ok = WFD_Pipeline_InitScratchBuffers(pPipeline);
        }

        if (ok)
        {
            ok = WFD_Pipeline_InitBindings(pPipeline);
        }


        if (ok)
        {
            pPipeline->handle = WFD_Handle_Create(WFD_PIPELINE_HANDLE, pPipeline);
            handle = pPipeline->handle;
        }

        ok = ok && (handle != WFD_INVALID_HANDLE);

    }

    if (!ok && pPipeline) /* error exit, clean-up resources */
    {
        WFD_Handle_Delete(pPipeline->handle);
        pPipeline->handle = WFD_INVALID_HANDLE;
        OWF_Array_RemoveItem(&pDevice->pipelines, pPipeline);
        DESTROY(pPipeline);
    }
    else
    {
        WFD_Pipeline_Preconfiguration(pPipeline);
    }

    DPRINT(("WFD_Pipeline_Allocate: pipeline %d, object = %p (handle = 0x%08x)",
            pipelineId, pPipeline, handle));

    OWF_AttributeList_Commit(&pPipeline->attributes,
                             WFD_PIPELINE_ID,
                             WFD_PIPELINE_GLOBAL_ALPHA,
			     WORKING_ATTR_VALUE_INDEX);

    return handle;
}


OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_Release(WFD_DEVICE* pDevice,
                      WFD_PIPELINE* pPipeline) OWF_APIEXIT
{
    WFD_PORT* bPort; /* bound port */
    WFD_PORT* cPort; /* cached port */

    OWF_ASSERT(pPipeline && pPipeline->config && pPipeline->bindings);

    bPort = pPipeline->bindings->boundPort;
    cPort = pPipeline->bindings->cachedPort;

    pDevice = pDevice;

    DPRINT(("WFD_Pipeline_Release, pipeline %d (%p)", pPipeline->config->id, pPipeline));

    WFD_Handle_Delete(pPipeline->handle);
    pPipeline->handle = WFD_INVALID_HANDLE;

    /* tear off pipeline/port connections */
    if (bPort)
    {
        WFD_Port_AcquireLock(bPort);
        WFD_Pipeline_PortRemoveBinding(bPort, pPipeline, WFD_FALSE);
        WFD_Port_ReleaseLock(bPort);
    }

    if (cPort)
    {
        WFD_Port_AcquireLock(cPort);
        WFD_Pipeline_PortRemoveBinding(cPort, pPipeline, WFD_TRUE);
        WFD_Port_ReleaseLock(cPort);
    }

    /* remove source/mask bindings */
    WFD_Pipeline_SourceRemoveBinding(pPipeline);
    WFD_Pipeline_MaskRemoveBinding(pPipeline);

    /* remove device-pipeline connection */
    OWF_Array_RemoveItem(&pDevice->pipelines, pPipeline);

    DESTROY(pPipeline);
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Pipeline_IsAllocated(WFD_DEVICE* pDevice,
                         WFDint id) OWF_APIEXIT
{
    WFDint                  i;
    WFD_DEVICE_CONFIG*      devConfig;
    WFD_PIPELINE_CONFIG*    plConfig = NULL;

    OWF_ASSERT (pDevice && pDevice->config);

    devConfig = pDevice->config;

    for (i = 0; i < devConfig->pipelineCount; i++)
    {
        plConfig = &devConfig->pipelines[i];
        if (plConfig->id == id)
        {
            return (plConfig->inUse == NULL) ? WFD_ERROR_NONE : WFD_ERROR_IN_USE;
        }
    }
    return WFD_ERROR_ILLEGAL_ARGUMENT;
}


OWF_API_CALL WFD_PIPELINE_CONFIG* OWF_APIENTRY
WFD_Pipeline_FindById(WFD_DEVICE* pDevice,
                      WFDint id) OWF_APIEXIT
{
    WFDint                    ii;
    WFD_PIPELINE_CONFIG*      pl = NULL;

    OWF_ASSERT(pDevice && pDevice->config);

    for (ii = 0; ii < pDevice->config->pipelineCount; ii++) {
        if (pDevice->config->pipelines[ii].id == id) {
            pl = &pDevice->config->pipelines[ii];
            break;
        }
    }
    return pl;
}

OWF_API_CALL WFD_PIPELINE* OWF_APIENTRY
WFD_Pipeline_FindByHandle(WFD_DEVICE* pDevice, WFDPipeline pipeline) OWF_APIEXIT
{
    WFD_PIPELINE* pPipeline;

    OWF_ASSERT(pDevice);

    /* get pipeline data */
    pPipeline = (WFD_PIPELINE*) WFD_Handle_GetObj(pipeline, WFD_PIPELINE_HANDLE);

    /* pipeline must be associated with device */
    return (pPipeline && pPipeline->device == pDevice) ? pPipeline : NULL;
}

/* ================================================================== */
/*     P I P E L I N E  A T T R I B U T E   H A N D L I N G           */
/* ================================================================== */



OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Pipeline_GetAttribi(WFD_PIPELINE* pipeline,
                        WFDPipelineConfigAttrib attrib,
                        WFDint* value) OWF_APIEXIT
{
    WFDErrorCode ec;

    OWF_ASSERT(pipeline && value);

    if (attrib == WFD_PIPELINE_GLOBAL_ALPHA)
    {
        WFDfloat ga;

        ga = OWF_Attribute_GetValuef(&pipeline->attributes, attrib);
        ec = OWF_AttributeList_GetError(&pipeline->attributes);
        if (ec == ATTR_ERROR_NONE)
        {
            *value = (WFDint)WFD_Util_Float2Byte(ga);
        }
        return ec;
    }
    else
    {
        *value = OWF_Attribute_GetValuei(&pipeline->attributes, attrib);
        ec = OWF_AttributeList_GetError(&pipeline->attributes);
    }
    return WFD_Util_AttrEc2WfdEc(ec);

}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Pipeline_GetAttribf(WFD_PIPELINE* pipeline,
                        WFDPipelineConfigAttrib attrib,
                        WFDfloat* value) OWF_APIEXIT
{
    OWF_ASSERT(pipeline && value);

    *value = OWF_Attribute_GetValuef(&pipeline->attributes, attrib);
    return WFD_Util_AttrEc2WfdEc(OWF_AttributeList_GetError(&pipeline->attributes));
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Pipeline_GetAttribiv(WFD_PIPELINE* pipeline,
                          WFDPipelineConfigAttrib attrib,
                          WFDint count,
                          WFDint* value) OWF_APIEXIT
{
    WFDint          temp;
    WFDint          aLength;

    OWF_ASSERT(pipeline && value);
    OWF_ASSERT(count > 0);

    /* check that count is ok for given attrib */
    aLength = OWF_Attribute_GetValueiv(&pipeline->attributes, attrib, 0, NULL);
    if (aLength != count)
    {
        return WFD_ERROR_ILLEGAL_ARGUMENT;
    }

    temp = OWF_Attribute_GetValueiv(&pipeline->attributes, attrib, count, value);
    if (0 != value && temp < count)
    {
        return WFD_ERROR_ILLEGAL_ARGUMENT;
    }
    return WFD_Util_AttrEc2WfdEc(OWF_AttributeList_GetError(&pipeline->attributes));
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Pipeline_GetAttribfv(WFD_PIPELINE* pipeline,
                          WFDPipelineConfigAttrib attrib,
                          WFDint count,
                          WFDfloat* value) OWF_APIEXIT
{

    WFDint          temp;
    WFDint          aLength;

    OWF_ASSERT(pipeline && value);
    OWF_ASSERT(count > 0);

    /* check that count is ok for given attrib */
    aLength = OWF_Attribute_GetValuefv(&pipeline->attributes, attrib, 0, NULL);
    if (aLength != count)
    {
        return WFD_ERROR_ILLEGAL_ARGUMENT;
    }

    temp = OWF_Attribute_GetValuefv(&pipeline->attributes, attrib, count, value);
    if (0 != value && temp < count)
    {
        return WFD_ERROR_ILLEGAL_ARGUMENT;
    }
    return WFD_Util_AttrEc2WfdEc(OWF_AttributeList_GetError(&pipeline->attributes));
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Pipeline_SetAttribi(WFD_PIPELINE* pipeline,
                        WFDPipelineConfigAttrib attrib,
                        WFDint value) OWF_APIEXIT
{
    WFDErrorCode            ec = WFD_ERROR_NONE;

    OWF_ASSERT(pipeline);

    ec = WFD_Pipeline_ValidateAttribi(pipeline, attrib, value);
    if (WFD_ERROR_NONE == ec)
    {
        if (attrib == WFD_PIPELINE_GLOBAL_ALPHA)
        {
            WFDfloat ga;

            ga = (WFDfloat)value / 255.0;
            OWF_Attribute_SetValuef(&pipeline->attributes, attrib, ga);
        }
        else
        {
           OWF_Attribute_SetValuei(&pipeline->attributes, attrib, value);
        }
        ec = WFD_Util_AttrEc2WfdEc(OWF_AttributeList_GetError(&pipeline->attributes));
    }
    return ec;
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Pipeline_SetAttribf(WFD_PIPELINE* pipeline,
                        WFDPipelineConfigAttrib attrib,
                        WFDfloat value) OWF_APIEXIT
{
    WFDErrorCode            ec = WFD_ERROR_NONE;

    OWF_ASSERT(pipeline);

    ec = WFD_Pipeline_ValidateAttribf(pipeline, attrib, value);
    if (WFD_ERROR_NONE == ec)
    {
        OWF_Attribute_SetValuef(&pipeline->attributes, attrib, value);
        ec = WFD_Util_AttrEc2WfdEc(OWF_AttributeList_GetError(&pipeline->attributes));
    }
    return ec;
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Pipeline_SetAttribiv(WFD_PIPELINE* pipeline,
                          WFDPipelineConfigAttrib attrib,
                          WFDint count,
                          const WFDint* values) OWF_APIEXIT
{
    WFDErrorCode            ec = WFD_ERROR_NONE;

    OWF_ASSERT(pipeline && values);
    OWF_ASSERT(count > 0);

    ec = WFD_Pipeline_ValidateAttribiv(pipeline, attrib, count, values);
    if (WFD_ERROR_NONE == ec)
    {
        OWF_Attribute_SetValueiv(&pipeline->attributes, attrib, count, values);
        ec = WFD_Util_AttrEc2WfdEc(OWF_AttributeList_GetError(&pipeline->attributes));
    }
    return ec;
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Pipeline_SetAttribfv(WFD_PIPELINE* pipeline,
                          WFDPipelineConfigAttrib attrib,
                          WFDint count,
                          const WFDfloat* values) OWF_APIEXIT
{
    WFDErrorCode            ec = WFD_ERROR_NONE;

    OWF_ASSERT(pipeline && values);
    OWF_ASSERT(count > 0);

    ec = WFD_Pipeline_ValidateAttribfv(pipeline, attrib, count, values);
    if (WFD_ERROR_NONE == ec)
    {
        OWF_Attribute_SetValuefv(&pipeline->attributes, attrib, count, values);
        ec = WFD_Util_AttrEc2WfdEc(OWF_AttributeList_GetError(&pipeline->attributes));
    }
    return ec;
}


static WFDErrorCode
WFD_Pipeline_ValidateAttribi(WFD_PIPELINE* pipeline,
                             WFDPipelineConfigAttrib attrib,
                             WFDint value)
{
    WFDErrorCode            result = WFD_ERROR_NONE;

    OWF_ASSERT(pipeline);

    DPRINT(("WFD_Pipeline_ValidateAttribi(pipeline=%d, attrib=0x%x, value=%d",
            pipeline->config->id, attrib, value));

    switch (attrib)
    {
        case WFD_PIPELINE_FLIP:
        case WFD_PIPELINE_MIRROR:
        {
            DPRINT(("Attribute: WFD_PIPELINE_FLIP or WFD_PIPELINE_MIRROR"));

            /* allowed values: WFD_FALSE & WFD_TRUE */
            if (!(value == WFD_TRUE || value == WFD_FALSE))
            {
                DPRINT(("  Invalid pipeline %s value: %d",
                        WFD_PIPELINE_FLIP == attrib ? "flip" : "mirror",
                        value));
                result = WFD_ERROR_ILLEGAL_ARGUMENT;
            }
            break;
        }

        case WFD_PIPELINE_ROTATION:
        {
            DPRINT(("Attribute: WFD_PIPELINE_ROTATION"));

            if (pipeline->config->rotationSupport != WFD_ROTATION_SUPPORT_NONE)
            {
                DPRINT(("  Pipeline supports rotation"));
                if (pipeline->config->rotationSupport == WFD_ROTATION_SUPPORT_LIMITED &&
                    !(  0 == value ||  90 == value ||
                      180 == value || 270 == value))
                {
                    DPRINT(("  Invalid pipeline rotation value: %d", value));
                    result = WFD_ERROR_ILLEGAL_ARGUMENT;
                }
            }
            else
            {
                /* if rotation is not supported, the only allowed value is zero. */
                if (0 != value)
                {
                    DPRINT(("  Invalid pipeline rotation value: %d", value));
                    result = WFD_ERROR_ILLEGAL_ARGUMENT;
                }
            }
            break;
        }

        case WFD_PIPELINE_SCALE_FILTER:
        {
            DPRINT(("Attribute: WFD_PIPELINE_SCALE_FILTER"));

            if (!(WFD_SCALE_FILTER_NONE     == value ||
                  WFD_SCALE_FILTER_FASTER   == value ||
                  WFD_SCALE_FILTER_BETTER   == value))
            {
                DPRINT(("  Invalid pipeline scaling filter: %d ", value));
                result = WFD_ERROR_ILLEGAL_ARGUMENT;
            }
            break;
        }

        case WFD_PIPELINE_TRANSPARENCY_ENABLE:
        {
            WFDboolean valid = WFD_FALSE;

            DPRINT(("Attribute: WFD_PIPELINE_TRANSPARENCY_ENABLE"));

            valid = WFD_Pipeline_IsTransparencyFeatureSupported(pipeline,
                                                                (WFDbitfield)value);
            if (!valid)
            {
                DPRINT(("  Invalid pipeline transparency mode: %x", value));
                result = WFD_ERROR_ILLEGAL_ARGUMENT;
            }
            break;
        }

        case WFD_PIPELINE_GLOBAL_ALPHA:
        {
            WFDfloat        alpha = value / 255.0f;

            result  = WFD_Pipeline_ValidateAttribf(pipeline, attrib, alpha);
            break;
        }

        default:
        {
            DPRINT(("  Invalid attribute %d", attrib));
            result = WFD_ERROR_BAD_ATTRIBUTE;
            break;
        }

    }
    return result;
}

static WFDErrorCode
WFD_Pipeline_ValidateAttribf(WFD_PIPELINE* pipeline,
                             WFDPipelineConfigAttrib attrib,
                             WFDfloat value)
{
    WFDErrorCode            result = WFD_ERROR_NONE;

    DPRINT(("WFD_Pipeline_ValidateAttribf(pipeline=%d, attrib=0x%x, value=%.2f",
            pipeline->config->id, attrib, value));

    switch (attrib)
    {
        case WFD_PIPELINE_GLOBAL_ALPHA:
        {
            DPRINT(("Attribute: WFD_PIPELINE_GLOBAL_ALPHA"));
            if (value < 0.0f || value > 1.0f)
            {
                DPRINT(("  Invalid pipeline global alpha value: %.2f", value));
                result = WFD_ERROR_ILLEGAL_ARGUMENT;
            }
            break;
        }

        default:
        {
            DPRINT(("  Invalid attribute %d", attrib));
            result = WFD_ERROR_BAD_ATTRIBUTE;
            break;
        }

    }
    return result;
}

static WFDErrorCode
WFD_Pipeline_ValidateAttribiv(WFD_PIPELINE* pipeline,
                              WFDPipelineConfigAttrib attrib,
                              WFDint count,
                              const WFDint* values)
{
    WFDErrorCode    result = WFD_ERROR_NONE;
    WFDint          aLength;

    OWF_ASSERT(pipeline && values);
    OWF_ASSERT(count > 0);

    DPRINT(("WFD_Pipeline_ValidateAttribiv(pipeline=%d, attrib=0x%x, count=%d, "
            "values=%p)",
            pipeline->config->id, attrib, count, values));

    /* check that count is ok for given attrib */
    aLength = OWF_Attribute_GetValueiv(&pipeline->attributes, attrib, 0, NULL);
    if (aLength != count)
    {
        DPRINT(("  Wrong number of vector arguments (%d instead of %d)",
                 count, aLength));
        return WFD_ERROR_ILLEGAL_ARGUMENT;
    }



    switch (attrib)
    {
        case WFD_PIPELINE_DESTINATION_RECTANGLE:
        {
            DPRINT(("Attribute: WFD_PIPELINE_DESTINATION_RECTANGLE"));
            /* value cannot be negative */

            if (!WFD_Util_IsRectValid(values, count))
            {
                DPRINT(("  Rectangle invalid (negative values or overflow"));
                result = WFD_ERROR_ILLEGAL_ARGUMENT;
            }

            /* the actual values will be validated during commit */
            break;
        }

        case WFD_PIPELINE_SOURCE_RECTANGLE:
        {
            WFDint          maxWidth, maxHeight;

            DPRINT(("Attribute: WFD_PIPELINE_SOURCE_RECTANGLE"));

            maxWidth = pipeline->config->maxSourceSize[0];
            maxHeight = pipeline->config->maxSourceSize[1];

            /* 5.7.1.5 WFD_PIPELINE_MAX_SOURCE_SIZE defines the maximum
             * size of the source crop rectangle - it's the only
             * thing we can check here. the rest will be checked upon
             * commit
             */
             if ((maxWidth > 0   && values[RECT_WIDTH] > maxWidth) ||
                 (maxHeight > 0  && values[RECT_HEIGHT] > maxHeight))
             {
                 DPRINT(("  Pipeline source rectangle size (%dx%d) is "
                         "exceeds the maximum size (%dx%d)",
                         values[RECT_WIDTH], values[RECT_HEIGHT], maxWidth, maxHeight));
                 result = WFD_ERROR_ILLEGAL_ARGUMENT;
             }
             else if (!WFD_Util_IsRectValid(values, count))
             {
                 DPRINT(("  Rectangle invalid (negative values or overflow"));
                 result = WFD_ERROR_ILLEGAL_ARGUMENT;
             }
             /* the actual values will be validated during commit */

            break;
        }

        default:
        {
            DPRINT(("  Invalid attribute %d", attrib));
            result = WFD_ERROR_BAD_ATTRIBUTE;
            break;
        }
    }
    return result;
}

static WFDErrorCode
WFD_Pipeline_ValidateAttribfv(WFD_PIPELINE* pipeline,
                              WFDPipelineConfigAttrib attrib,
                              WFDint count,
                              const WFDfloat* values)
{
    WFDErrorCode    result = WFD_ERROR_NONE;
    WFDint          aLength;

    OWF_ASSERT(pipeline && values);
    OWF_ASSERT(count > 0);

    DPRINT(("WFD_Pipeline_ValidateAttribfv(pipeline=%d, attrib=0x%x, count=%d, "
            "values=%p)",
            pipeline->config->id, attrib, count, values));

    /* check that count is ok for given attrib */
    aLength = OWF_Attribute_GetValuefv(&pipeline->attributes, attrib, 0, NULL);
    if (aLength != count)
    {
        DPRINT(("  Wrong number of vector arguments (%d instead of %d)",
                 count, aLength));
        return WFD_ERROR_ILLEGAL_ARGUMENT;
    }

    switch (attrib)
    {
        case WFD_PIPELINE_SOURCE_RECTANGLE:
        case WFD_PIPELINE_DESTINATION_RECTANGLE:
        {
            WFDint      rect[RECT_SIZE];
            WFDint      i;

            DPRINT(("Attribute: WFD_PIPELINE_SOURCE_RECTANGLE or "
                    "WFD_PIPELINE_DESTINATION_RECTANGLE"));
            DPRINT(("  Float rect = {%.2f, %.2f, %.2f, %.2f}",
                    values[RECT_OFFSETX], values[RECT_OFFSETY],
                    values[RECT_WIDTH], values[RECT_HEIGHT]));

            for (i = 0; i < RECT_SIZE; i++)
            {
                 rect[i] = floor(values[i]);
            }

            DPRINT(("  Integer rect = {%d, %d, %d, %d}",
                      rect[RECT_OFFSETX], rect[RECT_OFFSETY],
                      rect[RECT_WIDTH], rect[RECT_HEIGHT]));

           result = WFD_Pipeline_ValidateAttribiv(pipeline, attrib, count, rect);
           break;
        }

        default:
        {
            DPRINT(("  Invalid attribute %d", attrib));
            result = WFD_ERROR_BAD_ATTRIBUTE;
            break;
        }
    }
    return result;
}


OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Pipeline_IsTransparencyFeatureSupported(WFD_PIPELINE* pPipeline,
                                            WFDbitfield feature) OWF_APIEXIT
{

    WFDint       tCount;
    WFDbitfield* tFeatures;

    OWF_ASSERT(pPipeline && pPipeline->config);

    /* no transparency is the default */
    if (feature == WFD_TRANSPARENCY_NONE)
    {
        return WFD_TRUE;
    }

    tCount    =  pPipeline->config->transparencyFeatureCount;
    tFeatures =  pPipeline->config->transparencyFeatures;

    if (tFeatures && tCount  > 0)
    {
        while (tCount--)
        {
            if (tFeatures[tCount] == feature)
            {
               return WFD_TRUE;
            }
        }
    }

    return WFD_FALSE;
}

OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Pipeline_IsTransparencySupported(WFD_PIPELINE* pPipeline,
                                     WFDTransparency trans) OWF_APIEXIT
{
    WFDint       tCount;
    WFDbitfield* tFeatures;

    OWF_ASSERT(pPipeline && pPipeline->config);

    tCount    =  pPipeline->config->transparencyFeatureCount;
    tFeatures =  pPipeline->config->transparencyFeatures;

    if (tFeatures && tCount  > 0)
    {
        while (tCount--)
        {
            if (tFeatures[tCount] & trans)
            {
                return WFD_TRUE;
            }
        }
    }

    return WFD_FALSE;
}

OWF_API_CALL WFDint OWF_APIENTRY
WFD_Pipeline_GetTransparencyFeatures(WFD_PIPELINE* pipeline,
                                     WFDbitfield *trans,
                                     WFDint transCount) OWF_APIEXIT
{
    WFDint count = 0;
    WFDint i;

    OWF_ASSERT(pipeline && pipeline->config);

    if (trans == NULL)
    {
        return pipeline->config->transparencyFeatureCount;
    }

    for (i=0; i < transCount; i++)
    {
        if (i < pipeline->config->transparencyFeatureCount)
        {
            trans[i] = pipeline->config->transparencyFeatures[i];
            count++;
        }
        else
        {
            trans[i] = WFD_TRANSPARENCY_NONE;
        }
    }

    return count;
}

OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_SetTSColor(WFD_PIPELINE* pipeline,
                        WFDTSColorFormat colorFormat,
                        WFDint count,
                        const void *color) OWF_APIEXIT
{
    OWF_ASSERT(pipeline && color);
    OWF_ASSERT(WFD_Util_IsValidTSColor(colorFormat, count, color));
    OWF_ASSERT(WFD_Pipeline_IsTransparencySupported(pipeline,
                          WFD_TRANSPARENCY_SOURCE_COLOR));

    WFD_Util_ConverTSColor(colorFormat, count, color, &pipeline->tsColor);

    DPRINT(("Transparent source color is: r:%f, g:%f, b:%f",
            pipeline->tsColor.color.color.red,
            pipeline->tsColor.color.color.green,
            pipeline->tsColor.color.color.blue));
}


/* ================================================================== */
/*               P I P E L I N E   B I N D I N G S                    */
/* ================================================================== */

static WFDboolean
WFD_Pipeline_SizeIsValid(WFD_PIPELINE* pPipeline,
                         WFDint width,
                         WFDint height)
{
    WFDint maxSize[2];
    WFDErrorCode ec;

    ec = WFD_Pipeline_GetAttribiv(pPipeline,
                                  WFD_PIPELINE_MAX_SOURCE_SIZE,
                                  2, maxSize);
    if (ec != WFD_ERROR_NONE)
    {
        return WFD_FALSE;
    }

    if (width > maxSize[0] || height > maxSize[1])
    {
        return WFD_FALSE;
    }

    return WFD_TRUE;
}

static WFDboolean
WFD_Pipeline_ImageSizeIsValid(WFD_PIPELINE* pPipeline,
                              WFDEGLImage image)
{
    OWF_IMAGE* img = (OWF_IMAGE*)image;

    return WFD_Pipeline_SizeIsValid(pPipeline, img->width, img->height);
}

static WFDboolean
WFD_Pipeline_StreamSizeIsValid(WFD_PIPELINE* pPipeline,
                               WFDNativeStreamType stream)
{
    WFDint width, height;

    owfNativeStreamGetHeader(stream, &width, &height,
                             NULL, NULL, NULL);

    return WFD_Pipeline_SizeIsValid(pPipeline, width, height);
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Pipeline_IsImageValidSource(WFD_PIPELINE* pPipeline,
                                WFDEGLImage image) OWF_APIEXIT
{
    OWF_ASSERT(pPipeline && pPipeline->config);

    if (image == NULL /* || image is not valid EGLImage */)
    {
        return WFD_ERROR_ILLEGAL_ARGUMENT;
    }

    if (!WFD_Pipeline_ImageSizeIsValid(pPipeline, image))
    {
        return WFD_ERROR_NOT_SUPPORTED;
    }

    return WFD_ERROR_NONE;
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Pipeline_IsStreamValidSource(WFD_PIPELINE* pPipeline,
                                 WFDNativeStreamType stream) OWF_APIEXIT
{
    OWF_ASSERT(pPipeline && pPipeline->config);

    if (stream == WFD_INVALID_HANDLE /* || stream is not valid WFDNativeStreamType */)
    {
        return WFD_ERROR_ILLEGAL_ARGUMENT;
    }

    if (!WFD_Pipeline_StreamSizeIsValid(pPipeline, stream))
    {
        return WFD_ERROR_NOT_SUPPORTED;
    }

    /* NOTE: stream busy condition cannot happen with SI native streams */
    return WFD_ERROR_NONE;
}


OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Pipeline_IsImageValidMask(WFD_PIPELINE* pPipeline,
                              WFDEGLImage image) OWF_APIEXIT
{
    OWF_ASSERT(pPipeline && pPipeline->config);

    if (image == NULL /* || image is not valid EGLImage */)
    {
        return WFD_ERROR_ILLEGAL_ARGUMENT;
    }

    if (!WFD_Pipeline_IsTransparencySupported(pPipeline, WFD_TRANSPARENCY_MASK))
    {
        return WFD_ERROR_NOT_SUPPORTED;
    }

    if (!WFD_Pipeline_ImageSizeIsValid(pPipeline, image))
    {
        return WFD_ERROR_NOT_SUPPORTED;
    }

    return WFD_ERROR_NONE;
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Pipeline_IsStreamValidMask(WFD_PIPELINE* pPipeline,
                               WFDNativeStreamType stream) OWF_APIEXIT
{
    OWF_ASSERT(pPipeline && pPipeline->config);

    if (stream == WFD_INVALID_HANDLE /* || stream is not valid WFDNativeStreamType */)
    {
        return WFD_ERROR_ILLEGAL_ARGUMENT;
    }

    if (!WFD_Pipeline_IsTransparencySupported(pPipeline, WFD_TRANSPARENCY_MASK))
    {
        return WFD_ERROR_NOT_SUPPORTED;
    }

    if (!WFD_Pipeline_StreamSizeIsValid(pPipeline, stream))
    {
        return WFD_ERROR_NOT_SUPPORTED;
    }

    /* NOTE: stream busy condition not valid with SI native streams */
    return WFD_ERROR_NONE;
}

OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_SourceCacheBinding(WFD_PIPELINE* pPipeline,
                                WFD_SOURCE* pSource,
                                WFDTransition transition,
                                const WFDRect *region) OWF_APIEXIT
{
    OWF_ASSERT(pPipeline && pPipeline->config && pPipeline->bindings);

    pPipeline->bindings->sourceDirty = WFD_TRUE;

    /* remove uncommitted reference, if any */
    REMREF(pPipeline->bindings->cachedSource);

    ADDREF(pPipeline->bindings->cachedSource, pSource);
    pPipeline->bindings->cachedSrcTransition = transition;

    if (region)
    {
        pPipeline->bindings->cachedRegion.offsetX = region->offsetX;
        pPipeline->bindings->cachedRegion.offsetY = region->offsetY;
        pPipeline->bindings->cachedRegion.height  = region->height;
        pPipeline->bindings->cachedRegion.width   = region->width;
    }
    else
    {
        pPipeline->bindings->cachedRegion.offsetX = 0;
        pPipeline->bindings->cachedRegion.offsetY = 0;
        pPipeline->bindings->cachedRegion.height  = 0;
        pPipeline->bindings->cachedRegion.width   = 0;
    }
}

OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_MaskCacheBinding(WFD_PIPELINE* pPipeline,
                              WFD_MASK* pMask,
                              WFDTransition transition) OWF_APIEXIT
{
    OWF_ASSERT(pPipeline && pPipeline->config && pPipeline->bindings);

    pPipeline->bindings->maskDirty = WFD_TRUE;

    /* remove uncommitted reference, if any */
    REMREF(pPipeline->bindings->cachedMask);

    ADDREF(pPipeline->bindings->cachedMask, pMask);
    pPipeline->bindings->cachedMaskTransition = transition;
}


/* generate an event when image transition complete */
OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_SourceBindComplete(WFD_PIPELINE* pPipeline) OWF_APIEXIT
{
    WFD_EVENT               event;
    WFD_SOURCE*             source;

    OWF_ASSERT(pPipeline && pPipeline->config && pPipeline->bindings);

    source = pPipeline->bindings->boundSource;
    if (pPipeline->bindings->boundSrcTransition != WFD_TRANSITION_INVALID)
    {
        event.type = WFD_EVENT_PIPELINE_BIND_SOURCE_COMPLETE;
        event.data.pipelineBindEvent.pipelineId = pPipeline->config->id;
        event.data.pipelineBindEvent.handle = (source) ? source->handle : WFD_INVALID_HANDLE;
        event.data.pipelineBindEvent.overflow = WFD_FALSE;

        WFD_Event_InsertAll(pPipeline->device, &event);
        pPipeline->bindings->boundSrcTransition = WFD_TRANSITION_INVALID;

        DPRINT(("EVENT: Bind source complete, pipeline %d, source %p",
                ID(pPipeline),
                event.data.pipelineBindEvent.handle));
    }

}

OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_MaskBindComplete(WFD_PIPELINE* pPipeline) OWF_APIEXIT
{
    WFD_EVENT       event;
    WFD_MASK*       mask;

    OWF_ASSERT(pPipeline && pPipeline->config && pPipeline->bindings);

    mask = pPipeline->bindings->boundMask;
    if (pPipeline->bindings->boundMaskTransition != WFD_TRANSITION_INVALID)
    {
        event.type = WFD_EVENT_PIPELINE_BIND_MASK_COMPLETE;
        event.data.pipelineBindEvent.pipelineId = pPipeline->config->id;
        event.data.pipelineBindEvent.handle = (mask) ? mask->handle : WFD_INVALID_HANDLE;
        event.data.pipelineBindEvent.overflow = WFD_FALSE;

        WFD_Event_InsertAll(pPipeline->device, &event);
        pPipeline->bindings->boundMaskTransition = WFD_TRANSITION_INVALID;

        DPRINT(("EVENT: Bind mask complete, pipeline %d, mask %p",
                ID(pPipeline),
                event.data.pipelineBindEvent.handle));
    }

}

OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_SourceRemoveBinding(WFD_PIPELINE* pipeline) OWF_APIEXIT
{
    OWF_ASSERT(pipeline && pipeline->bindings);

    REMREF(pipeline->bindings->boundSource);
    REMREF(pipeline->bindings->cachedSource);
}

OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_MaskRemoveBinding(WFD_PIPELINE* pipeline)
{
    OWF_ASSERT(pipeline && pipeline->bindings);

    REMREF(pipeline->bindings->boundMask);
    REMREF(pipeline->bindings->cachedMask);
}

OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_PortRemoveBinding(WFD_PORT* port,
                               WFD_PIPELINE* pipeline,
                               WFDboolean cached)
{
    WFDint pipelineInd;

    pipelineInd = WFD_Port_PipelineNbr(port, pipeline);

    if (pipelineInd >= 0)
    {
        if (cached)
        {
            REMREF(port->bindings[pipelineInd].cachedPipeline);
        }
        else
        {
            REMREF(port->bindings[pipelineInd].boundPipeline);
        }
    }

    if (cached)
    {
        REMREF(pipeline->bindings->cachedPort);
    }
    else
    {
        REMREF(pipeline->bindings->boundPort);
    }
}

/* ================================================================== */
/*                  P I P E L I N E   C O M M I T                     */
/* ================================================================== */

static WFDboolean
WFD_Pipeline_IsMaskCommitConsistent(WFD_PIPELINE* pipeline)
{
    WFDboolean              consistent = WFD_TRUE;
    WFD_PIPELINE_BINDINGS*  bindings;
    WFDTransparency         transparency;

    OWF_ASSERT(pipeline && pipeline->config && pipeline->bindings);

    transparency =
        OWF_Attribute_GetValuei(&pipeline->attributes, WFD_PIPELINE_TRANSPARENCY_ENABLE);

    bindings = pipeline->bindings;
    if (bindings->maskDirty && bindings->cachedMask != NULL)
    {
        WFDint          width, height;
        WFDint          rect[RECT_SIZE] = {0, 0, 0, 0};

        OWF_Attribute_GetValueiv(&pipeline->attributes,
                                 WFD_PIPELINE_DESTINATION_RECTANGLE,
                                 RECT_SIZE, rect);

        WFD_ImageProvider_GetDimensions(bindings->cachedMask, &width, &height);

        /* mask dimensions must match the destination rectangle dimensions */
        if (width != rect[RECT_WIDTH] || height != rect[RECT_HEIGHT])
        {
            consistent = WFD_FALSE;
            DPRINT(("  INCONSISTENT: mask does not match to dest rectangle. pipeline %d", ID(pipeline)));
        }

        /* if transparency is enabled, mask must be set as well */
        if ((transparency & WFD_TRANSPARENCY_MASK) && !bindings->cachedMask)
        {
            DPRINT(("  INCONSISTENT: no cached mask. pipeline %d", ID(pipeline)));
            consistent = WFD_FALSE;
        }
    }
    else
    {
        /* if transparency is enabled, mask must be set as well */
        if ((transparency & WFD_TRANSPARENCY_MASK) && !bindings->boundMask)
        {
            consistent = WFD_FALSE;
            DPRINT(("  INCONSISTENT: mask is not specified, pipeline %d", ID(pipeline)));
        }
    }

    return consistent;
}

static WFDboolean
WFD_Pipeline_IsSrcRectCommitConsistent(WFD_PIPELINE* pipeline)
{
    WFDboolean              consistent = WFD_TRUE;
    WFD_PIPELINE_BINDINGS*  bindings;

    OWF_ASSERT(pipeline && pipeline->config && pipeline->bindings);

    bindings = pipeline->bindings;
    if (bindings->sourceDirty && bindings->cachedSource != NULL)
    {
        WFDint              width, height; /* image width and height */
        WFDint              rect[RECT_SIZE] = {0, 0, 0, 0};

        OWF_Attribute_GetValueiv(&pipeline->attributes,
                                 WFD_PIPELINE_SOURCE_RECTANGLE,
                                 RECT_SIZE, rect);

        WFD_ImageProvider_GetDimensions(bindings->cachedSource, &width, &height);

        consistent = WFD_Util_RectIsFullyContained(rect, RECT_SIZE, width, height);
    }

    if (!consistent)
    {
        DPRINT(("  pipeline %d source rectangle is not commit consistent", ID(pipeline)));
    }

    return consistent;
}

static WFDboolean
WFD_Pipeline_IsDstRectCommitConsistent(WFD_PIPELINE* pipeline)
{
    WFDboolean              consistent = WFD_TRUE;
    WFD_PIPELINE_BINDINGS*  bindings;
    WFD_PORT*               pPort;

    OWF_ASSERT(pipeline && pipeline->config && pipeline->bindings);

    bindings = pipeline->bindings;
    pPort = (bindings->portDirty) ? bindings->cachedPort : bindings->boundPort;

    if (pPort)
    {
        WFD_PORT_MODE*          pMode;

        if ( (pMode = WFD_Port_GetModePtr(pPort)) )
        {
            WFDint              width, height; /* port width and height */
            WFDint              rect[RECT_SIZE] = {0, 0, 0, 0};

            WFD_PortMode_GetAttribi(pMode, WFD_PORT_MODE_WIDTH, &width);
            WFD_PortMode_GetAttribi(pMode, WFD_PORT_MODE_HEIGHT, &height);

            OWF_Attribute_GetValueiv(&pipeline->attributes,
                                     WFD_PIPELINE_DESTINATION_RECTANGLE,
                                     RECT_SIZE, rect);

            consistent = WFD_Util_RectIsFullyContained(rect, RECT_SIZE, width, height);

            if (!consistent)
            {
                DPRINT(("  pipeline %d destination rectangle is not commit consistent", ID(pipeline)));
                DPRINT(("  [%d, %d, %d, %d]", rect[0], rect[1], rect[2], rect[3]));
            }
        }
     }
    return consistent;
}

static WFDboolean
WFD_Pipeline_IsScaleRangeCommitConsistent(WFD_PIPELINE* pipeline)
{
    WFDfloat            scaleFactor;
    WFDfloat            scaleRange[2] = {1.0, 1.0};
    WFDint              srcRect[RECT_SIZE] = {0, 0, 0, 0};
    WFDint              dstRect[RECT_SIZE] = {0, 0, 0, 0};

    OWF_Attribute_GetValueiv(&pipeline->attributes,
                             WFD_PIPELINE_SOURCE_RECTANGLE,
                             RECT_SIZE, srcRect);
    OWF_Attribute_GetValueiv(&pipeline->attributes,
                             WFD_PIPELINE_DESTINATION_RECTANGLE,
                             RECT_SIZE, dstRect);
    OWF_Attribute_GetValuefv(&pipeline->attributes,
                             WFD_PIPELINE_SCALE_RANGE,
                             2, scaleRange);

    if (dstRect[RECT_WIDTH] * dstRect[RECT_HEIGHT] <= 0 ||
        srcRect[RECT_WIDTH] * srcRect[RECT_HEIGHT] <= 0)
    {
        /* pipeline is disabled */
        return WFD_TRUE;
    }

    /* check scale factor separately for WIDTH and HEIGHT */

    scaleFactor = dstRect[RECT_WIDTH] / (WFDfloat)srcRect[RECT_WIDTH];
    if (scaleFactor < scaleRange[0] || scaleFactor > scaleRange[1])
    {
        DPRINT(( "Scale factor not within range: scaleFactor=%f, min=%f, max=%f",
                  scaleFactor, scaleRange[0], scaleRange[1] ));
        return WFD_FALSE;
    }

    scaleFactor = dstRect[RECT_HEIGHT] / (WFDfloat)srcRect[RECT_HEIGHT];
    if (scaleFactor < scaleRange[0] || scaleFactor > scaleRange[1])
    {
        DPRINT(( "Scale factor not within range: scaleFactor=%f, min=%f, max=%f",
                  scaleFactor, scaleRange[0], scaleRange[1] ));
        return WFD_FALSE;
    }

    return WFD_TRUE;
}

OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Pipeline_IsCommitConsistent(WFD_PIPELINE* pipeline, WFDCommitType type) OWF_APIEXIT
{
    WFDboolean              consistent = WFD_TRUE;

    OWF_ASSERT(pipeline && pipeline->config && pipeline->bindings);

    if (type == WFD_COMMIT_PIPELINE)
    {
        /* Not consistent if the port binding has to change from one port to another */
        if (WFD_TRUE == pipeline->bindings->portDirty)
        {
            if ( pipeline->bindings->cachedPort && pipeline->bindings->boundPort &&
                 (pipeline->bindings->cachedPort != pipeline->bindings->boundPort) )
            {
                consistent = WFD_FALSE;
            }
        }
    }

    consistent = consistent && WFD_Pipeline_IsMaskCommitConsistent(pipeline);
    consistent = consistent && WFD_Pipeline_IsSrcRectCommitConsistent(pipeline);
    consistent = consistent && WFD_Pipeline_IsDstRectCommitConsistent(pipeline);
    consistent = consistent && WFD_Pipeline_IsScaleRangeCommitConsistent(pipeline);

    if (!consistent)
    {
        DPRINT(("  pipeline is not commit consistent %d", ID(pipeline)));
    }

    return consistent;
}

static WFDboolean
WFD_Pipeline_CommitSource(WFD_PIPELINE* pipeline)
{
    WFD_PIPELINE_BINDINGS*  bindings;

    WFD_SOURCE*         newSource;
    WFD_SOURCE*         oldSource;
    WFDRect             newRect;
    WFDRect             oldRect;

    OWF_ASSERT(pipeline && pipeline->config && pipeline->bindings);

    bindings  = pipeline->bindings;

    if (!bindings->sourceDirty)
    {
        return WFD_FALSE;
    }

    newSource = bindings->cachedSource;
    oldSource = bindings->boundSource;
    newRect   = bindings->boundRegion;
    oldRect   = bindings->cachedRegion;

    DPRINT(("Source transition, pipeline %d:", ID(pipeline) ));
    DPRINT(("  old source = 0x%08x, new source = 0x%08x",
            oldSource ? oldSource->handle : WFD_INVALID_HANDLE,
            newSource ? newSource->handle : WFD_INVALID_HANDLE));

    if (NULL != oldSource && WFD_SOURCE_STREAM == oldSource->sourceType)
    {
        owfNativeStreamRemoveObserver(oldSource->source.stream->handle,
                                      WFD_Pipeline_SourceStreamUpdated,
                                      pipeline);
    }
    if (NULL != newSource && WFD_SOURCE_STREAM == newSource->sourceType)
    {
        owfNativeStreamAddObserver(newSource->source.stream->handle,
                                   WFD_Pipeline_SourceStreamUpdated,
                                   pipeline);
        owfNativeStreamEnableUpdateNotifications(
            newSource->source.stream->handle, OWF_TRUE);
    }

    bindings->sourceDirty           = WFD_FALSE;

    REMREF(bindings->boundSource);

    if (newSource)
    {
        ADDREF(bindings->boundSource, newSource);
        REMREF(bindings->cachedSource);
    }
    bindings->boundSrcTransition = bindings->cachedSrcTransition;

    newRect.offsetX = oldRect.offsetX;
    newRect.offsetY = oldRect.offsetY;
    newRect.height  = oldRect.height;
    newRect.width   = oldRect.width;

    return (bindings->boundSrcTransition == WFD_TRANSITION_IMMEDIATE);
}

static WFDboolean
WFD_Pipeline_CommitMask(WFD_PIPELINE* pipeline)
{
    WFD_PIPELINE_BINDINGS*  bindings;

    WFD_MASK*           newMask;
    WFD_MASK*           oldMask;

    OWF_ASSERT(pipeline && pipeline->config && pipeline->bindings);

    bindings  = pipeline->bindings;

    if (!bindings->maskDirty)
    {
        return WFD_FALSE;
    }

    newMask = bindings->cachedMask;
    oldMask = bindings->boundMask;

    DPRINT(("Mask transition:"));
    DPRINT(("  old mask = 0x%08x, new mask = 0x%08x",
            oldMask ? oldMask->handle : WFD_INVALID_HANDLE,
            newMask ? newMask->handle : WFD_INVALID_HANDLE));

    if (NULL != oldMask && WFD_SOURCE_STREAM == oldMask->sourceType)
    {
        owfNativeStreamRemoveObserver(oldMask->source.stream->handle,
                                      WFD_Pipeline_SourceStreamUpdated,
                                      pipeline);
    }
    if (NULL != newMask && WFD_SOURCE_STREAM == newMask->sourceType)
    {
        owfNativeStreamAddObserver(newMask->source.stream->handle,
                                   WFD_Pipeline_SourceStreamUpdated,
                                   pipeline);
        owfNativeStreamEnableUpdateNotifications(
            newMask->source.stream->handle, OWF_TRUE);
    }

    bindings->maskDirty             = WFD_FALSE;

    REMREF(bindings->boundMask);

    if (newMask)
    {
        ADDREF(bindings->boundMask, newMask);
        REMREF(bindings->cachedMask);
    }
    bindings->boundMaskTransition   = bindings->cachedMaskTransition;

    return (bindings->boundMaskTransition == WFD_TRANSITION_IMMEDIATE);
}


OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Pipeline_CommitImageProviders(WFD_PIPELINE* pipeline) OWF_APIEXIT
{
    WFDboolean              immTrans = WFD_FALSE;
    WFD_PIPELINE_BINDINGS*  bindings;

    OWF_ASSERT(pipeline && pipeline->config && pipeline->bindings);
    bindings  = pipeline->bindings;

    if (bindings)
    {
        if (WFD_Pipeline_CommitSource(pipeline))
        {
            immTrans = WFD_TRUE;
        }

        if (WFD_Pipeline_CommitMask(pipeline))
        {
            immTrans =  WFD_TRUE;
        }
    }

    return immTrans;
}


OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Pipeline_Commit(WFD_PIPELINE* pipeline, WFD_PORT* port) OWF_APIEXIT
{
    WFDboolean hasImmT = WFD_FALSE;  /* immediate transition unless we determine otherwise */

    OWF_ASSERT(pipeline && pipeline->config);

    OWF_AttributeList_Commit(&pipeline->attributes,
                             WFD_PIPELINE_ID,
                             WFD_PIPELINE_GLOBAL_ALPHA, COMMIT_ATTR_DIRECT_FROM_WORKING);

    hasImmT = WFD_Pipeline_CommitImageProviders(pipeline);

    if (!port)
    {
        WFD_Port_CommitForSinglePipeline(pipeline, hasImmT);
    }

    return (hasImmT);
}

static void
WFD_Pipeline_SourceStreamUpdated(OWFNativeStreamType stream,
                                 OWFNativeStreamEvent event,
                                 void* data)
{
    WFD_PIPELINE*           pipeline = (WFD_PIPELINE*) data;

    stream = stream;

    if (OWF_STREAM_UPDATED == event)
    {
        if (pipeline->bindings && pipeline->bindings->boundPort)
        {
            OWF_Message_Send(&pipeline->bindings->boundPort->msgQueue,
                             WFD_MESSAGE_SOURCE_UPDATED, 0);
        }
    }
}



/* ================================================================== */
/*                    I M A G E   P I P E L I N E                     */
/* ================================================================== */

#define SWAP_IMG_PTRS(img1, img2) \
{ \
    OWF_IMAGE* tmp; \
    tmp = img2; \
    img2 = img1; \
    img1 = tmp; \
}

OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_Clear(WFD_PIPELINE* pPipeline) OWF_APIEXIT
{
    DPRINT(("WFD_Pipeline_Clear for pipeline %d", pPipeline->config->id));

    OWF_ASSERT(pPipeline);

    pPipeline->frontBuffer = NULL;
}

OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_Execute(WFD_PIPELINE* pPipeline, WFD_SOURCE* pSource) OWF_APIEXIT
{
    OWF_IMAGE*          pImg;
    OWF_RECTANGLE       srcRect, dstRect, tmpRect;
    WFDint              plRotation;
    OWF_FLIP_DIRECTION  flip = 0;
    WFDScaleFilter      scaleFilter = 0;

     DPRINT(("WFD_Pipeline_Execute for pipeline %d", pPipeline->config->id));

     OWF_ASSERT(pPipeline);
     OWF_ASSERT(pSource);

    /* copy pipeline attributes */
    OWF_Rect_Set(&srcRect,
            pPipeline->config->sourceRectangle[RECT_OFFSETX],
            pPipeline->config->sourceRectangle[RECT_OFFSETY],
            pPipeline->config->sourceRectangle[RECT_WIDTH],
            pPipeline->config->sourceRectangle[RECT_HEIGHT]);
    OWF_Rect_Set(&dstRect,
            pPipeline->config->destinationRectangle[RECT_OFFSETX],
            pPipeline->config->destinationRectangle[RECT_OFFSETY],
            pPipeline->config->destinationRectangle[RECT_WIDTH],
            pPipeline->config->destinationRectangle[RECT_HEIGHT]);

    if (pPipeline->config->flip)
    {
        flip |= OWF_FLIP_VERTICALLY;
    }
    if (pPipeline->config->mirror)
    {
        flip |= OWF_FLIP_HORIZONTALLY;
    }

    plRotation = pPipeline->config->rotation;

    /* get image or stream buffer */
    pImg = WFD_ImageProvider_LockForReading(pSource);

    /* Pipeline stages */
    {
        OWF_IMAGE* outImg;
        OWF_IMAGE* inpImg;
        OWFfloat   srcRectFloat[4];

        /* 1. Source conversion */
        inpImg = pImg;
        outImg = pPipeline->scratch[0];

        /* NOTE: Image size is restricted to max source size
         * of the pipeline. It is checked at source creation time.
         * This restriction can be relaxed if cropping were
         * done before source format conversion.
         */
        OWF_Image_SetSize(outImg, inpImg->width, inpImg->height);
        outImg->format.premultiplied = inpImg->format.premultiplied;
        outImg->format.linear        = inpImg->format.linear;
        OWF_Image_SourceFormatConversion(outImg, inpImg);

        /* set-up for buffer pointer swapping */
        inpImg = pPipeline->scratch[1];
        OWF_Image_SetSize(inpImg, srcRect.width, srcRect.height);
        inpImg->format.premultiplied = outImg->format.premultiplied;
        inpImg->format.linear        = outImg->format.linear;

        /* 2. crop */
        if (srcRect.x != 0 || srcRect.y != 0 ||
            srcRect.height != pImg->height ||
            srcRect.width != pImg->width)
        {
            /* crop only if source rect differs from image size */
            SWAP_IMG_PTRS(inpImg, outImg);

            OWF_Rect_Set(&tmpRect, 0, 0, srcRect.width, srcRect.height);
            OWF_Image_Blit(outImg, &tmpRect, inpImg, &srcRect);
        }

        /* 3. flip & mirror */
        if (flip != 0)
        {
            /* flipping & mirrorig is done in-image */
            OWF_Image_Flip(outImg, flip);
        }

        /* 4. rotate */
        if (plRotation)
        {
            OWF_ROTATION rotation = OWF_ROTATION_0;

            SWAP_IMG_PTRS(inpImg, outImg);

            switch (plRotation)
            {
                case   0: rotation = OWF_ROTATION_0;   break;
                case  90: rotation = OWF_ROTATION_90;  break;
                case 180: rotation = OWF_ROTATION_180; break;
                case 270: rotation = OWF_ROTATION_270; break;
                default:  OWF_ASSERT(0);
            }

            OWF_Image_Rotate(outImg, inpImg, rotation);
            if (rotation == OWF_ROTATION_90 || rotation == OWF_ROTATION_270)
            {
                OWF_Image_SwapWidthAndHeight(outImg);
            }
        }

        /* 5. scale & filter, positioning */
        srcRectFloat[0] = 0;
        srcRectFloat[1] = 0;
        srcRectFloat[2] = outImg->width;
        srcRectFloat[3] = outImg->height;        
        if (dstRect.height != srcRectFloat[3] || dstRect.width != srcRectFloat[2])
        {
            OWF_FILTERING    owfFilter;
            WFDboolean sizeOK;

            SWAP_IMG_PTRS(inpImg, outImg);

            switch (scaleFilter)
            {
                case WFD_SCALE_FILTER_BETTER:
                    owfFilter = OWF_FILTER_BILINEAR;
                    break;
                case WFD_SCALE_FILTER_FASTER:
                    /* no faster filtering */
                case WFD_SCALE_FILTER_NONE:
                default:
                    owfFilter = OWF_FILTER_POINT_SAMPLING;
                    break;
            }

            sizeOK = OWF_Image_SetSize(outImg, dstRect.width, dstRect.height);
            OWF_ASSERT(sizeOK);
            OWF_Rect_Set(&tmpRect, 0, 0, dstRect.width, dstRect.height);
            OWF_Image_Stretch(outImg, &tmpRect, inpImg, srcRectFloat, owfFilter);
        }

        /*  At this point pipeline has rendered image to pipeline
         *  output buffer. It still needed to blit to right offset
         *  on the destination area and blended with mask/alpha value.
         *  This is done layer by layer.
         */

        /* swap buffers */
        pPipeline->frontBuffer = outImg;

        /* 6.offset, 7. layer & blend  - left for port */
    }

    /* unlock source */
    WFD_ImageProvider_Unlock(pSource);
}


OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Pipeline_Disabled(WFD_PIPELINE* pipeline) OWF_APIEXIT
{
    WFDboolean disabled;

    if (!pipeline)
    {
        return WFD_TRUE;
    }

    DPRINT(("Pipeline source rect = {%d, %d, %d, %d}",
            pipeline->config->sourceRectangle[0],
            pipeline->config->sourceRectangle[1],
            pipeline->config->sourceRectangle[2],
            pipeline->config->sourceRectangle[3]));

    DPRINT(("Pipeline destination rect = {%d, %d, %d, %d}",
            pipeline->config->destinationRectangle[0],
            pipeline->config->destinationRectangle[1],
            pipeline->config->destinationRectangle[2],
            pipeline->config->destinationRectangle[3]));

    disabled = (pipeline->config->sourceRectangle[RECT_WIDTH] <= 0)       ||
               (pipeline->config->sourceRectangle[RECT_HEIGHT] <= 0 )     ||
               (pipeline->config->destinationRectangle[RECT_WIDTH] <= 0 ) ||
               (pipeline->config->destinationRectangle[RECT_HEIGHT] <= 0 );

    if (disabled)
    {
        DPRINT(("Pipeline disabled %d", pipeline->config->id));
    }

    return disabled;
}

#ifdef __cplusplus
}
#endif
