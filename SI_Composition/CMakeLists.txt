#############################################################################
# OpenWF: Composition library (libwfc)
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
PROJECT(OpenWF)
INCLUDE(${OPENWF_SRC_ROOT}/OpenWF_Common.cmake)
    
INCLUDE_DIRECTORIES(${OPENWF_KHRONOS_INC}
					${OPENWF_SI_COMPOSITION_INC}                    
                    ${OPENWF_SI_COMMON_INC}
                    ${OPENWF_SI_ADAPTATION_INC}
                    ${OPENWF_SI_ADAPTATION_PLATFORM_INC})

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/SI_Adaptation
                 ${CMAKE_BINARY_DIR}/SI_Common)

SET(WFC_SOURCES
    src/wfcapi.c
    src/wfcelement.c
    src/wfccontext.c
    src/wfcdevice.c
    src/wfcimageprovider.c
    src/wfcscene.c
    src/wfcpipeline.c)

ADD_LIBRARY(WFC SHARED ${WFC_SOURCES})
TARGET_LINK_LIBRARIES(WFC owfadaptation)

ADD_SUBDIRECTORY(example)
