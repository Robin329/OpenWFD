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
 *  \file wfdstructs.h
 *  \brief Display SI internal data structure definitions
 */

#ifndef WFDSTRUCTS_H_
#define WFDSTRUCTS_H_

#include <WF/wfd.h>

#include "owfarray.h"
#include "owfattributes.h"
#include "owfcond.h"
#include "owfhash.h"
#include "owfimage.h"
#include "owflinkedlist.h"
#include "owfmessagequeue.h"
#include "owfmutex.h"
#include "owfstream.h"
#include "owfthread.h"
#include "owftypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WFD_SOURCE_IMAGE = 0xA000,
    WFD_SOURCE_STREAM
} WFD_IMAGE_PROVIDER_SOURCE_TYPE;

typedef enum {
    WFD_IMAGE_SOURCE = 0xB000,
    WFD_IMAGE_MASK
} WFD_IMAGE_PROVIDER_TYPE;

/* forward references */
typedef struct WFD_DEVICE_ WFD_DEVICE;
typedef struct WFD_PIPELINE_ WFD_PIPELINE;
typedef struct WFD_PORT_ WFD_PORT;
typedef struct WFD_PIPELINE_BINDINGS_ WFD_PIPELINE_BINDINGS;

/* ======================================================== */
/*   I M A G E   P R O V I D E R S                          */
/* ======================================================== */

/*! Run-time reprentation of an image provider.
 *  This structure is used for stream and EGLimage type
 *  of providers, and also when a provider is used as source
 *  or as mask.
 */
typedef struct WFD_IMAGE_PROVIDER_ {
    WFD_DEVICE *device;
    WFD_PIPELINE *pipeline;
    WFDHandle handle;
    WFD_IMAGE_PROVIDER_TYPE type;
    WFD_IMAGE_PROVIDER_SOURCE_TYPE sourceType;
    union {
        OWF_IMAGE *image;
        OWF_STREAM *stream;
    } source;
} WFD_IMAGE_PROVIDER;

typedef struct WFD_IMAGE_PROVIDER_ WFD_SOURCE;
typedef struct WFD_IMAGE_PROVIDER_ WFD_MASK;

/* ======================================================== */
/*   P O R T                                                */
/* ======================================================== */

/*! Number of scratch buffers created per port */
#define WFD_PORT_SCRATCH_COUNT 3
/*! Index of the scratch buffer used for masks */
#define WFD_PORT_MASK_INDEX 2

/*! Pointers for bound and cached pipelines. Only
 * the other of the two is in use at any moment of time,
 * but this structure fits best the ref counting implementation.
 */
typedef struct WFD_PORT_BINDING_ {
    WFD_PIPELINE *boundPipeline;
    WFD_PIPELINE *cachedPipeline;
} WFD_PORT_BINDING;

/*! Port mode attributes defined by OpenWF display specification.
 *  These are all READ-ONLY attributes and never cached.
 */
typedef struct {
    WFDPortMode id;
    WFDint width;
    WFDint height;
    WFDfloat refreshRate;
    WFDboolean flipMirrorSupport;
    WFDRotationSupport rotationSupport;
    WFDboolean interlaced;
} WFD_PORT_MODE;

typedef struct {
    WFDDisplayDataFormat format;
    WFDint dataSize;
    WFDuint8 *data;
} WFD_DISPLAY_DATA;

/*! Port attributes defined by OpenWF display specification.
 *
 */
typedef struct WFD_PORT_CONFIG_ {
    /*! A pointer to current pipeline object, NULL if pipeline is
     * unallocated
     */
    WFD_PORT *inUse;

    WFDint id;
    WFDPortType type;
    WFDboolean detachable;
    WFDboolean attached;
    WFDint nativeResolution[2];
    WFDfloat physicalSize[2];
    WFDboolean fillPortArea;
    WFDfloat backgroundColor[3];
    WFDboolean flip;
    WFDboolean mirror;
    WFDint rotation;
    WFDPowerMode powerMode;
    WFDfloat gammaRange[2];
    WFDfloat gamma;
    WFDPartialRefresh partialRefreshSupport;
    WFDint partialRefreshMaximum[2];
    WFDPartialRefresh partialRefreshEnable;
    WFDint partialRefreshRectangle[4];
    WFDint pipelineIdCount;
    WFDint *pipelineIds;
    WFDboolean protectionEnable;

    /*! Port mode count */
    WFDint modeCount;
    /*! Preconfigured port mode */
    WFDPortMode preconfiguredMode;
    /*! An array of pointers to port mode configurations */
    WFD_PORT_MODE *modes;

    WFDint displayDataCount;
    /*! An array of pointers to display data blocks */
    WFD_DISPLAY_DATA *displayData;

} WFD_PORT_CONFIG;

struct WFD_PORT_ {
    /*! my handle */
    WFDPort handle;
    /*! backpointer to device */
    WFD_DEVICE *device;

    /*! hardware configuration area */
    WFD_PORT_CONFIG *config;

    /*! attribute cache */
    OWF_ATTRIBUTE_LIST attributes;

    /*! current port mode */
    WFD_PORT_MODE *currentMode;
    /*! non-committed cached mode */
    WFD_PORT_MODE *cachedMode;
    /*! caching flag */
    WFDboolean modeDirty;

    /*! mutex protecting busyCond variable and portBusy flag */
    OWF_MUTEX portMutex;
    /*! condition variable for waiting port becoming available */
    OWF_COND busyCond;
    /*! flag indicating port is busy doing commit or rendering */
    WFDboolean portBusy;

    /*! flag indicating ongoing port destroy operator */
    WFDboolean destroyPending;

    /*! scratch buffers  for the SI */
    OWF_IMAGE *scratch[WFD_PORT_SCRATCH_COUNT];
    /*! Final port image buffers */
    OWF_IMAGE *surface[2];
    /*! Mutex protecting frameBuffer field */
    OWF_MUTEX frMutex;
    /*! index to current surface */
    WFDint frameBuffer;
    /*! internal  screen number for the SI */
    WFDint screenNumber;

    /*! queue for rendering messages */
    OWF_MESSAGE_QUEUE msgQueue;

    /*! An array for current pipeline bindings; One item
     * for each bindable pipeline */
    WFD_PORT_BINDING *bindings;

    /*! Screen refresher thread */
    OWF_THREAD blitter;
    /*! Rendering thread */
    OWF_THREAD blender;
};

/* ========================================================== */
/*   P I P E L I N E                                          */
/* ========================================================== */

/*! number of scratch buffers allocated for a pipeline at creation */
#define WFD_PIPELINE_SCRATCH_COUNT 2

/*! transparent source color */
typedef struct WFD_TS_COLOR_ {
    WFDTSColorFormat colorFormat;
    OWFpixel color;
} WFD_TS_COLOR;

struct WFD_PIPELINE_BINDINGS_ {
    /*! backpointer to pipeline */
    WFD_PIPELINE *pipeline;

    WFD_PORT *boundPort;
    WFD_PORT *cachedPort;
    WFDboolean portDirty;

    WFD_SOURCE *boundSource;
    WFDTransition boundSrcTransition;
    WFDboolean sourceDirty;

    WFD_SOURCE *cachedSource;
    WFDTransition cachedSrcTransition;

    WFDRect boundRegion;
    WFDRect cachedRegion;

    WFD_MASK *boundMask;
    WFDTransition boundMaskTransition;
    WFDboolean maskDirty;
    WFD_MASK *cachedMask;
    WFDTransition cachedMaskTransition;
};

/*! Pipeline attributes defined by OpenWF display specification.
 *
 */
typedef struct WFD_PIPELINE_CONFIG_ {
    /*! A pointer to current pipeline object, NULL if pipeline is
     * unallocated
     */
    WFD_PIPELINE *inUse;

    WFDint id;
    WFDPort portId;
    WFDint layer;
    WFDboolean shareable;
    WFDboolean directRefresh;
    WFDint maxSourceSize[2];
    WFDint sourceRectangle[4];
    WFDboolean flip;
    WFDboolean mirror;
    WFDRotationSupport rotationSupport;
    WFDint rotation;
    WFDfloat scaleRange[2];
    WFDint scaleFilter;
    WFDint destinationRectangle[4];
    WFDTransparency transparencyEnable;
    WFDfloat globalAlpha;

    /*! number of transparency features supported */
    WFDint transparencyFeatureCount;
    /*! array of different transparency features */
    WFDbitfield *transparencyFeatures;
} WFD_PIPELINE_CONFIG;

struct WFD_PIPELINE_ {
    /*! my handle */
    WFDPipeline handle;

    /*! backpointer to device */
    WFD_DEVICE *device;

    /*! hardware configuration area */
    WFD_PIPELINE_CONFIG *config;

    /*! attribute cache */
    OWF_ATTRIBUTE_LIST attributes;

    /*! transparent source color */
    WFD_TS_COLOR tsColor;

    /*! current and cached bindings */
    WFD_PIPELINE_BINDINGS *bindings;

    /*! scratch buffers for the SI */
    OWF_IMAGE *scratch[WFD_PIPELINE_SCRATCH_COUNT];

    /*! latest rendered pipeline image  (one of the scratch buffers) */
    OWF_IMAGE *frontBuffer;
};

typedef struct WFD_EVENT_ {
    WFDEventType type;
    union {
        struct {
            WFDint portId;
            WFDboolean attached;
        } portAttachEvent;

        struct {
            WFDint pipelineId;
            WFDHandle handle;
            WFDboolean overflow;
        } pipelineBindEvent;

        struct {
            WFDint portId;
        } portProtectionEvent;

    } data;
} WFD_EVENT;

#define WFD_FIRST_FILTERED WFD_EVENT_NONE
#define WFD_LAST_FILTERED WFD_EVENT_PIPELINE_BIND_MASK_COMPLETE
/*! \brief Number of filtered event types  */
#define WFD_EVENT_FILTER_SIZE (WFD_LAST_FILTERED - WFD_FIRST_FILTERED + 1)

typedef struct WFD_EVENT_CONTAINER_ {
    /*! backpointer to device object */
    WFD_DEVICE *device;

    /*! handle of this container */
    WFDEvent handle;

    /*! current event filter */
    WFDboolean eventFilter[WFD_EVENT_FILTER_SIZE];

    /*! maximum number of binding events that can be queued */
    WFDint pipelineBindQueueSize;

    /*! latest signaled event, not in queue */
    WFD_EVENT *event;

    /*! display associated with sync */
    WFDEGLDisplay display;
    /*! stored sync object */
    WFDEGLSync sync;

    /*! current length of the event queue */
    WFDint queueLength;
    /*! all queued events */
    OWF_NODE *eventQueue;

    /*! pre-allocated pool of list nodes */
    OWF_POOL *nodePool;
    /*! pre-allocated pool of event records */
    OWF_POOL *eventPool;

    /*! mutex protecting event container access */
    OWF_MUTEX mutex;
    /*! a flag indicating that someone is waiting in this container */
    WFDboolean waiting;
    /*! condition variable used for event waiting */
    OWF_COND cond;
} WFD_EVENT_CONTAINER;

typedef struct WFD_DEVICE_CONFIG_ {
    /*! A pointer to current device object, NULL if device is
     * unallocated
     */
    WFD_DEVICE *inUse;

    /*! Device id */
    WFDint id;
    WFDint portCount;
    /*! An array of pointers to port configurations */
    WFD_PORT_CONFIG *ports;

    WFDint pipelineCount;
    /* An array of pointers to pipeline configurations */
    WFD_PIPELINE_CONFIG *pipelines;
} WFD_DEVICE_CONFIG;

struct WFD_DEVICE_ {
    /*! handle of this device */
    WFDDevice handle;

    /*! a pointer to hw configuration */
    WFD_DEVICE_CONFIG *config;

    WFDErrorCode lastUnreadError;

    /*! ports created for device */
    OWF_ARRAY ports;
    /*! pipelines created for device */
    OWF_ARRAY pipelines;
    /*! containers created for device */
    OWF_ARRAY eventConts;

    /*! mutex protecting busy-flag */
    OWF_MUTEX commitMutex;
    /* a flag indicating on-going commit activity */
    WFDboolean busyFlag;

    /*! container for stream handles */
    OWF_HASHTABLE *streams;
    /*! container for source/mask handles */
    OWF_HASHTABLE *imageProviders;
};

/*! System configuration root */
typedef struct WFD_CONFIG_ {
    WFDint devCount;
    WFD_DEVICE_CONFIG *devices;
} WFD_CONFIG;

#ifdef __cplusplus
}
#endif

#endif /*WFDSTRUCTS_H_*/
