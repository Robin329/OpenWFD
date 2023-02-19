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
 *  \file wfdpipeline.h
 *  \brief Interface to display pipelines
 *
 */
#ifndef WFD_PIPELINE_H_
#define WFD_PIPELINE_H_

#include "WF/wfd.h"
#include "wfdstructs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Allocate pipeline and assign a handle for it
 *
 * \param pDevice Pointer to device object
 * \param pipelineId pipeline id
 *
 * \return Pipeline handle or WFD_INVALID_HANDLE if allocation failed
 * or pipeline id is unknown
 */
OWF_API_CALL WFDPipeline OWF_APIENTRY
WFD_Pipeline_Allocate(WFD_DEVICE* pDevice, WFDint pipelineId) OWF_APIEXIT;

/*!\brief Release all resources reserved for pipeline
 *
 * \param pDevice Pointer to device object
 * \param pPipeline Pointer to pipeline object
 */
OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_Release(WFD_DEVICE* pDevice, WFD_PIPELINE* pPipeline) OWF_APIEXIT;

/*!\brief Get ids of available pipelines in the system
 *
 * \param pDevice Pointer to device object
 * \param idsList Optional; An array of pipeline ids to be filled
 *  by this routine
 * \param listCapacity Maximum number of slots in idsList
 *
 * \return Number of pipelines found or when idList is not NULL, number
 * of ids written to idsList array.
 */
OWF_API_CALL WFDint OWF_APIENTRY WFD_Pipeline_GetIds(
    WFD_DEVICE* pDevice, WFDint* idsList, WFDint listCapacity) OWF_APIEXIT;

/*!\brief Check if a hw pipeline is already in use by OpenWF Display
 *
 * \param pDevice Pointer to device object
 * \param pipelineId
 *
 * \return WFD_ERROR_NONE pipeline is available
 * \return WFD_ERROR_IN_USE pipeline is not available
 * \return WFD_ERROR_ILLEGAL_ARGUMENT pipeline id is invalid
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Pipeline_IsAllocated(WFD_DEVICE* pDevice, WFDint pipelineId) OWF_APIEXIT;

/*! \brief Locate pipeline's static config area
 *
 * Note that this function returns a pointer to  pipeline's static
 * configuration area, which is not used when pipeline is active.
 * A copy of the area is taken at pipeline's creation time.
 * All attribute values that are set during the lifetime of the
 * pipeline are stored into the copy area and forgotten when
 * the pipeline is destroyed.
 *
 * This function can also be used to check if a pipeline by given id
 * exists in the system.
 *
 * \param pDevice Pointer to device object
 * \param pipelineId Pipeline id
 *
 * \return A pointer to configuration are or NULL if id is not found
 */
OWF_API_CALL WFD_PIPELINE_CONFIG* OWF_APIENTRY
WFD_Pipeline_FindById(WFD_DEVICE* pDevice, WFDint pipelineId) OWF_APIEXIT;

/*!\brief Find pipeline object
 *
 * \param pDevice Pointer to device object
 * \param handle handle to pipeline object
 *
 * \return A pointer to pipeline object or NULL if handle is invalid
 * or is not a pipeline handle
 */
OWF_API_CALL WFD_PIPELINE* OWF_APIENTRY
WFD_Pipeline_FindByHandle(WFD_DEVICE* pDevice, WFDPipeline handle) OWF_APIEXIT;

/*!\brief
 *
 * \param pPipeline Pointer to pipeline object
 * \param image
 *
 * \return
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY WFD_Pipeline_IsImageValidSource(
    WFD_PIPELINE* pPipeline, WFDEGLImage image) OWF_APIEXIT;

/*! \brief Check if stream is a valid stream for pipeline
 *
 *  \param pPipeline Pointer to pipeline object
 *  \param stream Native stream handle
 *
 *  \return WFD_ERROR_NONE stream can be used
 *  \return WFD_ERROR_ILLEGAL_ARGUMENT
 *      stream is not a valid WFDNativeStreamType
 *  \return WFD_ERROR_BUSY
 *      stream can not be used at this time due to other usages of stream
 *  \return WFD_ERROR_NOT_SUPPORTED
 *     stream is not suitable for use by device or pipeline
 *
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY WFD_Pipeline_IsStreamValidSource(
    WFD_PIPELINE* pPipeline, WFDNativeStreamType stream) OWF_APIEXIT;

/*!\brief
 *
 * \param pPipeline Pointer to pipeline object
 * \param image
 *
 * \return
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY WFD_Pipeline_IsImageValidMask(
    WFD_PIPELINE* pPipeline, WFDEGLImage image) OWF_APIEXIT;

/*! \brief Check if stream is a valid mask for pipeline
 *
 *  \param pPipeline Pointer to pipeline object
 *  \param stream Native stream handle
 *
 *  \return WFD_ERROR_NONE stream can be used
 *  \return WFD_ERROR_ILLEGAL_ARGUMENT
 *      stream is not a valid WFDNativeStreamType
 *  \return WFD_ERROR_BUSY
 *      stream can not be used at this time due to other usages of stream
 *  \return WFD_ERROR_NOT_SUPPORTED
 *     stream is not suitable for use by device or pipeline
 *
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY WFD_Pipeline_IsStreamValidMask(
    WFD_PIPELINE* pPipeline, WFDNativeStreamType stream) OWF_APIEXIT;

/*!\brief Update pipeline's binding cache with a reference to mask object
 *
 * Routine places a reference to a mask object into pipeline's binding cache.
 * Binding takes effect after commit.
 *
 * \param pPipeline Pointer to pipeline object
 * \param pMask Pointer to mask object
 * \param transition Desired transition type
 */
OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_MaskCacheBinding(WFD_PIPELINE* pPipeline, WFD_MASK* pMask,
                              WFDTransition transition) OWF_APIEXIT;

/*!\brief  Update pipeline's binding cache with a reference to source object
 *
 * Routine places a reference to a source object into pipeline's binding cache.
 * Binding takes effect after commit.
 *
 * \param pPipeline Pointer to pipeline object
 * \param pSource Pointer to source object
 * \param transition Desired transition type
 * \param region a rectangle of the image that has
 *  changed from the previous image
 */
OWF_API_CALL void OWF_APIENTRY WFD_Pipeline_SourceCacheBinding(
    WFD_PIPELINE* pPipeline, WFD_SOURCE* pSource, WFDTransition transition,
    const WFDRect* region) OWF_APIEXIT;

/*!\brief Generate an event after source transition completed
 *
 * \param pPipeline Pointer to pipeline object
 *
 * \return
 */
OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_SourceBindComplete(WFD_PIPELINE* pPipeline) OWF_APIEXIT;

/*!\brief Generate an event after mask transition completed
 *
 * \param pPipeline Pointer to pipeline object
 *
 */
OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_MaskBindComplete(WFD_PIPELINE* pPipeline) OWF_APIEXIT;

/*!\brief Remove all source references from pipeline bindings
 *
 * \param pPipeline Pointer to pipeline object
 *
 */
OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_SourceRemoveBinding(WFD_PIPELINE* pPipeline) OWF_APIEXIT;

/*!\brief Remove all mask references from pipeline bindings
 *
 * \param pPipeline Pointer to pipeline object
 */
OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_MaskRemoveBinding(WFD_PIPELINE* pPipeline) OWF_APIEXIT;

/*!\brief Remove all port references from pipeline bindings
 *
 * \param pPort
 * \param pPipeline Pointer to pipeline object
 * \param cached Remove cached binding (otherwise committed binding)
 */
OWF_API_CALL void OWF_APIENTRY WFD_Pipeline_PortRemoveBinding(
    WFD_PORT* pPort, WFD_PIPELINE* pPipeline, WFDboolean cached);

/*!\brief Get integer attribute value
 *
 * See OpenWF Display Specification for further documentation
 *
 * \return Error code indicating result of the operation
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Pipeline_GetAttribi(WFD_PIPELINE* pPipeline, WFDPipelineConfigAttrib attrib,
                        WFDint* value) OWF_APIEXIT;

/*!\brief Get float attribute value
 *
 * See OpenWF Display Specification for further documentation
 *
 *
 * \return Error code indicating result of the operation
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Pipeline_GetAttribf(WFD_PIPELINE* pPipeline, WFDPipelineConfigAttrib attrib,
                        WFDfloat* value) OWF_APIEXIT;

/*!\brief Get integer attribute vector value
 *
 * See OpenWF Display Specification for further documentation
 *
 *
 * \return Error code indicating result of the operation
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY WFD_Pipeline_GetAttribiv(
    WFD_PIPELINE* pPipeline, WFDPipelineConfigAttrib attrib, WFDint count,
    WFDint* value) OWF_APIEXIT;

/*!\brief Get float attribute vector value
 *
 * See OpenWF Display Specification for further documentation
 *
 *
 * \return Error code indicating result of the operation
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY WFD_Pipeline_GetAttribfv(
    WFD_PIPELINE* pPipeline, WFDPipelineConfigAttrib attrib, WFDint count,
    WFDfloat* value) OWF_APIEXIT;

/*!\brief Set integer attribute value
 *
 * See OpenWF Display Specification for further documentation
 *
 *
 * \return Error code indicating result of the operation
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Pipeline_SetAttribi(WFD_PIPELINE* pPipeline, WFDPipelineConfigAttrib attrib,
                        WFDint value) OWF_APIEXIT;

/*!\brief Set float attribute value
 *
 * See OpenWF Display Specification for further documentation
 *
 *
 * \return Error code indicating result of the operation
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY
WFD_Pipeline_SetAttribf(WFD_PIPELINE* pPipeline, WFDPipelineConfigAttrib attrib,
                        WFDfloat value) OWF_APIEXIT;

/*!\brief Set integer attribute vector value
 *
 * See OpenWF Display Specification for further documentation
 *
 *
 * \return Error code indicating result of the operation
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY WFD_Pipeline_SetAttribiv(
    WFD_PIPELINE* pPipeline, WFDPipelineConfigAttrib attrib, WFDint count,
    const WFDint* values) OWF_APIEXIT;

/*!\brief Set float attribute vector value
 *
 * See OpenWF Display Specification for further documentation
 *
 *
 * \return Error code indicating result of the operation
 */
OWF_API_CALL WFDErrorCode OWF_APIENTRY WFD_Pipeline_SetAttribfv(
    WFD_PIPELINE* pPipeline, WFDPipelineConfigAttrib attrib, WFDint count,
    const WFDfloat* values) OWF_APIEXIT;

/*!\brief Check if a transparency combination is supported by the pipeline
 *
 *
 * \param pPipeline Pointer to pipeline object
 * \param feature An array of bits each of which is representing a
 * single transparency feature.
 *
 * \return WFD_TRUE or WFD_FALSE
 */
OWF_API_CALL WFDboolean WFD_Pipeline_IsTransparencyFeatureSupported(
    WFD_PIPELINE* pPipeline, WFDbitfield feature) OWF_APIEXIT;

/*!\brief Check if a single transparency feature is present in any
 *  of the supported transparency feature combinations
 *
 * \param pPipeline Pointer to pipeline object
 * \param trans An individual transparency feature
 *
 * \return WFD_TRUE or WFD_FALSE
 */
OWF_API_CALL WFDboolean WFD_Pipeline_IsTransparencySupported(
    WFD_PIPELINE* pPipeline, WFDTransparency trans) OWF_APIEXIT;

/*!\brief Retrieve all transparency feature combination supported
 * by the pipeline
 *
 * \param pPipeline Pointer to pipeline object
 * \param trans Optional; An array of bitfields that is populated by
 * this routine
 * \param transCount Maximum number of slots in trans array
 *
 * \return Number of supported transparency feature combination or, when
 * trans is not NULL, the number of transparency feature combinations written
 * into trans array.
 */
OWF_API_CALL WFDint OWF_APIENTRY WFD_Pipeline_GetTransparencyFeatures(
    WFD_PIPELINE* pPipeline, WFDbitfield* trans, WFDint transCount) OWF_APIEXIT;

/*!\brief Set transparency source color
 *
 * See wfdSetPipelineTSColor() from OpenWFDisplay specification
 * for further documentation of parameter usage.
 *
 * \param pPipeline Pointer to pipeline object
 * \param colorFormat Format of color parameter. Determines also
 * the size of items in color array
 * \param count Number of items in color parameter
 * \param color An array of color elements
 *
 * \return
 */
OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_SetTSColor(WFD_PIPELINE* pPipeline, WFDTSColorFormat colorFormat,
                        WFDint count, const void* color) OWF_APIEXIT;

/*! \brief Render an image from source to pipeline's front buffer
 *
 * - get data buffer from source
 * - execute pipeline stages
 *   -# source conversion
 *   -# cropping
 *   -# flip & mirror
 *   -# rotate
 *   -# scale & filter
 *
 *  In the end the image is in pipeline front buffer and
 *  can be blended with mask and port image.
 *
 * \param pPipeline Pointer to pipeline object
 * \param pSource Pointer to bound source object
 */
OWF_API_CALL void OWF_APIENTRY
WFD_Pipeline_Execute(WFD_PIPELINE* pPipeline, WFD_SOURCE* pSource) OWF_APIEXIT;

/*!\brief Check if pipeline is currently disabled
 *
 * See OpenWF Display specification for conditions when
 * pipeline is disabled
 *
 * \param pPipeline Pointer to pipeline object
 *
 * \return WFD_TRUE or WFD_FALSE
 */
OWF_API_CALL WFDboolean OWF_APIENTRY
WFD_Pipeline_Disabled(WFD_PIPELINE* pPipeline) OWF_APIEXIT;

/*!\brief Check if changes to pipeline can be committed
 *
 * \param pPipeline Pointer to pipeline object
 * \param type Commit type
 *
 * \return WFD_TRUE or WFD_FALSE
 */
OWF_API_CALL WFDboolean WFD_Pipeline_IsCommitConsistent(
    WFD_PIPELINE* pPipeline, WFDCommitType type) OWF_APIEXIT;

/*! \brief Commit changes to pipeline
 *
 * \param pPipeline Pointer to pipeline object
 * \param pPort Pointer to port object
 *
 *  If pPort is NULL:
 *  - the commit is only to the pipeline
 *  - the commit has to check if the pipeline is
 *    bound to a port and if the pipeline has to be
 *    rendered immediately.
 *
 * \return WFD_TRUE or WFD_FALSE
 */
OWF_API_CALL WFDboolean WFD_Pipeline_Commit(WFD_PIPELINE* pPipeline,
                                            WFD_PORT* pPort) OWF_APIEXIT;

/*! \brief Clear pipeline's front buffer
 *
 * When pipeline's front buffer is cleared it
 * cannot contribute to final port image. This is done, for
 * example,
 * when source is unbound from the pipeline.
 *
 * \param pPipeline Pointer to pipeline object
 */
OWF_API_CALL void OWF_APIENTRY WFD_Pipeline_Clear(WFD_PIPELINE* pPipeline)
    OWF_APIEXIT;

#ifdef __cplusplus
}
#endif

#endif
