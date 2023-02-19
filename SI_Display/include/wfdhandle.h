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
 *  \file wfdhandle.h
 *  \brief Handle interface for display objects
 *
 */

#ifndef WFDHANDLE_H_
#define WFDHANDLE_H_

#include "owfhandle.h"
#include "owftypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!  \brief Allowed handle (object) types */
typedef enum {
    WFD_DEVICE_HANDLE = 0xE0,
    WFD_EVENT_HANDLE,
    WFD_PORT_HANDLE,
    WFD_PORT_MODE_HANDLE,
    WFD_PIPELINE_HANDLE,
    WFD_SOURCE_HANDLE,
    WFD_MASK_HANDLE
} WFD_HANDLE_TYPE;

/*! \brief Create a access handle for an object
 *
 *  Routine creates an object handle that is unambiguous throughout
 *  the whole system.
 *
 *  \param objType Type of the object. Used for type checking on object access
 *  \param obj Pointer to the object
 *
 *  \return Created handle or OWF_INVALID_HANDLE is creation failed
 */
OWF_API_CALL OWFHandle WFD_Handle_Create(WFD_HANDLE_TYPE objType, void* obj);

/*! \brief Retrieve an object by handle
 *
 *  Routine returns a pointer to the object that is associated
 *  with given handle. A type check is made.
 *
 *  \param  handle Handle
 *  \param objType Desired object type
 *
 *  \return pointer to an object or NULL if handle is invalid or the
 *  object does not match the given type.
 */
OWF_API_CALL void* WFD_Handle_GetObj(OWFHandle handle, WFD_HANDLE_TYPE objType);

/*! \brief Delete a handle
 *
 *  \param  handle Handle
 */
OWF_API_CALL void WFD_Handle_Delete(OWFHandle handle);

#ifdef __cplusplus
}
#endif

#endif /* WFDHANDLE_H_ */
