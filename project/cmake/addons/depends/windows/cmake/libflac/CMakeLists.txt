project(libflac)

cmake_minimum_required(VERSION 2.8)

add_definitions(-DVERSION="1.2.1"
                -DFLAC__NO_DLL -DFLAC__HAS_OGG -DFLAC__CPU_IA32 -DFLAC__HAS_NASM -DFLAC__USE_3DNOW
                -Dfseeko=_fseeki64 -Dftello=_ftelli64) # work around a bug in the code that leads to "undefined reference errors"

# function to setup a custom command for a compiled assembler file
function (build_nasm filename_we)
  add_custom_command(OUTPUT src/libFLAC/ia32/${filename_we}.obj
                     COMMAND ${CMAKE_INSTALL_PREFIX}/bin/nasmw.exe -f win32 -d OBJ_FORMAT_win32 -i ${PROJECT_SOURCE_DIR}/src/libFLAC/ia32/ ${PROJECT_SOURCE_DIR}/src/libFLAC/ia32/${filename_we}.nasm -o ${PROJECT_BINARY_DIR}/src/libFLAC/ia32/${filename_we}.obj
                     WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endfunction (build_nasm filename)

# make sure that the output directory for the compiled assembler files exists
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/src/libFLAC/ia32)
  
# setup custom commands for the compiled assembler files
build_nasm(bitreader_asm)
build_nasm(cpu_asm)
build_nasm(fixed_asm)
build_nasm(lpc_asm)
build_nasm(stream_encoder_asm)

set(LIBFLAC_SOURCES src/libFLAC/bitmath.c
                    src/libFLAC/bitreader.c
                    src/libFLAC/bitwriter.c
                    src/libFLAC/cpu.c
                    src/libFLAC/crc.c
                    src/libFLAC/fixed.c
                    src/libFLAC/float.c
                    src/libFLAC/format.c
                    src/libFLAC/lpc.c
                    src/libFLAC/md5.c
                    src/libFLAC/memory.c
                    src/libFLAC/metadata_iterators.c
                    src/libFLAC/metadata_object.c
                    src/libFLAC/ogg_decoder_aspect.c
                    src/libFLAC/ogg_encoder_aspect.c
                    src/libFLAC/ogg_helper.c
                    src/libFLAC/ogg_mapping.c
                    src/libFLAC/stream_decoder.c
                    src/libFLAC/stream_encoder.c
                    src/libFLAC/stream_encoder_framing.c
                    src/libFLAC/window.c
                    src/libFLAC/ia32/bitreader_asm.obj
                    src/libFLAC/ia32/cpu_asm.obj
                    src/libFLAC/ia32/fixed_asm.obj
                    src/libFLAC/ia32/lpc_asm.obj
                    src/libFLAC/ia32/stream_encoder_asm.obj)

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src/libFLAC/include ${CMAKE_INSTALL_PREFIX}/include)

add_library(flac ${LIBFLAC_SOURCES})

set(HEADERS ${PROJECT_SOURCE_DIR}/include/FLAC/all.h
            ${PROJECT_SOURCE_DIR}/include/FLAC/assert.h
            ${PROJECT_SOURCE_DIR}/include/FLAC/callback.h
            ${PROJECT_SOURCE_DIR}/include/FLAC/export.h
            ${PROJECT_SOURCE_DIR}/include/FLAC/format.h
            ${PROJECT_SOURCE_DIR}/include/FLAC/metadata.h
            ${PROJECT_SOURCE_DIR}/include/FLAC/ordinals.h
            ${PROJECT_SOURCE_DIR}/include/FLAC/stream_decoder.h
            ${PROJECT_SOURCE_DIR}/include/FLAC/stream_encoder.h)
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/FLAC)
install(TARGETS flac DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
