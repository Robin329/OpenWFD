#----------------------------------------------------------
# OpenWF Display example
#
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
PROJECT(OpenWF)
INCLUDE(${OPENWF_SRC_ROOT}/OpenWF_Common.cmake)

INCLUDE_DIRECTORIES(${OPENWF_KHRONOS_INC})

LINK_DIRECTORIES(. 
                 ${CMAKE_BINARY_DIR}/SI_Common
                 ${CMAKE_BINARY_DIR}/SI_Adaptation 
                 ${CMAKE_BINARY_DIR}/SI_Display)

ADD_DEFINITIONS(-D_BSD_SOURCE)

SET(WFD_CONFIG_FILE display_config.xml)

SET(WFD_EXAMPLE_SRC
	wfd_example.c)

ADD_EXECUTABLE(wfd_example ${WFD_EXAMPLE_SRC})

TARGET_LINK_LIBRARIES(wfd_example pthread WFD m)

SET_TARGET_PROPERTIES(wfd_example PROPERTIES
	OUTPUT_NAME wfd_example)

CONFIGURE_FILE(${OPENWF_SI_DISPLAY_DIR}/example/display_config.xml
               ${CMAKE_BINARY_DIR}/SI_Display/example/display_config.xml)
