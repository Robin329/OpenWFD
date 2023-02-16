############################################################################ 
# OpenWF: Adaptation library (libadaptation)
# - porting layer for conformance tests 
# - common routines for composition, display and conformance tests
#
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
PROJECT(OpenWF)
INCLUDE(${OPENWF_SRC_ROOT}/OpenWF_Common.cmake)

INCLUDE_DIRECTORIES(${OPENWF_SI_ADAPTATION_INC}
					${OPENWF_SI_ADAPTATION_PLATFORM_INC}					
					${OPENWF_SI_COMMON_INC}                    
                    ${OPENWF_KHRONOS_INC}
                    ${OPENWF_SRC_ROOT}
                    /usr/include/libxml2)
LINK_DIRECTORIES(. 
                 ${CMAKE_BINARY_DIR}/SI_Common)	

# platform independent sources
SET(OWFCOMMON_SOURCES
    ${OPENWF_SI_COMMON_SRC}/owfhstore.c
    ${OPENWF_SI_COMMON_SRC}/owfhandle.c
	${OPENWF_SI_COMMON_SRC}/owfimage.c
    ${OPENWF_SI_COMMON_SRC}/owfattributes.c
    ${OPENWF_SI_COMMON_SRC}/owfutils.c 
    ${OPENWF_SI_COMMON_SRC}/owflinkedlist.c
    ${OPENWF_SI_COMMON_SRC}/owfmemory.c
    ${OPENWF_SI_COMMON_SRC}/owfarray.c
    ${OPENWF_SI_COMMON_SRC}/owfpool.c
    ${OPENWF_SI_COMMON_SRC}/owfobject.c
    ${OPENWF_SI_COMMON_SRC}/owfhash.c
    ${OPENWF_SI_COMMON_SRC}/owfstream.c
    ${OPENWF_SI_COMMON_SRC}/owfdebug.c)

# plaform dependent sources
SET(OWFA_SOURCES
    ${OPENWF_SI_ADAPTATION_PLATFORM_GRAPHICS_DIR}/owfnativestream.c    
	${OPENWF_SI_ADAPTATION_PLATFORM_OS_DIR}/${OPENWF_PLATFORM}/EGL/eglsync.c
    ${OPENWF_SI_ADAPTATION_PLATFORM_OS_DIR}/${OPENWF_PLATFORM}/owfsemaphore.c
    ${OPENWF_SI_ADAPTATION_PLATFORM_OS_DIR}/${OPENWF_PLATFORM}/owfmessagequeue.c
    ${OPENWF_SI_ADAPTATION_PLATFORM_OS_DIR}/${OPENWF_PLATFORM}/owfmutex.c
    ${OPENWF_SI_ADAPTATION_PLATFORM_OS_DIR}/${OPENWF_PLATFORM}/owfthread.c
    ${OPENWF_SI_ADAPTATION_PLATFORM_OS_DIR}/${OPENWF_PLATFORM}/owfbarrier.c
    ${OPENWF_SI_ADAPTATION_PLATFORM_OS_DIR}/${OPENWF_PLATFORM}/owfcond.c
    ${OPENWF_SI_ADAPTATION_PLATFORM_OS_DIR}/${OPENWF_PLATFORM}/owfconfig.c
    ${OPENWF_SI_ADAPTATION_PLATFORM_GRAPHICS_DIR}/${OPENWF_PLATFORM}/owfdisplaycontext.c)

SET(OWFA_X11_SOURCES
    ${OWFCOMMON_SOURCES}
    ${OWFA_SOURCES}
    ${OPENWF_SI_ADAPTATION_PLATFORM_GRAPHICS_DIR}/${OPENWF_PLATFORM}/owfscreen_X11.c)
   
SET(OWFA_SDL_SOURCES
    ${OWFCOMMON_SOURCES}
    ${OWFA_SOURCES}
    ${OPENWF_SI_ADAPTATION_PLATFORM_GRAPHICS_DIR}/${OPENWF_PLATFORM}/owfscreen_SDL.c)

   
ADD_LIBRARY(owfadaptation STATIC ${OWFA_SDL_SOURCES})
ADD_LIBRARY(owfadaptation_X11 STATIC ${OWFA_X11_SOURCES})

ADD_DEFINITIONS(-D_REENTRANT)
ADD_DEFINITIONS(-D_BSD_SOURCE)
ADD_DEFINITIONS(-fPIC)

TARGET_LINK_LIBRARIES(owfadaptation SDL)
TARGET_LINK_LIBRARIES(owfadaptation_X11 X11 xml2)

