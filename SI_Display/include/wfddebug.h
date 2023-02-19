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

#ifndef WFCDEBUG_H_
#define WFCDEBUG_H_

/*! \ingroup wfd
 *  \file wfddebug.h
 *  \brief Debugging helper routines
 *
 *  Macro DEBUG must be defined in compilation in order to
 *  use the routines. More debugging helpers are declared in
 *  owfdebug.h which is included in DEBUG compilation.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG

#include <stdarg.h>
#include <stdio.h>

#ifdef OWF_DEBUG_PREFIX
#undef OWF_DEBUG_PREFIX
#endif

#define OWF_DEBUG_PREFIX "WFD: "

#include "owfdebug.h"

OWF_API_CALL void OWF_APIENTRY WFD_Debug_DumpConfiguration() OWF_APIEXIT;

#else

#include "owfdebug.h"

#endif

#ifdef __cplusplus
}
#endif

#endif
