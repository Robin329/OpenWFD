#----------------------------------------------------------
# OpenWF Composition example
#
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
PROJECT(OpenWF)
INCLUDE(${OPENWF_SRC_ROOT}/OpenWF_Common.cmake)

INCLUDE_DIRECTORIES(${OPENWF_KHRONOS_INC})

LINK_DIRECTORIES(. 
                 ${CMAKE_BINARY_DIR}/SI_Common
                 ${CMAKE_BINARY_DIR}/SI_Adaptation 
                 ${CMAKE_BINARY_DIR}/SI_Composition)

ADD_DEFINITIONS(-D_BSD_SOURCE)

SET(WFC_EXAMPLE_SRC
	wfc_example.c)

ADD_EXECUTABLE(wfc_example ${WFC_EXAMPLE_SRC})

TARGET_LINK_LIBRARIES(wfc_example pthread WFC m)

SET_TARGET_PROPERTIES(wfc_example PROPERTIES
	OUTPUT_NAME wfc_example)

