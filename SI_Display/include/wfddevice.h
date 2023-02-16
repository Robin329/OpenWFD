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

#ifndef WFD_DEVICE_H_
#define WFD_DEVICE_H_

#include "WF/wfd.h"
#include "wfdstructs.h"

/*! \ingroup wfd
 *  \file wfddevice.h
 *  \brief Device handling interface
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/*!
 *  \brief Allocates a device and assign a handle for it
 *
 *  Routine creates a device object and assigns a temporary
 *  handle for it. The device object represents a real hardware
 *  display control block.
 *
 *  There can be only one device object (and handle) per real
 *  device in the system.
 *
 *  The routine allocates all necessary data structures for
 *  the device.
 *
 *  \param deviceId Device identification number
 *
 *  \return Device handle or WFD_INVALID_HANDLE if allocation failed
 *  or device id is unknown
 *
 */
OWF_API_CALL WFDDevice OWF_APIENTRY
WFD_Device_Allocate(WFDint deviceId) OWF_APIEXIT;


/*!
 * \brief Releases a device.
 *
 *  Routine releases the device for further use. Device handle
 *  is invalid when this routine return.
 *
 *  All port, pipeline and event container objects associated
 *  to the device object are also released.
 *
 *  The device object may still exist in the system after
 *  return. It is deleted when its reference counter reaches
 *  zero.
 *
 * \param device A pointer to device object
 *
 */
OWF_API_CALL void OWF_APIENTRY
WFD_Device_Release(WFD_DEVICE* device) OWF_APIEXIT;

/*! \brief Check if the device is in use
 *
 * \param  deviceId device id
 *
 * \return WFD_TRUE of WFD_FALSE
 */
OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Device_IsAllocated(WFDint deviceId) OWF_APIEXIT;

/*! \brief Get list of device ids in the system
 *
 * \param idsList Optional, pointer to array of device ids
 * \param listCapacity maximum number of items in idsList
 *
 * \return number of devices in the system
 */
OWF_API_CALL WFDint OWF_APIENTRY
WFD_Device_GetIds(WFDint *idsList,
                  WFDint listCapacity) OWF_APIEXIT;

/*! \brief Get list of device ids in the system
 *
 * Returns devices matching given filtering rules. See
 * OpenWF specification for defining filter list.
 *
 * \param idsList Optional, pointer to array of device ids
 * \param listCapacity Maximum number of items in idsList
 * \param filterList Pointer to filter list
 *
 * \return number of devices matching filter rules
 */
OWF_API_CALL WFDint OWF_APIENTRY
WFD_Device_FilterIds(WFDint *idsList,
                     WFDint listCapacity,
                     const WFDint *filterList) OWF_APIEXIT;

/*! \brief Find device object matching a device handle
 *
 * \param device Device handle
 *
 * \return Pointer to device object or NULL
 */
OWF_API_CALL WFD_DEVICE* OWF_APIENTRY
WFD_Device_FindByHandle(WFDDevice device) OWF_APIEXIT;

/*! \brief Find static device configuration matching device id
 *
 * \param deviceId device id
 *
 * \return Pointer to device configuration
 */
OWF_API_CALL WFD_DEVICE_CONFIG* OWF_APIENTRY
WFD_Device_FindById(WFDint deviceId) OWF_APIEXIT;


/*! \brief Get integer attribute value
 *
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Device_GetAttribi(WFD_DEVICE* device,
                      WFDDeviceAttrib attrib,
                      WFDint* attrValue) OWF_APIEXIT;

/*! \brief Get integer attribute value
 *
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Device_SetAttribi(WFD_DEVICE* device,
                      WFDDeviceAttrib attrib,
                      WFDint attrValue) OWF_APIEXIT;

/*! \brief Set device error code
 *
 * Set internal error code. Code is set if there is no
 * previous error code set.
 *
 * \param device Pointer to device object
 * \param error Error code
 *
 */
OWF_API_CALL void OWF_APIENTRY
WFD_Device_SetError(WFD_DEVICE* device,
                    WFDErrorCode error) OWF_APIEXIT;

/*! \brief Get device error code
 *
 * Return device error code and reset it to WFD_ERROR_NONE.
 *
 * \param device Pointer to device object
 *
 * \return Error code
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Device_GetError(WFD_DEVICE* device) OWF_APIEXIT;

/*! \brief Commit changes to device configuration
 *
 * Commit all changes done to device object, ports and pipelines
 * since previous commit. If port object is specified, only
 * changes to that port are committed.  If pipeline object is
 * specified, only changes to that pipeline are committed.
 *
 * \param device Pointer to device object
 * \param port Optional, pointer to port object
 * \param pipeline Optional, pointer to pipeline object
 *
 * \return error code
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Device_Commit(WFD_DEVICE* device,
                  WFD_PORT* port,
                  WFD_PIPELINE* pipeline) OWF_APIEXIT;



/*! \brief Create a stream provider object for a pipeline
 *
 * \param device device object
 * \param pipeline pipeline object
 * \param source native stream
 * \param type specifies whether to create source or mask object
 *
 * \return pointer to image provide object
 */
OWF_API_CALL WFD_IMAGE_PROVIDER* OWF_APIENTRY
WFD_Device_CreateStreamProvider(WFD_DEVICE* device,
                                WFD_PIPELINE* pipeline,
                                WFDNativeStreamType source,
                                WFD_IMAGE_PROVIDER_TYPE type) OWF_APIEXIT;

/*! \brief Create an image provider object for a pipeline
 *
 * \param device device object
 * \param pipeline pipeline object
 * \param source EGL image
 * \param type specifies whether to create source or mask object
 *
 * \return pointer to image provider object
 */
OWF_API_CALL WFD_IMAGE_PROVIDER* OWF_APIENTRY
WFD_Device_CreateImageProvider(WFD_DEVICE* device,
                               WFD_PIPELINE* pipeline,
                               WFDEGLImage source,
                               WFD_IMAGE_PROVIDER_TYPE type) OWF_APIEXIT;

/*! \brief Destroy a stream or image provider
 *
 * \param device Pointer to a device object
 * \param handle Handle to image provider
 *
 * \return status code
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Device_DestroyImageProvider(WFD_DEVICE* device,
                                WFDHandle handle) OWF_APIEXIT;
#ifdef __cplusplus
}
#endif

#endif
