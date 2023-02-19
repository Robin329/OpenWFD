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

#include <WF/wfc.h>

#include "owfdisplaycontextgeneral.h"
#include "owftypes.h"

static OWFint sActiveScreens = 0;

OWF_DISPCTX OWF_DisplayContext_Create(OWFint32 screenNum) {
    if (screenNum >= 0 && screenNum < 32) {
        sActiveScreens |= (1 << screenNum);
        /* platform impl creates own storage here. Any value except NULL is good
         * enough for SI */
        return (OWF_DISPCTX)(screenNum | 0x10000);
    } else {
        if (screenNum == OWF_RESERVED_BAD_SCREEN_NUMBER) {
            return (OWF_DISPCTX)screenNum;
        } else {
            return OWF_INVALID_HANDLE;
        }
    }
}

void OWF_DisplayContext_Destroy(OWFint32 screenNum, OWF_DISPCTX dc) {
    (void)dc;
    if (screenNum >= 0 && screenNum < 32) {
        sActiveScreens &= ~(1 << screenNum);
    }
}
OWFboolean OWF_DisplayContext_IsLive(OWFint32 screenNum) {
    if (screenNum >= 0 && screenNum < 32) {
        return (sActiveScreens & (1 << screenNum)) != 0;
    } else {
        return KHR_BOOLEAN_FALSE;
    }
}
