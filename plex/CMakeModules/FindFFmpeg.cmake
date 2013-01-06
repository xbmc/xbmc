# Locate ffmpeg
# This module defines
# FFMPEG_LIBRARIES
# FFMPEG_FOUND, if false, do not try to link to ffmpeg
# FFMPEG_INCLUDE_DIR, where to find the headers
#
# $FFMPEG_DIR is an environment variable that would
# correspond to the ./configure --prefix=$FFMPEG_DIR
#
# Created by Robert Osfield.


#In ffmpeg code, old version use "#include <header.h>" and newer use "#include <libname/header.h>"
#In OSG ffmpeg plugin, we use "#include <header.h>" for compatibility with old version of ffmpeg

#We have to search the path which contain the header.h (usefull for old version)
#and search the path which contain the libname/header.h (usefull for new version)

#Then we need to include ${FFMPEG_libname_INCLUDE_DIRS} (in old version case, use by ffmpeg header and osg plugin code)
#                                                       (in new version case, use by ffmpeg header) 
#and ${FFMPEG_libname_INCLUDE_DIRS/libname}             (in new version case, use by osg plugin code)


# Macro to find header and lib directories
# example: FFMPEG_FIND(AVFORMAT avformat avformat.h)
MACRO(FFMPEG_FIND varname shortname headername)
    # old version of ffmpeg put header in $prefix/include/[ffmpeg]
    # so try to find header in include directory
    FIND_PATH(FFMPEG_${varname}_INCLUDE_DIRS ${headername}
        PATHS
        ${FFMPEG_ROOT}/include
        $ENV{FFMPEG_DIR}/include
        $ENV{OSGDIR}/include
        $ENV{OSG_ROOT}/include
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local/include
        /usr/include
        /sw/include # Fink
        /opt/local/include # DarwinPorts
        /opt/csw/include # Blastwave
        /opt/include
        /usr/freeware/include
        PATH_SUFFIXES ffmpeg
        DOC "Location of FFMPEG Headers"
    )

    # newer version of ffmpeg put header in $prefix/include/[ffmpeg/]lib${shortname}
    # so try to find lib${shortname}/header in include directory
    IF(NOT FFMPEG_${varname}_INCLUDE_DIRS)
        FIND_PATH(FFMPEG_${varname}_INCLUDE_DIRS lib${shortname}/${headername}
            ${FFMPEG_ROOT}/include
            $ENV{FFMPEG_DIR}/include
            $ENV{OSGDIR}/include
            $ENV{OSG_ROOT}/include
            ~/Library/Frameworks
            /Library/Frameworks
            /usr/local/include
            /usr/include/
            /sw/include # Fink
            /opt/local/include # DarwinPorts
            /opt/csw/include # Blastwave
            /opt/include
            /usr/freeware/include
            PATH_SUFFIXES ffmpeg
            DOC "Location of FFMPEG Headers"
        )
    ENDIF(NOT FFMPEG_${varname}_INCLUDE_DIRS)

    FIND_LIBRARY(FFMPEG_${varname}_LIBRARIES
        NAMES ${shortname}
        PATHS
        ${FFMPEG_ROOT}/lib
        $ENV{FFMPEG_DIR}/lib
        $ENV{OSGDIR}/lib
        $ENV{OSG_ROOT}/lib
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local/lib
        /usr/local/lib64
        /usr/lib
        /usr/lib64
        /sw/lib
        /opt/local/lib
        /opt/csw/lib
        /opt/lib
        /usr/freeware/lib64
        DOC "Location of FFMPEG Libraries"
    )

    IF(FFMPEG_${varname}_FOUND)
      SET(QUIET 1)
    ENDIF()

    IF (FFMPEG_${varname}_LIBRARIES AND FFMPEG_${varname}_INCLUDE_DIRS)
        SET(FFMPEG_${varname}_FOUND 1 CACHE STRING "FFMPEG library found")
        IF(NOT QUIET)
        	message(STATUS "Looking for FFmpeg library ${shortname} - found")
	ENDIF()
    ELSE(FFMPEG_${varname}_LIBRARIES AND FFMPEG_${varname}_INCLUDE_DIRS)
       	message(STATUS "Looking for FFmpeg library ${shortname} - not found")
    ENDIF(FFMPEG_${varname}_LIBRARIES AND FFMPEG_${varname}_INCLUDE_DIRS)

ENDMACRO(FFMPEG_FIND)

SET(FFMPEG_ROOT "$ENV{FFMPEG_DIR}" CACHE PATH "Location of FFMPEG")

FFMPEG_FIND(LIBAVFORMAT avformat avformat.h)
FFMPEG_FIND(LIBAVDEVICE avdevice avdevice.h)
FFMPEG_FIND(LIBAVCODEC  avcodec  avcodec.h)
FFMPEG_FIND(LIBAVUTIL   avutil   avutil.h)
FFMPEG_FIND(LIBAVFILTER   avfilter   avfilter.h)
FFMPEG_FIND(LIBPOSTPROC   postproc   postprocess.h)
FFMPEG_FIND(LIBSWSCALE  swscale  swscale.h)  # not sure about the header to look for here.
FFMPEG_FIND(LIBSWRESAMPLE swresample  swresample.h)  # not sure about the header to look for here.

SET(FFMPEG_FOUND "NO")
# Note we don't check FFMPEG_LIBSWSCALE_FOUND here, it's optional.
IF   (FFMPEG_LIBAVFORMAT_FOUND AND FFMPEG_LIBAVDEVICE_FOUND AND FFMPEG_LIBAVCODEC_FOUND AND FFMPEG_LIBAVUTIL_FOUND AND FFMPEG_LIBSWSCALE_FOUND AND FFMPEG_LIBSWRESAMPLE_FOUND AND FFMPEG_LIBAVFILTER_FOUND AND FFMPEG_LIBPOSTPROC_FOUND)

    SET(FFMPEG_FOUND "YES")

    SET(FFMPEG_INCLUDE_DIRS ${FFMPEG_LIBAVFORMAT_INCLUDE_DIRS})

    SET(FFMPEG_LIBRARY_DIRS ${FFMPEG_LIBAVFORMAT_LIBRARY_DIRS})

    # Note we don't add FFMPEG_LIBSWSCALE_LIBRARIES here, it will be added if found later.
    SET(FFMPEG_LIBRARIES
        ${FFMPEG_LIBAVFORMAT_LIBRARIES}
        ${FFMPEG_LIBAVDEVICE_LIBRARIES}
        ${FFMPEG_LIBAVCODEC_LIBRARIES}
        ${FFMPEG_LIBAVUTIL_LIBRARIES}
        ${FFMPEG_LIBSWSCALE_LIBRARIES}
        ${FFMPEG_LIBSWRESAMPLE_LIBRARIES}
        ${FFMPEG_LIBPOSTPROC_LIBRARIES}
        ${FFMPEG_LIBAVFILTER_LIBRARIES})


ELSE (FFMPEG_LIBAVFORMAT_FOUND AND FFMPEG_LIBAVDEVICE_FOUND AND FFMPEG_LIBAVCODEC_FOUND AND FFMPEG_LIBAVUTIL_FOUND AND FFMPEG_LIBSWSCALE_FOUND AND FFMPEG_LIBSWRESAMPLE_FOUND AND FFMPEG_LIBAVFILTER_FOUND AND FFMPEG_LIBPOSTPROC_FOUND)

   MESSAGE(FATAL_ERROR "Could not find FFMPEG")

ENDIF(FFMPEG_LIBAVFORMAT_FOUND AND FFMPEG_LIBAVDEVICE_FOUND AND FFMPEG_LIBAVCODEC_FOUND AND FFMPEG_LIBAVUTIL_FOUND AND FFMPEG_LIBSWSCALE_FOUND AND FFMPEG_LIBSWRESAMPLE_FOUND AND FFMPEG_LIBAVFILTER_FOUND AND FFMPEG_LIBPOSTPROC_FOUND)
