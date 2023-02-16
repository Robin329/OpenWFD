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
 * 
 * OWF_DISPCTX is a handle to adaptation extensions to the WFC_CONTEXT.
 * This could be merged with SCREEN, but that is currently instanced at the DEVICE level
 * 
 */
#ifndef OWFDISPLAYCONTEXTGENERAL_H_
#define OWFDISPLAYCONTEXTGENERAL_H_

#include "owftypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef OWFHandle   OWF_DISPCTX;

/**
 * Create an extension object to be attached to the context
 * @param screenNum The screen number index to create it for. Negative indicates off-screen contexts
 * @return OWF_DISPCTX handle to context object.
 **/
OWF_DISPCTX OWF_DisplayContext_Create(OWFint32 screenNum);
/**
 * Destroy the extension object to attached to the context
 * @param dc The display context handle of the object to be destroyed.
 * @param screenNum The screen number index associated with it. Negative indicates off-screen contexts
 * @return OWF_DISPCTX handle to context object.
 **/
void OWF_DisplayContext_Destroy(OWFint32 screenNum, OWF_DISPCTX dc);

/**
 * Determine whether a particular screen number is connected to an on-screen context.
 * @param screenNum The screen number to check. Negative or out of range always returns false.
 * @return TRUE if the screen number is active, i.e. associated with a context. FALSE if available.
 **/
OWFboolean OWF_DisplayContext_IsLive(OWFint32 screenNum);

#ifdef __cplusplus
}
#endif

#endif /* OWFDISPLAYCONTEXTGENERAL_H_ */
