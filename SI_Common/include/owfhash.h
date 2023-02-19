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

#ifndef OWFHASH_H_
#define OWFHASH_H_

#include "owfmutex.h"
#include "owftypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OWF_HASHNODE_ OWF_HASHNODE;
typedef struct OWF_HASHTABLE_ OWF_HASHTABLE;
typedef OWFuint32 OWF_HASHKEY;
typedef OWFuint32 (*OWF_HASHFUNC)(const OWF_HASHTABLE *, OWF_HASHKEY);

struct OWF_HASHNODE_ {
    OWF_HASHKEY key;
    void *data; /* OWF_OBJECT* */
    OWF_HASHNODE *next;
};

struct OWF_HASHTABLE_ {
    OWFuint32 tblSize;
    OWF_HASHFUNC hashFunc;
    OWF_MUTEX mutex;
    OWF_HASHNODE **tbl;
    OWFuint count;
};

/*! \brief Bit mask hash function
 *
 * Note that table size must be power of 2 when
 * this hash function is used
 *
 *  \parma tbl hash table
 *  \param key
 *  \return hash table index
 */
OWFuint32 OWF_Hash_BitMaskHash(const OWF_HASHTABLE *tbl, OWF_HASHKEY key);

OWF_API_CALL OWF_HASHTABLE *OWF_Hash_TableCreate(OWFuint32 tsize,
                                                 OWF_HASHFUNC hashFunc);

OWF_API_CALL void OWF_Hash_TableDelete(OWF_HASHTABLE *hash);

/*! \brief Insert key to hash table
 * \param key hashed key
 * \param data pointer to data
 * \return OWF_TRUE, insertion succeeded
 * \return OWF_FALSE, operation failed (no memory)
 */
OWF_API_CALL OWFboolean OWF_Hash_Insert(OWF_HASHTABLE *tbl, OWF_HASHKEY key,
                                        void *data);

/*! \brief Delete key from hash table
 * \param key hashed key
 * \return OWF_TRUE, operation succeeded
 * \return OWF_FALSE, operation failed (key not found)
 */
OWF_API_CALL OWFboolean OWF_Hash_Delete(OWF_HASHTABLE *tbl, OWF_HASHKEY key);

/*! \brief Lookup key in hash table
 * \param key hashed key
 * \return NULL, key not found
 * \return pointer to data
 */
OWF_API_CALL void *OWF_Hash_Lookup(OWF_HASHTABLE *tbl, OWF_HASHKEY key);

/*! \brief Dump hash table contents
 * This is for testing purposes only
 */
OWF_API_CALL void OWF_Hash_Dump(const OWF_HASHTABLE *tbl);

OWF_API_CALL OWFuint32 OWF_Hash_Size(OWF_HASHTABLE *tbl);

OWF_API_CALL OWFuint OWF_Hash_ToArray(OWF_HASHTABLE *tbl, OWF_HASHKEY *keyarray,
                                      void **valarray, OWFuint maxsize);

#ifdef __cplusplus
}
#endif

#endif /* OWFHASH_H_ */
