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

#ifndef _owfscreen_h
#define _owfscreen_h

/*
 * owfscreen.h
 *
 * These functions connect composition/display to physical screen devices
 */


#include "owftypes.h"
#include "owfimage.h"


#ifdef __cplusplus
extern "C" {
#endif

#define OWF_INVALID_SCREEN_NUMBER   (0)

/* On-screen context size and color format */
#define OWF_SURFACE_WIDTH           (172)
#define OWF_SURFACE_HEIGHT          (220)
#define OWF_SURFACE_PIXEL_FORMAT    (OWF_IMAGE_ARGB8888)
#define OWF_SURFACE_LINEAR          (OWF_FALSE)
#define OWF_SURFACE_ROWPADDING          (4)
#define OWF_SURFACE_PREMULTIPLIED       (OWF_TRUE)
#define OWF_SCREEN_ROTATION_SUPPORT     (OWF_FALSE)
#define OWF_SUPPORT_ROTATION_0	        (0x1)
#define OWF_SUPPORT_ROTATION_90	        (0x2)
#define OWF_SUPPORT_ROTATION_180        (0x4)
#define OWF_SUPPORT_ROTATION_270        (0x8)
#define OWF_SUPPORT_ROTATION_ALL        (0xF)

typedef struct {
    OWFint width;
    OWFint height;
    OWFint stride;
} OWF_ORIENTATION_INFO;
        

typedef struct {
    OWFuint supportedRotations;
    OWF_ROTATION initialRotation;
    OWF_ROTATION currentRotation;
    OWF_PIXEL_FORMAT pixelFormat;
    OWF_ORIENTATION_INFO normal;
    OWF_ORIENTATION_INFO flipped;
    OWFboolean      inUse;    
} OWF_SCREEN;

typedef void (*OWF_SCREEN_CALLBACK)(void* obj, OWFint screenNumber, char event);

/*!
 * \brief Module initialization
 *
 * \return OWF_TRUE if initialization ok
 * \return OWF_FALSE if initialization failed
 */
OWF_API_CALL OWFboolean
OWF_Screen_Initialize();

/*!
 * \brief Module termination
 *
 * Delete screen and free resources
 */
OWF_API_CALL void
OWF_Screen_Terminate();

/*!
 * \brief Get systems's default screen
 *
 * \return The screen number of systems default screen
 */
OWF_API_CALL OWFint
OWF_Screen_GetDefaultNumber();

/*!
 * \brief Get screen's dimensions
 *
 * \param screen screen number
 * \param header returned screen attributes
 *
 * \return OWF_TRUE if screen exists
 * \return OWF_FALSE if screen does not exist
 */
OWF_API_CALL OWFboolean
OWF_Screen_GetHeader(OWFint screen, OWF_SCREEN* header);

/*!
 * \brief Create a new screen
 *
 * \param width
 * \param height
 *
 * \return screen number
 */
OWF_API_CALL OWFint
OWF_Screen_Create(OWFint width, OWFint height, OWF_SCREEN_CALLBACK func, void* obj);

/*!
 * \brief Destroy a screen
 *
 * \param screen number
 *
 * \return OWF_TRUE if screen exists
 * \return OWF_FALSE if screen does not exist
 */
OWF_API_CALL void
OWF_Screen_Destroy(OWFint screenNumber);

/*!
 * \brief Resize screen
 *
 * \param width
 * \param height
 *
 * \return OWF_TRUE if resize OK
 * \return OWF_FALSE if resize failed.
 */
OWF_API_CALL OWFboolean
OWF_Screen_Resize(OWFint screen, OWFint width, OWFint height);

/*!
 * \brief Blit image to screen
 *
 * \param screen screen number
 * \param rotation the context rotation
 *
 * \return OWF_TRUE if screen exists and blitting is  OK
 * \return OWF_FALSE if screen does not exist or blitting is not allowed.
 */
OWF_API_CALL OWFboolean
OWF_Screen_Blit(OWFint screenNumber, void* buffer, OWF_ROTATION rotation);

/*!
 * \brief Notify objects
 *
 * Send data to objects associated with screens
 *
 * \param data
 *
 */
OWF_API_CALL void
OWF_Screen_Notify(void* data);


/*!
 * \brief Checks if the screen number is valid
 *
 * \param screen the screen number
 *
 * \return OWF_TRUE if screen exists
 * \return OWF_FALSE if screen does not exist
 */
OWF_API_CALL OWFboolean
OWF_Screen_Valid(OWFint screenNumber);

/*!
 * \brief Checks if the screen rotation support is enabled
 *
 * \param screen screen number
 *
 * \return OWF_TRUE if the screen rotation is enabled
 * \return OWF_FALSE if the screen rotation is enabled
 */
OWF_API_CALL OWFboolean
OWF_Screen_Rotation_Supported(OWFint screenNumber);

#ifdef __cplusplus
}
#endif


#endif
