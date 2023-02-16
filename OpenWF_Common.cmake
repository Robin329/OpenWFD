#----------------------------------------------------------
# OpenWF: compiler flags and common setups
#

# Determine the build platform here
# value of OPENWF_PLATFORM refers to the name of platform specific
# subdirectory of adaptation layer of the implementation 
SET(OPENWF_PLATFORM "linux")

# Project structure
SET(OPENWF_SRC_ROOT                 ${CMAKE_SOURCE_DIR})

SET(OPENWF_KHRONOS_INC              ${OPENWF_SRC_ROOT}/khronos_include)

SET(OPENWF_SI_ADAPTATION_DIR           ${OPENWF_SRC_ROOT}/SI_Adaptation)
SET(OPENWF_SI_ADAPTATION_SRC           ${OPENWF_SI_ADAPTATION_DIR}/src)
SET(OPENWF_SI_ADAPTATION_INC           ${OPENWF_SI_ADAPTATION_DIR}/include)
SET(OPENWF_SI_ADAPTATION_PLATFORM_DIR  			${OPENWF_SI_ADAPTATION_SRC}/Platform)
SET(OPENWF_SI_ADAPTATION_PLATFORM_OS_DIR  		${OPENWF_SI_ADAPTATION_PLATFORM_DIR}/OS)
SET(OPENWF_SI_ADAPTATION_PLATFORM_GRAPHICS_DIR  ${OPENWF_SI_ADAPTATION_PLATFORM_DIR}/Graphics)
SET(OPENWF_SI_ADAPTATION_PLATFORM_INC 			${OPENWF_SI_ADAPTATION_INC}/Platform/${OPENWF_PLATFORM})

SET(OPENWF_SI_COMMON_DIR          	${OPENWF_SRC_ROOT}/SI_Common)
SET(OPENWF_SI_COMMON_SRC           	${OPENWF_SI_COMMON_DIR}/src)
SET(OPENWF_SI_COMMON_INC           	${OPENWF_SI_COMMON_DIR}/include)

SET(OPENWF_SI_COMPOSITION_DIR       ${OPENWF_SRC_ROOT}/SI_Composition)
SET(OPENWF_SI_COMPOSITION_SRC       ${OPENWF_SI_COMPOSITION_DIR}/src)
SET(OPENWF_SI_COMPOSITION_INC       ${OPENWF_SI_COMPOSITION_DIR}/include)

SET(OPENWF_SI_DISPLAY_DIR           ${OPENWF_SRC_ROOT}/SI_Display)
SET(OPENWF_SI_DISPLAY_SRC           ${OPENWF_SI_DISPLAY_DIR}/src)
SET(OPENWF_SI_DISPLAY_INC           ${OPENWF_SI_DISPLAY_DIR}/include)

SET(OPENWF_ARDITES_TEST_DIR         ${OPENWF_SRC_ROOT}/Ardites_Internal/test)

# Debug build by default
IF (NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Debug")
ENDIF (NOT CMAKE_BUILD_TYPE)

# Uncomment following line to force GNU C++ compilation of C files
#SET(CMAKE_C_COMPILER "g++")

# compiler & linker flags
# Keep GCC error messages in one line 
IF(CMAKE_COMPILER_IS_GNUCC)
  SET(CMAKE_C_FLAGS "-pedantic -Wextra -fdiagnostics-show-location=once")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0 ")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c89 -pedantic -Wno-long-long")
ENDIF(CMAKE_COMPILER_IS_GNUCC)
IF(CMAKE_COMPILER_IS_GNUCXX)
  SET(CMAKE_CXX_FLAGS "-Wextra -fdiagnostics-show-location=once")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmessage-length=0 ")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c89 -pedantic -Wno-long-long")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

IF (MSVC)
  MESSAGE(STATUS "*** MSVC ***")
	SET(CMAKE_LINK_FLAGS "-lm")
ENDIF (MSVC)

IF (CMAKE_BUILD_TYPE MATCHES "Debug")
  MESSAGE(STATUS "*** Debug build ***")
  ADD_DEFINITIONS("-DDEBUG")
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O0 -g3")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O0 -g3")
ENDIF (CMAKE_BUILD_TYPE MATCHES "Debug")

IF(COMMAND cmake_policy)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)

#ADD_DEFINITIONS(-DSAFE_MEMORY)

