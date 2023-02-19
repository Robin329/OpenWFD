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

#include "owfhash.h"

#include <stdio.h>
#include <stdlib.h>

#include "owfdebug.h"
#include "owfmemory.h"
#include "owfmutex.h"
#include "owfobject.h"

OWF_API_CALL OWF_HASHTABLE *OWF_Hash_TableCreate(OWFuint32 tblSize,
                                                 OWF_HASHFUNC hashFunc) {
    OWFuint32 i;
    OWF_HASHTABLE *tbl = xalloc(sizeof(OWF_HASHTABLE), 1);

    if (tbl) {
        tbl->tblSize = tblSize;
        tbl->hashFunc = hashFunc;
        tbl->count = 0;
        OWF_Mutex_Init(&tbl->mutex);
        tbl->tbl = xalloc(tblSize, sizeof(OWF_HASHNODE *));

        if (tbl->tbl && tbl->mutex) {
            for (i = 0; i < tblSize; i++) {
                tbl->tbl[i] = NULL;
            }
        } else {
            if (tbl->mutex) {
                OWF_Mutex_Destroy(&tbl->mutex);
                tbl->mutex = NULL;
            }
            if (tbl->tbl) {
                xfree(tbl->tbl);
                tbl->tbl = NULL;
            }
            xfree(tbl);
            tbl = NULL;
        }
    }

    return tbl;
}

OWF_API_CALL void OWF_Hash_TableDelete(OWF_HASHTABLE *tbl) {
    if (!tbl) {
        return;
    }

    if (tbl) {
        OWF_Mutex_Lock(&tbl->mutex);
        if (tbl->tbl) {
            OWFuint32 i;

            for (i = 0; i < tbl->tblSize; i++) {
                OWF_HASHNODE *np = tbl->tbl[i];
                while (np != NULL) {
                    OWF_HASHNODE *pnp = np;
                    np = np->next;
                    xfree(pnp);
                }
                tbl->tbl[i] = NULL;
            }
            xfree(tbl->tbl);
            tbl->tbl = NULL;
        }
        tbl->count = 0;
        OWF_Mutex_Unlock(&tbl->mutex);
        OWF_Mutex_Destroy(&tbl->mutex);
        xfree(tbl);
    }
}

OWF_API_CALL OWFuint32 OWF_Hash_BitMaskHash(const OWF_HASHTABLE *tbl,
                                            OWF_HASHKEY key) {
    OWFuint32 i;
    OWFuint32 mask = tbl->tblSize - 1;

    i = key & mask;

    OWF_ASSERT(i < tbl->tblSize);

    return i;
}

OWF_API_CALL OWFboolean OWF_Hash_Insert(OWF_HASHTABLE *tbl, OWF_HASHKEY key,
                                        void *data) {
    OWFint i;
    OWF_HASHNODE *np = NULL;

    OWF_ASSERT(tbl != NULL);

    i = tbl->hashFunc(tbl, key);
    np = xalloc(1, sizeof(OWF_HASHNODE));

    if (np) {
        /* insert to chain head */
        np->key = key;
        np->data = data;
        OWF_Mutex_Lock(&tbl->mutex);
        np->next = tbl->tbl[i];
        tbl->tbl[i] = np;
        ++tbl->count;
        OWF_Mutex_Unlock(&tbl->mutex);
        return OWF_TRUE;
    }

    return OWF_FALSE;
}

OWF_API_CALL OWFboolean OWF_Hash_Delete(OWF_HASHTABLE *tbl, OWF_HASHKEY key) {
    OWFuint32 i;
    OWF_HASHNODE *np = NULL;
    OWF_HASHNODE **pnp = NULL;

    OWF_ASSERT(tbl != NULL);

    i = tbl->hashFunc(tbl, key);

    OWF_Mutex_Lock(&tbl->mutex);

    np = tbl->tbl[i];
    pnp = &tbl->tbl[i]; /* pointer to previous next pointer */

    while (np != NULL && np->key != key) {
        pnp = &np->next;
        np = np->next;
    }

    if (np) {
        *pnp = np->next;
        --tbl->count;
    }

    OWF_Mutex_Unlock(&tbl->mutex);

    if (np) {
        xfree(np);
        return OWF_TRUE;
    }

    return OWF_FALSE;
}

OWF_API_CALL void *OWF_Hash_Lookup(OWF_HASHTABLE *tbl, OWF_HASHKEY key) {
    OWFuint32 i;
    OWF_HASHNODE *np;

    OWF_ASSERT(tbl != NULL);

    i = tbl->hashFunc(tbl, key);

    OWF_Mutex_Lock(&tbl->mutex);

    np = tbl->tbl[i];

    while (np != NULL && np->key != key) {
        np = np->next;
    }

    OWF_Mutex_Unlock(&tbl->mutex);

    return (np) ? np->data : NULL;
}

OWF_API_CALL OWFuint32 OWF_Hash_Size(OWF_HASHTABLE *tbl) { return tbl->count; }

OWF_API_CALL OWFuint OWF_Hash_ToArray(OWF_HASHTABLE *tbl, OWF_HASHKEY *keyarray,
                                      void **valarray, OWFuint maxsize) {
    OWFuint i, o = 0;
    OWF_HASHNODE *np;

    OWF_ASSERT(tbl != NULL);

    for (i = 0; i < tbl->tblSize && o < maxsize; i++) {
        np = tbl->tbl[i];
        while (np != NULL && o < maxsize) {
            if (keyarray) {
                keyarray[o] = np->key;
            }
            if (valarray) {
                valarray[o] = np->data;
            }

            np = np->next;
            o++;
        }
    }
    return o;
}

/*-------------------------------------------------------
 *  test & debugging section
 */

OWF_API_CALL void OWF_Hash_Dump(const OWF_HASHTABLE *tbl) {
    OWFuint32 i;
    OWF_HASHNODE *np;

    OWF_ASSERT(tbl != NULL);

    for (i = 0; i < tbl->tblSize; i++) {
        np = tbl->tbl[i];
        while (np != NULL) {
            DPRINT(("%d: key == 0x%08x, data == %p\n", i, np->key, np->data));
            np = np->next;
        }
    }
}

#ifdef __cplusplus
}
#endif
