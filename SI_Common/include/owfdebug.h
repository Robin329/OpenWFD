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

/*
 * owfdebug.h
 *
 */

#ifndef OWFDEBUG_H_
#define OWFDEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "owftypes.h"

#define DPRINT(x) OWF_Debug_Print x
#define ENTER(x) DPRINT(("%s:", #x))
#define LEAVE(x)
#define TRACE(x) OWF_Debug_Trace x
#define INDENT OWF_Debug_TraceIndent()
#define UNDENT OWF_Debug_TraceUndent()
#define OWF_ASSERT(c) assert(c);

OWF_API_CALL void OWF_Debug_Print(const char *format, ...);
OWF_API_CALL void OWF_Debug_Trace(const char *fmt, ...);
OWF_API_CALL void OWF_Debug_TraceIndent();
OWF_API_CALL void OWF_Debug_TraceUndent();

#else /* NOT DEBUG */

#define DPRINT(x) /* do nothing */
#define ENTER(x)
#define LEAVE(x)
#define TRACE(x)
#define INDENT
#define UNDENT
#define OWF_ASSERT(x)

#endif /* DEBUG */

#ifdef __cplusplus
}
#endif

#endif /* OWFDEBUG_H_ */
