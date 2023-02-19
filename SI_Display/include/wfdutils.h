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
 *  \file wfdutils.h
 *  \brief Helper routine collection for Display SI
 *
 */

#ifndef WFDUTIL_H_
#define WFDUTIL_H_

#include "owfattributes.h"
#include "owfimage.h"
#include "wfdstructs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RECT_SIZE 4 /* rectangle vector size */
#define RECT_OFFSETX 0
#define RECT_OFFSETY 1
#define RECT_WIDTH 2
#define RECT_HEIGHT 3

#define BG_SIZE 3 /* background color vector size */

typedef void (*ATTR_ACCESSOR)(void);

/*! \brief Check attribute accessor validity
 *
 * Check if function is a valid accessor for attribute
 *
 * \param attrib attribute identifier
 * \param func pointer to accessor function
 *
 * \return true or false
 */
OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Util_ValidAccessorForAttrib(WFDint attrib, ATTR_ACCESSOR func) OWF_APIEXIT;

/*! \brief Convert an error code
 *
 *  Convert an error code form  OWF_ATTRIBUTE domain to
 *  WFD domain.
 *
 *  \param attrError OWF_ATTRIBUTE error code
 *
 *  \return WFDErrorCode
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Util_AttrEc2WfdEc(OWF_ATTRIBUTE_LIST_STATUS attrError) OWF_APIEXIT;

/*! \brief Check an event attribute against an event type
 *
 * Check if an event attribute can be used in
 * association with the given event type.
 *
 *  \param eventType Type of an event
 *  \param attrib Event attribute
 *
 *  \return true or false
 */
OWF_API_CALL WFDboolean OWF_APIENTRY WFD_Util_ValidAttributeForEvent(
    WFDEventType eventType, WFDEventAttrib attrib) OWF_APIEXIT;

/*! \brief Convert a float to unsigned byte
 *
 *  \param f Float value
 *
 *  \return 8-bit unsigned value
 */
OWF_API_CALL WFDuint8 OWF_APIENTRY WFD_Util_Float2Byte(WFDfloat f) OWF_APIEXIT;

/*! \brief Convert background color from floating point vector presentation to
 * 32-bit integer presentation.
 *
 *
 *  \param count Number of elements in value array. Must always be 3.
 *  \param  value An array of floating point color elements
 *
 *  \return 32-bit integer presentation of the background color.
 *  Background color is alway fully opaque, so the 8 least significant bits
 *  are always set to value 0xFF.
 */
OWF_API_CALL WFDint OWF_APIENTRY
WFD_Util_BgFv2Int(WFDint count, const WFDfloat* value) OWF_APIEXIT;

/*! \brief Convert background color from  32-bit integer presentation to
 * floating point vector presentation.
 *
 *  \param value 32-bit value to be converted
 *  \param count Number of elements in result array. Should be at least 3 and
 *  at most 4 elements are filled.
 *  \param result An array of floating point color elements that contain the
 *  floating point representation when this routine returns.
 *
 */
OWF_API_CALL void OWF_APIENTRY WFD_Util_BgInt2Fv(WFDint value, WFDint count,
                                                 WFDfloat* result) OWF_APIEXIT;

/*! \brief Convert background color from  32-bit integer vector presentation to
 * floating point  vector presentation.
 *
 */
OWF_API_CALL void OWF_APIENTRY WFD_Util_BgFv2Iv(WFDint count,
                                                const WFDfloat* value,
                                                WFDint* result) OWF_APIEXIT;

/*! \brief   Convert background color from floating point vector presentation to
 * 32-bit integer  vector presentation..
 *
 */
OWF_API_CALL void OWF_APIENTRY WFD_Util_BgIv2Fv(WFDint count,
                                                const WFDint* value,
                                                WFDfloat* result) OWF_APIEXIT;

/*! \brief Initialize an scratch buffer array
 *
 *  Initialize a vector of images of indentical size.
 *
 *  \return WFD_TRUE when initialization succeeds
 *  \return WFD_FALSE when no memory is available for scratch buffers
 *
 */

OWF_API_CALL WFDboolean OWF_APIENTRY WFD_Util_InitScratchBuffer(
    OWF_IMAGE** scratchArray, WFDint arraySize, WFDint w, WFDint h) OWF_APIEXIT;

/*! \brief Check if given color can be used as transparent source color for a
 * pipeline
 *
 *  See WFD_Pipeline_SetTSColor()
 *
 *  \return WFD_TRUE when color is valid
 *  \return WFD_FALSE when color is invalid
 */
OWF_API_CALL WFDboolean OWF_APIENTRY WFD_Util_IsValidTSColor(
    WFDTSColorFormat colorFormat, WFDint count, const void* color) OWF_APIEXIT;
/*! \brief Converts a color specification to internal color format
 *
 * \param colorFormat original color format
 * \param count number of items in color array
 * \param color an array of color elements
 * \param tsColor target color presentation
 */
OWF_API_CALL void OWF_APIENTRY
WFD_Util_ConverTSColor(WFDTSColorFormat colorFormat, WFDint count,
                       const void* color, WFD_TS_COLOR* tsColor) OWF_APIEXIT;

/*! \brief Convert transparency feature to internal presentation.
 *
 *  \return Internal transparency presentation
 */
OWF_API_CALL OWF_TRANSPARENCY WFD_Util_GetBlendMode(
    WFDTransparency transparency, WFDboolean hasMask) OWF_APIEXIT;

/*! \brief Check if a rectangle is fully contained inside an image
 *
 *  \param rect value array
 *  \param count count of values in rect
 *  \param width image width
 *  \param height image height
 *
 *  \return WFD_TRUE or WFD_FALSE
 */
OWF_API_CALL WFDboolean WFD_Util_RectIsFullyContained(
    const WFDint* rect, WFDint count, WFDint width, WFDint height) OWF_APIEXIT;

/*! \brief Check rectangle values
 *
 *  Check that rectangle offsets, width and height are positive,
 *  and offsetx+width or offsety+height does not overflow
 *
 *  \param rect value array
 *  \param count count of values in rect
 *
 *  \return WFD_TRUE or WFD_FALSE
 */
OWF_API_CALL WFDboolean WFD_Util_IsRectValid(const WFDint* rect,
                                             WFDint count) OWF_APIEXIT;

#ifdef __cplusplus
}
#endif

#endif /* WFDUTIL_H_ */
