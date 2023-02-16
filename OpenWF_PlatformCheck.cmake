#---------------------------------------------------
# OpenWF - check platform dependencies before build
#

IF (UNIX)
    FIND_PATH(SDL_INC SDL.h /usr/include/SDL /usr/local/include/SDL)
    IF (NOT SDL_INC)
      MESSAGE(FATAL_ERROR "Cannot find SDL.h. Install SDL development package first. Aborted!")
    ENDIF (NOT SDL_INC)
    
    FIND_LIBRARY(SDL_LIB NAMES SDL PATH /usr/lib /usr/local/lib)
    IF (NOT SDL_LIB)
      MESSAGE(FATAL_ERROR "Cannot find libSDL library. Aborted!")
    ENDIF (NOT SDL_LIB)
    
    FIND_PATH(XML_INC xmlreader.h /usr/include/libxml
                                  /usr/include/libxml2/libxml 
                                  /usr/local/include/libxml
                                  /usr/local/include/libxml2/libxml)
    IF (NOT XML_INC)
      MESSAGE(FATAL_ERROR "Cannot find xmlreader.h. Install libxml2 development package first. Aborted!")
    ENDIF (NOT XML_INC)
    
    FIND_LIBRARY(XML_LIB NAMES xml2 PATH /usr/lib /usr/local/lib)
    IF (NOT SDL_LIB)
      MESSAGE(FATAL_ERROR "Cannot find libxml2 library. Aborted!")
    ENDIF (NOT SDL_LIB)
    
ENDIF (UNIX)
