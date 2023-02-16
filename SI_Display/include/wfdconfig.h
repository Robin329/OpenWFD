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
 *  \file wfdconfig.h
 *  \brief Interface for reading device configuration
 *
 */


#ifndef WFDCONFIG_H_
#define WFDCONFIG_H_

#include "WF/wfd.h"
#include "wfdstructs.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*! \brief Retrieve static hardware configuration
 *
 *  Routine fills an array of device configurations and
 *  returns number of devices in the array. A pointer to configuration
 *  memory can be passed back to caller in a reference parameter.
 *
 *  \param configs Optional. Pointer to a pointer that upon return of the routine
 *  will store the address of  the static configuration area.
 *
 *  \return Number of display devices in the system
 */
OWF_API_CALL WFDint OWF_APIENTRY
WFD_Config_GetDevices(WFD_DEVICE_CONFIG** configs) OWF_APIEXIT;

#ifdef __cplusplus
}
#endif


#endif /* WFDCONFIG_H_ */
