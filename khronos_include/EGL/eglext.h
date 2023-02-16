#ifndef __eglext_h_
#define __eglext_h_

#ifdef __cplusplus
extern "C" {
#endif

/*
** Copyright (c) 2007-2009 The Khronos Group Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
**
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#include <EGL/eglplatform.h>
#include <KHR/khrplatform.h>
/*************************************************************/

/* Header file version number */
/* Current version at http://www.khronos.org/registry/egl/ */
/* $Revision: 7244 $ on $Date: 2009-01-20 17:06:59 -0800 (Tue, 20 Jan 2009) $ */
#define EGL_EGLEXT_VERSION 3

#ifndef EGL_KHR_config_attribs
#define EGL_KHR_config_attribs 1
#define EGL_CONFORMANT_KHR          0x3042  /* EGLConfig attribute */
#define EGL_VG_COLORSPACE_LINEAR_BIT_KHR    0x0020  /* EGL_SURFACE_TYPE bitfield */
#define EGL_VG_ALPHA_FORMAT_PRE_BIT_KHR     0x0040  /* EGL_SURFACE_TYPE bitfield */
#endif

#ifndef EGL_KHR_lock_surface
#define EGL_KHR_lock_surface 1
#define EGL_READ_SURFACE_BIT_KHR        0x0001  /* EGL_LOCK_USAGE_HINT_KHR bitfield */
#define EGL_WRITE_SURFACE_BIT_KHR       0x0002  /* EGL_LOCK_USAGE_HINT_KHR bitfield */
#define EGL_LOCK_SURFACE_BIT_KHR        0x0080  /* EGL_SURFACE_TYPE bitfield */
#define EGL_OPTIMAL_FORMAT_BIT_KHR      0x0100  /* EGL_SURFACE_TYPE bitfield */
#define EGL_MATCH_FORMAT_KHR            0x3043  /* EGLConfig attribute */
#define EGL_FORMAT_RGB_565_EXACT_KHR        0x30C0  /* EGL_MATCH_FORMAT_KHR value */
#define EGL_FORMAT_RGB_565_KHR          0x30C1  /* EGL_MATCH_FORMAT_KHR value */
#define EGL_FORMAT_RGBA_8888_EXACT_KHR      0x30C2  /* EGL_MATCH_FORMAT_KHR value */
#define EGL_FORMAT_RGBA_8888_KHR        0x30C3  /* EGL_MATCH_FORMAT_KHR value */
#define EGL_MAP_PRESERVE_PIXELS_KHR     0x30C4  /* eglLockSurfaceKHR attribute */
#define EGL_LOCK_USAGE_HINT_KHR         0x30C5  /* eglLockSurfaceKHR attribute */
#define EGL_BITMAP_POINTER_KHR          0x30C6  /* eglQuerySurface attribute */
#define EGL_BITMAP_PITCH_KHR            0x30C7  /* eglQuerySurface attribute */
#define EGL_BITMAP_ORIGIN_KHR           0x30C8  /* eglQuerySurface attribute */
#define EGL_BITMAP_PIXEL_RED_OFFSET_KHR     0x30C9  /* eglQuerySurface attribute */
#define EGL_BITMAP_PIXEL_GREEN_OFFSET_KHR   0x30CA  /* eglQuerySurface attribute */
#define EGL_BITMAP_PIXEL_BLUE_OFFSET_KHR    0x30CB  /* eglQuerySurface attribute */
#define EGL_BITMAP_PIXEL_ALPHA_OFFSET_KHR   0x30CC  /* eglQuerySurface attribute */
#define EGL_BITMAP_PIXEL_LUMINANCE_OFFSET_KHR   0x30CD  /* eglQuerySurface attribute */
#define EGL_LOWER_LEFT_KHR          0x30CE  /* EGL_BITMAP_ORIGIN_KHR value */
#define EGL_UPPER_LEFT_KHR          0x30CF  /* EGL_BITMAP_ORIGIN_KHR value */
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLBoolean EGLAPIENTRY eglLockSurfaceKHR (EGLDisplay display, EGLSurface surface, const EGLint *attrib_list);
EGLAPI EGLBoolean EGLAPIENTRY eglUnlockSurfaceKHR (EGLDisplay display, EGLSurface surface);
#endif /* EGL_EGLEXT_PROTOTYPES */
#endif

#ifndef EGL_KHR_image
#define EGL_KHR_image 1
#define EGL_NATIVE_PIXMAP_KHR           0x30B0  /* eglCreateImageKHR target */
typedef void *EGLImageKHR;
#define EGL_NO_IMAGE_KHR            ((EGLImageKHR)0)
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLImageKHR EGLAPIENTRY eglCreateImageKHR (EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
EGLAPI EGLBoolean EGLAPIENTRY eglDestroyImageKHR (EGLDisplay dpy, EGLImageKHR image);
#endif /* EGL_EGLEXT_PROTOTYPES */
#endif

#ifndef EGL_KHR_vg_parent_image
#define EGL_KHR_vg_parent_image 1
#define EGL_VG_PARENT_IMAGE_KHR         0x30BA  /* eglCreateImageKHR target */
#endif

#ifndef EGL_KHR_gl_texture_2D_image
#define EGL_KHR_gl_texture_2D_image 1
#define EGL_GL_TEXTURE_2D_KHR           0x30B1  /* eglCreateImageKHR target */
#define EGL_GL_TEXTURE_LEVEL_KHR        0x30BC  /* eglCreateImageKHR attribute */
#endif

#ifndef EGL_KHR_gl_texture_cubemap_image
#define EGL_KHR_gl_texture_cubemap_image 1
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR  0x30B3  /* eglCreateImageKHR target */
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR  0x30B4  /* eglCreateImageKHR target */
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR  0x30B5  /* eglCreateImageKHR target */
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR  0x30B6  /* eglCreateImageKHR target */
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR  0x30B7  /* eglCreateImageKHR target */
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR  0x30B8  /* eglCreateImageKHR target */
#endif

#ifndef EGL_KHR_gl_texture_3D_image
#define EGL_KHR_gl_texture_3D_image 1
#define EGL_GL_TEXTURE_3D_KHR           0x30B2  /* eglCreateImageKHR target */
#define EGL_GL_TEXTURE_ZOFFSET_KHR      0x30BD  /* eglCreateImageKHR attribute */
#endif

#ifndef EGL_KHR_gl_renderbuffer_image
#define EGL_KHR_gl_renderbuffer_image 1
#define EGL_GL_RENDERBUFFER_KHR         0x30B9  /* eglCreateImageKHR target */
#endif

#ifndef EGL_KHR_image_base
#define EGL_KHR_image_base 1
/* Most interfaces defined by EGL_KHR_image_pixmap above */
#define EGL_IMAGE_PRESERVED_KHR         0x30D2  /* eglCreateImageKHR attribute */
#endif

#ifndef EGL_KHR_image_pixmap
#define EGL_KHR_image_pixmap 1
/* Interfaces defined by EGL_KHR_image above */
#endif



/*
 *  EGLSync definitions from draft specification
 */

/*
 * EGLSyncKHR is an opaque handle to an EGL sync object
 */
typedef void* EGLSyncKHR;

/*
 * EGLTimeKHR is a 64-bit unsigned integer representing intervals
 * in nanoseconds.  Here, it is defined as appropriate for an ISO
 * C compiler.
 */

typedef khronos_utime_nanoseconds_t EGLTimeKHR;

/*
 *   Accepted in the condition parameter of eglCreateFenceSyncKHR, and
 *   returned in value when eglGetSyncAttribKHR is called with attribute
 *   EGL_SYNC_CONDITION_KHR:
 */
#define  EGL_SYNC_PRIOR_COMMANDS_COMPLETE_KHR    0x30F0

/*
 *   Accepted as an attribute name in the attrib_list parameter of
 *   eglCreateFenceSyncKHR, and by the attribute parameter of
 *  eglGetSyncAttribKHR:
 */
#define  EGL_SYNC_STATUS_KHR                     0x30F1

/*
 *   Accepted as an attribute value in the attrib_list parameter of
 *   eglCreateFenceSyncKHR for the EGL_SYNC_STATUS_KHR attribute, by
 *   the mode parameter of eglSignalSyncKHR and returned in value
 *   when eglGetSyncAttribKHR is called with attribute
 *   EGL_SYNC_STATUS_KHR:
 */
#define  EGL_SIGNALED_KHR                        0x30F2
#define  EGL_UNSIGNALED_KHR                      0x30F3

/*   Accepted as an attribute name in the <attrib_list> parameter of
 *   eglCreateFenceSyncKHR, and by the <attribute> parameter of
 *   eglGetSyncAttribKHR:
 */
#define EGL_AUTO_RESET_KHR                      0x30F4

/*   Accepted in the flags parameter of eglClientWaitSyncKHR: */

#define  EGL_SYNC_FLUSH_COMMANDS_BIT_KHR         0x0001

/*   Accepted in the timeout parameter of eglClientWaitSyncKHR: */
#define EGL_FOREVER_KHR                         0xFFFFFFFFFFFFFFFFull

/*   Returned by eglClientWaitSyncKHR: */

#define EGL_TIMEOUT_EXPIRED_KHR                 0x30F5
#define EGL_CONDITION_SATISFIED_KHR             0x30F6

/*   Accepted in the attribute parameter of eglGetSyncAttribKHR: */

#define EGL_SYNC_TYPE_KHR                       0x30F7
#define EGL_SYNC_CONDITION_KHR                  0x30F8

/*   Returned in value when eglGetSyncAttribKHR is called with
    attribute EGL_SYNC_TYPE_KHR: */

#define EGL_SYNC_FENCE_KHR                      0x30F9

/*   Accepted by the <type> parameter of eglCreateSyncKHR, and returned
 *   in <value> when eglGetSyncAttribKHR is called with <attribute>
 *   EGL_SYNC_TYPE_KHR:
 */
#define EGL_SYNC_REUSABLE_KHR			0x30FA

/*   Returned by eglCreateFenceSyncKHR in the event of an error: */

#define EGL_NO_SYNC_KHR                         ((EGLSyncKHR)0)

/* API functions */

/*!
 * Creates a sync object of the specified type assocaited with the
 * specified display <dpy>, and returns a handle to the new object.
 * <attrib_list> is an attribute-value list specifying other attributes
 * of the sync object, terminated by an attribute entry EGL_NONE.
 * Attributes not specified in the list will be assigned their default
 * values.
 *
 * \return Returns a handle to valid sync object. If <attrib_list> is
 *  neither NULL nor empty (containing only EGL_NONE_, EGL_NO_SYNC_KHR
 * is returned. If dpy is not the name of a valid, initialized 
 * EGLDisplay, EGL_NO_SYNC_KHR is returned.
 *
 *
 *
 */
EGLAPI EGLSyncKHR EGLAPIENTRY
eglCreateSyncKHR( EGLDisplay dpy,
                  EGLenum type,
                  const EGLint *attrib_list );



/*!
 * Is used to destroy an existing sync object. If any
 * eglClientWaitSyncKHR commands are blocking on sync when
 * eglDestroySyncKHR is called, the object is destroyd.
 * After calling eglDestroySyncKHR, sync is no longer a valid sync object.
 *
 * \return Assuming no errors are generated, EGL_TRUE is returned.
 * If sync is not a valid sync object, EGL_FALSE is returned
 *
 */
EGLAPI EGLBoolean EGLAPIENTRY
eglDestroySyncKHR( EGLDisplay dpy, EGLSyncKHR sync );

/*!
 *
 */
EGLAPI EGLBoolean EGLAPIENTRY
eglFenceKHR( EGLDisplay dpy, EGLSyncKHR sync );

/*!
 *     Blocks the calling thread until the specified sync object sync
 *  is signaled, or until a specified timeout value expires.
 *  If sync is signaled at the time eglClientWaitSyncKHR is called,
 *  then eglClientWaitSyncKHR will not block.
 *
 *  \return  glClientWaitSyncKHR returns one of four status values
 *  describing the reason for returning. A return value of
 *  EGL_ALREADY_SIGNALED_KHR will always be returned if sync
 *  was signaled when eglClientWaitSyncKHR was called, even if
 *  timeout is zero. A return value of EGL_TIMEOUT_EXPIRED_KHR
 *  indicates that the specified timeout period expired before
 *  sync was signaled. A return value of
 *  EGL_CONDITION_SATISFIED_KHR indicates that sync was signaled
 *  before the timeout expired. If an error occurs then an error
 *  is generated and EGL_FALSE is returned.
 *
 */
EGLAPI EGLint EGLAPIENTRY
eglClientWaitSyncKHR( EGLDisplay dpy,
                      EGLSyncKHR sync,
                      EGLint flags,
                      EGLTimeKHR timeout );

/*!
 *
 * Signals or unsignals the sync object sync by changing its
 * status to mode, which must be either EGL_SIGNALED_KHR or
 * EGL_UNSIGNALED_KHR. If, as a result of calling eglSignalSyncKHR,
 * the status of sync transitions from unsignaled to signaled,
 * then any eglClientWaitSyncKHR commands blocking on sync will
 * unblock.
 *
 * \return Assuming no errors are generated, EGL_TRUE is returned.
 * If sync is not a valid sync object, EGL_FALSE is returned.
 * If mode has invalid value, EGL_FALSE is returned.
 *
 */
EGLAPI EGLBoolean EGLAPIENTRY
eglSignalSyncKHR( EGLDisplay dpy,
                  EGLSyncKHR sync,
                  EGLenum mode );

/*!
 *
 */
EGLAPI EGLBoolean EGLAPIENTRY
eglGetSyncAttribKHR( EGLDisplay dpy,
                     EGLSyncKHR sync,
                     EGLint attribute,
                     EGLint *value );



#ifdef __cplusplus
}
#endif

#endif
