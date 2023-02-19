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
 *  \file wfdutils.c
 *  \brief OpenWF Display SI, helper functions implementation.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "wfdutils.h"

#include <math.h>
#include <stdio.h>

#include "owftypes.h"
#include "wfddebug.h"

/* Configuration attributes' getters */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Util_AttrEc2WfdEc(OWF_ATTRIBUTE_LIST_STATUS attrError) OWF_APIEXIT {
    WFDErrorCode wfdError = WFD_ERROR_NONE;

    switch (attrError) {
        case ATTR_ERROR_NONE:
            wfdError = WFD_ERROR_NONE;
            break;
        case ATTR_ERROR_INVALID_TYPE:
        case ATTR_ERROR_INVALID_ARGUMENT:
            wfdError = WFD_ERROR_ILLEGAL_ARGUMENT;
            break;
        case ATTR_ERROR_ACCESS_DENIED:
        case ATTR_ERROR_INVALID_ATTRIBUTE:
            wfdError = WFD_ERROR_BAD_ATTRIBUTE;
            break;
        default:
            wfdError = WFD_ERROR_BAD_ATTRIBUTE;
    }
    return wfdError;
}

OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Util_ValidAccessorForAttrib(WFDint attrib, ATTR_ACCESSOR func) OWF_APIEXIT {
    switch (attrib) {
        /* Device attributes: RO */
        case WFD_DEVICE_ID:
            return func == (ATTR_ACCESSOR)wfdGetDeviceAttribi;

        /* Event container attributes: RO */
        case WFD_EVENT_PIPELINE_BIND_QUEUE_SIZE:
        case WFD_EVENT_TYPE:
        case WFD_EVENT_PORT_ATTACH_PORT_ID:
        case WFD_EVENT_PORT_ATTACH_STATE:
        case WFD_EVENT_PORT_PROTECTION_PORT_ID:
        case WFD_EVENT_PIPELINE_BIND_PIPELINE_ID:
        case WFD_EVENT_PIPELINE_BIND_SOURCE:
        case WFD_EVENT_PIPELINE_BIND_MASK:
        case WFD_EVENT_PIPELINE_BIND_QUEUE_OVERFLOW:
            return func == (ATTR_ACCESSOR)wfdGetEventAttribi;

        /* Port mode attributes: RO */
        case WFD_PORT_MODE_WIDTH:
        case WFD_PORT_MODE_HEIGHT:
        case WFD_PORT_MODE_FLIP_MIRROR_SUPPORT:
        case WFD_PORT_MODE_ROTATION_SUPPORT:
        case WFD_PORT_MODE_INTERLACED:
            return func == (ATTR_ACCESSOR)wfdGetPortModeAttribi;

        case WFD_PORT_MODE_REFRESH_RATE:
            return func == (ATTR_ACCESSOR)wfdGetPortModeAttribi ||
                   func == (ATTR_ACCESSOR)wfdGetPortModeAttribf;

        /* Port attributes: RO */
        case WFD_PORT_ID:
        case WFD_PORT_TYPE:
        case WFD_PORT_DETACHABLE:
        case WFD_PORT_ATTACHED:
        case WFD_PORT_FILL_PORT_AREA:
        case WFD_PORT_PARTIAL_REFRESH_SUPPORT:
        case WFD_PORT_PIPELINE_ID_COUNT:
            return func == (ATTR_ACCESSOR)wfdGetPortAttribi;

        case WFD_PORT_NATIVE_RESOLUTION:
        case WFD_PORT_PARTIAL_REFRESH_MAXIMUM:
            return func == (ATTR_ACCESSOR)wfdGetPortAttribiv;

        case WFD_PORT_PHYSICAL_SIZE:
        case WFD_PORT_GAMMA_RANGE:
            return func == (ATTR_ACCESSOR)wfdGetPortAttribfv;

            /* Port attributes: R/W*/

        case WFD_PORT_BACKGROUND_COLOR:
            return func == (ATTR_ACCESSOR)wfdGetPortAttribi ||
                   func == (ATTR_ACCESSOR)wfdGetPortAttribiv ||
                   func == (ATTR_ACCESSOR)wfdGetPortAttribfv ||
                   func == (ATTR_ACCESSOR)wfdSetPortAttribi ||
                   func == (ATTR_ACCESSOR)wfdSetPortAttribiv ||
                   func == (ATTR_ACCESSOR)wfdSetPortAttribfv;

        case WFD_PORT_FLIP:
        case WFD_PORT_MIRROR:
        case WFD_PORT_ROTATION:
        case WFD_PORT_POWER_MODE:
        case WFD_PORT_PARTIAL_REFRESH_ENABLE:
        case WFD_PORT_PROTECTION_ENABLE:
            return func == (ATTR_ACCESSOR)wfdGetPortAttribi ||
                   func == (ATTR_ACCESSOR)wfdSetPortAttribi;

        case WFD_PORT_GAMMA:
            return func == (ATTR_ACCESSOR)wfdGetPortAttribf ||
                   func == (ATTR_ACCESSOR)wfdSetPortAttribf;

        case WFD_PORT_PARTIAL_REFRESH_RECTANGLE:
        case WFD_PORT_BINDABLE_PIPELINE_IDS:
            return func == (ATTR_ACCESSOR)wfdGetPortAttribiv ||
                   func == (ATTR_ACCESSOR)wfdSetPortAttribiv;

        /* Pipeline attributes: RO */
        case WFD_PIPELINE_ID:
        case WFD_PIPELINE_PORTID:
        case WFD_PIPELINE_LAYER:
        case WFD_PIPELINE_SHAREABLE:
        case WFD_PIPELINE_DIRECT_REFRESH:
        case WFD_PIPELINE_ROTATION_SUPPORT:
            return func == (ATTR_ACCESSOR)wfdGetPipelineAttribi;

        case WFD_PIPELINE_MAX_SOURCE_SIZE:
            return func == (ATTR_ACCESSOR)wfdGetPipelineAttribiv ||
                   func == (ATTR_ACCESSOR)wfdGetPipelineAttribfv;

        case WFD_PIPELINE_SCALE_RANGE:
            return func == (ATTR_ACCESSOR)wfdGetPipelineAttribfv;

        /* Pipeline attributes: R/W */
        case WFD_PIPELINE_SOURCE_RECTANGLE:
        case WFD_PIPELINE_DESTINATION_RECTANGLE:
            return func == (ATTR_ACCESSOR)wfdGetPipelineAttribiv ||
                   func == (ATTR_ACCESSOR)wfdGetPipelineAttribfv ||
                   func == (ATTR_ACCESSOR)wfdSetPipelineAttribiv ||
                   func == (ATTR_ACCESSOR)wfdSetPipelineAttribfv;

        case WFD_PIPELINE_FLIP:
        case WFD_PIPELINE_MIRROR:
        case WFD_PIPELINE_ROTATION:
        case WFD_PIPELINE_SCALE_FILTER:
        case WFD_PIPELINE_TRANSPARENCY_ENABLE:
            return func == (ATTR_ACCESSOR)wfdGetPipelineAttribi ||
                   func == (ATTR_ACCESSOR)wfdSetPipelineAttribi;

        case WFD_PIPELINE_GLOBAL_ALPHA:
            return func == (ATTR_ACCESSOR)wfdGetPipelineAttribi ||
                   func == (ATTR_ACCESSOR)wfdGetPipelineAttribf ||
                   func == (ATTR_ACCESSOR)wfdSetPipelineAttribi ||
                   func == (ATTR_ACCESSOR)wfdSetPipelineAttribf;
        default:
            break;
    }

    return WFD_FALSE;
}

OWF_API_CALL WFDboolean OWF_APIENTRY WFD_Util_ValidAttributeForEvent(
    WFDEventType et, WFDEventAttrib at) OWF_APIEXIT {
    WFDboolean result = WFD_FALSE;

    if (at == WFD_EVENT_PIPELINE_BIND_QUEUE_SIZE) {
        return WFD_TRUE;
    }

    switch (et) {
        case WFD_EVENT_DESTROYED: {
            break;
        }
        case WFD_EVENT_PORT_ATTACH_DETACH: {
            result = (WFD_EVENT_PORT_ATTACH_PORT_ID == at ||
                      WFD_EVENT_PORT_ATTACH_STATE == at);
            break;
        }
        case WFD_EVENT_PIPELINE_BIND_SOURCE_COMPLETE: {
            result = (WFD_EVENT_PIPELINE_BIND_PIPELINE_ID == at ||
                      WFD_EVENT_PIPELINE_BIND_SOURCE == at ||
                      WFD_EVENT_PIPELINE_BIND_QUEUE_OVERFLOW == at);
            break;
        }
        case WFD_EVENT_PIPELINE_BIND_MASK_COMPLETE: {
            result = (WFD_EVENT_PIPELINE_BIND_PIPELINE_ID == at ||
                      WFD_EVENT_PIPELINE_BIND_MASK == at ||
                      WFD_EVENT_PIPELINE_BIND_QUEUE_OVERFLOW == at);
            break;
        }
        case WFD_EVENT_PORT_PROTECTION_FAILURE: {
            result = (WFD_EVENT_PORT_PROTECTION_PORT_ID == at);
            break;
        }
        case WFD_EVENT_NONE: {
            result = (WFD_EVENT_TYPE == at);
            break;
        }
        case WFD_EVENT_INVALID: {
            result = (WFD_EVENT_TYPE == at);
            break;
            default:
                break;
        }
    }

    return result;
}

OWF_API_CALL WFDuint8 OWF_APIENTRY WFD_Util_Float2Byte(WFDfloat f) OWF_APIEXIT {
    WFDfloat tmp1, tmp2;

    tmp1 = (WFDfloat)ceil(f * 255.0);
    tmp2 = f * 255.0;

    /* poor man's round: C89 library does not support round() */
    if (tmp1 - tmp2 >= 0.50) {
        tmp1 = tmp1 - 1;
    }

    return (WFDuint8)tmp1 & 0xFF;
}

OWF_API_CALL WFDint OWF_APIENTRY
WFD_Util_BgFv2Int(WFDint count, const WFDfloat *value) OWF_APIEXIT {
    /* pre-conditions: values in value array must be [0,1] */

    WFDint i;
    unsigned result = 0;

    for (i = 0; i < 3 && i < count; i++) {
        WFDuint8 byte;

        byte = WFD_Util_Float2Byte(value[i]);
        result = (result << 8) | byte;
    }

    while (i++ < 4) {
        result = (result << 8);
    }

    /* background always fully opaque */
    return (WFDint)result | 0xFF;
}

OWF_API_CALL void OWF_APIENTRY WFD_Util_BgInt2Fv(WFDint value, WFDint count,
                                                 WFDfloat *result) OWF_APIEXIT {
    if (count > 0) {
        result[0] = (WFDfloat)((OWFuint32)value >> 24) / 255.0;
    }
    if (count > 1) {
        result[1] = (WFDfloat)(((OWFuint32)value >> 16) & 0xFF) / 255.0;
    }
    if (count > 2) {
        result[2] = (WFDfloat)(((OWFuint32)value >> 8) & 0xFF) / 255.0;
    }
    if (count > 3) {
        result[3] = (WFDfloat)((OWFuint32)value & 0xFF) / 255.0;
    }
}

OWF_API_CALL void OWF_APIENTRY WFD_Util_BgFv2Iv(WFDint count,
                                                const WFDfloat *value,
                                                WFDint *result) OWF_APIEXIT {
    if (count > 0) {
        result[0] = WFD_Util_Float2Byte(value[0]) & 0xFF;
    }
    if (count > 1) {
        result[1] = WFD_Util_Float2Byte(value[1]) & 0xFF;
    }
    if (count > 2) {
        result[2] = WFD_Util_Float2Byte(value[2]) & 0xFF;
    }
    if (count > 3) {
        result[3] = WFD_Util_Float2Byte(value[3]) & 0xFF;
    }
}

OWF_API_CALL void OWF_APIENTRY WFD_Util_BgIv2Fv(WFDint count,
                                                const WFDint *value,
                                                WFDfloat *result) OWF_APIEXIT {
    if (count > 0) {
        result[0] = (WFDfloat)value[0] / 255.0;
    }
    if (count > 1) {
        result[1] = (WFDfloat)value[1] / 255.0;
    }
    if (count > 2) {
        result[2] = (WFDfloat)value[2] / 255.0;
    }
    if (count > 3) {
        result[3] = (WFDfloat)value[3] / 255.0;
    }
}

OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Util_InitScratchBuffer(OWF_IMAGE **scratchArray, WFDint arraySize, WFDint w,
                           WFDint h) OWF_APIEXIT {
    WFDint i;
    WFDboolean ret = WFD_TRUE;

    OWF_ASSERT(scratchArray && arraySize > 0);

    for (i = 0; i < arraySize; i++) {
        OWF_IMAGE_FORMAT format;

        format.pixelFormat = OWF_IMAGE_ARGB_INTERNAL;
        format.linear = OWF_FALSE;
        format.premultiplied = OWF_FALSE;
        format.rowPadding = OWF_Image_GetFormatPadding(format.pixelFormat);

        scratchArray[i] = OWF_Image_Create(w, h, &format, NULL, 0);
        ret = ret && scratchArray[i];
    }

    if (!ret) {
        /* clean up if any image creation failed */
        for (i = 0; i < arraySize; i++) {
            if (scratchArray[i]) {
                OWF_Image_Destroy(scratchArray[i]);
            }
        }
    }

    return ret;
}

OWF_API_CALL WFDboolean OWF_APIENTRY WFD_Util_IsValidTSColor(
    WFDTSColorFormat colorFormat, WFDint count, const void *color) OWF_APIEXIT {
    WFDboolean valid = WFD_FALSE;
    WFDuint8 *rgb;

    switch (colorFormat) {
        case WFD_TSC_FORMAT_UINT8_RGB_8_8_8_LINEAR:
            if (count == 3) {
                valid = WFD_TRUE;
            }
            break;

        case WFD_TSC_FORMAT_UINT8_RGB_5_6_5_LINEAR:
            rgb = (WFDuint8 *)color;
            if (count == 3 && rgb[0] < 32 && rgb[1] < 64 && rgb[2] < 32) {
                valid = WFD_TRUE;
            }
            break;

        default:
            valid = WFD_FALSE; /* not supported */
    }

    return valid;
}

OWF_API_CALL void OWF_APIENTRY
WFD_Util_ConverTSColor(WFDTSColorFormat colorFormat, WFDint count,
                       const void *color, WFD_TS_COLOR *tsColor) OWF_APIEXIT {
    /* pre-conditions: color is valid color */

    WFDuint8 *rgb;

    OWF_ASSERT(color && count == BG_SIZE);
    OWF_ASSERT(tsColor);

    tsColor->colorFormat = colorFormat;

    /* only 8-bit formats currently in use (spec version 1.0) */
    switch (colorFormat) {
        case WFD_TSC_FORMAT_UINT8_RGB_8_8_8_LINEAR:
            rgb = (WFDuint8 *)color;

            tsColor->color.color.red = rgb[0] / 255.0;
            tsColor->color.color.green = rgb[1] / 255.0;
            tsColor->color.color.blue = rgb[2] / 255.0;
            tsColor->color.color.alpha = 1.0;

            break;
        case WFD_TSC_FORMAT_UINT8_RGB_5_6_5_LINEAR:
            rgb = (WFDuint8 *)color;

            tsColor->color.color.red = rgb[0] / 31.0f;
            tsColor->color.color.green = rgb[1] / 63.0f;
            tsColor->color.color.blue = rgb[2] / 31.0f;
            tsColor->color.color.alpha = 1.0;

            break;

        default:
            OWF_ASSERT(0); /* should never happen */
    }
}

OWF_API_CALL OWF_TRANSPARENCY OWF_APIENTRY WFD_Util_GetBlendMode(
    WFDTransparency transparency, WFDboolean hasMask) OWF_APIEXIT {
    OWF_TRANSPARENCY blendMode = OWF_TRANSPARENCY_NONE;

    if (transparency & WFD_TRANSPARENCY_GLOBAL_ALPHA) {
        blendMode |= OWF_TRANSPARENCY_GLOBAL_ALPHA;
        DPRINT(("  blend mode contains OWF_TRANSPARENCY_GLOBAL_ALPHA"));
    }

    if (transparency & WFD_TRANSPARENCY_SOURCE_ALPHA) {
        blendMode |= OWF_TRANSPARENCY_SOURCE_ALPHA;
        DPRINT(("  blend mode contains OWF_TRANSPARENCY_SOURCE_ALPHA"));
    }

    if ((transparency & WFD_TRANSPARENCY_MASK) && hasMask) {
        blendMode |= OWF_TRANSPARENCY_MASK;
        DPRINT(("  blend mode contains OWF_TRANSPARENCY_MASK"));
    }

    if (blendMode == OWF_TRANSPARENCY_NONE) {
        DPRINT(("  blend mode is OWF_TRANSPARENCY_NONE"));
    }

    return blendMode;
}

OWF_API_CALL WFDboolean OWF_APIENTRY WFD_Util_RectIsFullyContained(
    const WFDint *rect, WFDint count, WFDint width, WFDint height) OWF_APIEXIT {
    WFDboolean ok = WFD_TRUE;

    OWF_ASSERT(rect && count == RECT_SIZE);

    ok = ok && (rect[RECT_OFFSETX] >= 0);
    ok = ok && (rect[RECT_OFFSETY] >= 0);

    ok = ok && (width >= rect[RECT_OFFSETX] + rect[RECT_WIDTH]);
    ok = ok && (height >= rect[RECT_OFFSETY] + rect[RECT_HEIGHT]);

    return ok;
}

OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Util_IsRectValid(const WFDint *values, WFDint count) OWF_APIEXIT {
    if (count != RECT_SIZE) {
        return WFD_FALSE;
    }

    if (values[RECT_OFFSETX] < 0 || values[RECT_OFFSETY] < 0) {
        return WFD_FALSE;
    }

    if (values[RECT_OFFSETX] + values[RECT_WIDTH] < values[RECT_WIDTH] ||
        values[RECT_OFFSETX] + values[RECT_HEIGHT] < values[RECT_HEIGHT]) {
        return WFD_FALSE;
    }

    return WFD_TRUE;
}
#ifdef __cplusplus
}
#endif
