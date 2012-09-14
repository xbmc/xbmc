add_definitions(
	-D_FILE_OFFSET_BITS=64
	-D_LARGEFILE_SOURCE
	-DPIC
	-DHAVE_AV_CONFIG_H
	-DNDEBUG=1
	-D_DARWIN_C_SOURCE
	-Dattribute_deprecated=
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch i386 -w -std=c99 -fomit-frame-pointer -fPIC -Wtype-limits -Wdeclaration-after-statement -Wall -Wno-parentheses -Wno-switch -Wdisabled-optimization -Wpointer-arith -Wredundant-decls -Wno-pointer-sign -Wcast-qual -Wwrite-strings -O3 -Wundef -fno-math-errno -fno-signed-zeros -Qunused-arguments -MMD")

# clear includes from parent
set_property(DIRECTORY PROPERTY INCLUDE_DIRECTORIES "")
include_directories(.)

add_subdirectory(libavcodec)