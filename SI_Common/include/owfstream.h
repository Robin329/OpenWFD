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


#ifndef OWFSTREAM_H_
#define OWFSTREAM_H_

#include "owftypes.h"
#include "owfimage.h"
#include "owfnativestream.h"
#include "owfcond.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct OWF_STREAM_  OWF_STREAM;

struct OWF_STREAM_ {
    OWFNativeStreamType     handle;
    OWFNativeStreamBuffer   buffer;
    OWFint                  useCount;
    OWFint                  lockCount;
    OWFboolean              write;
    OWF_IMAGE*              image;
    OWF_MUTEX               mutex;
};

OWF_API_CALL OWFboolean
OWF_Stream_Destroy(OWF_STREAM* stream);

OWF_API_CALL OWF_STREAM*
OWF_Stream_AddReference(OWF_STREAM* stream);

OWF_API_CALL void
OWF_Stream_RemoveReference(OWF_STREAM* stream);

OWF_API_CALL OWF_STREAM*
OWF_Stream_Create(OWFNativeStreamType stream,
                  OWFboolean write);

OWF_API_CALL OWF_IMAGE*
OWF_Stream_LockForReading(OWF_STREAM* stream);

OWF_API_CALL void
OWF_Stream_Unlock(OWF_STREAM* stream);

OWF_API_CALL void
OWF_Stream_GetSize(OWF_STREAM* stream, OWFint* width, OWFint* height);


#ifdef __cplusplus
}
#endif

#endif /* OWFSTREAM_H_ */
