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

#ifndef OWFHANDLE_H_
#define OWFHANDLE_H_

/*
 * owfhandle.h
 *
 */

#include "owftypes.h"
#include "owfhash.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct OWF_HDESC_s
{
    OWF_HASHTABLE*     hash;
    OWF_MUTEX          mutex;
    OWFint             next;
} OWF_HANDLE_DESC;


/*! \brief Associates a new handle with an object
 * Function allocates a new handle and associate an object
 * to it. An object type may be specified when handle is
 * created. Each object type has a separate handle space.
 *
 *  \param objType an integer used to differentiate between
 *         different types of handles
 *  \param obj pointer to object the new handle refers to
 *
 *  \return handle
 *  \return INVALID_HANDLE if handle could not be allocated
 *
 */
OWF_API_CALL OWFHandle
OWF_Handle_Create(OWF_HANDLE_DESC* hDesc, OWFuint8 objType, void* obj);

/*! \brief Get pointer to object a handle is associated with
 * Look for an object associated with a handle.
 *
 *  \param handle Object handle
 *  \param objType Object type is used to validate that
 *         handle is associated with right kind of object
 *
 *  \return pointer to an object or NULL if handle does not
 *         exist.
 */
OWF_API_CALL void*
OWF_Handle_GetObj(OWF_HANDLE_DESC* hDesc, OWFHandle handle, OWFuint8 objType);

/*! \brief Remove association between handle and object
 * Handle is deleted and no object can be accessed through
 * that handle after the call.
 *
 *  \param handle Object handle
 */
OWF_API_CALL void
OWF_Handle_Delete(OWF_HANDLE_DESC* hDesc, OWFHandle handle);


#ifdef __cplusplus
}
#endif

#endif /* OWFHANDLE_H_ */
