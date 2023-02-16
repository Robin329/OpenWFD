#############################################################################
# OpenWF: Display library (libwfd)
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

PROJECT(OpenWF)
INCLUDE(${CMAKE_SOURCE_DIR}/OpenWF_Common.cmake)

INCLUDE_DIRECTORIES(${OPENWF_SI_DISPLAY_INC} 
                    ${OPENWF_KHRONOS_INC}
                    ${OPENWF_SI_COMMON_INC}
                    ${OPENWF_SI_ADAPTATION_INC}
                    ${OPENWF_SI_ADAPTATION_PLATFORM_INC})


LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/SI_Adaptation)

SET(WFD_CONFIG_FILE display_config.xml)

SET(WFD_SOURCES
	src/wfdapi.c
	src/wfdhandle.c	
	src/wfdconfig.c
	src/wfddevice.c
	src/wfdevent.c
    src/wfdpipeline.c
	src/wfdport.c
	src/wfdutils.c
	src/wfdimageprovider.c)

ADD_LIBRARY(WFD SHARED ${WFD_SOURCES})
TARGET_LINK_LIBRARIES(WFD owfadaptation_X11)

#copy config xml to targets
CONFIGURE_FILE(${OPENWF_SI_DISPLAY_DIR}/${WFD_CONFIG_FILE}
               ${CMAKE_BINARY_DIR}/SI_Display/${WFD_CONFIG_FILE})

ADD_SUBDIRECTORY(example)
               
