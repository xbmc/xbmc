# Deflate64 decoder from zlib 1.3.2 contrib/infback9. The upstream sources are
# unmodified; zutil.h replaces zlib's private header of the same name.
set(SOURCES infback9.c
            inftree9.c)

set(HEADERS infback9.h
            inffix9.h
            inflate9.h
            inftree9.h
            zutil.h)

add_library(infback9 STATIC ${SOURCES} ${HEADERS})
target_link_libraries(infback9 PUBLIC LIBRARY::ZLIB)
target_include_directories(infback9 SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(infback9 PROPERTIES FOLDER lib)

target_link_libraries(lib${APP_NAME_LC} PUBLIC infback9)
