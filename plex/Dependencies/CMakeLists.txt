set(DEPEND_ARGS osx)

if(TARGET_OSX)
  if(OSX_ARCH STREQUAL "x86_64")
    set(DEPEND_ARGS osx64)
  endif()
elseif(TARGET_IOS)
  set(DEPEND_ARGS ios)
endif()

execute_process(
  COMMAND ${plexdir}/scripts/fetch-depends.sh ${DEPEND_ARGS}
  WORKING_DIRECTORY ${root}
)

find_program(GIT_PATH git HINTS /usr/bin /usr/local/bin)
if(GIT_PATH MATCHES "NOTFOUND")
  message(WARNING "git not found! we might have problem..")
endif()

execute_process(
  COMMAND ${GIT_PATH} rev-list -1 HEAD -- ${PROJECT_SOURCE_DIR}/tools/darwin/depends
  COMMAND cut -c1-8
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_VARIABLE DEPEND_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND ${GIT_PATH} rev-list -1 HEAD -- ${PROJECT_SOURCE_DIR}/lib/ffmpeg
  COMMAND cut -c1-8
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_VARIABLE FFMPEG_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(TARGET_OSX)
  set(dependdir ${plexdir}/Dependencies/macosx${OSX_SDK_VERSION}_${OSX_ARCH}-xbmc-${DEPEND_HASH} CACHE STRING "path to dependencies")
  set(ffmpegdir ${plexdir}/Dependencies/macosx${OSX_SDK_VERSION}_${OSX_ARCH}-ffmpeg-${FFMPEG_HASH} CACHE STRING "path to ffmpeg")
elseif(TARGET_IOS)
  set(dependdir ${plexdir}/Dependencies/iphoneos${IPHONEOS_SDK_VERSION}_armv7-xbmc-${DEPEND_HASH})
  set(ffmpegdir ${plexdir}/Dependencies/iphoneos${IPHONEOS_SDK_VERSION}_armv7-ffmpeg-${DEPEND_HASH})
endif()

file(COPY ${root}/tools/darwin/depends/libcrystalhd/libcrystalhd DESTINATION ${dependdir}/include)