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


#include <SDL/SDL.h>

#include "owfdebug.h"
#include "owfscreen.h"
#include "owfimage.h"
#include "owfdisplaycontextgeneral.h"



#define NBR_SCREENS 1
#define DEFAULT_SCREEN 1


/*
         *  This module hardware screen setup.
 *
 *  Only one 'physical' screen is available for the time being.
 *  It is using up the whole video surface.
 *
 *  More screens can be arranged by dividing SDL surface
 *  among screens, and storing screen information in a table.
 */

static OWF_SCREEN defaultScreen;

OWF_API_CALL OWFint
OWF_Screen_GetDefaultNumber()
{
    return DEFAULT_SCREEN;
}

OWF_API_CALL OWFboolean
OWF_Screen_GetHeader(OWFint screenNumber, OWF_SCREEN* header)
{
    if (screenNumber < 1 || screenNumber > NBR_SCREENS)
    {
        return OWF_FALSE;
    }

    memcpy(header, &defaultScreen, sizeof(*header));
    header->inUse = OWF_DisplayContext_IsLive(screenNumber);
    return OWF_TRUE;
}


OWF_API_CALL OWFboolean
OWF_Screen_Initialize()
{

    SDL_Surface* videoSurf;
    OWFint bpp; /* bits per pixel */
    
    defaultScreen.supportedRotations = OWF_SUPPORT_ROTATION_ALL;
    defaultScreen.initialRotation = OWF_ROTATION_0;
    defaultScreen.currentRotation = OWF_ROTATION_0;
    defaultScreen.pixelFormat = OWF_IMAGE_ARGB8888;
    defaultScreen.normal.width = OWF_SURFACE_WIDTH;
    defaultScreen.normal.height = OWF_SURFACE_HEIGHT;
    defaultScreen.normal.stride = defaultScreen.normal.width * OWF_SURFACE_PIXEL_FORMAT;
    defaultScreen.flipped.width = OWF_SURFACE_HEIGHT;
    defaultScreen.flipped.height = OWF_SURFACE_WIDTH;
    defaultScreen.flipped.stride = defaultScreen.flipped.width * OWF_SURFACE_PIXEL_FORMAT;

    bpp = 8 * OWF_Image_GetFormatPixelSize(OWF_SURFACE_PIXEL_FORMAT);

    videoSurf = SDL_SetVideoMode(defaultScreen.normal.width,
                                 defaultScreen.normal.height,
                                 bpp,
                                 SDL_SWSURFACE);
    atexit(OWF_Screen_Terminate); /* set SDL exit handler */

    DPRINT(("SDL: OWF_Screen_Initialize (Exit)"));
    return videoSurf != NULL;
}

OWF_API_CALL OWFboolean
OWF_Screen_Resize(OWFint screen, OWFint width, OWFint height)
{
        SDL_Surface* videoSurf;
        OWFint bpp;
        if (screen < 1 || screen > NBR_SCREENS)
        {
                return OWF_FALSE;
        }
        
        SDL_Quit();
        
        bpp = 8 * OWF_Image_GetFormatPixelSize(OWF_SURFACE_PIXEL_FORMAT);

        videoSurf = SDL_SetVideoMode(width,
                                     height,
                                     bpp,
                                     SDL_SWSURFACE);
        return videoSurf != NULL;
}


static OWFboolean
OWF_Rotation_Supported(OWFint screenNumber, OWF_ROTATION rotation)
{
    OWFint mask = 0;
    OWF_ROTATION supportedRotation = 0;
    if (OWF_Screen_Rotation_Supported(screenNumber))
    {
        supportedRotation = defaultScreen.supportedRotations;
        switch (rotation)
        {
            case OWF_ROTATION_0:
                mask = OWF_SUPPORT_ROTATION_0;
                break;
            case OWF_ROTATION_90:
                mask = OWF_SUPPORT_ROTATION_90;
                break;
            case OWF_ROTATION_180:
                mask = OWF_SUPPORT_ROTATION_180;
                break;
            case OWF_ROTATION_270:
                mask = OWF_SUPPORT_ROTATION_270;
                break;
            default:
                break;
        }
    }
    
    return supportedRotation & mask;
}

OWF_API_CALL OWFboolean
OWF_Screen_Blit(OWFint screenNumber, void* buffer, OWF_ROTATION rotation)
{
    SDL_Surface* surf = NULL;

    if (screenNumber < 1 || screenNumber > NBR_SCREENS)
    {
        return OWF_FALSE;
    }

    if (OWF_Rotation_Supported (screenNumber, rotation))
    {
        if (rotation != defaultScreen.currentRotation)
        {
            OWFint width = defaultScreen.normal.width;
            OWFint height = defaultScreen.normal.height;
            
            if (rotation == OWF_ROTATION_90 || rotation == OWF_ROTATION_270)
            {
                width = defaultScreen.flipped.width;
                height = defaultScreen.flipped.height;
            }
            
            OWF_Screen_Resize(screenNumber, width, height);
            defaultScreen.currentRotation = rotation;
        }
    }
    
    surf = SDL_GetVideoSurface();
    
    if (surf && SDL_LockSurface(surf)==0)
    {
        OWF_ASSERT(buffer);
        OWF_ASSERT(surf->pixels);

        memcpy(surf->pixels,
               buffer,
               surf->w*surf->h*surf->format->BytesPerPixel );
        SDL_UnlockSurface(surf);
        SDL_UpdateRect(surf, 0, 0, 0, 0);
        DPRINT(("SDL: OWF_Screen_Blit (Exit good)"));
        return OWF_TRUE;
    }

    return OWF_FALSE;
}


OWF_API_CALL void
OWF_Screen_Terminate()
{
    SDL_Quit();
}

OWF_API_CALL OWFboolean 
OWF_Screen_Valid(OWFint screenNumber)
{
    return (screenNumber >= 1 && screenNumber <= NBR_SCREENS) ? OWF_TRUE : OWF_FALSE;
}


OWF_API_CALL OWFboolean
OWF_Screen_Rotation_Supported(OWFint screenNumber)
{
    return (screenNumber >= 1 && screenNumber <= NBR_SCREENS && OWF_SCREEN_ROTATION_SUPPORT) ? OWF_TRUE : OWF_FALSE;
}

#ifdef __cplusplus
}
#endif

