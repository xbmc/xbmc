########################################################################
#
# Copyright (c) 2008, Lawrence Livermore National Security, LLC.  
# Produced at the Lawrence Livermore National Laboratory  
# Written by bremer5@llnl.gov,pascucci@sci.utah.edu.  
# LLNL-CODE-406031.  
# All rights reserved.  
#   
# This file is part of "Simple and Flexible Scene Graph Version 2.0."
# Please also read BSD_ADDITIONAL.txt.
#   
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#   
# @ Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the disclaimer below.
# @ Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the disclaimer (as noted below) in
#   the documentation and/or other materials provided with the
#   distribution.
# @ Neither the name of the LLNS/LLNL nor the names of its contributors
#   may be used to endorse or promote products derived from this software
#   without specific prior written permission.
#   
#  
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL LAWRENCE
# LIVERMORE NATIONAL SECURITY, LLC, THE U.S. DEPARTMENT OF ENERGY OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING
#
########################################################################


#
#
# Try to find the FREETYPE2 libraries
# Once done this will define
#
# FREETYPE2_FOUND          - system has ftgl
# FREETYPE2_INCLUDE_DIR    - path to freetype2/freetype
# FREETYPE2_FT_CONFIG       - path to freetype_config
# FREETYPE2_LIBRARIES      - the library that must be included
#
#


FIND_PATH(FREETYPE2_INCLUDE_DIR freetype/config/ftheader.h
       ${ADDITIONAL_INCLUDE_PATH}
      /usr/include
      /usr/include/freetype2
      /usr/X11/include
      /usr/X11/include/freetype2
      /usr/X11R6/include
      /usr/X11R6/include/freetype2
      /sw/include
      /sw/include/freetype2
      ${VISUS_INCLUDE}
      ${VISUS_INCLUDE}/freetype2
      NO_DEFAULT_PATH
)

FIND_PATH(FREETYPE2_FT2BUILD ft2build.h
       ${ADDITIONAL_INCLUDE_PATH}
      /usr/include
      /usr/X11/include
      /usr/X11R6/include
      /sw/include
      ${VISUS_INCLUDE}
      NO_DEFAULT_PATH
)

FIND_PATH(FREETYPE2_FT_CONFIG bin/freetype-config
       ${ADDITIONAL_BINARY_PATH}
      /usr
      /usr/local
      /usr/X11R6
      /sw/bin
      ${VISUS_BINARY_DIR}
      NO_DEFAULT_PATH
)

IF (NOT WIN32)

FIND_LIBRARY(FREETYPE2_LIBRARIES freetype 
       ${ADDITIONAL_LIBRARY_PATH}
      /usr/lib
      /usr/X11R6/lib
      /sw/lib
      ${VISUS_LIBRARIES}   
      NO_DEFAULT_PATH
)

ELSE (NOT WIN32)

FIND_LIBRARY(FREETYPE2_LIBRARIES NAMES freetype234 freetype237
      PATHS
       ${ADDITIONAL_LIBRARY_PATH}
      /usr/lib
      /usr/X11R6/lib
      /sw/lib
      ${VISUS_LIBRARIES}   
)

ENDIF (NOT WIN32)

#IF (FREETYPE2_INCLUDE_DIR AND FREETYPE2_FT2BUILD AND FREETYPE2_FT2BUILD AND FREETYPE2_LIBRARIES)
#   INCLUDE(CheckCSourceRuns)
#   SET (CMAKE_REQUIRED_INCLUDES FREETYPE2_INCLUDE_DIR)
#   CHECK_C_SOURCE_RUNS("FreetypeVersionCheck.c" FREETYPE_VERSION_OK)
#   IF (NOT FREETYPE_VERSION_OK)
#      SET(FREETYPE2_INCLUDE_DIR FREETYPE2_INCLUDE_DIR-NOTFOUND)      
#      SET(FREETYPE2_FT2BUILD FREETYPE2_FT2BUILD-NOTFOUND)
#      SET(FREETYPE2_LIBRARIES FREETYPE2_LIBRARIES-NOTFOUND)
#   ENDIF (NOT FREETYPE_VERSION_OK)
#ENDIF (FREETYPE2_INCLUDE_DIR AND FREETYPE2_FT2BUILD AND FREETYPE2_LIBRARIES)


IF (FREETYPE2_INCLUDE_DIR AND FREETYPE2_FT2BUILD AND FREETYPE2_FT_CONFIG AND FREETYPE2_LIBRARIES)

   SET(FREETYPE2_FOUND "YES")
   IF (CMAKE_VERBOSE_MAKEFILE)
      MESSAGE("Using FREETYPE2_INCLUDE_DIR = " ${FREETYPE2_INCLUDE_DIR}) 
      MESSAGE("Using FREETYPE2_FT_CONFIG    = " ${FREETYPE2_FT_CONFIG}) 
      MESSAGE("Using FREETYPE2_LIBRARIES   = " ${FREETYPE2_LIBRARIES}) 
   ENDIF (CMAKE_VERBOSE_MAKEFILE)

ELSE (FREETYPE2_INCLUDE_DIR AND FREETYPE2_FT2BUILD AND FREETYPE2_FT_CONFIG AND FREETYPE2_LIBRARIES)
   
   IF (CMAKE_VERBOSE_MAKEFILE)
      MESSAGE("************************************")
      MESSAGE("  Necessary libfreetype2 files not found")
      MESSAGE("  FREETYPE2_INCLUDE_DIR = " ${FREETYPE2_INCLUDE_DIR})
      MESSAGE("  FREETYPE2_FT_CONFIG    = " ${FREETYPE2_FT_CONFIG}) 
      MESSAGE("  FREETYPE2_LIBRARIES   = " ${FREETYPE2_LIBRARIES})
      MESSAGE("  libfreetype will be build locally")
      MESSAGE("************************************")
   ENDIF (CMAKE_VERBOSE_MAKEFILE)
   
   IF (NOT EXISTS ${VISUS_EXTLIBS}/freetype-2.3.4)    
      EXECUTE_PROCESS(                                        
           COMMAND gzip -cd ${VISUS_SOURCE_DIR}/ext-libs/freetype-2.3.4.tar.gz 
           COMMAND tar xv
           WORKING_DIRECTORY ${VISUS_EXTLIBS}     
      )
   ENDIF (NOT EXISTS ${VISUS_EXTLIBS}/freetype-2.3.4)      
   
   EXECUTE_PROCESS(                                        
      COMMAND ./configure --prefix=${VISUS_EXT_PREFIX} 
      WORKING_DIRECTORY ${VISUS_EXTLIBS}/freetype-2.3.4     
   )
        
   EXECUTE_PROCESS(                                        
      COMMAND make install
      WORKING_DIRECTORY ${VISUS_EXTLIBS}/freeype-2.3.4
   )

   FIND_PATH(FREETYPE2_INCLUDE_DIR freetype/config/ftheader.h
        ${VISUS_EXT_PREFIX}/include 
        ${VISUS_EXT_PREFIX}/include/freetype2
       NO_DEFAULT_PATH
   )

   FIND_PATH(FREETYPE2_FT_CONFIG freetype-config
       ${VISUS_EXT_PREFIX}/bin
       NO_DEFAULT_PATH
   )

   FIND_LIBRARY(FREETYPE2_LIBRARIES freetype
       ${VISUS_EXT_PREFIX}/lib
       ${ADDITIONAL_SEARCH_PATH}  
       NO_DEFAULT_PATH
   )

   IF (FREETYPE2_LIBRARIES AND FREETYPE2_FT2BUILD AND FREETYPE2_FT_CONFIG AND FREETYPE2_INCLUDE_DIR)
      SET(FREETYPE2_FOUND "YES")
      IF (CMAKE_VERBOSE_MAKEFILE)     
         MESSAGE("Using FREETYPE2_INCLUDE_DIR = " ${FREETYPE2_INCLUDE_DIR}) 
         MESSAGE("Using FREETYPE2_FT_CONFIG    = " ${FREETYPE2_FT_CONFIG}) 
         MESSAGE("Using FREETYPE2_LIBRARIES   = " ${FREETYPE2_LIBRARIES}) 
      ENDIF (CMAKE_VERBOSE_MAKEFILE)     
   ELSE (FREETYPE2_LIBRARIES AND FREETYPE2_FT2BUILD AND AND FREETYPE2_INCLUDE_DIR)
      MESSAGE("ERROR freetype library not found on the system and could not be build")
         MESSAGE("Using FREETYPE2_INCLUDE_DIR = " ${FREETYPE2_INCLUDE_DIR})
         MESSAGE("Using FREETYPE2_FT_CONFIG    = " ${FREETYPE2_FT_CONFIG})
         MESSAGE("Using FREETYPE2_LIBRARIES   = " ${FREETYPE2_LIBRARIES})

   ENDIF (FREETYPE2_LIBRARIES AND FREETYPE2_FT2BUILD AND FREETYPE2_FT_CONFIG AND FREETYPE2_INCLUDE_DIR)

ENDIF (FREETYPE2_INCLUDE_DIR AND FREETYPE2_FT2BUILD AND FREETYPE2_FT_CONFIG AND FREETYPE2_LIBRARIES)

