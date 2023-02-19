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
 *  \file wfdport.c
 *  \brief OpenWF Display SI, port implementation.
 *
 *  Port module has two set of functions. The first set serves as an
 *  executor of port API commands (create, destroy, set/get attributes etc.
 *  The other set are run in two parallel threads: Blender (rendering
 *  thread) and Blitter (screen updater). Threads are created at
 *  port creation time.
 *
 *  Blitter simulates periodic screen refreshing and it feeds VSYNC commands
 *  to Blender's message queue, and also writes ports front buffer contents
 *  to 'display' (X11 window) buffer.
 *
 *  Each time Blender is run, it executes all bound pipelines and blends
 *  pipeline images to port's backbuffer. As last task in a round, it
 *  swaps back and front buffers.
 *
 *  Blender routines may be called synchronously at commit time, if
 *  some pipeline bindings have IMMEDIATE transition attribute set.
 *
 *  Any changes to port attributes or bindings must be protected with
 *  a port lock. Also rendering is done holding the port lock.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "wfdport.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "owfattributes.h"
#include "owfmemory.h"
#include "owfobject.h"
#include "owfscreen.h"
#include "owftypes.h"
#include "wfddebug.h"
#include "wfdevent.h"
#include "wfdhandle.h"
#include "wfdimageprovider.h"
#include "wfdpipeline.h"
#include "wfdstructs.h"
#include "wfdutils.h"

#define WAIT_FOREVER -1

#define ID(x) (x->config->id)
#define PLCOUNT(x) (x->config->pipelineIdCount)
#define BINDABLE_PL_INDEX_2_PL_LAYER(x) (x + 1)

#define BG_SIZE 3 /* background color vector size */

#define ENABLE_SYNCHRONOUS_PIPELINES 0

#if ENABLE_SYNCHRONOUS_PIPELINES
/* set this false when threading is used
 * synchronousPipelines can be enabled for debugging purposes
 */
static WFDboolean synchronousPipelines = WFD_TRUE;
#endif

static WFDboolean doTransition(WFD_MESSAGES cmd, WFDTransition trans);

static void WFD_Port_Render(WFD_PORT *port, WFD_MESSAGES cmd);

static void *WFD_Port_BlitterThread(void *data);

static void *WFD_Port_BlenderThread(void *data);

static WFDErrorCode WFD_Port_ValidateAttribi(WFD_PORT *port,
                                             WFDPortConfigAttrib attrib,
                                             WFDint value);
static WFDErrorCode WFD_Port_ValidateAttribf(WFD_PORT *port,
                                             WFDPortConfigAttrib attrib,
                                             WFDfloat value);
static WFDErrorCode WFD_Port_ValidateAttribiv(WFD_PORT *port,
                                              WFDPortConfigAttrib attrib,
                                              WFDint count,
                                              const WFDint *values);

static WFDErrorCode WFD_Port_ValidateAttribfv(WFD_PORT *port,
                                              WFDPortConfigAttrib attrib,
                                              WFDint count,
                                              const WFDfloat *values);

static void WFD_Port_LayerAndBlend(WFD_PORT *pPort, WFD_PIPELINE *pPipeline,
                                   WFD_MASK *pMask);

static void WFD_Port_PipelineRemoveBinding(WFD_PORT *pPort, WFDint pipelineInd);

static void WFD_Port_PowerOff(WFD_PORT *pPort);

static void WFD_Port_PowerOn(WFD_PORT *pPort);

static WFDboolean WFD_Port_PowerIsOn(WFD_PORT *pPort);

static void WFD_Port_ChangePowerMode(WFD_PORT *port, WFDPowerMode currentMode,
                                     WFDPowerMode newMode);

static void WFD_Port_StartRendering(WFD_PORT *pPort);

static void WFD_Port_StopRendering(WFD_PORT *pPort);

static void WFD_Port_SetFrameBufferBackground(WFD_PORT *port, WFDfloat *color);
static void WFD_Port_DoCommit(WFD_PORT *port);

static void WFD_Port_CommitPortMode(WFD_PORT *port);

static void WFD_Port_CommitPipelineBindings(WFD_PORT *pPort, WFDint i,
                                            WFD_PIPELINE *pPipeline);

static WFDboolean WFD_Port_IsPartialRefeshCommitConsistent(WFD_PORT *port);

static WFDboolean WFD_Port_IsPortModeCommitConsistent(WFD_PORT *port);

static void WFD_Port_ImageFinalize(WFD_PORT *pPort);

static void WFD_Port_RenderInit(WFD_PORT *port);

static void WFD_Port_SetBlendParams(OWF_BLEND_INFO *blend, WFD_PORT *port,
                                    WFD_PIPELINE *pPipeline, OWF_IMAGE *pMask,
                                    OWF_RECTANGLE *dstRect,
                                    OWF_RECTANGLE *srcRect);

static WFDboolean WFD_Port_SetBlendRects(const WFD_PORT *pPort,
                                         const WFD_PIPELINE *pPipeline,
                                         OWF_RECTANGLE *dstRect,
                                         OWF_RECTANGLE *srcRect);

static void WFD_Port_Blit(WFD_PORT *port);

static void WFD_Port_AttachDetach(void *obj, WFDint screenNumber, char event);

/* ================================================================== */
/*       P O R T  A L L O C A T E   A N D   R E L E A S E             */
/* ================================================================== */

void WFD_PORT_Ctor(void *self) { self = self; }

void WFD_PORT_Dtor(void *payload) {
    WFD_PORT *pPort;
    WFD_DEVICE *pDevice = NULL;
    WFD_PORT_CONFIG *prtConfig = NULL;
    WFDint portId;
    WFDint i;

    OWF_ASSERT(payload);

    pPort = (WFD_PORT *)payload;

    OWF_ASSERT(pPort->config);

    portId = pPort->config->id;

    pDevice = pPort->device;
    REMREF(pPort->device);

    if (pPort->handle != WFD_INVALID_HANDLE) {
        WFD_Handle_Delete(pPort->handle);
        pPort->handle = WFD_INVALID_HANDLE;
    }

    OWF_MessageQueue_Destroy(&pPort->msgQueue);

    for (i = 0; i < WFD_PORT_SCRATCH_COUNT; i++) {
        OWF_Image_Destroy(pPort->scratch[i]);
    }

    OWF_Image_Destroy(pPort->surface[0]);
    OWF_Image_Destroy(pPort->surface[1]);

    OWF_Mutex_Destroy(&pPort->frMutex);
    pPort->frMutex = NULL;

    OWF_Cond_Destroy(&pPort->busyCond);
    pPort->busyCond = NULL;

    OWF_Mutex_Destroy(&pPort->portMutex);
    pPort->portMutex = NULL;

    xfree(pPort->bindings);
    pPort->bindings = NULL;

    OWF_AttributeList_Destroy(&pPort->attributes);

    xfree(pPort->config);
    pPort->config = NULL;

    /* locate static config area and mark port free */
    prtConfig = WFD_Port_FindById(pDevice, portId);
    if (prtConfig) {
        prtConfig->inUse = NULL;
    }

    /* Port is freed by REMREF macro */
}

OWF_API_CALL WFDboolean OWF_APIENTRY WFD_Port_InitAttributes(WFD_PORT *pPort)
    OWF_APIEXIT {
    WFDint ec;
    WFD_PORT_CONFIG *prtConfig;

    OWF_ASSERT(pPort && pPort->config);

    prtConfig = pPort->config;

    /* initialize attributes interface */
    DPRINT(("  Creating port attribute list"));

    OWF_AttributeList_Create(&pPort->attributes, WFD_PORT_ID,
                             WFD_PORT_PROTECTION_ENABLE);

    ec = OWF_AttributeList_GetError(&pPort->attributes);
    if (ec != ATTR_ERROR_NONE) {
        DPRINT(("Error at port attribute list creation (%d)", ec));
        return WFD_FALSE;
    }

    OWF_Attribute_Initi(&pPort->attributes, WFD_PORT_ID,
                        (OWFint *)&prtConfig->id, OWF_TRUE);

    OWF_Attribute_Initi(&pPort->attributes, WFD_PORT_TYPE,
                        (OWFint *)&prtConfig->type, OWF_TRUE);

    OWF_Attribute_Initb(&pPort->attributes, WFD_PORT_DETACHABLE,
                        (OWFboolean *)&prtConfig->detachable, OWF_TRUE);

    OWF_Attribute_Initb(&pPort->attributes, WFD_PORT_ATTACHED,
                        (OWFboolean *)&prtConfig->attached, OWF_TRUE);

    OWF_Attribute_Initiv(&pPort->attributes, WFD_PORT_NATIVE_RESOLUTION, 2,
                         (OWFint *)prtConfig->nativeResolution, OWF_TRUE);

    OWF_Attribute_Initfv(&pPort->attributes, WFD_PORT_PHYSICAL_SIZE, 2,
                         (OWFfloat *)prtConfig->physicalSize, OWF_TRUE);

    OWF_Attribute_Initb(&pPort->attributes, WFD_PORT_FILL_PORT_AREA,
                        (OWFboolean *)&prtConfig->fillPortArea, OWF_TRUE);

    OWF_Attribute_Initfv(&pPort->attributes, WFD_PORT_BACKGROUND_COLOR, BG_SIZE,
                         prtConfig->backgroundColor, OWF_FALSE);

    OWF_Attribute_Initb(&pPort->attributes, WFD_PORT_FLIP,
                        (OWFboolean *)&prtConfig->flip, OWF_FALSE);

    OWF_Attribute_Initb(&pPort->attributes, WFD_PORT_MIRROR,
                        (OWFboolean *)&prtConfig->mirror, OWF_FALSE);

    OWF_Attribute_Initi(&pPort->attributes, WFD_PORT_ROTATION,
                        (OWFint *)&prtConfig->rotation, OWF_FALSE);

    OWF_Attribute_Initi(&pPort->attributes, WFD_PORT_POWER_MODE,
                        (OWFint *)&prtConfig->powerMode, OWF_FALSE);

    OWF_Attribute_Initfv(&pPort->attributes, WFD_PORT_GAMMA_RANGE, 2,
                         prtConfig->gammaRange, OWF_TRUE);

    OWF_Attribute_Initf(&pPort->attributes, WFD_PORT_GAMMA,
                        (OWFfloat *)&prtConfig->gamma, OWF_FALSE);

    OWF_Attribute_Initi(&pPort->attributes, WFD_PORT_PARTIAL_REFRESH_SUPPORT,
                        (OWFint *)&prtConfig->partialRefreshSupport, OWF_TRUE);

    OWF_Attribute_Initiv(&pPort->attributes, WFD_PORT_PARTIAL_REFRESH_MAXIMUM,
                         2, prtConfig->partialRefreshMaximum, OWF_TRUE);

    OWF_Attribute_Initi(&pPort->attributes, WFD_PORT_PARTIAL_REFRESH_ENABLE,
                        (OWFint *)&prtConfig->partialRefreshEnable, OWF_FALSE);

    OWF_Attribute_Initiv(
        &pPort->attributes, WFD_PORT_PARTIAL_REFRESH_RECTANGLE, RECT_SIZE,
        (OWFint *)&prtConfig->partialRefreshRectangle, OWF_FALSE);

    OWF_Attribute_Initi(&pPort->attributes, WFD_PORT_PIPELINE_ID_COUNT,
                        (OWFint *)&prtConfig->pipelineIdCount, OWF_FALSE);

    OWF_Attribute_Initiv(&pPort->attributes, WFD_PORT_BINDABLE_PIPELINE_IDS,
                         prtConfig->pipelineIdCount, prtConfig->pipelineIds,
                         OWF_TRUE);

    OWF_Attribute_Initb(&pPort->attributes, WFD_PORT_PROTECTION_ENABLE,
                        (OWFboolean *)&prtConfig->protectionEnable, OWF_FALSE);

    ec = OWF_AttributeList_GetError(&pPort->attributes);

    if (ec != ATTR_ERROR_NONE) {
        DPRINT(("Error at port attribute list initialization (%d)", ec));
        return WFD_FALSE;
    }

    return WFD_TRUE;
}

OWF_API_CALL void OWF_APIENTRY WFD_Port_AcquireLock(WFD_PORT *port)
    OWF_APIEXIT {
    OWF_Mutex_Lock(&port->portMutex);

    while (port->portBusy) {
        OWF_Cond_Wait(port->busyCond, (OWFtime)OWF_FOREVER);
    }

    port->portBusy = WFD_TRUE;

    DPRINT(("Port %d locked", port->config->id));

    OWF_Mutex_Unlock(&port->portMutex);
}

OWF_API_CALL void OWF_APIENTRY WFD_Port_ReleaseLock(WFD_PORT *port)
    OWF_APIEXIT {
    OWF_Mutex_Lock(&port->portMutex);
    port->portBusy = WFD_FALSE;
    OWF_Cond_Signal(port->busyCond);
    DPRINT(("Port %d released", port->config->id));
    OWF_Mutex_Unlock(&port->portMutex);
}

static WFDboolean WFD_Port_InitScratchBuffers(WFD_PORT *pPort) {
    WFDboolean ret = WFD_TRUE;
    WFDint i, max, w, h;
    WFD_PORT_MODE *modes;

    OWF_ASSERT(pPort);
    OWF_ASSERT(pPort->config);

    if (pPort->config->modeCount > 0) {
        OWF_ASSERT(pPort->config->modes != NULL);

        /* find out maximum pixel count for port  */
        modes = pPort->config->modes;

        for (max = 0, i = 1; i < pPort->config->modeCount; i++) {
            if (modes[i].width * modes[i].height >
                modes[max].width * modes[max].height) {
                max = i;
            }
        }

        w = modes[max].width;
        h = modes[max].height;

        ret = WFD_Util_InitScratchBuffer(pPort->scratch, WFD_PORT_SCRATCH_COUNT,
                                         w, h);
    }

    return ret;
}

static WFDboolean WFD_Port_InitFrameBuffers(WFD_PORT *pPort) {
    WFDboolean ret = WFD_TRUE;
    WFDint i, w, h;
    OWF_IMAGE_FORMAT format;
    OWF_IMAGE *scratch;

    /* precondition: scratch buffers have been initialized */

    OWF_ASSERT(pPort && pPort->scratch[0]);

    scratch = pPort->scratch[0];

    if (scratch->width > 0 && scratch->height > 0) {
        /* use same width and height as scratch buffers */
        w = scratch->width;
        h = scratch->height;

        /* allocate native format buffer for final image */
        format.pixelFormat = OWF_IMAGE_XRGB8888;
        format.linear = OWF_FALSE;
        format.premultiplied = OWF_FALSE;
        format.rowPadding = OWF_Image_GetFormatPadding(OWF_IMAGE_XRGB8888);

        for (i = 0; i < 2 /* double buffer - always two */; i++) {
            pPort->surface[i] = OWF_Image_Create(w, h, &format, NULL, 0);

            ret = ret && pPort->surface[i];
        }
    }

    if (!ret) {
        /* clean up if creation failed */
        for (i = 0; i < 2 /* double buffer - always two */; i++) {
            if (pPort->surface[i]) {
                OWF_Image_Destroy(pPort->surface[i]);
            }
        }
    }

    return ret;
}

static WFDboolean WFD_Port_InitBindings(WFD_PORT *pPort) {
    WFDboolean ret = WFD_FALSE;
    WFDint i;
    WFD_PORT_BINDING *bndgs;

    OWF_ASSERT(pPort);
    OWF_ASSERT(pPort->config);

    if (PLCOUNT(pPort) <= 0) {
        pPort->bindings = NULL;
        return WFD_TRUE;
    }

    bndgs = NEW0N(WFD_PORT_BINDING, PLCOUNT(pPort));

    if (bndgs) {
        for (i = 0; i < PLCOUNT(pPort); i++) {
            bndgs[i].cachedPipeline = NULL;
            bndgs[i].boundPipeline = NULL;
        }

        pPort->bindings = bndgs;
        ret = WFD_TRUE;
    }

    return ret;
}

static void WFD_Port_Preconfiguration(WFD_PORT *pPort) {
    /* if preconfigured mode exist
     * - set port mode and possible power mode
     * else
     * - clear preconfigured power mode
     */

    if (pPort->config->preconfiguredMode != WFD_INVALID_HANDLE) {
        pPort->currentMode =
            WFD_Port_FindMode(pPort, pPort->config->preconfiguredMode);

        if (pPort->currentMode &&
            pPort->config->powerMode == WFD_POWER_MODE_ON) {
            WFD_Port_PowerOn(pPort);
        }
    } else {
        pPort->config->powerMode = WFD_POWER_MODE_OFF;
    }
}

OWF_API_CALL WFDPort OWF_APIENTRY WFD_Port_Allocate(WFD_DEVICE *device,
                                                    WFDint portId) OWF_APIEXIT {
    WFD_PORT_CONFIG *prtConfig;
    WFD_PORT *pPort;
    WFDboolean ok = WFD_FALSE;
    WFDPort handle = WFD_INVALID_HANDLE;

    /* locate static config area */
    prtConfig = WFD_Port_FindById(device, portId);
    if (!prtConfig) {
        /* port does not exist */
        return WFD_INVALID_HANDLE;
    }

    pPort = CREATE(WFD_PORT);
    if (pPort) {
        printf("111\n");
        ADDREF(pPort->device, device);
        printf("222\n");
        OWF_Array_AppendItem(&device->ports, pPort);

        /* mark the port allocated */
        prtConfig->inUse = pPort;

        /* make copy of the static config are. this holds
         * committed port attributes during port's life-time
         */
        pPort->config = NEW0(WFD_PORT_CONFIG);
        ok = (pPort->config != NULL);

        if (ok) {
            memcpy(pPort->config, prtConfig, sizeof(WFD_PORT_CONFIG));
        }

        ok = WFD_Port_InitAttributes(pPort);
        if (ok) {
            ok = WFD_Port_InitScratchBuffers(pPort);
        }

        if (ok) {
            ok = WFD_Port_InitFrameBuffers(pPort);
        }

        /* this mutex is used for frame buffer updates */
        if (ok) {
            ok = (OWF_Mutex_Init(&pPort->frMutex) == 0);
        }

        /* busy flag tells that port busy doing commit or
         * rendering. both are not allowed at the same time */
        if (ok) {
            ok = (OWF_Mutex_Init(&pPort->portMutex) == 0);
        }

        if (ok) {
            ok = (OWF_Cond_Init(&pPort->busyCond, pPort->portMutex) == 0);
            pPort->portBusy = WFD_FALSE;
        }

        /* initialize bindings structure - in bindings array
         * there is a item per bindable pipeline. Items are
         * in the layer order, bottom layer first
         */
        if (ok) {
            ok = WFD_Port_InitBindings(pPort);
        }

        /* launch port threads - rendering and vsync thread */
#if ENABLE_SYNCHRONOUS_PIPELINES
        /* synchronous pipelines can be enabled for debugging purposes */
        if (ok && !synchronousPipelines)
#else
        if (ok)
#endif
        {
            /* set-up message queue */
            ok = (OWF_MessageQueue_Init(&pPort->msgQueue) == 0);

            /* rendering and blitting threads are launched
             * when port power is turned on
             */
        }

        if (ok) {
            pPort->handle = WFD_Handle_Create(WFD_PORT_HANDLE, pPort);
            handle = pPort->handle;
        }

        ok = ok && (handle != WFD_INVALID_HANDLE);
    }

    if (!ok && pPort) /* error exit, clean-up resources */
    {
        WFD_Handle_Delete(pPort->handle);
        pPort->handle = WFD_INVALID_HANDLE;
        OWF_Array_RemoveItem(&device->ports, pPort);
        DESTROY(pPort);
    }

    else {
        WFD_Port_Preconfiguration(pPort);
        WFD_Port_StartRendering(pPort);

        DPRINT(("WFD_Port_Allocate: port %d, object = %p (handle = 0x%08x)",
                pPort->config->id, pPort, handle));
    }

    OWF_AttributeList_Commit(&pPort->attributes, WFD_PORT_ID,
                             WFD_PORT_PROTECTION_ENABLE,
                             WORKING_ATTR_VALUE_INDEX);

    return handle;
}

OWF_API_CALL void OWF_APIENTRY WFD_Port_Release(WFD_DEVICE *device,
                                                WFD_PORT *pPort) OWF_APIEXIT {
    WFDint i;

    DPRINT(("WFD_Port_Release, port %d", ID(pPort)));

    WFD_Port_StopRendering(pPort);

    WFD_Handle_Delete(pPort->handle);
    pPort->handle = WFD_INVALID_HANDLE;

    /* no need to lock port because rendering has stopped */
    for (i = 0; i < PLCOUNT(pPort); i++) {
        WFD_Port_PipelineRemoveBinding(pPort, i);
    }

    /* remove device-port connection */
    OWF_Array_RemoveItem(&device->ports, pPort);

    DESTROY(pPort);
}

static void WFD_Port_StartRendering(WFD_PORT *pPort) {
    OWF_ASSERT(pPort && pPort->config);

    /* create a window for port */
    if (pPort->screenNumber == OWF_INVALID_SCREEN_NUMBER) {
        WFDint w, h;
        WFDfloat black[3] = {0.0, 0.0, 0.0};

        /* get maximum port mode width */
        if (pPort->config->modeCount > 0) {
            WFDint max, i;
            WFD_PORT_MODE *modes;

            OWF_ASSERT(pPort->config->modes != NULL);

            /* find out maximum width for  port  */
            modes = pPort->config->modes;

            for (max = 0, i = 1; i < pPort->config->modeCount; i++) {
                if (modes[i].width > modes[max].width) {
                    max = i;
                }
            }

            w = modes[max].width;
            h = modes[max].height;
        } else {
            w = pPort->config->nativeResolution[0];
            h = pPort->config->nativeResolution[1];
        }

        pPort->screenNumber =
            OWF_Screen_Create(w, h, WFD_Port_AttachDetach, (void *)pPort);

        WFD_Port_SetFrameBufferBackground(pPort, black);

        if (pPort->currentMode) {
            OWF_Screen_Resize(pPort->screenNumber, pPort->currentMode->width,
                              pPort->currentMode->height);
        }
    }

    if (!pPort->blender) {
        /* empty message queue first */
        while (OWF_MessageQueue_Empty(&pPort->msgQueue)) {
            OWF_MESSAGE msg;
            OWF_Message_Wait(&pPort->msgQueue, &msg, 0);
        }

        /* launch port threads */
        DPRINT(("WFD_Port_BlenderThread launch for port %d", ID(pPort)));
        pPort->blender = OWF_Thread_Create(WFD_Port_BlenderThread, pPort);
    }

    if (!pPort->blitter) {
        /* launch blitting thread */
        DPRINT(("WFD_Port_BlitterThread launch for port %d", ID(pPort)));
        pPort->blitter = OWF_Thread_Create(WFD_Port_BlitterThread, pPort);
    }
}

static void WFD_Port_StopRendering(WFD_PORT *pPort) {
    if (pPort->blitter) {
        pPort->destroyPending = WFD_TRUE;
        DPRINT(("WFD_Port_BlitterThread cancel, port %d", ID(pPort)));
        OWF_Thread_Destroy(pPort->blitter);
        DPRINT(("    blitter dead, port %d", ID(pPort)));
        pPort->blitter = NULL;
    }

    if (pPort->blender) {
        DPRINT(("WFD_Port_BlenderThread cancel, port %d", ID(pPort)));
        OWF_Thread_Destroy(pPort->blender);
        DPRINT(("    blender dead, port %d", ID(pPort)));
        pPort->blender = NULL;
    }

    /* destroy port screen */
    if (pPort->screenNumber != OWF_INVALID_SCREEN_NUMBER) {
        OWF_Screen_Destroy(pPort->screenNumber);
        pPort->screenNumber = OWF_INVALID_SCREEN_NUMBER;
    }
}
/* ================================================================== */
/*               P O R T  A T T A C H / D E T A C H                   */
/* ================================================================== */

static void WFD_Port_SendAttachDetachEvent(WFD_PORT *pPort,
                                           WFDboolean attached) {
    WFD_EVENT event;

    OWF_ASSERT(pPort && pPort->config);

    event.type = WFD_EVENT_PORT_ATTACH_DETACH;
    event.data.portAttachEvent.portId = pPort->config->id;
    event.data.portAttachEvent.attached = attached;

    WFD_Event_InsertAll(pPort->device, &event);
}

OWF_API_CALL WFDboolean OWF_APIENTRY WFD_Port_Attach(WFD_PORT *pPort)
    OWF_APIEXIT {
    OWF_ASSERT(pPort && pPort->config);

    DPRINT(("WFD_Port_Attach(%p)", pPort));

    WFD_Port_AcquireLock(pPort);

    if (!pPort->config->detachable) {
        WFD_Port_ReleaseLock(pPort);
        return WFD_FALSE;
    }

    if (pPort->config->attached) {
        WFD_Port_ReleaseLock(pPort);
        return WFD_FALSE;
    }

    pPort->config->attached = WFD_TRUE;

    WFD_Port_ReleaseLock(pPort);

    /* determine available port modes and
     * update available display formats */

    /* SI: these values are available in configuration data */

    /* generate port attach event */

    WFD_Port_SendAttachDetachEvent(pPort, WFD_TRUE);

    DPRINT(("   port is attached (%p)", pPort));

    return WFD_TRUE;
}

OWF_API_CALL WFDboolean OWF_APIENTRY WFD_Port_Detach(WFD_PORT *pPort)
    OWF_APIEXIT {
    WFDfloat black[3] = {0.0, 0.0, 0.0};

    DPRINT(("WFD_Port_Detach(%p)", pPort));

    OWF_ASSERT(pPort && pPort->config);

    WFD_Port_AcquireLock(pPort);

    if (!pPort->config->detachable) {
        WFD_Port_ReleaseLock(pPort);
        return WFD_FALSE;
    }

    if (!pPort->config->attached) {
        WFD_Port_ReleaseLock(pPort);
        return WFD_FALSE;
    }

    pPort->config->attached = WFD_FALSE;

    /* revert port mode being unset, clear cached settings  */
    pPort->currentMode = NULL;
    pPort->modeDirty = WFD_FALSE;
    pPort->cachedMode = NULL;

    WFD_Port_SetFrameBufferBackground(pPort, black);

    WFD_Port_ReleaseLock(pPort);

    /* set available port modes and display data formats to zero */
    /* SI: this is handled by returning zero when corresponding
     * mode or format is queried or set. */

    /*  generate port detach event */
    WFD_Port_SendAttachDetachEvent(pPort, WFD_FALSE);

    DPRINT(("   port is detached (%p)", pPort));

    return WFD_TRUE;
}

/* ================================================================== */
/*                   P O R T  L O O K U P                             */
/* ================================================================== */

OWF_API_CALL WFDint OWF_APIENTRY WFD_Port_GetIds(
    WFD_DEVICE *device, WFDint *idsList, WFDint listCapacity) OWF_APIEXIT {
    WFDint count = 0;
    WFDint i;
    WFD_DEVICE_CONFIG *devConfig;

    DPRINT(("WFD_Port_GetIds(%p,%p,%d)", device, idsList, listCapacity));

    OWF_ASSERT(device && device->config);

    devConfig = device->config;

    if (!idsList) {
        return devConfig->portCount;
    }

    DPRINT(("  port count = %d", devConfig->portCount));

    for (i = 0; i < devConfig->portCount && count < listCapacity; i++) {
        DPRINT(("  port %d, id = %d", i, devConfig->ports[i].id));
        if (devConfig->ports[i].id != WFD_INVALID_PORT_ID) {
            idsList[count] = devConfig->ports[i].id;
            ++count;
        }
    }

    for (i = count; i < listCapacity; i++) {
        idsList[i] = WFD_INVALID_PORT_ID;
    }

    return count;
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Port_IsAllocated(WFD_DEVICE *device, WFDint id) OWF_APIEXIT {
    WFDint i;
    WFD_DEVICE_CONFIG *devConfig;
    WFD_PORT_CONFIG *portConfig = NULL;

    OWF_ASSERT(device && device->config);

    devConfig = device->config;

    for (i = 0; i < devConfig->portCount; i++) {
        portConfig = &devConfig->ports[i];
        if (portConfig->id == id) {
            return (portConfig->inUse == NULL) ? WFD_ERROR_NONE
                                               : WFD_ERROR_IN_USE;
        }
    }

    return WFD_ERROR_ILLEGAL_ARGUMENT;
}

OWF_API_CALL WFD_PORT_CONFIG *OWF_APIENTRY
WFD_Port_FindById(WFD_DEVICE *device, WFDint id) OWF_APIEXIT {
    WFDint i;
    WFD_DEVICE_CONFIG *devConfig;

    OWF_ASSERT(device && device->config);

    devConfig = device->config;

    for (i = 0; i < devConfig->portCount; i++) {
        if (devConfig->ports[i].id == id) {
            return &devConfig->ports[i];
        }
    }
    return NULL;
}

OWF_API_CALL WFD_PORT *OWF_APIENTRY
WFD_Port_FindByHandle(WFD_DEVICE *device, WFDPort handle) OWF_APIEXIT {
    WFD_PORT *pPort;

    pPort = (WFD_PORT *)WFD_Handle_GetObj(handle, WFD_PORT_HANDLE);

    /* port must be associated with device */
    return (pPort && pPort->device == device) ? pPort : NULL;
}

OWF_API_CALL WFDint OWF_APIENTRY
WFD_Port_GetMaxRefreshRate(WFD_PORT_CONFIG *prtConfig) OWF_APIEXIT {
    WFDint i;
    WFDint maxRefresh = 0;

    OWF_ASSERT(prtConfig);

    if (prtConfig && prtConfig->modes) {
        WFD_PORT_MODE *modes = prtConfig->modes;

        for (i = 0; i < prtConfig->modeCount; i++) {
            maxRefresh = MAX(maxRefresh, (WFDint)ceil(modes[i].refreshRate));
        }
    }

    return maxRefresh;
}

/* ================================================================== */
/*                     P O R T  M O D E S                             */
/* ================================================================== */

OWF_API_CALL WFD_PORT_MODE *OWF_APIENTRY
WFD_Port_FindMode(WFD_PORT *port, WFDPortMode mode) OWF_APIEXIT {
    WFDint ii;

    OWF_ASSERT(port && port->config);

    for (ii = 0; ii < port->config->modeCount; ii++) {
        if (port->config->modes[ii].id == mode) {
            return &port->config->modes[ii];
        }
    }
    return NULL;
}

OWF_API_CALL WFDint OWF_APIENTRY WFD_Port_GetModes(
    WFD_PORT *port, WFDPortMode *modes, WFDint modesCount) OWF_APIEXIT {
    WFDint count;

    OWF_ASSERT(port && port->config);

    if (!port->config->attached) {
        return 0;
    }

    if (!modes) {
        count = port->config->modeCount;
    } else {
        WFDint i;

        OWF_ASSERT(modesCount > 0);
        OWF_ASSERT(port->config);
        OWF_ASSERT(port->config->modes);

        for (count = 0; count < port->config->modeCount; count++) {
            if (count > modesCount - 1) break;

            modes[count] = port->config->modes[count].id;
        }

        for (i = count; i < modesCount; i++) {
            modes[i] = WFD_INVALID_HANDLE;
        }
    }

    return count;
}

OWF_API_CALL WFD_PORT_MODE *OWF_APIENTRY WFD_Port_GetModePtr(WFD_PORT *port)
    OWF_APIEXIT {
    OWF_ASSERT(port);

    if (port->modeDirty) {
        return port->cachedMode;
    } else {
        return port->currentMode;
    }
}

OWF_API_CALL WFDPortMode OWF_APIENTRY WFD_Port_GetCurrentMode(WFD_PORT *port)
    OWF_APIEXIT {
    WFD_PORT_MODE *currentMode;
    WFDint i;

    OWF_ASSERT(port && port->config);

    currentMode = WFD_Port_GetModePtr(port);

    for (i = 0; i < port->config->modeCount; i++) {
        if (currentMode == &port->config->modes[i]) {
            return port->config->modes[i].id;
        }
    }

    return WFD_INVALID_HANDLE;
}

OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Port_SetMode(WFD_PORT *pPort, WFDPortMode mode) OWF_APIEXIT {
    WFD_PORT_MODE *pPortMode;

    OWF_ASSERT(pPort);

    pPortMode = WFD_Port_FindMode(pPort, mode);
    if (pPortMode) {
        pPort->cachedMode = pPortMode;
        pPort->modeDirty = WFD_TRUE;
        return WFD_TRUE;
    }

    return WFD_FALSE;
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_PortMode_GetAttribf(WFD_PORT_MODE *pPortMode, WFDPortModeAttrib attrib,
                        WFDfloat *attrValue) OWF_APIEXIT {
    if (attrib == WFD_PORT_MODE_REFRESH_RATE) {
        *attrValue = pPortMode->refreshRate;
        return WFD_ERROR_NONE;
    }

    return WFD_ERROR_BAD_ATTRIBUTE;
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_PortMode_GetAttribi(WFD_PORT_MODE *pPortMode, WFDPortModeAttrib attrib,
                        WFDint *attrValue) OWF_APIEXIT {
    WFDint value = 0;

    OWF_ASSERT(pPortMode);

    /* these are all read-only attributes - no attribute list needed */
    switch (attrib) {
        case WFD_PORT_MODE_WIDTH:
            value = pPortMode->width;
            break;

        case WFD_PORT_MODE_HEIGHT:
            value = pPortMode->height;
            break;

        case WFD_PORT_MODE_REFRESH_RATE:
            value = floor(pPortMode->refreshRate);
            break;

        case WFD_PORT_MODE_FLIP_MIRROR_SUPPORT:
            value = pPortMode->flipMirrorSupport;
            break;

        case WFD_PORT_MODE_ROTATION_SUPPORT:
            value = pPortMode->rotationSupport;
            break;

        case WFD_PORT_MODE_INTERLACED:
            value = pPortMode->interlaced;
            break;

        default:
            *attrValue = 0;
            return WFD_ERROR_BAD_ATTRIBUTE;
    }

    *attrValue = value;

    return WFD_ERROR_NONE;
}

/* ================================================================== */
/*                 P O R T  A T T R I B U T E S                       */
/* ================================================================== */

OWF_API_CALL WFDErrorCode OWF_APIENTRY WFD_Port_GetAttribi(
    WFD_PORT *port, WFDPortConfigAttrib attrib, WFDint *value) OWF_APIEXIT {
    OWF_ATTRIBUTE_LIST_STATUS ec;

    OWF_ASSERT(port && value);

    if (attrib == WFD_PORT_BACKGROUND_COLOR) {
        WFDfloat bg[BG_SIZE];
        WFDint temp;

        temp = OWF_Attribute_GetValuefv(&port->attributes, attrib, BG_SIZE, bg);
        if (temp != BG_SIZE) {
            return WFD_ERROR_ILLEGAL_ARGUMENT;
        }
        ec = OWF_AttributeList_GetError(&port->attributes);

        if (ec == ATTR_ERROR_NONE) {
            *value = WFD_Util_BgFv2Int(BG_SIZE, bg);
        }
    } else {
        *value = OWF_Attribute_GetValuei(&port->attributes, attrib);
        ec = OWF_AttributeList_GetError(&port->attributes);
    }

    return WFD_Util_AttrEc2WfdEc(ec);
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY WFD_Port_GetAttribf(
    WFD_PORT *port, WFDPortConfigAttrib attrib, WFDfloat *value) OWF_APIEXIT {
    OWF_ASSERT(port && value);

    *value = OWF_Attribute_GetValuef(&port->attributes, attrib);
    return WFD_Util_AttrEc2WfdEc(OWF_AttributeList_GetError(&port->attributes));
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Port_GetAttribiv(WFD_PORT *port, WFDPortConfigAttrib attrib, WFDint count,
                     WFDint *value) OWF_APIEXIT {
    WFDint temp;
    OWF_ATTRIBUTE_LIST_STATUS ec;
    WFDint aLength;

    OWF_ASSERT(port && value);
    OWF_ASSERT(count > 0);

    /* check that count is ok for given attrib */
    aLength = OWF_Attribute_GetValueiv(&port->attributes, attrib, 0, NULL);
    if (attrib != WFD_PORT_BINDABLE_PIPELINE_IDS) {
        if (aLength != count) {
            return WFD_ERROR_ILLEGAL_ARGUMENT;
        }
    } else {
        /* pipelines ids may be queried less than element count */
        if (count > aLength) {
            return WFD_ERROR_ILLEGAL_ARGUMENT;
        }
    }

    if (attrib == WFD_PORT_BACKGROUND_COLOR) {
        WFDfloat bg[BG_SIZE];

        temp = OWF_Attribute_GetValuefv(&port->attributes, attrib, count, bg);
        if (temp != count) {
            return WFD_ERROR_ILLEGAL_ARGUMENT;
        }

        ec = OWF_AttributeList_GetError(&port->attributes);

        if (ec == ATTR_ERROR_NONE) {
            WFD_Util_BgFv2Iv(count, bg, value);
        }
    } else {
        temp =
            OWF_Attribute_GetValueiv(&port->attributes, attrib, count, value);
        if (temp != count) {
            return WFD_ERROR_ILLEGAL_ARGUMENT;
        }
        ec = OWF_AttributeList_GetError(&port->attributes);
    }

    return WFD_Util_AttrEc2WfdEc(ec);
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Port_GetAttribfv(WFD_PORT *port, WFDPortConfigAttrib attrib, WFDint count,
                     WFDfloat *value) OWF_APIEXIT {
    WFDint temp;
    WFDint aLength;

    OWF_ASSERT(port && value);
    OWF_ASSERT(count > 0);

    /* check that count is ok for given attrib */
    aLength = OWF_Attribute_GetValuefv(&port->attributes, attrib, 0, NULL);
    if (aLength != count) {
        return WFD_ERROR_ILLEGAL_ARGUMENT;
    }

    temp = OWF_Attribute_GetValuefv(&port->attributes, attrib, count, value);
    if (temp != count) {
        return WFD_ERROR_ILLEGAL_ARGUMENT;
    }
    return WFD_Util_AttrEc2WfdEc(OWF_AttributeList_GetError(&port->attributes));
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY WFD_Port_SetAttribi(
    WFD_PORT *port, WFDPortConfigAttrib attrib, WFDint value) OWF_APIEXIT {
    WFDErrorCode ec;

    ec = WFD_Port_ValidateAttribi(port, attrib, value);
    if (WFD_ERROR_NONE == ec) {
        if (attrib == WFD_PORT_BACKGROUND_COLOR) {
            WFDfloat bg[BG_SIZE];

            WFD_Util_BgInt2Fv(value, BG_SIZE, bg);
            OWF_Attribute_SetValuefv(&port->attributes, attrib, BG_SIZE, bg);
        } else {
            OWF_Attribute_SetValuei(&port->attributes, attrib, value);
        }
        ec = WFD_Util_AttrEc2WfdEc(
            OWF_AttributeList_GetError(&port->attributes));
    }
    return ec;
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY WFD_Port_SetAttribf(
    WFD_PORT *port, WFDPortConfigAttrib attrib, WFDfloat value) OWF_APIEXIT {
    WFDErrorCode ec;

    ec = WFD_Port_ValidateAttribf(port, attrib, value);
    if (WFD_ERROR_NONE == ec) {
        OWF_Attribute_SetValuef(&port->attributes, attrib, value);
        ec = WFD_Util_AttrEc2WfdEc(
            OWF_AttributeList_GetError(&port->attributes));
    }
    return ec;
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Port_SetAttribiv(WFD_PORT *port, WFDPortConfigAttrib attrib, WFDint count,
                     const WFDint *values) OWF_APIEXIT {
    WFDErrorCode ec;

    ec = WFD_Port_ValidateAttribiv(port, attrib, count, values);
    if (WFD_ERROR_NONE == ec) {
        if (attrib == WFD_PORT_BACKGROUND_COLOR) {
            WFDfloat bg[BG_SIZE];

            WFD_Util_BgIv2Fv(count, values, bg);
            OWF_Attribute_SetValuefv(&port->attributes, attrib, count, bg);
        } else {
            OWF_Attribute_SetValueiv(&port->attributes, attrib, count, values);
        }
        ec = WFD_Util_AttrEc2WfdEc(
            OWF_AttributeList_GetError(&port->attributes));
    }
    return ec;
}

OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Port_SetAttribfv(WFD_PORT *port, WFDPortConfigAttrib attrib, WFDint count,
                     const WFDfloat *values) OWF_APIEXIT {
    WFDErrorCode ec;

    ec = WFD_Port_ValidateAttribfv(port, attrib, count, values);
    if (WFD_ERROR_NONE == ec) {
        OWF_Attribute_SetValuefv(&port->attributes, attrib, count, values);
        ec = WFD_Util_AttrEc2WfdEc(
            OWF_AttributeList_GetError(&port->attributes));
    }
    return ec;
}

static WFDErrorCode WFD_Port_ValidateAttribi(WFD_PORT *port,
                                             WFDPortConfigAttrib attrib,
                                             WFDint value) {
    WFDErrorCode result = WFD_ERROR_NONE;

    OWF_ASSERT(port && port->config);

    DPRINT(("WFD_Port_ValidateAttribi(pipeline=%d, attrib=%d, value=%d",
            ID(port), attrib, value));

    switch (attrib) {
        case WFD_PORT_FLIP:
        case WFD_PORT_MIRROR: {
            WFD_PORT_MODE *mode;

            /* check boolean */
            if (!(WFD_FALSE == value || WFD_TRUE == value)) {
                result = WFD_ERROR_ILLEGAL_ARGUMENT;
            } else {
                mode = WFD_Port_GetModePtr(port);
                if (NULL != mode) {
                    if (!mode->flipMirrorSupport && WFD_TRUE == value) {
                        result = WFD_ERROR_ILLEGAL_ARGUMENT;
                    }
                } else {
                    result = WFD_ERROR_ILLEGAL_ARGUMENT;
                }
            }

            break;
        }

        case WFD_PORT_ROTATION: {
            WFD_PORT_MODE *mode;

            mode = WFD_Port_GetModePtr(port);
            if (NULL != mode) {
                if (mode->rotationSupport == WFD_ROTATION_SUPPORT_NONE) {
                    if (value != 0) {
                        result = WFD_ERROR_ILLEGAL_ARGUMENT;
                    }
                } else if (mode->rotationSupport ==
                           WFD_ROTATION_SUPPORT_LIMITED) {
                    if (value != 0 && value != 90 && value != 180 &&
                        value != 270) {
                        result = WFD_ERROR_ILLEGAL_ARGUMENT;
                    }
                } else {
                    result = WFD_ERROR_ILLEGAL_ARGUMENT;
                }

            } else {
                result = WFD_ERROR_ILLEGAL_ARGUMENT;
            }
            break;
        }

        case WFD_PORT_PROTECTION_ENABLE: {
            /* check boolean */
            if (!(WFD_FALSE == value || WFD_TRUE == value)) {
                result = WFD_ERROR_ILLEGAL_ARGUMENT;
            } else {
                /* Real implementation should check here
                 * whether the HW is capable of providing protection
                 */

                /* Sample implementation on virtual hw does have the
                 * information - content protection is never supported
                 * Setting value to WFD_FALSE is allowed
                 */
                if (value != WFD_FALSE) {
                    result = WFD_ERROR_ILLEGAL_ARGUMENT;
                }
            }
            break;
        }

        case WFD_PORT_POWER_MODE: {
            if (!(WFD_POWER_MODE_OFF == value ||
                  WFD_POWER_MODE_SUSPEND == value ||
                  WFD_POWER_MODE_LIMITED_USE == value ||
                  WFD_POWER_MODE_ON == value)) {
                result = WFD_ERROR_ILLEGAL_ARGUMENT;
            }
            break;
        }

        case WFD_PORT_PARTIAL_REFRESH_ENABLE: {
            WFDPartialRefresh supported = port->config->partialRefreshSupport;

            if (value == WFD_PARTIAL_REFRESH_NONE) {
                result = WFD_ERROR_NONE;
            } else if (value == WFD_PARTIAL_REFRESH_VERTICAL) {
                if (supported != WFD_PARTIAL_REFRESH_VERTICAL &&
                    supported != WFD_PARTIAL_REFRESH_BOTH) {
                    result = WFD_ERROR_ILLEGAL_ARGUMENT;
                }
            } else if (value == WFD_PARTIAL_REFRESH_HORIZONTAL) {
                if (supported != WFD_PARTIAL_REFRESH_HORIZONTAL &&
                    supported != WFD_PARTIAL_REFRESH_BOTH) {
                    result = WFD_ERROR_ILLEGAL_ARGUMENT;
                }
            } else if (value == WFD_PARTIAL_REFRESH_BOTH) {
                if (supported != WFD_PARTIAL_REFRESH_BOTH) {
                    result = WFD_ERROR_ILLEGAL_ARGUMENT;
                }
            } else {
                result = WFD_ERROR_ILLEGAL_ARGUMENT;
            }
            break;
        }

        case WFD_PORT_BACKGROUND_COLOR: {
            if ((value & 0xFF) != 0xFF) /* alpha always 0xFF when set or get */
            {
                result = WFD_ERROR_ILLEGAL_ARGUMENT;
            } else {
                WFDfloat bg[BG_SIZE];

                WFD_Util_BgInt2Fv(value, BG_SIZE, bg);
                result = WFD_Port_ValidateAttribfv(port, attrib, BG_SIZE, bg);
            }
            break;
        }

        default: {
            DPRINT(("  Invalid port attribute: %d", attrib));
            result = WFD_ERROR_ILLEGAL_ARGUMENT;
            break;
        }
    }
    return result;
}

static WFDErrorCode WFD_Port_ValidateAttribf(WFD_PORT *port,
                                             WFDPortConfigAttrib attrib,
                                             WFDfloat value) {
    WFDErrorCode result = WFD_ERROR_NONE;

    OWF_ASSERT(port);

    DPRINT(("WFD_Port_ValidateAttribi(pipeline=%d, attrib=%d, value=%d",
            ID(port), attrib, value));

    switch (attrib) {
        case WFD_PORT_GAMMA: {
            if (!(value >= port->config->gammaRange[0] &&
                  value <= port->config->gammaRange[1])) {
                result = WFD_ERROR_ILLEGAL_ARGUMENT;
            }
            break;
        }

        default: {
            DPRINT(("  Invalid port attribute: %d", attrib));
            result = WFD_ERROR_ILLEGAL_ARGUMENT;
            break;
        }
    }
    return result;
}

static WFDErrorCode WFD_Port_ValidateAttribiv(WFD_PORT *port,
                                              WFDPortConfigAttrib attrib,
                                              WFDint count,
                                              const WFDint *values) {
    WFDErrorCode result = WFD_ERROR_NONE;

    OWF_ASSERT(port);

    DPRINT(("WFD_Port_ValidateAttribi(pipeline=%d, attrib=%d, value=%d",
            ID(port), attrib, values));

    switch (attrib) {
        case WFD_PORT_PARTIAL_REFRESH_RECTANGLE: {
            if (RECT_SIZE != count) {
                result = WFD_ERROR_ILLEGAL_ARGUMENT;
                break;
            }

            if (!(values[RECT_OFFSETX] >= 0 && values[RECT_OFFSETY] >= 0 &&
                  values[RECT_WIDTH] <=
                      port->config->partialRefreshMaximum[0] &&
                  values[RECT_HEIGHT] <=
                      port->config->partialRefreshMaximum[1])) {
                result = WFD_ERROR_ILLEGAL_ARGUMENT;
            }
            break;
        }

        case WFD_PORT_BACKGROUND_COLOR: {
            WFDfloat bg[BG_SIZE];

            if (BG_SIZE != count) {
                result = WFD_ERROR_ILLEGAL_ARGUMENT;
                break;
            }

            WFD_Util_BgIv2Fv(count, values, bg);
            result = WFD_Port_ValidateAttribfv(port, attrib, count, bg);
            break;
        }

        default: {
            DPRINT(("  Invalid port attribute: %d", attrib));
            result = WFD_ERROR_ILLEGAL_ARGUMENT;
            break;
        }
    }
    return result;
}

static WFDErrorCode WFD_Port_ValidateAttribfv(WFD_PORT *port,
                                              WFDPortConfigAttrib attrib,
                                              WFDint count,
                                              const WFDfloat *values) {
    WFDErrorCode result = WFD_ERROR_NONE;

    OWF_ASSERT(port);

    DPRINT(("WFD_Port_ValidateAttribi(pipeline=%d, attrib=%d, value=%d",
            ID(port), attrib, values));

    switch (attrib) {
        case WFD_PORT_BACKGROUND_COLOR: {
            if (BG_SIZE != count) {
                result = WFD_ERROR_ILLEGAL_ARGUMENT;
                break;
            }

            if (!(INRANGE(values[0], 0.0f, 1.0f) &&
                  INRANGE(values[1], 0.0f, 1.0f) &&
                  INRANGE(values[2], 0.0f, 1.0f))) {
                result = WFD_ERROR_ILLEGAL_ARGUMENT;
            }
            break;
        }
        default: {
            DPRINT(("  Invalid port attribute: %d", attrib));
            result = WFD_ERROR_BAD_ATTRIBUTE;
            break;
        }
    }
    return result;
}

/* ================================================================== */
/*                   P O R T   B I N D I N G S                        */
/* ================================================================== */

OWF_API_CALL WFDint OWF_APIENTRY
WFD_Port_PipelineNbr(WFD_PORT *port, WFD_PIPELINE *pipeline) OWF_APIEXIT {
    WFDint i;

    OWF_ASSERT(port && pipeline);
    if (!port || !pipeline) return -1;

    for (i = 0; i < PLCOUNT(port); i++) {
        if (ID(pipeline) == port->config->pipelineIds[i]) {
            return i;
        }
    }
    return -1;
}

OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Port_PipelineBindable(WFD_PORT *port, WFDint pipelineId) OWF_APIEXIT {
    WFDint i;

    for (i = 0; i < PLCOUNT(port); i++) {
        if (port->config->pipelineIds[i] == pipelineId) {
            return WFD_TRUE;
        }
    }
    return WFD_FALSE;
}

OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Port_PipelineBound(WFD_PORT *port, WFD_PIPELINE *pipeline) OWF_APIEXIT {
    OWF_ASSERT(port && pipeline && pipeline->bindings);
    if (!port || !pipeline) return WFD_FALSE;

    return (pipeline->bindings->boundPort == port);
}

OWF_API_CALL void OWF_APIENTRY WFD_Port_PipelineCacheBinding(
    WFD_PORT *port, WFD_PIPELINE *pipeline) OWF_APIEXIT {
    WFDint pipelineNbr;

    OWF_ASSERT(port && port->bindings);
    OWF_ASSERT(pipeline && pipeline->bindings);

    pipelineNbr = WFD_Port_PipelineNbr(port, pipeline);
    if (pipelineNbr >= 0) {
        WFD_PORT *oldPort;

        oldPort = pipeline->bindings->cachedPort;

        REMREF(port->bindings[pipelineNbr].cachedPipeline);
        ADDREF(port->bindings[pipelineNbr].cachedPipeline, pipeline);

        /* check if earlier cached binding exist */
        if (oldPort && oldPort != port) {
            WFDint ind;

            WFD_Port_AcquireLock(oldPort);
            ind = WFD_Port_PipelineNbr(oldPort, pipeline);
            REMREF(oldPort->bindings[ind].cachedPipeline);
            WFD_Port_ReleaseLock(oldPort);
        }

        REMREF(pipeline->bindings->cachedPort);
        ADDREF(pipeline->bindings->cachedPort, port);
        pipeline->bindings->portDirty = WFD_TRUE;
        return;
    }

    DPRINT(("Cannot cache port-pipeline binding %d, %d", ID(port),
            pipeline->config->id));
}

static void WFD_Port_PipelineRemoveBinding(WFD_PORT *pPort,
                                           WFDint pipelineInd) {
    WFD_PORT_BINDING *portBinding;
    WFD_PIPELINE_BINDINGS *plBindings = NULL;

    OWF_ASSERT(pPort && pPort->config);
    OWF_ASSERT(pipelineInd < pPort->config->pipelineIdCount);

    portBinding = &pPort->bindings[pipelineInd];
    if (!portBinding) return;

    if (portBinding->boundPipeline) {
        plBindings = portBinding->boundPipeline->bindings;
        REMREF(plBindings->boundPort);
        portBinding->boundPipeline->config->layer = WFD_INVALID_PIPELINE_LAYER;
        portBinding->boundPipeline->config->portId = WFD_INVALID_PORT_ID;
        REMREF(portBinding->boundPipeline);
    }

    if (portBinding->cachedPipeline) {
        plBindings = portBinding->cachedPipeline->bindings;
        REMREF(plBindings->cachedPort);
        REMREF(portBinding->cachedPipeline);
    }
}

OWF_API_CALL WFDint OWF_APIENTRY WFD_Port_QueryPipelineLayerOrder(
    WFD_PORT *pPort, WFD_PIPELINE *pPipeline) OWF_APIEXIT {
    WFDint i;

    OWF_ASSERT(pPort && pPort->config && pPipeline && pPipeline->config);

    i = WFD_Port_PipelineNbr(pPort, pPipeline);

    if (i >= 0) {
        return BINDABLE_PL_INDEX_2_PL_LAYER(i);
    }

    /* pipeline not bindable */
    return WFD_INVALID_PIPELINE_ID;
}

/* ================================================================== */
/*                    D I S P L A Y   D A T A                         */
/* ================================================================== */

OWF_API_CALL WFDint OWF_APIENTRY
WFD_Port_GetDisplayDataFormats(WFD_PORT *port, WFDDisplayDataFormat *format,
                               WFDint formatCount) OWF_APIEXIT {
    WFDint i;
    WFDint count = 0;

    OWF_ASSERT(port && port->config);

    if (!port->config->attached) {
        return 0;
    }

    if (!format) {
        return port->config->displayDataCount;
    }

    for (i = 0; i < formatCount; i++) {
        if (i < port->config->displayDataCount) {
            format[i] = port->config->displayData[i].format;
            count++;
        }
    }

    return count;
}

OWF_API_CALL WFDboolean OWF_APIENTRY WFD_Port_HasDisplayData(
    WFD_PORT *port, WFDDisplayDataFormat format) OWF_APIEXIT {
    WFDint i;

    OWF_ASSERT(port && port->config);

    if (!port->config->attached) {
        return WFD_FALSE;
    }

    for (i = 0; i < port->config->displayDataCount; i++) {
        if (port->config->displayData[i].format == format) {
            return WFD_TRUE;
        }
    }

    return WFD_FALSE;
}

OWF_API_CALL WFDint OWF_APIENTRY
WFD_Port_GetDisplayData(WFD_PORT *port, WFDDisplayDataFormat format,
                        WFDuint8 *data, WFDint dataCount) OWF_APIEXIT {
    WFDint i;
    WFDint count = 0;

    OWF_ASSERT(port && port->config);

    if (!port->config->attached) {
        return 0;
    }

    for (i = 0; i < port->config->displayDataCount; i++) {
        WFD_DISPLAY_DATA *displayData = &port->config->displayData[i];

        if (displayData->format == format) {
            WFDint j;

            if (!data) {
                count = displayData->dataSize;
            } else {
                for (j = 0; j < dataCount; j++) {
                    if (j < displayData->dataSize) {
                        data[j] = displayData->data[j];
                        count++;
                    } else {
                        data[j] = 0;
                    }
                }
            }

            break;
        }
    }

    return count;
}

/* ================================================================== */
/*                   P O W E R    M O D E S                           */
/* ================================================================== */

static void WFD_Port_ChangePowerMode(WFD_PORT *port, WFDPowerMode currentPower,
                                     WFDPowerMode newPower) {
    /* pre-conditions: port mode already committed */
    /*                 port mode exists            */

    OWF_ASSERT(port && port->config);

    if (newPower == currentPower) {
        return;
    }

    switch (newPower) {
        case WFD_POWER_MODE_OFF:
        case WFD_POWER_MODE_SUSPEND:
            /* Sample implementation does not distinguish between these
             * two states because the only difference is hardware power
             * consumption. Rendering will be shut down in both cases.
             */

            WFD_Port_PowerOff(port);
            break;

        case WFD_POWER_MODE_LIMITED_USE:
            /* Code for limited use mode here!
             * sample implementation does not differentiate
             * between power on and limited use modes */
            /* flow through */
        case WFD_POWER_MODE_ON:
            WFD_Port_PowerOn(port);
            break;

        default:
            OWF_ASSERT(0); /* this should never happen */
    }
}

static WFDboolean WFD_Port_IsAttached(WFD_PORT *pPort) {
    OWF_ASSERT(pPort && pPort->config);

    return (pPort->config->attached);
}

static WFDboolean WFD_Port_PowerIsOn(WFD_PORT *pPort) {
    OWF_ASSERT(pPort && pPort->config);

    /* SI: power is considered be on when power mode is set correctly
     * and screen exists
     */

    return (pPort->config->powerMode == WFD_POWER_MODE_LIMITED_USE ||
            pPort->config->powerMode == WFD_POWER_MODE_ON) &&
           pPort->screenNumber != OWF_INVALID_SCREEN_NUMBER;
}

static void WFD_Port_PowerOff(WFD_PORT *pPort) {
    WFDfloat black[BG_SIZE] = {0.0, 0.0, 0.0};

    OWF_ASSERT(pPort);

    DPRINT(("Port going power off: %d (%p)", ID(pPort), pPort));

    WFD_Port_SetFrameBufferBackground(pPort, black);
}

static void WFD_Port_PowerOn(WFD_PORT *pPort) {
    OWF_ASSERT(pPort && pPort->currentMode);

    DPRINT(("Port going power on: %d (%p)", ID(pPort), pPort));

    WFD_Port_SetFrameBufferBackground(pPort, pPort->config->backgroundColor);
}

static void WFD_Port_SetFrameBufferBackground(WFD_PORT *port, WFDfloat *color) {
    /* this can be called when port lock is in posession */

    OWFsubpixel red;
    OWFsubpixel green;
    OWFsubpixel blue;

    WFDint w, h;

    OWF_ASSERT(port && port->config);
    OWF_ASSERT(port->frameBuffer == 0 || port->frameBuffer == 1);

    if (port->currentMode) {
        w = port->currentMode->width;
        h = port->currentMode->height;
    } else {
        w = port->config->nativeResolution[0];
        h = port->config->nativeResolution[1];
    }

    OWF_Image_SetSize(port->scratch[0], w, h);

    red = color[0];
    green = color[1];
    blue = color[2];

    OWF_Image_Clear(port->scratch[0], red, green, blue, OWF_FULLY_OPAQUE);

    OWF_Mutex_Lock(&port->frMutex);
    {
        OWF_Image_SetSize(port->surface[port->frameBuffer], w, h);
        OWF_Image_DestinationFormatConversion(port->surface[port->frameBuffer],
                                              port->scratch[0]);
    }
    OWF_Mutex_Unlock(&port->frMutex);
}

/*! \brief Catching detach/attach events
 *
 * Generate an event to device's event queues after
 * a port has bee detached or attached.
 *
 * SI: This is a callback which is called from the screen module
 * after user has attached or detached a port.
 *
 * In a real implementation, an interrupt handler of
 * these hardware events would replace this function.
 *
 * \param obj           Pointer to port object
 * \param screenNumber  Screen giving the callback
 * \param event         A character denoting the type of the event
 */
static void WFD_Port_AttachDetach(void *obj, WFDint screenNumber, char event) {
    WFD_PORT *pPort = (WFD_PORT *)obj;

    /* search port matching screen number */
    if (pPort->screenNumber == screenNumber) {
        if (pPort->config->detachable) {
            switch (event) {
                case 'a':
                case 'A':
                    if (!pPort->config->attached) {
                        WFD_Port_Attach(pPort);
                    }
                    break;
                case 'd':
                case 'D':
                    if (pPort->config->attached) {
                        WFD_Port_Detach(pPort);
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

/* ================================================================== */
/*                     P O R T  C O M M I T                           */
/* ================================================================== */

/* \brief Commit port
 *
 *  Device's commit lock is set!
 *  All attributes has been checked for consistency!
 *
 */

OWF_API_CALL void OWF_APIENTRY WFD_Port_Commit(WFD_PORT *port) OWF_APIEXIT {
    WFD_Port_AcquireLock(port);
    WFD_Port_DoCommit(port);
    WFD_Port_ReleaseLock(port);
}

static void WFD_Port_DoCommit(WFD_PORT *port) {
    WFDPowerMode newPower, currentPower;
    WFDboolean hasImmT = WFD_FALSE; /* port has immediate transitions */
    WFDint i;

    /* pre-condition: port is commit consistent */

    OWF_ASSERT(port && port->config);

    WFD_Port_CommitPortMode(port);

    currentPower = port->config->powerMode;
    newPower = OWF_Attribute_GetValuei(&port->attributes, WFD_PORT_POWER_MODE);

    /* commit attribute list before changing port power mode */
    OWF_AttributeList_Commit(&port->attributes, WFD_PORT_ID,
                             WFD_PORT_PROTECTION_ENABLE,
                             COMMIT_ATTR_DIRECT_FROM_WORKING);

    WFD_Port_ChangePowerMode(port, currentPower, newPower);

    for (i = 0; i < PLCOUNT(port); i++) {
        WFD_PIPELINE *pl;

        pl = port->bindings[i].cachedPipeline;
        if (pl) {
            WFD_Port_CommitPipelineBindings(port, i, pl);
        }

        /* at this point, cached pipeline should be bound to port */
        pl = port->bindings[i].boundPipeline;
        if (pl) {
            hasImmT = WFD_Pipeline_Commit(pl, port) || hasImmT;
        }
    }

    {
        WFDboolean imm = WFD_FALSE; /* immediate render? */

#if ENABLE_SYNCHRONOUS_PIPELINES
        /* note: synchronousPipelines may be enabled for debugging purposes */
        imm = synchronousPipelines || hasImmT;
#else
        imm = hasImmT;
#endif
        /* do not render if power is off */
        imm = imm && WFD_Port_PowerIsOn(port) && WFD_Port_IsAttached(port);

        if (imm) {
            /* immediate transition: render & blit */
            WFD_Port_Render(port, WFD_MESSAGE_IMMEDIATE);
            WFD_Port_Blit(port);
        }
    }
}

static void WFD_Port_CommitPortMode(WFD_PORT *port) {
    OWF_ASSERT(port);

    if (port && port->modeDirty) {
        port->modeDirty = WFD_FALSE;

        if (port->cachedMode) {
            DPRINT(("  changing port mode %d -> %d",
                    (port->currentMode) ? port->currentMode->id : 0,
                    (port->cachedMode) ? port->cachedMode->id : 0));

            port->currentMode = port->cachedMode;

            if (port->screenNumber != OWF_INVALID_SCREEN_NUMBER) {
                OWF_Screen_Resize(port->screenNumber, port->currentMode->width,
                                  port->currentMode->height);
            }

            /* Initialize frame buffers with port background color.
             * This prevents tearing effects during refresh.
             * Port lock is on, so it is safe to use scratch buffer.
             * Frame mutex must be grabbed when frame buffers are touched.
             */

            WFD_Port_SetFrameBufferBackground(port,
                                              port->config->backgroundColor);
        }
    }
}

static void WFD_Port_CommitPipelineBindings(WFD_PORT *pPort, WFDint i,
                                            WFD_PIPELINE *pPipeline) {
    WFD_PORT_BINDING *portBinding = &pPort->bindings[i];

    if (!pPipeline) {
        return; /* nothing to commit */
    } else {
        WFD_PIPELINE_BINDINGS *plBindings = pPipeline->bindings;
        WFD_PIPELINE_CONFIG *plConfig = pPipeline->config;

        if (portBinding && plBindings && plConfig) {
            /* remove old binding, if any */
            if (plBindings->boundPort) {
                WFDboolean lockPort = plBindings->boundPort != pPort;
                WFD_PORT *oldPort = plBindings->boundPort;

                if (lockPort) {
                    WFD_Port_AcquireLock(oldPort);
                }

                WFD_Pipeline_PortRemoveBinding(plBindings->boundPort,
                                               plBindings->pipeline, WFD_FALSE);
                if (lockPort) {
                    WFD_Port_ReleaseLock(oldPort);
                }

                plConfig->layer = WFD_INVALID_PIPELINE_LAYER;
                plConfig->portId = WFD_INVALID_PORT_ID;
            }

            /* move cached binding to bound binding */
            ADDREF(portBinding->boundPipeline, portBinding->cachedPipeline);
            REMREF(portBinding->cachedPipeline); /* remove cached binding */
            ADDREF(plBindings->boundPort, plBindings->cachedPort);
            REMREF(plBindings->cachedPort); /* remove cached binding */
            plBindings->portDirty = WFD_FALSE;

            plConfig->layer = WFD_Port_QueryPipelineLayerOrder(
                pPort, portBinding->boundPipeline);
            plConfig->portId = pPort->config->id;
        }
    }
}

OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Port_IsCommitConsistent(WFD_PORT *port, WFDCommitType type) OWF_APIEXIT {
    WFDboolean consistent = WFD_TRUE;
    WFDint i;

    OWF_ASSERT(port && port->config);

    consistent = WFD_Port_IsPartialRefeshCommitConsistent(port);
    consistent = consistent && WFD_Port_IsPortModeCommitConsistent(port);

    for (i = 0; i < PLCOUNT(port) && consistent; i++) {
        WFD_PORT_BINDING *portBinding = &port->bindings[i];
        WFD_PIPELINE *pl;

        if (portBinding->cachedPipeline) {
            pl = portBinding->cachedPipeline;

            if (type == WFD_COMMIT_ENTIRE_PORT &&
                pl->bindings->boundPort != NULL &&
                pl->bindings->boundPort != port) {
                /* cannot do port only commit if it requires releasing
                 * pipeline binding of another port
                 */
                consistent = WFD_FALSE;
            }
        } else {
            pl = portBinding->boundPipeline;
        }

        if (pl && consistent) {
            consistent =
                consistent && WFD_Pipeline_IsCommitConsistent(pl, type);
        }
    }

    if (!consistent) {
        DPRINT(("  port is not commit consistent %d", ID(port)));
    }

    return consistent;
}

static WFDboolean WFD_Port_IsPartialRefeshCommitConsistent(WFD_PORT *port) {
    WFDboolean consistent = WFD_TRUE;
    WFDPartialRefresh enabled;
    WFD_PORT_MODE *pMode;

    OWF_ASSERT(port && port->config);

    WFD_Port_GetAttribi(port, WFD_PORT_PARTIAL_REFRESH_ENABLE,
                        (WFDint *)&enabled);

    if (enabled != WFD_PARTIAL_REFRESH_NONE) {
        pMode = WFD_Port_GetModePtr(port);
        consistent = (pMode != NULL);
    }

    if (enabled != WFD_PARTIAL_REFRESH_NONE && consistent) {
        WFDint pW, pH;
        WFDint prRect[RECT_SIZE];

        WFD_PortMode_GetAttribi(pMode, WFD_PORT_MODE_WIDTH, &pW);
        WFD_PortMode_GetAttribi(pMode, WFD_PORT_MODE_HEIGHT, &pH);

        WFD_Port_GetAttribiv(port, WFD_PORT_PARTIAL_REFRESH_RECTANGLE,
                             RECT_SIZE, prRect);

        if (enabled != WFD_PARTIAL_REFRESH_VERTICAL) {
            consistent =
                consistent && (pW >= prRect[RECT_OFFSETX] + prRect[RECT_WIDTH]);
        }

        if (enabled != WFD_PARTIAL_REFRESH_HORIZONTAL) {
            consistent = consistent &&
                         (pH >= prRect[RECT_OFFSETY] + prRect[RECT_HEIGHT]);
        }
    }

    if (!consistent) {
        DPRINT(
            ("  partial refresh attributes are not commit consistent for port "
             "%d",
             ID(port)));
    }

    return consistent;
}

static WFDboolean WFD_Port_IsPortModeCommitConsistent(WFD_PORT *port) {
    WFDboolean consistent = WFD_TRUE;
    WFD_PORT_MODE *pMode;

    OWF_ASSERT(port && port->config);

    pMode = WFD_Port_GetModePtr(port);

    if (pMode != NULL) {
        /* check that port flip/mirror settings does not
         * violate port mode flip/mirror  constraints */

        WFDboolean pmFlip = WFD_FALSE;
        WFDboolean pFlip = WFD_FALSE;
        WFDboolean pMirror = WFD_FALSE;

        WFD_PortMode_GetAttribi(pMode, WFD_PORT_MODE_FLIP_MIRROR_SUPPORT,
                                (WFDint *)&pmFlip);
        WFD_Port_GetAttribi(port, WFD_PORT_FLIP, (WFDint *)&pFlip);
        WFD_Port_GetAttribi(port, WFD_PORT_MIRROR, (WFDint *)&pMirror);

        consistent = consistent && ((pmFlip) || (!pmFlip && pFlip));
        consistent = consistent && ((pmFlip) || (!pmFlip && pMirror));

        if (!consistent) {
            DPRINT(
                ("  port %d flip/mirror attributes does not match port mode "
                 "settings",
                 ID(port)));
        }
    }

    if (pMode != NULL && consistent) {
        /* check that port rotation is set correctly */
        WFDRotationSupport pmRot = WFD_ROTATION_SUPPORT_NONE;
        WFDRotationSupport pRot = WFD_ROTATION_SUPPORT_NONE;

        WFD_PortMode_GetAttribi(pMode, WFD_PORT_MODE_ROTATION_SUPPORT,
                                (WFDint *)&pmRot);
        WFD_Port_GetAttribi(port, WFD_PORT_ROTATION, (WFDint *)&pRot);

        consistent = (pmRot == WFD_ROTATION_SUPPORT_LIMITED) || (pmRot == pRot);

        if (!consistent) {
            DPRINT(
                ("  port %d rotation attribute does not match port mode "
                 "settings",
                 ID(port)));
        }
    }

    if (!consistent) {
        DPRINT(("  port %d is not commit consistent after port mode change",
                ID(port)));
    }

    return consistent;
}

OWF_API_CALL void OWF_APIENTRY WFD_Port_CommitForSinglePipeline(
    WFD_PIPELINE *pipeline, WFDboolean hasImmT) OWF_APIEXIT {
    WFD_PORT *cPort = NULL;

    /* Commit any port binding here */
    cPort = pipeline->bindings->cachedPort;

    if (cPort) {
        WFDint pipelineNbr;

        WFD_Port_AcquireLock(cPort);

        pipelineNbr = WFD_Port_PipelineNbr(cPort, pipeline);

        if (pipelineNbr >= 0) {
            WFD_Port_CommitPipelineBindings(cPort, pipelineNbr, pipeline);
        }

        if (hasImmT && cPort) {
            WFDboolean imm = WFD_FALSE; /* immediate render? */

#if ENABLE_SYNCHRONOUS_PIPELINES
            /* note: synchronousPipelines may be enabled for debugging purposes
             */
            imm = synchronousPipelines || hasImmT;
#else
            imm = hasImmT;
#endif
            /* do not render if power is off */
            imm =
                imm && WFD_Port_PowerIsOn(cPort) && WFD_Port_IsAttached(cPort);

            if (imm) {
                /* immediate transition: render & blit */
                WFD_Port_Render(cPort, WFD_MESSAGE_IMMEDIATE);
                WFD_Port_Blit(cPort);
            }
        }

        WFD_Port_ReleaseLock(cPort);
    }
}

/* ================================================================== */
/*                       B L E N D E R                                */
/* ================================================================== */

static WFDboolean WFD_Port_CanRender(WFD_PORT *port) {
    if (WFD_Port_PowerIsOn(port) && WFD_Port_IsAttached(port) &&
        port->currentMode) {
        return WFD_TRUE;
    }

    return WFD_FALSE;
}
/*
 * \brief Compose image from port pipelines
 *
 *  Preconditions
 *  - all source bindings exists
 *  Postconditions
 *  -
 *
 */
static void *WFD_Port_BlenderThread(void *data) {
    WFD_PORT *port;
    WFDint ec;
    OWF_MESSAGE msg;

    ADDREF(port, (WFD_PORT *)data);

    DPRINT(("WFD_Port_BlenderThread starting %d", ID(port)));

    OWF_ASSERT(port);

    memset(&msg, 0, sizeof(OWF_MESSAGE));

    /* Loop until quit message detected. Periodic blitter should
     * always be feeding VSYNC events, so deadlock shouldn't
     * be possible. */
    while (msg.id != WFD_MESSAGE_QUIT) {
        ec = OWF_Message_Wait(&port->msgQueue, &msg, WAIT_FOREVER);

        if (ec >= 0) {
            if (msg.id == WFD_MESSAGE_QUIT) {
                break;
            }

            /* port lock is needed to prevent configuration change during
             * rendering */
            WFD_Port_AcquireLock(port);

            if (WFD_Port_CanRender(port)) {
                WFD_Port_Render(port, msg.id);
            } else {
                if (!WFD_Port_IsAttached(port)) {
                    DPRINT(("Port is not attached %d", ID(port)));
                }
                if (!WFD_Port_PowerIsOn(port)) {
                    DPRINT(("Port power is off %d", ID(port)));
                }
                if (!port->currentMode) {
                    DPRINT(("Port mode in not set %d", ID(port)));
                }
            }

            WFD_Port_ReleaseLock(port);
        }
    }

    DPRINT(("WFD_Port_BlenderThread quitting %d", ID(port)));

    REMREF(port);
    OWF_Thread_Exit(NULL);

    return NULL;
}

static void WFD_Port_Render(WFD_PORT *port, WFD_MESSAGES cmd) {
    WFDint i;

    DPRINT(("WFD_Port_Render, port %d", ID(port)));

    WFD_Port_RenderInit(port);

    /* Run all pipelines */
    for (i = 0; i < PLCOUNT(port); i++) {
        WFD_PIPELINE *pipeline = port->bindings[i].boundPipeline;
        WFD_PIPELINE_BINDINGS *bndgs = NULL;
        WFDboolean srcTransition = WFD_FALSE;
        WFDboolean maskTransition = WFD_FALSE;

        if (!pipeline) /* no pipeline */
        {
            DPRINT((">>>>> Pipeline %d not bound to the port",
                    port->config->pipelineIds[i]));
            continue;
        }

        OWF_ASSERT(pipeline->bindings);
        bndgs = pipeline->bindings;

        srcTransition = doTransition(cmd, bndgs->boundSrcTransition);
        maskTransition = doTransition(cmd, bndgs->boundMaskTransition);

        /* render source image to pipeline output */
        if (srcTransition) {
            if (WFD_Pipeline_Disabled(pipeline)) {
                DPRINT((">>>>> Pipeline %d is disabled",
                        port->config->pipelineIds[i]));
            } else if (bndgs->boundSource == NULL) {
                DPRINT((">>>>> No source bound to pipeline %d",
                        port->config->pipelineIds[i]));
                WFD_Pipeline_Clear(pipeline);
            } else {
                /* This could be done in parallel for all pipelines. */
                WFD_Pipeline_Execute(pipeline, bndgs->boundSource);
            }
        }

        /* Blend pipeline result into port memory.
         * This must be done in serial layer by layer, bottom first.
         * List of bindings for port is expected to have correct order.
         * NOTE: also 'old' image must be blended
         */
        WFD_Port_LayerAndBlend(port, pipeline, bndgs->boundMask);

        /* this pipeline is done - send events */
        if (srcTransition) {
            WFD_Pipeline_SourceBindComplete(pipeline);
        }

        if (maskTransition) {
            WFD_Pipeline_MaskBindComplete(pipeline);
        }

    } /* end for all pipelines */

    WFD_Port_ImageFinalize(port);
}

static void WFD_Port_RenderInit(WFD_PORT *port) {
    WFD_PORT_MODE *portMode;
    OWFsubpixel red;
    OWFsubpixel green;
    OWFsubpixel blue;

    OWF_ASSERT(port && port->config && port->currentMode);
    OWF_ASSERT(port->scratch[0] && port->scratch[1]);

    /* set-up scratch buffers */
    portMode = port->currentMode;
    OWF_Image_SetSize(port->scratch[0], portMode->width, portMode->height);
    OWF_Image_SetSize(port->scratch[1], portMode->width, portMode->height);

    /* Background color must be always set before blending, if transparency
     * is used (otherwise earlier port image is visible below)
     *
     * If fill port area is set, scratch buffer is cleared with black
     * Otherwise, port background color is used.
     */
    if (port->config->fillPortArea) {
        red = green = blue = 0.0;
    } else {
        red = port->config->backgroundColor[0];
        green = port->config->backgroundColor[1];
        blue = port->config->backgroundColor[2];
    }

    OWF_Image_Clear(port->scratch[0], red, green, blue, OWF_FULLY_OPAQUE);
}

/* \brief Blend pipeline output and mask to final port output
 *
 */
static void WFD_Port_LayerAndBlend(WFD_PORT *pPort, WFD_PIPELINE *pPipeline,
                                   WFD_MASK *pMask) {
    OWF_BLEND_INFO blend;
    OWF_RECTANGLE dstRect, srcRect;
    OWF_TRANSPARENCY blendMode;
    OWF_IMAGE *maskImage = NULL;
    WFDboolean hasMask = WFD_FALSE;
    WFDboolean pipelineVisible;

    OWF_ASSERT(pPort && pPort->config);
    OWF_ASSERT(pPipeline && pPipeline->config);

    if (pPipeline->frontBuffer == NULL) {
        DPRINT(
            ("Nothing in front buffer for pipeline %d", pPipeline->config->id));
        return;
    }

    if (WFD_Pipeline_Disabled(pPipeline)) {
        return;
    }

    if (pMask) {
        maskImage = WFD_ImageProvider_LockForReading(pMask);
        /* NOTE: mask could be converted at the time it is bound */
        OWF_Image_SetSize(pPort->scratch[WFD_PORT_MASK_INDEX], maskImage->width,
                          maskImage->height);
        hasMask = OWF_Image_ConvertMask(pPort->scratch[WFD_PORT_MASK_INDEX],
                                        maskImage);
    }

    pipelineVisible =
        WFD_Port_SetBlendRects(pPort, pPipeline, &dstRect, &srcRect);

    /* blend result only if pipeline is in refresh area */
    if (pipelineVisible) {
        WFD_Port_SetBlendParams(
            &blend, pPort, pPipeline,
            (hasMask) ? pPort->scratch[WFD_PORT_MASK_INDEX] : NULL, &dstRect,
            &srcRect);
        blendMode = WFD_Util_GetBlendMode(pPipeline->config->transparencyEnable,
                                          hasMask);
        OWF_Image_PremultiplyAlpha(pPipeline->frontBuffer);
        OWF_Image_Blend(&blend, blendMode);
    }

    if (pMask) {
        WFD_ImageProvider_Unlock(pMask);
    }
}

static void WFD_Port_SetBlendParams(OWF_BLEND_INFO *blend, WFD_PORT *pPort,
                                    WFD_PIPELINE *pPipeline, OWF_IMAGE *pMask,
                                    OWF_RECTANGLE *dstRect,
                                    OWF_RECTANGLE *srcRect) {
    /* setup blending parameters */
    blend->destination.image = pPort->scratch[0];
    blend->destination.rectangle = dstRect;
    blend->source.image = pPipeline->frontBuffer;
    blend->source.rectangle = srcRect;
    blend->mask = pMask;
    blend->globalAlpha = pPipeline->config->globalAlpha;
    blend->destinationFullyOpaque = WFD_TRUE;

    if (pPipeline->config->transparencyEnable & WFD_TRANSPARENCY_SOURCE_COLOR) {
        blend->tsColor = &pPipeline->tsColor.color;
        DPRINT(("  blend mode = WFD_TRANSPARENCY_SOURCE_COLOR: %f, %f, %f",
                pPipeline->tsColor.color.color.red,
                pPipeline->tsColor.color.color.green,
                pPipeline->tsColor.color.color.blue));
    } else {
        blend->tsColor = NULL;
    }

    DPRINT(("Blending parameters:"));
    DPRINT(("  dest image = %p", blend->destination.image));
    DPRINT(("  dest rect = {%d, %d, %d, %d}", blend->destination.rectangle->x,
            blend->destination.rectangle->y,
            blend->destination.rectangle->width,
            blend->destination.rectangle->height));
    DPRINT(("  src image = %p", blend->source.image));
    DPRINT(("  src rect = {%d, %d, %d, %d}", blend->source.rectangle->x,
            blend->source.rectangle->y, blend->source.rectangle->width,
            blend->source.rectangle->height));
    DPRINT(("  mask = %p", blend->mask));
    DPRINT(("  global alpha = %d", blend->globalAlpha));
}

static WFDboolean WFD_Port_SetBlendRects(const WFD_PORT *pPort,
                                         const WFD_PIPELINE *pPipeline,
                                         OWF_RECTANGLE *dstRect,
                                         OWF_RECTANGLE *srcRect) {
    WFDint *plRect;         /* pipeline's destination rectangle */
    OWF_RECTANGLE pRefRect; /* partial refresh rectangle in port dimensions */
    OWF_RECTANGLE
    sPartRect; /* partial refresh rectangle in pipeline dimensions */
    WFDboolean visible;

    OWF_ASSERT(pPort && pPort->config);
    OWF_ASSERT(pPipeline && pPipeline->config);
    OWF_ASSERT(dstRect && srcRect);

    plRect = pPipeline->config->destinationRectangle;

    /* blend source defaults to whole pipeline area */
    OWF_Rect_Set(srcRect, 0, 0, plRect[RECT_WIDTH], plRect[RECT_HEIGHT]);

    /* blend destination defaults to pipeline destination rectangle */
    OWF_Rect_Set(dstRect, plRect[RECT_OFFSETX], plRect[RECT_OFFSETY],
                 plRect[RECT_WIDTH], plRect[RECT_HEIGHT]);

    switch (pPort->config->partialRefreshEnable) {
        case WFD_PARTIAL_REFRESH_NONE:

            /* nothing more to be done */
            return WFD_TRUE;

        case WFD_PARTIAL_REFRESH_VERTICAL:
            pRefRect.x = 0;
            pRefRect.y = pPort->config->partialRefreshRectangle[RECT_OFFSETY];
            pRefRect.width = pPort->currentMode->width;
            pRefRect.height =
                pPort->config->partialRefreshRectangle[RECT_HEIGHT];

            sPartRect.x = 0;
            sPartRect.y =
                (pRefRect.y > dstRect->y) ? pRefRect.y - dstRect->y : 0;
            sPartRect.width = dstRect->width;
            sPartRect.height = (pRefRect.y > dstRect->y)
                                   ? pRefRect.height
                                   : pRefRect.y + pRefRect.height - dstRect->y;
            break;

        case WFD_PARTIAL_REFRESH_HORIZONTAL:
            pRefRect.x = pPort->config->partialRefreshRectangle[RECT_OFFSETX];
            pRefRect.y = 0;
            pRefRect.width = pPort->config->partialRefreshRectangle[RECT_WIDTH];
            pRefRect.height = pPort->currentMode->height;

            sPartRect.x =
                (pRefRect.x > dstRect->x) ? pRefRect.x - dstRect->x : 0;
            sPartRect.y = 0;
            sPartRect.width = (pRefRect.x > dstRect->x)
                                  ? pRefRect.width
                                  : pRefRect.x + pRefRect.width - dstRect->x;
            sPartRect.height = dstRect->height;
            break;

        case WFD_PARTIAL_REFRESH_BOTH:
            pRefRect.x = pPort->config->partialRefreshRectangle[RECT_OFFSETX];
            pRefRect.y = pPort->config->partialRefreshRectangle[RECT_OFFSETY];
            pRefRect.width = pPort->config->partialRefreshRectangle[RECT_WIDTH];
            pRefRect.height =
                pPort->config->partialRefreshRectangle[RECT_HEIGHT];

            sPartRect.y =
                (pRefRect.y > dstRect->y) ? pRefRect.y - dstRect->y : 0;
            sPartRect.x =
                (pRefRect.x > dstRect->x) ? pRefRect.x - dstRect->x : 0;
            sPartRect.width = (pRefRect.x > dstRect->x)
                                  ? pRefRect.width
                                  : pRefRect.x + pRefRect.width - dstRect->x;
            sPartRect.height = (pRefRect.y > dstRect->y)
                                   ? pRefRect.height
                                   : pRefRect.y + pRefRect.height - dstRect->y;
            break;

        default:
            OWF_ASSERT(0 != 0); /* should never happen here */
    }

    /* partial refresh destination */
    visible = OWF_Rect_Clip(dstRect, dstRect, &pRefRect);

    if (visible) {
        /* source from pipeline for partial refresh */
        OWF_Rect_Clip(srcRect, srcRect, &sPartRect);
    }

    return visible;
}

/*!
 *  \brief Final steps of port output
 *
 * - Do port flip&mirror and rotation if required
 * - Convert image to desired hardware output format
 * - Swap frame buffer pointers
 *
 */
static void WFD_Port_ImageFinalize(WFD_PORT *pPort) {
    OWF_IMAGE *outImg = pPort->scratch[0];
    OWF_IMAGE *inpImg = pPort->scratch[0];
    OWF_FLIP_DIRECTION flip = 0;
    WFDint frame = 0;

    DPRINT(("WFD_Port_ImageFinalize %d", ID(pPort)));

    /* flip, mirror and rotate port */

    if (pPort->config->flip) {
        DPRINT(("  flip port"));
        flip |= OWF_FLIP_VERTICALLY;
    }
    if (pPort->config->mirror) {
        DPRINT(("  mirror port"));
        flip |= OWF_FLIP_HORIZONTALLY;
    }
    if (flip != 0) {
        OWF_Image_Flip(inpImg, flip);
    }

    if (pPort->config->rotation != 0) {
        OWF_ROTATION rotation = OWF_ROTATION_0;

        outImg = pPort->scratch[1];

        switch (pPort->config->rotation) {
            case 0:
                rotation = OWF_ROTATION_0;
                break;
            case 90:
                rotation = OWF_ROTATION_90;
                break;
            case 180:
                rotation = OWF_ROTATION_180;
                break;
            case 270:
                rotation = OWF_ROTATION_270;
                break;
            default:
                OWF_ASSERT(0);
        }

        DPRINT(("  rotate port %d degrees", pPort->config->rotation));

        if (rotation == OWF_ROTATION_90 || rotation == OWF_ROTATION_270) {
            OWF_Image_SwapWidthAndHeight(outImg);
        }
        OWF_Image_Rotate(outImg, inpImg, rotation);
    }

    if (pPort->config->gamma != 1.0f) {
        DPRINT(("  apply gamma %f", pPort->config->gamma));
        OWF_Image_Gamma(outImg, pPort->config->gamma);
    }

    DPRINT(("  destination conversion"));
    frame = (pPort->frameBuffer + 1) % 2;
    OWF_Image_SetSize(pPort->surface[frame], outImg->width, outImg->height);
    OWF_Image_DestinationFormatConversion(pPort->surface[frame], outImg);

    /* swap frame buffer index */
    OWF_Mutex_Lock(&pPort->frMutex);
    { pPort->frameBuffer = frame; }
    OWF_Mutex_Unlock(&pPort->frMutex);
}

static WFDboolean doTransition(WFD_MESSAGES cmd, WFDTransition trans) {
    if (cmd == WFD_MESSAGE_IMMEDIATE && trans == WFD_TRANSITION_IMMEDIATE)
        return WFD_TRUE;

    if (cmd == WFD_MESSAGE_VSYNC && trans == WFD_TRANSITION_AT_VSYNC)
        return WFD_TRUE;

    if (cmd == WFD_MESSAGE_SOURCE_UPDATED) {
        return WFD_TRUE;
    }

    /* note: NONE is a debugging command - meaning: transition always */
    if (cmd == WFD_MESSAGE_NONE && trans != WFD_TRANSITION_INVALID)
        return WFD_TRUE;

    return WFD_FALSE;
}

/* ================================================================== */
/*                         B L I T T E R                              */
/* ================================================================== */

/* \brief Periodic port image refresh
 *
 *  The function emulates periodic refresh by display hardware
 *  at vertical blanking intervals. After refresh, blender thread
 *  is instructed to prepare next image.
 *
 *  The thread is active only when port is allocated (created) and
 *  screen for port exists.
 *
 *  \param data Data should be a pointer to internal port
 *  structure in memory.
 */
static void *WFD_Port_BlitterThread(void *data) {
    WFD_PORT *pPort;
    WFDint frame = -1;

    ADDREF(pPort, (WFD_PORT *)data);

    DPRINT(("WFD_Port_BlitterThread starting for port %d", ID(pPort)));

    while (1) /* loop until thread is cancelled */
    {
        OWFuint32 sleepTime;

        if (frame != pPort->frameBuffer) {
            OWF_ASSERT(pPort->screenNumber != OWF_INVALID_SCREEN_NUMBER);
            DPRINT(("Blit port %d", ID(pPort)));
            WFD_Port_Blit(pPort);
        }

        /* send compose request to port (prepare for next VSYNC) */
        OWF_Message_Send(&pPort->msgQueue, WFD_MESSAGE_VSYNC, 0);

        if (!pPort->currentMode) {
            sleepTime = 200000; /* port mode not set */
        } else {
            /* refresh interval in microseconds (refresh rate) */
            sleepTime = 100000 / pPort->currentMode->refreshRate;
        }

        /*
         * This is not really accurate. We should use a periodic
         * timer process to wake up the blitter, but sample
         * implementation is not supposed to be RT
         *
         *  Sleep is also a thread cancellation point.
         */

        OWF_Thread_MicroSleep(sleepTime);

        if (pPort->destroyPending) {
            OWF_Message_Send(&pPort->msgQueue, WFD_MESSAGE_QUIT, 0);
            break; /* port going down */
        }
    }

    REMREF(pPort);
    OWF_Thread_Exit(NULL);

    return NULL;
}

static void WFD_Port_Blit(WFD_PORT *port) {
    /*
     * Keep frame buffer mutex locked until image is blitted
     * - this prevents renderer to change buffer at a critical time
     */

    OWF_Mutex_Lock(&port->frMutex);
    {
        OWF_SCREEN screen;
        OWF_IMAGE *img;

        /* image dimensions might have changed because of port rotation */
        img = port->surface[port->frameBuffer];
        OWF_Screen_GetHeader(port->screenNumber, &screen);

        if (screen.normal.width != img->width ||
            screen.normal.height != img->height) {
            OWF_Screen_Resize(port->screenNumber, img->width, img->height);
        }

        OWF_Screen_Blit(port->screenNumber,
                        port->surface[port->frameBuffer]->data, OWF_ROTATION_0);
    }
    OWF_Mutex_Unlock(&port->frMutex);
}

/* ================================================================== */
/*           T E S T   I M A G E   E X P O R T                        */
/* ================================================================== */

OWF_API_CALL WFDEGLImage OWF_APIENTRY
WFD_Port_AcquireCurrentImage(WFD_PORT *pPort) OWF_APIEXIT {
    if (pPort) {
        OWF_IMAGE *bufferCopy;

        /* If we can get port lock we can be sure that
         *  port has finished rendering. Lock can be released
         *  immediately */
        WFD_Port_AcquireLock(pPort);
        WFD_Port_ReleaseLock(pPort);

        OWF_Mutex_Lock(&pPort->frMutex);
        { bufferCopy = OWF_Image_Copy(pPort->surface[pPort->frameBuffer]); }
        OWF_Mutex_Unlock(&pPort->frMutex);
        return bufferCopy;
    }

    return NULL;
}

#ifdef __cplusplus
}
#endif
