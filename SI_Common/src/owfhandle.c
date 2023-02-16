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

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <stdlib.h>

#include "owfdebug.h"
#include "owfhandle.h"
#include "owfmutex.h"

#define HANDLE_MAX 0xFFFFFFul
#define HANDLE_BITS 24

/* hash table size MUST be power of 2
 * when bitmask hash is used */
#define TABLESIZE (0x100ul)

/*! \brief Construct handle out of an integer and type identifier
 *
 *
 *
 *  \param n handle sequence number
 *  \param type Type identifier
 */
static OWFHandle
OWF_Handle_Construct(OWFuint32 n, OWFuint8 type);

/*! \brief Extract type identifier from handle
 *
 *  \param h Handle
 *
 */
static OWFuint8
OWF_Handle_GetType(OWFHandle h);

static OWFuint32
OWF_Handle_GetNext(OWF_HANDLE_DESC* hDesc)
{
    OWFuint32 ret;

    OWF_Mutex_Lock(&hDesc->mutex);

    if (hDesc->next == HANDLE_MAX)
    {
        hDesc->next = 0;
    }
    ret =  ++hDesc->next;

    OWF_Mutex_Unlock(&hDesc->mutex);

    return ret;
}

static OWFHandle
OWF_Handle_Construct(OWFuint32 n, OWFuint8 t)
{
    return (n & HANDLE_MAX) | (OWFuint32)t << HANDLE_BITS;
}

static OWFuint8
OWF_Handle_GetType(OWFHandle h)
{
    return (OWFuint8)(h >> HANDLE_BITS);
}

OWF_API_CALL OWFHandle
OWF_Handle_Create(OWF_HANDLE_DESC* hDesc, OWFuint8 objType, void* obj)
{
    OWFuint32 rounds = 0;
    OWFHandle h;

    OWF_ASSERT(hDesc && hDesc->hash);

    h = OWF_Handle_Construct(OWF_Handle_GetNext(hDesc), objType);

    while (OWF_Hash_Lookup(hDesc->hash, h)!=NULL)
    {
        /* Created handle must be unambiguous
         * If handle already exists, allocate next  */
        rounds++;

        if (rounds > HANDLE_MAX)
        {
            /* all possible values checked */
            return OWF_INVALID_HANDLE;
        }

        h = OWF_Handle_Construct(OWF_Handle_GetNext(hDesc), objType);
    }

    OWF_Hash_Insert(hDesc->hash, h, obj);
    return h;
}

OWF_API_CALL void*
OWF_Handle_GetObj(OWF_HANDLE_DESC* hDesc, OWFuint32 h, OWFuint8 objType)
{
    OWF_ASSERT(hDesc && hDesc->hash);

    /* check that handle matches required type */
    if ((OWFuint32)objType != OWF_Handle_GetType(h))
    {
        return NULL;
    }
    return OWF_Hash_Lookup(hDesc->hash, h);
}

OWF_API_CALL void
OWF_Handle_Delete(OWF_HANDLE_DESC* hDesc, OWFuint32 h)
{
    OWF_ASSERT(hDesc && hDesc->hash);

    if (hDesc->hash)
    {
        OWF_Hash_Delete(hDesc->hash, h);
    }
}

#ifdef __cplusplus
}
#endif

