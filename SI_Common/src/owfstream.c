/* Copyright (c) 2009 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
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

#ifdef __cplusplus
extern "C" {
#endif

#include "owfstream.h"

#include "owfdebug.h"
#include "owfmemory.h"
#include "owfnativestream.h"
#include "owfobject.h"

/*!---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL OWFboolean OWF_Stream_Destroy(OWF_STREAM *stream) {
    OWFboolean result = OWF_FALSE;

    DPRINT(("OWF_Stream_Destroy(stream = %p)", stream));

    if (stream) {
        OWF_Stream_RemoveReference(stream);

        if (!stream->useCount && (stream->handle != OWF_INVALID_HANDLE)) {
            DPRINT(("Ok, the stream will go now"));
            owfNativeStreamDestroy(stream->handle);
            stream->handle = OWF_INVALID_HANDLE;
            xfree(stream);
            DPRINT(("  stream destroyed"));

            result = OWF_TRUE;
        }
    }

    return result;
}

/*!---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL OWF_STREAM *OWF_Stream_AddReference(OWF_STREAM *stream) {
    if (stream) {
        ++stream->useCount;
        DPRINT(("OWF_Stream_AddReference: Use count of stream %d is now %d",
                (OWFint)stream->handle & 0xFFFF, stream->useCount));
    }
    return stream;
}

/*!---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL void OWF_Stream_RemoveReference(OWF_STREAM *stream) {
    if (stream && stream->useCount > 0) {
        --stream->useCount;
        DPRINT(("OWF_Stream_RemoveReference: Use count of stream %p is now %d",
                (OWFint)stream->handle & 0xFFFF, stream->useCount));
    }
}

/*!---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL OWF_STREAM *OWF_Stream_Create(OWFNativeStreamType stream,
                                           OWFboolean write) {
    OWF_STREAM *strm;

    DPRINT(("Creating stream stream = %p (handle = %d), write = %d)", stream,
            (OWFint)stream & 0xFFFF, write));

    strm = NEW0(OWF_STREAM);

    if (strm) {
        owfNativeStreamAddReference(stream);
        strm->handle = stream;
        strm->lockCount = 0;
        strm->buffer = OWF_INVALID_HANDLE;
        strm->write = write;
        strm->useCount = 1;
        strm->image = NULL;
    }
    return strm;
}

/*!---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL OWF_IMAGE *OWF_Stream_LockForReading(OWF_STREAM *stream) {
    void *pixels;
    OWFint width, height, pixelSize = 0;
    OWF_IMAGE_FORMAT imgf;
    OWFint stride;

    if (!stream) {
        DPRINT(("stream = NULL or image = NULL"));
        return NULL;
    }

    DPRINT(("stream = %p", stream->handle));

    if (!stream->lockCount) {
        DPRINT(("About to acquire & lock a read buffer"));
        /* acquire buffer */
        stream->buffer = owfNativeStreamAcquireReadBuffer(stream->handle);
        DPRINT(("  Acquired read buffer stream=%p, buffer=%d", stream->handle,
                stream->buffer));

        /* Bind source image to pixel buffer */
        owfNativeStreamGetHeader(stream->handle, &width, &height, &stride,
                                 &imgf, &pixelSize);
        pixels = owfNativeStreamGetBufferPtr(stream->handle, stream->buffer);
        stream->image = OWF_Image_Create(width, height, &imgf, pixels, stride);

        OWF_ASSERT(stream->image);
    }

    ++stream->lockCount;
    DPRINT(("lock count = %d", stream->lockCount));

    return stream->image;
}

/*!---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL void OWF_Stream_Unlock(OWF_STREAM *stream) {
    if (!stream) {
        DPRINT(("stream = NULL"));
        return;
    }

    if (stream->lockCount > 0) {
        --stream->lockCount;
        DPRINT(("lock count = %d", stream->lockCount));

        if (!stream->lockCount) {
            DPRINT(("  Releasing read buffer stream=%p, buffer=%d",
                    stream->handle, stream->buffer));
            OWF_Image_Destroy(stream->image);
            stream->image = NULL;
            owfNativeStreamReleaseReadBuffer(stream->handle, stream->buffer);
        }
    }
}

/*!---------------------------------------------------------------------------
 *
 *----------------------------------------------------------------------------*/
OWF_API_CALL void OWF_Stream_GetSize(OWF_STREAM *stream, OWFint *width,
                                     OWFint *height) {
    if (stream) {
        owfNativeStreamGetHeader(stream->handle, width, height, NULL, NULL,
                                 NULL);
    }
}

#ifdef __cplusplus
}
#endif
