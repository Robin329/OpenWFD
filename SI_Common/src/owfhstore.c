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

#include "owfhstore.h"
#include "owfdebug.h"

#ifdef __cplusplus
extern "C" {
#endif


/* hash table size MUST be power of 2
 * when bitmask hash is used */
#define TABLESIZE (0x100ul)


static OWF_HANDLE_DESC* OWF_HStore_GetDesc();
static void  OWF_HStore_SetDesc(OWF_HANDLE_DESC* d);

static void OWF_HStore_ModuleTerminate()
{
    OWF_HANDLE_DESC*  hd = OWF_HStore_GetDesc();

    if (hd)
    {
        if (hd->hash)
        {
            OWF_Hash_TableDelete(hd->hash);
            hd->hash = NULL;
        }
        if (hd->mutex)
        {
            OWF_Mutex_Destroy(&hd->mutex);
            hd->mutex = NULL;
        }
        xfree(hd);
    }

    OWF_HStore_SetDesc(NULL);
}

static void
OWF_HStore_ModuleInitialize()
{
    OWF_HANDLE_DESC*  hd = NEW0(OWF_HANDLE_DESC);

    OWF_ASSERT(hd);

    if (hd)
    {
        hd->hash = OWF_Hash_TableCreate(
                TABLESIZE, OWF_Hash_BitMaskHash);
        OWF_ASSERT(hd->hash);
        OWF_Mutex_Init(&hd->mutex);
        hd->next = 0;
    }

    OWF_HStore_SetDesc(hd);
}


static OWF_HANDLE_DESC*
OWF_HStore_GetSetDesc(OWF_HANDLE_DESC* d, OWFboolean set)
{
    static OWF_HANDLE_DESC*  owfHandleDescriptor = NULL;
    static OWFboolean initialized = OWF_FALSE;

    if (!initialized)
    {
        initialized = OWF_TRUE;
        OWF_HStore_ModuleInitialize();
        atexit(OWF_HStore_ModuleTerminate);
    }

    if (set)
    {
        owfHandleDescriptor = d;
        return d;
    }
    else
    {
        return owfHandleDescriptor;
    }
}


static OWF_HANDLE_DESC*
OWF_HStore_GetDesc()
{
    return OWF_HStore_GetSetDesc(NULL, OWF_FALSE);
}

static void
OWF_HStore_SetDesc(OWF_HANDLE_DESC* d)
{
    OWF_HStore_GetSetDesc(d, OWF_TRUE);
}


OWF_API_CALL OWFHandle
OWF_HStore_HandleCreate(OWFuint8 objType, void* obj)
{
    return OWF_Handle_Create(OWF_HStore_GetDesc(), objType, obj);
}

OWF_API_CALL void*
OWF_HStore_GetObj(OWFHandle handle, OWFuint8 objType)
{
    return OWF_Handle_GetObj(OWF_HStore_GetDesc(), handle, objType);
}
OWF_API_CALL void
OWF_HStore_HandleDelete(OWFHandle handle)
{
    OWF_Handle_Delete(OWF_HStore_GetDesc(), handle);
}

#ifdef __cplusplus
}
#endif
