#----------------------------------------------------------
# Top level CMakeLists.txt for OpenWF project
#----------------------------------------------------------

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
PROJECT(OpenWF)
INCLUDE(OpenWF_PlatformCheck.cmake)
INCLUDE(OpenWF_Common.cmake)

#ADD_SUBDIRECTORY(SI_Common)

ADD_SUBDIRECTORY(SI_Adaptation)
ADD_DEFINITIONS(-DDEBUG)
set(CMAKE_C_FLAGS "-m32")
set(CMAKE_CXX_FLAGS "-m32")
add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=1)
# source package may contain
# either Composition or Display or both
IF (IS_DIRECTORY ${OPENWF_SI_COMPOSITION_DIR})
  ADD_SUBDIRECTORY(SI_Composition)
ENDIF (IS_DIRECTORY ${OPENWF_SI_COMPOSITION_DIR})

IF (IS_DIRECTORY ${OPENWF_SI_DISPLAY_DIR})
  ADD_SUBDIRECTORY(SI_Display)
ENDIF (IS_DIRECTORY ${OPENWF_SI_DISPLAY_DIR})

