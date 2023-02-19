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
 *  \file wfdhandle.c
 *  \brief OpenWF Display SI, handle module implementation.
 */

#include "wfdhandle.h"

#include "owfmemory.h"
#include "wfddebug.h"

#ifdef __cplusplus
extern "C" {
#endif

/* hash table size MUST be power of 2
 * when bitmask hash is used */
#define TABLESIZE (0x100ul)

OWF_PUBLIC OWF_HANDLE_DESC *WFD_Handle_GetSetDesc(OWF_HANDLE_DESC *d,
                                                  OWFboolean set);

static OWF_HANDLE_DESC *WFD_Handle_GetDesc();
static void WFD_Handle_SetDesc(OWF_HANDLE_DESC *d);

static void WFD_Handle_ModuleInitialize();
static void WFD_Handle_ModuleTerminate();

/*=========================================================================
 * PRIVATE PART
 *=========================================================================*/

static void WFD_Handle_ModuleInitialize() {
    OWF_HANDLE_DESC *hd = NEW0(OWF_HANDLE_DESC);

    OWF_ASSERT(hd);

    if (hd) {
        hd->hash = OWF_Hash_TableCreate(TABLESIZE, OWF_Hash_BitMaskHash);
        OWF_ASSERT(hd->hash);
        OWF_Mutex_Init(&hd->mutex);
        hd->next = 0;
    }

    WFD_Handle_SetDesc(hd);
}

static void WFD_Handle_ModuleTerminate() {
    OWF_HANDLE_DESC *hd = WFD_Handle_GetDesc();

    if (hd) {
        if (hd->hash) {
            OWF_Hash_TableDelete(hd->hash);
            hd->hash = NULL;
        }
        if (hd->mutex) {
            OWF_Mutex_Destroy(&hd->mutex);
            hd->mutex = NULL;
        }
        xfree(hd);
    }

    WFD_Handle_SetDesc(NULL);
}

static OWF_HANDLE_DESC *WFD_Handle_GetDesc() {
    return WFD_Handle_GetSetDesc(NULL, OWF_FALSE);
}

static void WFD_Handle_SetDesc(OWF_HANDLE_DESC *d) {
    WFD_Handle_GetSetDesc(d, OWF_TRUE);
}

/*! \brief Get or set the descriptor of wfd handle store
 *
 *  Routine manages a static handle descriptor. Descriptor contains a pointer
 *  to hash table where handles are store, a mutex that protects the hash table
 * and a counter to determine next handle value.
 *
 *  This routine is invoked only within wfdhandle.c module. Routine is yet
 * declared as OWF_PUBLIC. That is in order to ensure, that after linkage the
 * symbol WFD_Handle_GetSetDesc is unambiguous throughout the system and thus
 * only one handle descriptor for the wfd system exists at run-time.
 *
 *  \param d Optional (only when set); handle store descriptor structure
 *  \param set When OWF_TRUE set descriptor, when OWF_FALSE get descriptor
 *
 *  \return Pointer to static descriptor
 */
OWF_PUBLIC OWF_HANDLE_DESC *WFD_Handle_GetSetDesc(OWF_HANDLE_DESC *d,
                                                  OWFboolean set) {
    static OWF_HANDLE_DESC *wfdHandleDescriptor = NULL;
    static OWFboolean initialized = OWF_FALSE;

    if (!initialized) {
        initialized = OWF_TRUE;
        WFD_Handle_ModuleInitialize();
        atexit(WFD_Handle_ModuleTerminate);
    }

    if (set) {
        wfdHandleDescriptor = d;
        return d;
    } else {
        return wfdHandleDescriptor;
    }
}

/*=========================================================================
 * PUBLIC API
 *=========================================================================*/

OWF_API_CALL OWFHandle WFD_Handle_Create(WFD_HANDLE_TYPE objType, void *obj) {
    return OWF_Handle_Create(WFD_Handle_GetDesc(), (OWFuint8)objType, obj);
}

OWF_API_CALL void *WFD_Handle_GetObj(OWFHandle handle,
                                     WFD_HANDLE_TYPE objType) {
    return OWF_Handle_GetObj(WFD_Handle_GetDesc(), handle, (OWFuint8)objType);
}
OWF_API_CALL void WFD_Handle_Delete(OWFHandle handle) {
    OWF_Handle_Delete(WFD_Handle_GetDesc(), handle);
}

#ifdef __cplusplus
}
#endif
