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
 *  \file wfccontext.c
 *
 *  \brief SI Context handling
 */

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>

#include <WF/wfc.h>
#include <EGL/eglext.h>

#include "wfccontext.h"
#include "wfcpipeline.h"

#include "owfscreen.h"
#include "owfdisplaycontextgeneral.h"


/*! maximum number of elements per scene */
#define MAX_ELEMENTS            512
/*! maximum number of scenes per context */
#define MAX_SCENES              3


#define CONTEXT_SCENE_POOL_SIZE     MAX_SCENES
#define CONTEXT_ELEMENT_POOL_SIZE   MAX_SCENES * MAX_ELEMENTS
#define CONTEXT_NODE_POOL_SIZE      2 * CONTEXT_ELEMENT_POOL_SIZE

/*! almost 2^31 */
#define MAX_DELAY               2100000000

/*! 15ms */
#define AUTO_COMPOSE_DELAY      15000
#define FIRST_CONTEXT_HANDLE    2000

#define WAIT_FOREVER            -1

#ifdef __cplusplus
extern "C" {
#endif

static WFCHandle                nextContextHandle = FIRST_CONTEXT_HANDLE;

typedef enum
{
    WFC_MESSAGE_NONE,
    WFC_MESSAGE_QUIT,
    WFC_MESSAGE_COMPOSE,
    WFC_MESSAGE_COMMIT,
    WFC_MESSAGE_FENCE_1_DISPLAY,
    WFC_MESSAGE_FENCE_2_SYNCOBJECT,
    WFC_MESSAGE_ACTIVATE,
    WFC_MESSAGE_DEACTIVATE,
    WFC_MESSAGE_START_COUNTDOWN,
    WFC_MESSAGE_CANCEL
} WFC_MESSAGES;


static void*
WFC_Context_ComposerThread(void* data);


/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
void WFC_CONTEXT_Ctor(void* self)
{
    self = self;
}

/*---------------------------------------------------------------------------*/
static WFCboolean
WFC_Context_CreateState(WFC_CONTEXT* context)
    {   /* Must be called late enough that scratch buffers can be mapped and hardware rotation capability queried */
    OWF_IMAGE_FORMAT        fInt,
                            fExt;
    WFCint                  stride = 0;
    OWF_ASSERT(context);

    DPRINT(("WFC_Context_CreateContextState"));

    owfNativeStreamGetHeader(context->stream,
                             NULL, NULL, &stride, &fExt, NULL);
    /* internal image used as intermediate target */
    fInt.pixelFormat    = OWF_IMAGE_ARGB_INTERNAL;
    fInt.linear         = fExt.linear;
    fInt.premultiplied  = fExt.premultiplied;
    fInt.rowPadding     = 1;
    
     
    if (context->type == WFC_CONTEXT_TYPE_ON_SCREEN)
        {
        /* The unrotated target buffer: Can't get real address without locking for writing!  NO STRIDE */
        context->state.unrotatedTargetImage=OWF_Image_Create(context->targetWidth,context->targetHeight,&fExt,context->scratchBuffer[2],0);
        /* The rotated version of the target buffer for hardware rotation
         * or a de-rotated version of the internal buffer into another scratch buffer for software rotation
         */ 
        if (OWF_Screen_Rotation_Supported(context->screenNumber))
            {   /* The rotated version of the target buffer for hardware rotation */
            context->state.rotatedTargetImage=OWF_Image_Create(context->targetHeight,context->targetWidth,&fExt,context->scratchBuffer[2],0);
            }
        else
            {   /* a de-rotated version of the internal buffer into another scratch buffer for software rotation */
            context->state.rotatedTargetImage=OWF_Image_Create(context->targetWidth,context->targetHeight,&fInt,context->scratchBuffer[1],0);
            }
        }
    else
        {
        /* The unrotated target buffer: Can't get real address without locking for writing!  STRIDE HONOURED */
        context->state.unrotatedTargetImage=OWF_Image_Create(context->targetWidth,context->targetHeight,&fExt,context->scratchBuffer[2],stride);
        /* a de-rotated version of the internal buffer into another scratch buffer for software rotation */
        context->state.rotatedTargetImage=OWF_Image_Create(context->targetWidth,context->targetHeight,&fInt,context->scratchBuffer[1],0);
        }
    /* The internal target buffer composed to for 0 and 180 degree rotation */
    context->state.unrotatedInternalTargetImage=OWF_Image_Create(context->targetWidth,context->targetHeight,&fInt,context->scratchBuffer[0],stride);
    /* The internal target buffer composed to for 90 and 270 degree rotation */
    context->state.rotatedInternalTargetImage=OWF_Image_Create(context->targetHeight,context->targetWidth,&fInt,context->scratchBuffer[0],stride);

    if (context->state.unrotatedTargetImage && context->state.rotatedTargetImage && context->state.unrotatedInternalTargetImage && context->state.rotatedInternalTargetImage)
        {
        return WFC_TRUE;
        }
    else
        {
        return WFC_FALSE;
        }
    }
/*---------------------------------------------------------------------------*/
static void
WFC_Context_DestroyState(WFC_CONTEXT* context)
    {
    /* The unrotated target buffer */ 
    OWF_Image_Destroy(context->state.unrotatedTargetImage);
    /* The rotated version of the target buffer for hardware rotation, 
     * or a de-rotated version of the internal buffer into another scratch buffer for software rotation
     */ 
    OWF_Image_Destroy(context->state.rotatedTargetImage);
    /* The internal target buffer composed to for 0 and 180 degree rotation */
    OWF_Image_Destroy(context->state.unrotatedInternalTargetImage);
    /* The internal target buffer composed to for 90 and 270 degree rotation */
    OWF_Image_Destroy(context->state.rotatedInternalTargetImage);
    
    }
/*---------------------------------------------------------------------------
 * Should only be accessed indirectly by calls to WFC_Device_DestroyContext ot
 * WFC_Device_DestroyContexts
 *----------------------------------------------------------------------------*/
void WFC_CONTEXT_Dtor(void* self)
{
    OWFint                  ii = 0;
    WFC_CONTEXT*            context = NULL;

    OWF_ASSERT(self);
    DPRINT(("WFC_CONTEXT_Dtor(%p)", self));

    context = CONTEXT(self);

    OWF_ASSERT(context);
    
    WFC_Pipeline_DestroyState(context);
    WFC_Context_DestroyState(context);
    
    OWF_MessageQueue_Destroy(&context->composerQueue);

    /* make the stream destroyable */
    owfNativeStreamSetProtectionFlag(context->stream, OWF_FALSE);
    owfNativeStreamDestroy(context->stream);
    context->stream = OWF_INVALID_HANDLE;

    OWF_AttributeList_Destroy(&context->attributes);

    for (ii = 0; ii < SCRATCH_BUFFER_COUNT; ii++)
    {
        OWF_Image_FreeData(&context->scratchBuffer[ii]);
    }

    OWF_DisplayContext_Destroy(context->screenNumber, context->displayContextAdaptation);
    
    OWF_Pool_Destroy(context->scenePool);
    OWF_Pool_Destroy(context->elementPool);
    OWF_Pool_Destroy(context->nodePool);

    OWF_Semaphore_Destroy(&context->compositionSemaphore);
    OWF_Semaphore_Destroy(&context->commitSemaphore);
    OWF_Mutex_Destroy(&context->updateFlagMutex);
    OWF_Mutex_Destroy(&context->sceneMutex);
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL void
WFC_Context_Shutdown(WFC_CONTEXT* context)
{
    OWF_ASSERT(context);
    DPRINT(("WFC_Context_Shutdown(context = %d)", context->handle));

    DPRINT(("Waiting for composer thread termination"));
    OWF_Message_Send(&context->composerQueue, WFC_MESSAGE_QUIT, 0);
    OWF_Thread_Join(context->composerThread, NULL);
    OWF_Thread_Destroy(context->composerThread);
    context->composerThread = NULL;

    if (context->device)
    {
        /* #4604: added guard condition */
        WFC_Device_DestroyContextElements(context->device, context);
        WFC_Device_DestroyContextImageProviders(context->device, context);
    }

    WFC_Scene_Destroy(context->workScene);
    WFC_Scene_Destroy(context->snapshotScene);
    WFC_Scene_Destroy(context->committedScene);
    context->workScene = NULL;
    context->snapshotScene = NULL;
    context->committedScene = NULL;
}

/*---------------------------------------------------------------------------
 *  Initialize context attributes
 *----------------------------------------------------------------------------*/
OWF_API_CALL OWF_ATTRIBUTE_LIST_STATUS
WFC_Context_InitializeAttributes(WFC_CONTEXT* context,
                                 WFCContextType type)
{
    OWF_ATTRIBUTE_LIST_STATUS attribError=ATTR_ERROR_NONE;
    OWF_ASSERT(context);
    /* initialize attributes/properties */
    if (context->stream)
    {
        owfNativeStreamGetHeader(context->stream,
                                 &context->targetWidth,
                                 &context->targetHeight,
                                 NULL, NULL, NULL);
    }
    context->type               = type;
    context->rotation           = WFC_ROTATION_0;
    context->backgroundColor    = 0x000000FF;
    context->lowestElement      = WFC_INVALID_HANDLE;


    OWF_AttributeList_Create(&context->attributes,
                             WFC_CONTEXT_TYPE,
                             WFC_CONTEXT_BG_COLOR);
    attribError=OWF_AttributeList_GetError(&context->attributes);
    if (attribError!=ATTR_ERROR_NONE)
        {
        OWF_ASSERT(attribError==ATTR_ERROR_NO_MEMORY);
        return attribError;
        }

  
    /* The composition code reads the member variables directly, 
     * not via the attribute engine.
     */
    OWF_Attribute_Initi(&context->attributes,
                        WFC_CONTEXT_TYPE,
                        (WFCint*) &context->type,
                        OWF_TRUE);

    OWF_Attribute_Initi(&context->attributes,
                        WFC_CONTEXT_TARGET_WIDTH,
                        &context->targetWidth,
                        OWF_TRUE);

    OWF_Attribute_Initi(&context->attributes,
                        WFC_CONTEXT_TARGET_HEIGHT,
                        &context->targetHeight,
                        OWF_TRUE);

    OWF_Attribute_Initi(&context->attributes,
                        WFC_CONTEXT_ROTATION,
                        (WFCint*) &context->rotation,
                        OWF_FALSE);

    OWF_Attribute_Initi(&context->attributes,
                        WFC_CONTEXT_BG_COLOR,
                        (WFCint*) &context->backgroundColor,
                        OWF_FALSE);

    OWF_Attribute_Initi(&context->attributes,
                        WFC_CONTEXT_LOWEST_ELEMENT,
                        (OWFint*) &context->lowestElement,
                        OWF_TRUE);
    attribError=OWF_AttributeList_GetError(&context->attributes);

	/* After commit to working, writable attribute abstracted variables
	must not be written to directly. */
    OWF_AttributeList_Commit(&context->attributes,
                             WFC_CONTEXT_TYPE,
                             WFC_CONTEXT_BG_COLOR,
		             WORKING_ATTR_VALUE_INDEX );
    return attribError;
}


/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static WFC_CONTEXT*
WFC_Context_Initialize(WFC_CONTEXT* context,
                       WFC_DEVICE* device,
                       WFCNativeStreamType stream,
                       WFCContextType type,
                       WFCint screenNumber)
{
    void*                   scratch[SCRATCH_BUFFER_COUNT];
    OWFint                  err2    = 0;
    OWFint                  ii      = 0,
                            nbufs   = 0;
    OWFint                  fail    = 0;
    OWF_ATTRIBUTE_LIST_STATUS attribStatus = ATTR_ERROR_NONE;
    OWF_ASSERT(context);
    OWF_ASSERT(device);

    DPRINT(("WFC_Context_Initialize(%p,%p,%d,%d)", context, device, type, screenNumber));
    
		context->displayContextAdaptation = OWF_DisplayContext_Create(screenNumber);
     /*the following section of the code to be pushed to adaptation in future*/
    if (type == WFC_CONTEXT_TYPE_ON_SCREEN)
    {
        OWF_IMAGE_FORMAT        imageFormat;
        OWF_SCREEN              screen;
        WFCint width = 0;
        WFCint height = 0;
        WFCint normalSize = 0;
        WFCint flippedSize = 0;
        WFCNativeStreamType stream;
    
        /* Set up stream for sending data to screen */
        
        if (!OWF_Screen_GetHeader(screenNumber, &screen))
        {
            DPRINT(("WFC_Context_Initialize(): Could not retrieve the screen parameters"));
            return NULL;
        }

        /* Set on-screen pixel format */
        imageFormat.pixelFormat     = OWF_SURFACE_PIXEL_FORMAT;
        imageFormat.premultiplied   = OWF_SURFACE_PREMULTIPLIED;
        imageFormat.linear          = OWF_SURFACE_LINEAR;
        imageFormat.rowPadding      = OWF_SURFACE_ROWPADDING;

        width = screen.normal.width;
        height = screen.normal.height;
        
        normalSize = screen.normal.height * screen.normal.stride;
        flippedSize = screen.flipped.height * screen.flipped.stride;
        
        if (flippedSize > normalSize)
            {
            width = screen.flipped.width;
            height = screen.flipped.height;
            }
        
        stream = owfNativeStreamCreateImageStream(width,
                                                  height,
                                                  &imageFormat,
                                                  1);

        if (stream)
        {
            WFC_Context_SetTargetStream(context, stream);
            
            /* At this point the stream's refcount is 2 - we must decrement
             * it by one to ensure that the stream is destroyed when the
             * context (that "owns" it) is destroyed.
             */
            owfNativeStreamRemoveReference(stream);
        }
        else
        {
            DPRINT(("WFC_Context_Initialize(): cannot create internal target stream"));
            return NULL;
        }
    }
    else
    {
        WFC_Context_SetTargetStream(context, stream);
    }
    
    nbufs = SCRATCH_BUFFER_COUNT-1;
    for (ii = 0; ii < nbufs; ii++)
    {
        scratch[ii] = OWF_Image_AllocData(MAX_SOURCE_WIDTH,
                MAX_SOURCE_HEIGHT,
                OWF_IMAGE_ARGB_INTERNAL);
        fail = fail || (scratch[ii] == NULL);
    }

    /*
     * allocate one-channel buffer for alpha
     * obs! this assumes sizeof(OWFsubpixel) is 4.
     */
    scratch[nbufs] = OWF_Image_AllocData(MAX_SOURCE_WIDTH,
                                         MAX_SOURCE_HEIGHT,
                                         OWF_IMAGE_L32);
    fail = fail || (scratch[nbufs] == NULL);

    err2 = OWF_MessageQueue_Init(&context->composerQueue);
    fail = fail || (err2 != 0);

    if (fail)
    {
        OWF_MessageQueue_Destroy(&context->composerQueue);

        for (ii = 0; ii < SCRATCH_BUFFER_COUNT; ii++)
        {
            OWF_Image_FreeData(&scratch[ii]);
        }
        return NULL;
    }

    context->type               = type;
    context->device             = device;
    context->handle             = nextContextHandle;
    context->composerQueue      = context->composerQueue;
    context->screenNumber       = screenNumber;
    context->activationState    = WFC_CONTEXT_STATE_PASSIVE;
    context->sourceUpdateCount  = 0;
    ++nextContextHandle;

    for (ii = 0; ii < SCRATCH_BUFFER_COUNT; ii++)
    {
        context->scratchBuffer[ii] = scratch[ii];
    }
    
    if (!WFC_Pipeline_CreateState(context) || !WFC_Context_CreateState(context))
         {
         DPRINT(("WFC_Context_Initialize(): Could not create pipeline state object"));
         return NULL;
         }
    if (    OWF_Semaphore_Init(&context->compositionSemaphore, 1)
        ||  OWF_Semaphore_Init(&context->commitSemaphore, 1)
        ||  OWF_Mutex_Init(&context->updateFlagMutex)
        ||  OWF_Mutex_Init(&context->sceneMutex)
        
        )
        {
        DPRINT(("WFC_Context_Initialize(): Could not create mutexes and semaphores!"));
        return NULL;
        }

    attribStatus= WFC_Context_InitializeAttributes(context, type);

    if (attribStatus!=ATTR_ERROR_NONE)
        {
        return NULL;
        }

    context->scenePool = OWF_Pool_Create(sizeof(WFC_SCENE),
                                         CONTEXT_SCENE_POOL_SIZE);
    context->elementPool = OWF_Pool_Create(sizeof(WFC_ELEMENT),
                                           CONTEXT_ELEMENT_POOL_SIZE);
    context->nodePool = OWF_Pool_Create(sizeof(OWF_NODE),
                                        CONTEXT_NODE_POOL_SIZE);
    if (!( context->scenePool &&
          context->nodePool && context->elementPool))
    {
        /* must call these to remove references to context */

        context->workScene = NULL;
        context->committedScene = NULL;
        return NULL;
    }

    DPRINT(("  Creating scenes"));
    context->workScene = WFC_Scene_Create(context);
    context->committedScene = WFC_Scene_Create(context);
    context->snapshotScene = NULL;

    /* snapshotScene is initialized in InvokeCommit */

    /* context's refcount is now 3 */

    if (!(context->workScene && context->committedScene &&
          context->nodePool && context->elementPool))
    {
        /* must call these to remove references to context */
        WFC_Scene_Destroy(context->workScene);
        WFC_Scene_Destroy(context->committedScene);
        context->workScene = NULL;
        context->committedScene = NULL;
        return NULL;
    }


    context->composerThread = OWF_Thread_Create(WFC_Context_ComposerThread,
                                                context);
    if (!(context->composerThread))
        {
        /* must call these to remove references to context */
        WFC_Scene_Destroy(context->workScene);
        WFC_Scene_Destroy(context->committedScene);
        context->workScene = NULL;
        context->committedScene = NULL;
        return NULL;
        }
    
    return context;
}

/*---------------------------------------------------------------------------
 *  Create new context on device
 *
 *  \param device Device on which the context should be created
 *  \param type Context type (on- or off-screen)
 *
 *  \return New context object or NULL in case of failure
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFC_CONTEXT*
WFC_Context_Create(WFC_DEVICE* device,
                   WFCNativeStreamType stream,
                   WFCContextType type,
                   WFCint screenNum)
{
    WFC_CONTEXT*            context = NULL;

    OWF_ASSERT(device);
   context = CREATE(WFC_CONTEXT);

    if (context)
    {
        if (!WFC_Context_Initialize(context, device, stream, type, screenNum))
        {
            DESTROY(context);
        }
    }
    return context;
}

/*---------------------------------------------------------------------------
 *  Setup context rendering target
 *
 *  \param context Context
 *  \param stream Target stream to use for rendering
 *----------------------------------------------------------------------------*/
OWF_API_CALL void
WFC_Context_SetTargetStream(WFC_CONTEXT* context,
                            WFCNativeStreamType stream)
{
    OWF_ASSERT(context);
    context->stream = stream;

    owfNativeStreamAddReference(stream);

    owfNativeStreamGetHeader(stream,
                             &context->targetWidth, &context->targetHeight,
                             NULL, NULL, NULL);
}

/*---------------------------------------------------------------------------
 *  Find element from current scene
 *
 *  \param context Context object
 *  \param element Element to find
 *
 *  \return Element object or NULL if element hasn't been inserted
 *  into current scene.
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFC_ELEMENT*
WFC_Context_FindElement(WFC_CONTEXT* context,
                        WFCElement element)
{
    OWF_ASSERT(context);
    return WFC_Scene_FindElement(context->workScene, element);
}

/*---------------------------------------------------------------------------
 *  Commit context scene graph changes
 *
 *  \param context Context to commit
 *----------------------------------------------------------------------------*/
static void
WFC_Context_DoCommit(WFC_CONTEXT* context)
{
    OWF_ASSERT(context);
    DPRINT(("WFC_Context_DoCommit(context = %p)", context));

    OWF_ASSERT(context->snapshotScene);


    DPRINT(("COMMIT: Committing attribute list changes"));

    DPRINT(("COMMIT: Acquiring mutex"));
    OWF_Mutex_Lock(&context->sceneMutex);

    /* comitting scene attribute changes */
    DPRINT(("COMMIT: Committing scene attribute changes"));
    OWF_AttributeList_Commit(&context->attributes,
                             WFC_CONTEXT_TYPE,
                             WFC_CONTEXT_BG_COLOR,COMMITTED_ATTR_VALUE_INDEX);


    /* resolve sources and masks */
    DPRINT(("COMMIT: Committing scene changes"));
    WFC_Scene_Commit(context->snapshotScene);
    DPRINT(("COMMIT: Destroying old committed scene"));
    WFC_Scene_Destroy(context->committedScene);
    DPRINT(("COMMIT: Setting new snapshot scene as committed one."));
    context->committedScene = context->snapshotScene;
    context->snapshotScene = NULL;

    DPRINT(("COMMIT: Unlocking mutex"));
    OWF_Mutex_Unlock(&context->sceneMutex);

    DPRINT(("COMMIT: Signaling commit semaphore"));
    /* signal we're ready */
    OWF_Semaphore_Post(&context->commitSemaphore);
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static WFCboolean
WFC_Context_LockTargetForWriting(WFC_CONTEXT* context)
{
    OWF_ASSERT(context);

    DPRINT(("WFC_Context_LockTargetForWriting"));

    context->state.targetBuffer =
        owfNativeStreamAcquireWriteBuffer(context->stream);
    context->state.targetPixels =
        owfNativeStreamGetBufferPtr(context->stream,
                                    context->state.targetBuffer);

    if ((WFC_ROTATION_0 == context->rotation || WFC_ROTATION_180 == context->rotation) ||
        !OWF_Screen_Rotation_Supported(context->screenNumber))
    {
        /* final target, in target format */
        context->state.targetImage =context->state.unrotatedTargetImage;
    }
    else
    {
        /* final target, in target format */
        /* fExt stride/pading value is the unrotated value, which may not be correct */
        context->state.targetImage = context->state.rotatedTargetImage;
    }
    OWF_Image_SetPixelBuffer(context->state.targetImage,context->state.targetPixels);
    
    if (context->state.targetImage==NULL)
        {
        return WFC_FALSE;
        }

    /* take context rotation into account. */
    if (WFC_ROTATION_0   == context->rotation ||
        WFC_ROTATION_180 == context->rotation)
    {
        context->state.internalTargetImage = context->state.unrotatedInternalTargetImage;
    }
    else
    {
        context->state.internalTargetImage = context->state.rotatedInternalTargetImage;
    }

    if (context->state.internalTargetImage==NULL)
        {
        return WFC_FALSE;
        }
    return WFC_TRUE;
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static void
WFC_Context_UnlockTarget(WFC_CONTEXT* context)
{
    OWFNativeStreamBuffer   frontBuffer = OWF_INVALID_HANDLE;
    void*                   pixelDataPtr = NULL;
    OWF_ROTATION rotation;

    OWF_ASSERT(context);
    DPRINT(("WFC_Context_UnlockTarget"));
    DPRINT(("  Unlocking target stream=%d, buffer=%d",
            context->stream, context->state.targetBuffer));

    owfNativeStreamReleaseWriteBuffer(context->stream,
                                      context->state.targetBuffer,
                                      EGL_NO_DISPLAY,
                                      NULL);

    
    /* Refactor the code that follows so that it is triggered by the above releasewrite */
    
    /* Acquire target stream front buffer and blit to SDL screen */
    frontBuffer = owfNativeStreamAcquireReadBuffer(context->stream);
    DPRINT(("  Locking target stream=%d, buffer=%d",
            context->stream, frontBuffer));

    pixelDataPtr = owfNativeStreamGetBufferPtr(context->stream, frontBuffer);

    switch (context->rotation)
    {
        case WFC_ROTATION_0:
        {
            rotation = OWF_ROTATION_0;
            break;
        }
        case WFC_ROTATION_90:
        {
            rotation = OWF_ROTATION_90;
            break;
        }
        case WFC_ROTATION_180:
        {
            rotation = OWF_ROTATION_180;
            break;
        }
        case WFC_ROTATION_270:
        {
            rotation = OWF_ROTATION_270;
            break;
        }
        default:
        {
            OWF_ASSERT(0);
        }
    }
    
    OWF_Screen_Blit(context->screenNumber, pixelDataPtr, rotation);

    owfNativeStreamReleaseReadBuffer(context->stream, frontBuffer);
    DPRINT(("  Releasing target stream=%d, buffer=%d",
            context->stream, frontBuffer));

}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static void
WFC_Context_PrepareComposition(WFC_CONTEXT* context)
{
    OWFsubpixel             r, g, b, a;

    OWF_ASSERT(context);

    /* the first thing to do is to lock target stream and fetch
       write buffer to it. fetching blocks until one is available,
       but since only one stream can be target to only one context
       at the time, no stalls should occur */
    if (!WFC_Context_LockTargetForWriting(context))
        {
        OWF_ASSERT(!"Couldn't lock");
        }

    /* prepare for composition by "clearing the table" with
       background color.  */

    r = (OWFsubpixel) OWF_RED_MAX_VALUE * ((context->backgroundColor >> 24) &
        0xFF) / OWF_BYTE_MAX_VALUE;
    g = (OWFsubpixel) OWF_GREEN_MAX_VALUE * ((context->backgroundColor >> 16) &
        0xFF) / OWF_BYTE_MAX_VALUE;
    b = (OWFsubpixel) OWF_BLUE_MAX_VALUE * ((context->backgroundColor >> 8) &
        0xFF) / OWF_BYTE_MAX_VALUE;
    a = (OWFsubpixel) OWF_ALPHA_MAX_VALUE * (context->backgroundColor & 0xFF) /
        OWF_BYTE_MAX_VALUE;

    r = r * a / OWF_ALPHA_MAX_VALUE;
    g = g * a / OWF_ALPHA_MAX_VALUE;
    b = b * a / OWF_ALPHA_MAX_VALUE;

    OWF_Image_Clear(context->state.internalTargetImage, r, g, b, a);

    WFC_Scene_LockSourcesAndMasks(context->committedScene);
}



/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static void
WFC_Context_FinishComposition(WFC_CONTEXT* context)
{
    OWF_ROTATION            rotation = OWF_ROTATION_0;
    OWFint                  screenNumber;
    OWFboolean              screenRotation;

    OWF_ASSERT(context);

    screenNumber = context->screenNumber;
    screenRotation = OWF_Screen_Rotation_Supported(screenNumber);
    /* re-use scratch buffer 1 for context rotation */
    if (WFC_ROTATION_0   == context->rotation || screenRotation)
    {
 
        if (screenRotation)
        {
            if (WFC_ROTATION_90   == context->rotation || WFC_ROTATION_270   == context->rotation)
                {
                owfSetStreamFlipState(context->stream, OWF_TRUE);
                }
            else
                {
                owfSetStreamFlipState(context->stream, OWF_FALSE);
                }
        }
        OWF_Image_DestinationFormatConversion(context->state.targetImage, context->state.internalTargetImage);
    }
    else
    {
        switch (context->rotation)
        {
            case WFC_ROTATION_0:
            {
                rotation = OWF_ROTATION_0;
                break;
            }
            case WFC_ROTATION_90:
            {
                rotation = OWF_ROTATION_90;
                break;
            }
            case WFC_ROTATION_180:
            {
                rotation = OWF_ROTATION_180;
                break;
            }
            case WFC_ROTATION_270:
            {
                rotation = OWF_ROTATION_270;
                break;
            }
            default:
            {
                OWF_ASSERT(0);
            }
        }
     
        /* rotate */
        OWF_Image_Rotate(context->state.rotatedTargetImage,
                         context->state.internalTargetImage,
                         rotation);

        /* Note: support of different target formats  can be put here */

        OWF_Image_DestinationFormatConversion(context->state.targetImage, context->state.rotatedTargetImage);
    }
    WFC_Context_UnlockTarget(context);
    WFC_Scene_UnlockSourcesAndMasks(context->committedScene);
}

/*!---------------------------------------------------------------------------
 * \brief Actual composition routine.
 *  Mainly just calls other functions that executes different stages of
 *  the composition pipeline.
 *  \param context Context to compose.
 *----------------------------------------------------------------------------*/
static void
WFC_Context_DoCompose(WFC_CONTEXT* context)
{
    WFC_SCENE*              scene   = NULL;
    OWF_NODE*               node    = NULL;

    OWF_ASSERT(context);

    OWF_Mutex_Lock(&context->updateFlagMutex);
    context->sourceUpdateCount = 0;
    OWF_Mutex_Unlock(&context->updateFlagMutex);

    WFC_Context_PrepareComposition(context);

    DPRINT(("WFC_Context_Compose"));
    /* Composition always uses the committed version
     * of the scene.
     */

    OWF_Mutex_Lock(&context->sceneMutex);

    scene = context->committedScene;
    OWF_ASSERT(scene);

    for (node = scene->elements; NULL != node; node = node->next)
    {
        
        WFC_ELEMENT*            element = NULL;
        WFC_ELEMENT_STATE*      elementState = NULL;
        element = ELEMENT(node->data);

        if (element->skipCompose)
        {
             /* this element is somehow degraded, its source is missing or
              * something else; skip to next element */
            DPRINT(("  *** Skipping element %d", element->handle));
            continue;
        }

        DPRINT(("  Composing element %d", element->handle));

        /* BeginComposition may fail e.g. if the element's destination
         * rectangle is something bizarre, i.e. causes overflows or
         * something.
         */
        if ((elementState=WFC_Pipeline_BeginComposition(context, element))!=NULL)
        {
            WFC_Pipeline_ExecuteSourceConversionStage(context, elementState);
            WFC_Pipeline_ExecuteCropStage(context, elementState);
            WFC_Pipeline_ExecuteFlipStage(context, elementState);
            WFC_Pipeline_ExecuteRotationStage(context, elementState);
            WFC_Pipeline_ExecuteScalingStage(context, elementState);
            WFC_Pipeline_ExecuteBlendingStage(context, elementState);

            WFC_Pipeline_EndComposition(context, element,elementState);
        }
    }

    WFC_Context_FinishComposition(context);

    OWF_Mutex_Unlock(&context->sceneMutex);

    OWF_Semaphore_Post(&context->compositionSemaphore);
}

/*---------------------------------------------------------------------------
 *  Activate/deactivate auto-composition on context
 *
 *  \param context Context
 *  \param act Auto-composition enable/disable
 *----------------------------------------------------------------------------*/
OWF_API_CALL void
WFC_Context_Activate(WFC_CONTEXT* context,
                     WFCboolean act)
{
    OWF_ASSERT(context);

    DPRINT(("WFC_Context_Activate: %s", (act) ? "activate" : "deactivate"));

    if (act && !WFC_Context_Active(context))
    {
        DPRINT(("WFC_Context_Activate: WFC_CONTEXT_STATE_PASSIVE: activating"));
        context->activationState = WFC_CONTEXT_STATE_ACTIVATING;

        /* moved from composer loop - updates must be allowed
         * immediately after activation
         */
        WFC_Device_EnableContentNotifications(context->device,
                                              context,
                                              WFC_TRUE);

        OWF_Message_Send(&context->composerQueue,
                         WFC_MESSAGE_ACTIVATE,
                         0);
    }
    else if (!act && WFC_Context_Active(context))
    {
        DPRINT(("WFC_Context_Activate: WFC_CONTEXT_STATE_ACTIVE: deactivating"));
        context->activationState = WFC_CONTEXT_STATE_DEACTIVATING;
        OWF_Message_Send(&context->composerQueue,
                         WFC_MESSAGE_DEACTIVATE,
                         0);
    }
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCboolean
WFC_Context_InvokeComposition(WFC_DEVICE* device,
                              WFC_CONTEXT* context,
                              WFCboolean wait)
{
    WFCint              status = 0;

    OWF_ASSERT(context);
    OWF_ASSERT(device);

    device = device; /* suppress the compiler warning */

    status = OWF_Semaphore_TryWait(&context->compositionSemaphore);
    if (status)
    {
        if (!wait)
        {
            /* busy; processing last request */
            return WFC_FALSE;
        }
        /* wait previous frame composition to finish */
        OWF_Semaphore_Wait(&context->compositionSemaphore);
    }

    /* compositionSemaphore is posted/signaled in WFC_Context_Compose()
    after frame has been successfully composed */
    OWF_Message_Send(&context->composerQueue, WFC_MESSAGE_COMPOSE, 0);

    return WFC_TRUE;
}

OWF_API_CALL WFCErrorCode
WFC_Context_InvokeCommit(WFC_DEVICE* device,
                         WFC_CONTEXT* context,
                         WFCboolean wait)
{
    WFCint              status = 0;

    OWF_ASSERT(context);
    OWF_ASSERT(device);

    device = device; /* suppress the compiler warning */

    /* first check if there're inconsistensies in the scene */
    if (WFC_Scene_HasConflicts(context->workScene))
    {
        DPRINT(("WFC_Context_InvokeCommit: scene has inconsistensies"));
        return WFC_ERROR_INCONSISTENCY;
    }

    /* then commit - always asynchronously */
    status = OWF_Semaphore_TryWait(&context->commitSemaphore);
    DPRINT(("COMMIT: Commit semaphore status = %d", status));
    if (status)
    {
        if (!wait)
        {
            DPRINT(("COMMIT: Busy; exiting."));
            /* busy; processing last commit */
            return WFC_ERROR_BUSY;
        }

        DPRINT(("COMMIT: Waiting for previous commit to finish."));
        /* wait previous commit to finish */
        OWF_Semaphore_Wait(&context->commitSemaphore);
    }
    /* comitting scene attribute changes */
    DPRINT(("COMMIT: Cloning scene attribute changes"));
    OWF_AttributeList_Commit(&context->attributes,
                             WFC_CONTEXT_TYPE,
                             WFC_CONTEXT_BG_COLOR,SNAPSHOT_ATTR_VALUE_INDEX);

    DPRINT(("COMMIT: Cloning scene"));
    /* take snapshot of the current working copy - it will
     * be the new committed scene */
    context->snapshotScene = WFC_Scene_Clone(context->workScene);

    DPRINT(("COMMIT: Sending commit request"));
    /* invoke async commit */
    OWF_Message_Send(&context->composerQueue, WFC_MESSAGE_COMMIT, 0);
    return WFC_ERROR_NONE;
}

/*---------------------------------------------------------------------------
 *  \param device
 *  \param context
 *  \param semaphore
 *----------------------------------------------------------------------------*/
OWF_API_CALL void
WFC_Context_InsertFence(WFC_CONTEXT* context,
			WFCEGLDisplay dpy,
                        WFCEGLSync sync)
{
    OWF_ASSERT(context);
    OWF_ASSERT(sync);

    DPRINT(("WFC_Context_InsertFence: Sending fence sync: 0x%08x", sync));

    OWF_Message_Send(&context->composerQueue, WFC_MESSAGE_FENCE_1_DISPLAY, (void*)dpy);
    OWF_Message_Send(&context->composerQueue, WFC_MESSAGE_FENCE_2_SYNCOBJECT, sync);
}

/*---------------------------------------------------------------------------
 *  Insert element into context's scene
 *
 *  \param context
 *  \param element
 *  \param subordinate
 *
 *  \return WFCErrorCode
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode
WFC_Context_InsertElement(WFC_CONTEXT* context,
                          WFCElement element,
                          WFCElement subordinate)
{
    WFC_ELEMENT*            object = NULL;
    WFCErrorCode            result = WFC_ERROR_BAD_HANDLE;

    OWF_ASSERT(context);

    object = WFC_Device_FindElement(context->device, element);

    if (NULL != object && CONTEXT(object->context) == context)
    {
        /* set the sharing flag as the element will be
         * shared between the device and working copy scene.
         * this is to tell the scene that it must not destroy
         * this element.
         */
        object->shared = WFC_TRUE;
        result = WFC_Scene_InsertElement(context->workScene,
                                         object,
                                         subordinate);

        context->lowestElement = WFC_Scene_LowestElement(context->workScene);
    }
    return result;
}

/*---------------------------------------------------------------------------
 *  Remove element from context's scene
 *
 *  \param context
 *  \param element
 *
 *  \return WFCErrorCode
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode
WFC_Context_RemoveElement(WFC_CONTEXT* context,
                          WFCElement element)
{
    WFCErrorCode            err = WFC_ERROR_BAD_HANDLE;
    WFC_ELEMENT*            elemento = NULL;

    OWF_ASSERT(context);

    elemento = WFC_Context_FindElement(context, element);

    if (elemento)
    {
        WFC_Scene_RemoveElement(context->workScene, element);
        /* the element is no longer shared, as it only resides
         * in device from this point on
         */
        elemento->shared = WFC_FALSE;
        context->lowestElement = WFC_Scene_LowestElement(context->workScene);

        err = WFC_ERROR_NONE;
    }

    return err;
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode
WFC_Context_GetElementAbove(WFC_CONTEXT* context,
                            WFCElement element,
                            WFCElement* result)
{
    WFC_ELEMENT*            object = NULL;
    WFCErrorCode            error = WFC_ERROR_ILLEGAL_ARGUMENT;

    OWF_ASSERT(context);
    OWF_ASSERT(result);

    object = WFC_Context_FindElement(context, element);

    if (object)
    {
        WFCElement          temp;

        temp = WFC_Scene_GetNeighbourElement(context->workScene, element, 1);
        *result = temp;
        error = WFC_ERROR_NONE;
    }
    return error;
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode
WFC_Context_GetElementBelow(WFC_CONTEXT* context,
                            WFCElement element,
                            WFCElement* result)
{
    WFC_ELEMENT*            object = NULL;
    WFCErrorCode            error = WFC_ERROR_ILLEGAL_ARGUMENT;

    OWF_ASSERT(context);
    OWF_ASSERT(result);

    object = WFC_Context_FindElement(context, element);
    if (object)
    {
        WFCElement          temp;

        temp = WFC_Scene_GetNeighbourElement(context->workScene, element, -1);
        *result = temp;
        error = WFC_ERROR_NONE;
    }
    return error;
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode
WFC_Context_GetAttribi(WFC_CONTEXT* context,
                       WFCContextAttrib attrib,
                       WFCint* value)
{
    WFCint                  temp = 0;
    OWF_ATTRIBUTE_LIST_STATUS   err;
    WFCErrorCode            result = WFC_ERROR_NONE;

    OWF_ASSERT(context);
    OWF_ASSERT(value);

    temp    = OWF_Attribute_GetValuei(&context->attributes, attrib);
    err     = OWF_AttributeList_GetError(&context->attributes);

    if (err != ATTR_ERROR_NONE)
    {
        result = WFC_ERROR_BAD_ATTRIBUTE;
    }
    else
    {
        *value = temp;
    }
    return result;
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode
WFC_Context_SetAttribi(WFC_CONTEXT* context,
                       WFCContextAttrib attrib,
                       WFCint value)
{
    WFCErrorCode                result = WFC_ERROR_NONE;

    OWF_ASSERT(context);

    /* check value */
    switch (attrib)
    {
        case WFC_CONTEXT_BG_COLOR:
        {
            OWFint              alpha;

            /*
             *  Color format is RGBA NOT RGBA.
             */
            alpha = value & 0xFF;

            if (WFC_CONTEXT_TYPE_ON_SCREEN == context->type)
            {
                /* the only allowed value for on-screen context
                 * background alpha is 255 */
                if (alpha != 255)
                {
                    result = WFC_ERROR_ILLEGAL_ARGUMENT;
                }
            }
            break;
        }

        case WFC_CONTEXT_ROTATION:
        {
            if (!(WFC_ROTATION_0 == value ||
                  WFC_ROTATION_90 == value ||
                  WFC_ROTATION_180 == value ||
                  WFC_ROTATION_270 == value))
            {
               result = WFC_ERROR_ILLEGAL_ARGUMENT;
            }
            break;
        }

        case WFC_CONTEXT_TYPE:
        case WFC_CONTEXT_TARGET_HEIGHT:
        case WFC_CONTEXT_TARGET_WIDTH:
        case WFC_CONTEXT_LOWEST_ELEMENT:
        {
            result = WFC_ERROR_BAD_ATTRIBUTE;
            break;
        }

        case WFC_CONTEXT_FORCE_32BIT:
        default:
        {
            result = WFC_ERROR_BAD_ATTRIBUTE;
            break;
        }
    }

    if (WFC_ERROR_NONE == result)
    {
        OWF_ATTRIBUTE_LIST_STATUS   error;

        /* try changing the value */
        OWF_Attribute_SetValuei(&context->attributes, attrib, value);
        error = OWF_AttributeList_GetError(&context->attributes);

        /* transform errors */
        switch (error) {
            case ATTR_ERROR_ACCESS_DENIED:
            case ATTR_ERROR_INVALID_ATTRIBUTE:
            {
                result = WFC_ERROR_BAD_ATTRIBUTE;
                break;
            }
            default:
            {
                break;
            }
        }
    }

    return result;
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode
WFC_Context_GetAttribfv(WFC_CONTEXT* context,
                        WFCContextAttrib attrib,
                        WFCint count,
                        WFCfloat* values)
{
    WFCErrorCode            result = WFC_ERROR_NONE;

    OWF_ASSERT(context);
    OWF_ASSERT(values);

    switch (attrib)
    {
        case WFC_CONTEXT_BG_COLOR:
        {
            if (4 != count)
            {
                result = WFC_ERROR_ILLEGAL_ARGUMENT;
            }
            else
            {
                OWFuint32      color;
                OWF_ATTRIBUTE_LIST_STATUS   err;

                color = OWF_Attribute_GetValuei(&context->attributes, attrib);
                err = OWF_AttributeList_GetError(&context->attributes);

                if (err != ATTR_ERROR_NONE)
                {
                    result = WFC_ERROR_BAD_ATTRIBUTE;
                    break;
                }

                /* extract color channels and convert to float */
                values[0] = (WFCfloat) (color >> 24) /
                            (WFCfloat) OWF_BYTE_MAX_VALUE;
                values[1] = (WFCfloat) ((color >> 16) & 0xFF) /
                            (WFCfloat) OWF_BYTE_MAX_VALUE;
                values[2] = (WFCfloat) ((color >> 8) & 0xFF) /
                            (WFCfloat) OWF_BYTE_MAX_VALUE;
                values[3] = (WFCfloat) (color & 0xFF) /
                            (WFCfloat) OWF_BYTE_MAX_VALUE;
            }
            break;
        }

        default:
        {
            result = WFC_ERROR_BAD_ATTRIBUTE;
            break;
        }
    }

    return result;
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCErrorCode
WFC_Context_SetAttribfv(WFC_CONTEXT* context,
                        WFCContextAttrib attrib,
                        WFCint count,
                        const WFCfloat* values)
{
    WFCErrorCode            result = WFC_ERROR_NONE;

    OWF_ASSERT(context);
    OWF_ASSERT(values);

    switch (attrib)
    {
        case WFC_CONTEXT_BG_COLOR:
        {
            if (4 != count)
            {
                result = WFC_ERROR_ILLEGAL_ARGUMENT;
            }
            else
            {
                OWFuint32    color;

                /* Every color component value must fall within range [0, 1] */
                if (INRANGE(values[0], 0.0f, 1.0f) &&
                    INRANGE(values[1], 0.0f, 1.0f) &&
                    INRANGE(values[2], 0.0f, 1.0f) &&
                    INRANGE(values[3], 0.0f, 1.0f))
                {
                    color = (((OWFuint32) floor(values[0] * 255)) << 24) |
                            (((OWFuint32) floor(values[1] * 255)) << 16) |
                            (((OWFuint32) floor(values[2] * 255)) << 8) |
                            ((OWFuint32) floor(values[3] * 255));

                    /* delegate to integer accessor - it'll check the
                     * the rest of the value and update it eventually  */
                    result = WFC_Context_SetAttribi(context, attrib, color);
                }
                else
                {
                    result = WFC_ERROR_ILLEGAL_ARGUMENT;
                }
            }
            break;
        }

        default:
        {
            result = WFC_ERROR_BAD_ATTRIBUTE;
            break;
        }
    }

    return result;
}


static void
WFC_Context_AutoComposer(WFC_CONTEXT* context)
{
    OWF_Mutex_Lock(&context->updateFlagMutex);
    if (context->sourceUpdateCount > 0)
    {
        DPRINT(("WFC_Context_ComposerThread: %d sources were updated, "
                "invoking composition\n",
                context->sourceUpdateCount));

        OWF_Mutex_Unlock(&context->updateFlagMutex);
        WFC_Context_DoCompose(context);
    }
    else
    {
        OWF_Mutex_Unlock(&context->updateFlagMutex);
    }
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
static void*
WFC_Context_ComposerThread(void* data)
{
    WFC_CONTEXT*            context = (WFC_CONTEXT*) data;
    OWF_MESSAGE             msg;

    OWF_ASSERT(context);
    DPRINT(("WFC_Context_ComposerThread starting"));

    memset(&msg, 0, sizeof(OWF_MESSAGE));

    while (context->device && msg.id != WFC_MESSAGE_QUIT)
    {
        OWFint              err = -1;

        if (WFC_CONTEXT_STATE_ACTIVE == context->activationState)
        {
            err = OWF_Message_Wait(&context->composerQueue,
                                   &msg,
                                   AUTO_COMPOSE_DELAY);

            WFC_Context_AutoComposer(context);
        }
        else
        {
            DPRINT(("  ComposerThread waiting for message"));
            err = OWF_Message_Wait(&context->composerQueue, &msg, WAIT_FOREVER);
        }

        if (0 == err)
        {
            switch (msg.id)
            {
                case WFC_MESSAGE_ACTIVATE:
                {
                    DPRINT(("****** ENABLING AUTO-COMPOSITION ******"));
                    context->activationState = WFC_CONTEXT_STATE_ACTIVE;
                    break;
                }

                case WFC_MESSAGE_DEACTIVATE:
                {
                    /* cancel possible countdown so that update won't occur
                     * after deactivation */
                    DPRINT(("****** DISABLING AUTO-COMPOSITION ******"));
                    WFC_Device_EnableContentNotifications(context->device,
                                                          context,
                                                          WFC_FALSE);
                    context->activationState = WFC_CONTEXT_STATE_PASSIVE;
                    break;
                }

                case WFC_MESSAGE_COMMIT:
                {
                    DPRINT(("****** COMMITTING SCENE CHANGES ******"));

                    DPRINT(("COMMIT: Invoking DoCommit"));
                    WFC_Context_DoCommit(context);

                    if (!WFC_Context_Active(context))
                    {
                        DPRINT(("COMMIT: Context is inactive, composition "
                                "not needed.", context->handle));
                        break;
                    }
                    else
                    {
                        /* context is active; compose immediately after
                         * commit has completed */

                        DPRINT(("COMMIT: Invoking composition after commit"));
                    }
                    /* FLOW THROUGH */
                }

                case WFC_MESSAGE_COMPOSE:
                {
                    DPRINT(("****** COMPOSING SCENE ******"));
                    WFC_Context_DoCompose(context);

                    break;
                }

                case WFC_MESSAGE_FENCE_1_DISPLAY:
                {
                    DPRINT(("****** STORING EGLDISPLAY (%p) ******", msg.data));

                    context->nextSyncObjectDisplay = (WFCEGLDisplay)msg.data;
                    break;
                }

                case WFC_MESSAGE_FENCE_2_SYNCOBJECT:
                {
                    DPRINT(("****** BREAKING FENCE (%p) ******", msg.data));

                    eglSignalSyncKHR(context->nextSyncObjectDisplay,
                                     (WFCEGLSync) msg.data,
                                     EGL_SIGNALED_KHR);
                    break;
                }
            }
        }
    }

    DPRINT(("WFC_Context_ComposerThread terminating"));
    OWF_Thread_Exit(NULL);
    return NULL;
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL void
WFC_Context_SourceStreamUpdated(OWFNativeStreamType stream,
                                OWFNativeStreamEvent event,
                                void* data)
{
    WFC_CONTEXT*            context = NULL;


    DPRINT(("WFC_Context_SourceStreamUpdated(%p, %x, %p)",
            stream, event, data));

    stream = stream; /* suppress compiler warning */
    OWF_ASSERT(data);
    context = CONTEXT(data);
    OWF_ASSERT(context);

    if (OWF_STREAM_UPDATED == event && WFC_Context_Active(context))
    {
        OWF_Mutex_Lock(&context->updateFlagMutex);
        ++context->sourceUpdateCount;
        OWF_Mutex_Unlock(&context->updateFlagMutex);
    }
}

/*---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL WFCboolean
WFC_Context_Active(WFC_CONTEXT* context)
{
    OWF_ASSERT(context);

    return WFC_CONTEXT_STATE_ACTIVE == context->activationState ||
           WFC_CONTEXT_STATE_ACTIVATING == context->activationState;
}

#ifdef __cplusplus
}
#endif

