############################################################################
# OpenWF: Common library (libcommon)
# - common routines for composition, display and conformance tests
#
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
PROJECT(OpenWF)
INCLUDE(${OPENWF_SRC_ROOT}/OpenWF_Common.cmake)

INCLUDE_DIRECTORIES(${OPENWF_SI_COMMON_INC}
					${OPENWF_SI_ADAPTATION_INC}
                    ${OPENWF_SI_ADAPTATION_PLATFORM_INC}
                    ${OPENWF_KHRONOS_INC}
                    ${OPENWF_SRC_ROOT})
add_definitions(-DDEBUG)

SET(OWFCOMMON_SOURCES
    src/owfhandle.c
	src/owfimage.c
    src/owfattributes.c
    src/owfutils.c
    src/owflinkedlist.c
    src/owfmemory.c
    src/owfarray.c
    src/owfpool.c
    src/owfobject.c
    src/owfhash.c
    src/owfstream.c
    src/owfdebug.c)

ADD_LIBRARY(owfcommon SHARED ${OWFCOMMON_SOURCES})

ADD_DEFINITIONS(-D_REENTRANT)
ADD_DEFINITIONS(-D_BSD_SOURCE)

TARGET_LINK_LIBRARIES(owfcommon)
