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

#define DEBUG
#ifdef DEBUG

#include "owfdebug.h"

#include <pthread.h>

#include "owftypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OWF_DEBUG_PREFIX
#define OWF_DEBUG_PREFIX "OWF: "
#endif

OWF_API_CALL void OWF_Debug_Print(const char *format, ...) {
    va_list ap;
    char __spager[512];

    va_start(ap, format);
    __spager[0] = 0;
    vsnprintf(__spager, 511, format, ap);
    fprintf(stderr, "%s %s\n", OWF_DEBUG_PREFIX, __spager);
    va_end(ap);
}

OWF_API_CALL void OWF_Debug_Trace(const char *fmt, ...) { fmt = fmt; }

OWF_API_CALL void OWF_Debug_TraceIndent() {}

OWF_API_CALL void OWF_Debug_TraceUndent() {}

OWF_API_CALL void OWF_Debug_TraceEnter(const char *func) {
    if (func) {
        OWF_Debug_Trace("ENTER %s", func);
    }
    OWF_Debug_TraceIndent();
}

OWF_API_CALL void OWF_Debug_TraceExit(const char *func) {
    OWF_Debug_TraceUndent();
    if (func) {
        OWF_Debug_Trace("EXIT %s", func);
    }
}

#ifdef __cplusplus
}
#endif

#else

static const int
    this_is_to_keep_the_compiler_happy_and_not_complaining_about_empty_source_file =
        0xBAD0F00D;

#endif /* DEBUG */
