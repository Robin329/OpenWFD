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
 *  \file wfdport.h
 *  \brief Interface to display ports
 *
 */

#ifndef WFD_PORT_H_
#define WFD_PORT_H_

#include "WF/wfd.h"
#include "wfdstructs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Transition messages passed to rendering routine
 *
 */
typedef enum
{
    /*! debugging - transition always */
    WFD_MESSAGE_NONE = 0,
    /*! shutdown rendering */
    WFD_MESSAGE_QUIT = 0x1000,
    /*! synchronous rendering */
    WFD_MESSAGE_IMMEDIATE,
    /*! rendering at vsync intervals */
    WFD_MESSAGE_VSYNC,
    /*! autonomous rendering triggered by stream update */
    WFD_MESSAGE_SOURCE_UPDATED
} WFD_MESSAGES;



/*! \brief Retrieve port ids of the system
 *
 *  \param pDevice Pointer to previously allocated device structure
 *  \param idsList Pointer to array of values returned
 *  \param listCapacity  Number of slots in idsList
 *
 *  \return number of ids in idsList, or if idsList is NULL
 *  total number of ports in the system
 */
OWF_API_CALL WFDint OWF_APIENTRY
WFD_Port_GetIds(WFD_DEVICE* pDevice,
                WFDint* idsList,
                WFDint listCapacity) OWF_APIEXIT;

/*! \brief Find a port by port id
 *
 *  \param pDevice Pointer to device object that own the port
 *  \param id Port id
 *
 *  \return Pointer to static configuration area of the port
 *  or NULL if port does not exist
 */
OWF_API_CALL WFD_PORT_CONFIG* OWF_APIENTRY
WFD_Port_FindById(WFD_DEVICE* pDevice, WFDint id) OWF_APIEXIT;

/*! \brief Find a port by port handle
 *
 *  \param pDevice Pointer to device object
 *  \param handle Port handle
 *  \return on success a pointer to port object
 *  \return on failure NULL
 */
OWF_API_CALL WFD_PORT* OWF_APIENTRY
WFD_Port_FindByHandle(WFD_DEVICE* pDevice,WFDPort handle) OWF_APIEXIT;

/*! \brief Seize port lock for port update
 *
 *  Routine will block until port lock can be seized.
 *
 *  \param pPort pointer to port object
 */
OWF_API_CALL void OWF_APIENTRY
WFD_Port_AcquireLock(WFD_PORT* pPort) OWF_APIEXIT;

/*! \brief Release port lock
 *
 *  Release port lock and signal waiting threads for an available lock.
 *
 *  \param pPort pointer to port object
 */
OWF_API_CALL void OWF_APIENTRY
WFD_Port_ReleaseLock(WFD_PORT* pPort) OWF_APIEXIT;

/*! \brief Check if port is in use already
 *
 *  \param pDevice Pointer to device object
 *  \param portId Port id
 *
 *  \return WFD_ERROR_NONE port is available
 *  \return WFD_ERROR_IN_USE port is not available
 *  \return WFD_ERROR_ILLEGAL_ARGUMENT port id is invalid
 *
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Port_IsAllocated(WFD_DEVICE* pDevice,
                     WFDint portId) OWF_APIEXIT;

/*! \brief  Allocate port and assign a handle for it
 *
 *  Allocate pipeline and reserve necessary data structures
 *  and memory areas for rendering.
 *
 *  \param pDevice Pointer to device object
 *  \param portId Port id
 *
 *  \return on success return a handle to allocated pipeline
 *  \return on failure return WFD_INVALID_HANDLE
 */
OWF_API_CALL WFDPort OWF_APIENTRY
WFD_Port_Allocate(WFD_DEVICE* pDevice,
                  WFDint portId) OWF_APIEXIT;

/*! \brief elease all resources reserved for port
 *
 *  Release port resources, release all bindings to
 *  pipelines and invalidate port handle.
 *
 *  \param pDevice Pointer to device object
 *  \param pPort Pointer to port object
 */
OWF_API_CALL void OWF_APIENTRY
WFD_Port_Release(WFD_DEVICE* pDevice,
                 WFD_PORT* pPort) OWF_APIEXIT;

/*! \brief Find port mode
 *
 *  Find a port mode among the set of modes
 *  associated with a port.
 *
 *  \param pPort Pointer to port object
 *  \param mode Handle to port mode
 *
 *  \return Pointer to port mode
 */
OWF_API_CALL WFD_PORT_MODE* OWF_APIENTRY
WFD_Port_FindMode(WFD_PORT* pPort,
                  WFDPortMode mode) OWF_APIEXIT;

/*! \brief Retrieve all port modes of a port
 *
 *  \param pPort Pointer to port object
 *  \param modes Optional; Array to return modes into
 *  \param modesCount Maximum number of elements in modes array
 *
 *  \return if modes is NULL, total number of modes is returned
 *  \return if modes is non-NULL, number of modes written into modes
 *  array is returned
 */
OWF_API_CALL WFDint OWF_APIENTRY
WFD_Port_GetModes(WFD_PORT* pPort,
                  WFDPortMode* modes,
                  WFDint modesCount) OWF_APIEXIT;

/*! \brief Retrieve a pointer to current port mode object
 *
 * Routine returns a pointer to the current port mode,
 * which may be uncommitted.
 *
 *  \param pPort Pointer to port object
 */
OWF_API_CALL WFD_PORT_MODE* OWF_APIENTRY
WFD_Port_GetModePtr(WFD_PORT* pPort) OWF_APIEXIT;

/*! \brief Retrieve a handle to current port mode
 *
 *  \param pPort Pointer to port object
 */
OWF_API_CALL WFDPortMode OWF_APIENTRY
WFD_Port_GetCurrentMode(WFD_PORT* pPort) OWF_APIEXIT;

/*! \brief Set current port mode
 *
 *  \param pPort Pointer to port object
 *  \param mode Port mode to set
 */
OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Port_SetMode(WFD_PORT* pPort, WFDPortMode mode) OWF_APIEXIT;

/*! \brief Get port mode attribute value
 *
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_PortMode_GetAttribf(WFD_PORT_MODE* pPortMode,
                        WFDPortModeAttrib attrib,
                        WFDfloat* attrValue) OWF_APIEXIT;

/*! \brief Get port mode attribute value
 *
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Port_GetAttribi(WFD_PORT* pPort,
                    WFDPortConfigAttrib attrib,
                    WFDint* value) OWF_APIEXIT;

/*! \brief Get port mode attribute value
 *
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_PortMode_GetAttribi(WFD_PORT_MODE* pPortMode,
                        WFDPortModeAttrib attrib,
                        WFDint* attrValue) OWF_APIEXIT;

/*! \brief Get port attribute value
 *
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Port_GetAttribf(WFD_PORT* pPort,
                    WFDPortConfigAttrib attrib,
                    WFDfloat* value) OWF_APIEXIT;

/*! \brief Get port attribute value
 *
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Port_GetAttribiv(WFD_PORT* pPort,
                     WFDPortConfigAttrib attrib,
                     WFDint count,
                     WFDint* value) OWF_APIEXIT;

/*! \brief Get port attribute value
 *
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Port_GetAttribfv(WFD_PORT* pPort,
                     WFDPortConfigAttrib attrib,
                     WFDint count,
                     WFDfloat* value) OWF_APIEXIT;

/*! \brief Set port attribute value
 *
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Port_SetAttribi(WFD_PORT* pPort,
                    WFDPortConfigAttrib attrib,
                    WFDint value) OWF_APIEXIT;

/*! \brief Set port attribute value
 *
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Port_SetAttribf(WFD_PORT* pPort,
                    WFDPortConfigAttrib attrib,
                    WFDfloat value) OWF_APIEXIT;

/*! \brief Set port attribute value
 *
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Port_SetAttribiv(WFD_PORT* pPort,
                     WFDPortConfigAttrib attrib,
                     WFDint count,
                     const WFDint* values) OWF_APIEXIT;

/*! \brief Set port attribute value
 *
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Port_SetAttribfv(WFD_PORT* pPort,
                     WFDPortConfigAttrib attrib,
                     WFDint count,
                     const WFDfloat* values) OWF_APIEXIT;

/*! \brief Get index of pipeline in port's bindable pipelines array
 *
 *  \param pPort pointer to port object
 *  \param pPipeline pointer to pipeline object
 *
 *  \return index to bindable pipelines array.
 */
OWF_API_CALL WFDint OWF_APIENTRY
WFD_Port_PipelineNbr(WFD_PORT* pPort, WFD_PIPELINE* pPipeline) OWF_APIEXIT;

/*! \brief Check if pipeline can be bound to port
 *
 *  \param pPort pointer to port object
 *  \param pipelineId pipeline id
 *  \return WFD_TRUE or WFD_FALSE
 */
OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Port_PipelineBindable(WFD_PORT* pPort,
                          WFDint pipelineId) OWF_APIEXIT;

/*! \brief Check if pipeline is bound to port
 *
 *  \param pPort pointer to port object
 *  \param pPipeline pointer to pipeline object
 *  \return WFD_TRUE if pipeline is currently bound to port
 *  \return WFD_FALSE if pipeline is not bound to port or
 *  binding is not committed.
  */
OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Port_PipelineBound(WFD_PORT* pPort,
                       WFD_PIPELINE* pPipeline) OWF_APIEXIT;

/*! \brief Bind port to pipeline
 *
 *  Store port-pipeline binding to cache. Binding
 *  will take effect after next commit.
 *
 *  \param pPort pointer to port object
 *  \param pPipeline pointer to pipeline object
 */
OWF_API_CALL void OWF_APIENTRY
WFD_Port_PipelineCacheBinding(WFD_PORT* pPort,
                              WFD_PIPELINE* pPipeline) OWF_APIEXIT;

/*! \brief Get supported display data formats
 *
 *  \param pPort pointer to port object
 *  \param format Optional; array to return formats into
 *  \param formatCount Maximum number of formats to fit format array
 *
 *  \return if format is NULL, return total number of supported formats
 *  \return if format is non-NULL return number of formats written
 *  into format array
 */
OWF_API_CALL WFDint OWF_APIENTRY
WFD_Port_GetDisplayDataFormats(WFD_PORT* pPort,
                               WFDDisplayDataFormat *format,
                               WFDint formatCount) OWF_APIEXIT;

/*! \brief Check if port support a display data format
 *
 *  \param pPort pointer to port object
 *  \param format format to check support for
 *  \return WFD_TRUE or WFD_FALSE
 */
OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Port_HasDisplayData(WFD_PORT* pPort,
                        WFDDisplayDataFormat format) OWF_APIEXIT;

/*! \brief Retrieve display data of a format
 *
 *  \param pPort pointer to port object
 *  \param format desired display data format
 *  \param data Optional; caller provided array of bytes
 *  \param dataCount Maximum number of bytes that can be
 *  returned in data array

 *  \return if data is NULL, return total number of bytes in
 *  display data information.
 *  \return if data is non-NULL return number of bytes written
 *  into data array
 */
OWF_API_CALL WFDint OWF_APIENTRY
WFD_Port_GetDisplayData(WFD_PORT* pPort,
                        WFDDisplayDataFormat format,
                        WFDuint8* data,
                        WFDint dataCount) OWF_APIEXIT;

/*! \brief Commit all changes to port
 *
 *  \param pPort Pointer to port object
 */
OWF_API_CALL void OWF_APIENTRY
WFD_Port_Commit(WFD_PORT* pPort) OWF_APIEXIT;

/*! \brief Check if changes to port can be committed
 *
 *  \param pPort Pointer to port object
 *  \param type Requested commit type
 *  \return WFD_TRUE or WFD_FALSE
 */
OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Port_IsCommitConsistent(WFD_PORT* pPort, WFDCommitType type) OWF_APIEXIT;

/*! \brief Performs port-side actions when committing a single pipeline
 *
 *  \param pPipeline Pointer to pipeline object
 *  \param hasImmT The pipeline requires immediate transition.
 */
OWF_API_CALL void OWF_APIENTRY
WFD_Port_CommitForSinglePipeline(WFD_PIPELINE* pipeline, WFDboolean hasImmT) OWF_APIEXIT;

/*! \brief Get port's maximum refresh rate
 *
 *  Port's maximum refresh rate is the biggest refresh rate of port's
 *  port modes.
 *
 *  \param pPort Port's static configuration
 *  \return Ceiling of the port's maximum refresh rate
 */
OWF_API_CALL WFDint OWF_APIENTRY
WFD_Port_GetMaxRefreshRate(WFD_PORT_CONFIG* pPort) OWF_APIEXIT;

/*! \brief Query pipeline's relative layering order
 *
 * Pipeline does not have to be bound, but the layering order
 * value is the same that the value of the WFD_PIPELINE_LAYER
 * attribute would be if the pipeline were bound.
 *
 * \param pPort Pointer to port object
 * \param pPipeline Pointer to pipeline object
 *
 * \return The relative layering order of the pipeline or
 * WFD_INVALID_PIPELINE_LAYER if the pipeline cannot be bound
 * to the port.
 */
OWF_API_CALL WFDint OWF_APIENTRY
WFD_Port_QueryPipelineLayerOrder(WFD_PORT* pPort, WFD_PIPELINE* pPipeline);

/* ============================================================================= */
/*                           T E S T   O N L Y                                   */
/* ============================================================================= */

/*! \brief Make copy of port's current front buffer
 *
 * This routine serves purposes of OpenWF conformance testing.
 *
 *  \param pPort pointer to port object
 *
 *  \return copy of port's current front buffer
 */
OWF_API_CALL WFDEGLImage OWF_APIENTRY
WFD_Port_AcquireCurrentImage(WFD_PORT* pPort) OWF_APIEXIT;


#ifdef __cplusplus
}
#endif

#endif
