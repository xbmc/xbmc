# CPLUFF
externalproject_add(cpluff
  DEPENDS dependencies
	LIST_SEPARATOR @
	URL ${root}/lib/cpluff
	CONFIGURE_COMMAND ${plexdir}/scripts/configure_cpluff.sh ${dependdir} ${CMAKE_BINARY_DIR}/lib/cpluff/cpluff-prefix/src/cpluff
	BUILD_COMMAND make -j4
	BUILD_IN_SOURCE 1
	INSTALL_COMMAND ""
)

set(CMAKE_MODULE_LINKER_FLAGS "-arch i386 -undefined dynamic_lookup -read_only_relocs suppress")
set(CMAKE_SHARED_MODULE_PREFIX "lib")
file(GLOB cpluff_objects ${CMAKE_BINARY_DIR}/lib/cpluff/cpluff-prefix/src/cpluff/libcpluff/*.o)
add_library(cpluff-${ARCH} MODULE ${cpluff_objects} ${expat_LIBRARY})
add_dependencies(cpluff-${ARCH} cpluff)
set_target_properties(cpluff-${ARCH} PROPERTIES LINKER_LANGUAGE C)


install(TARGETS cpluff-${ARCH} DESTINATION ${CMAKE_INSTALL_PREFIX}/Plex.app/Contents/Resources/XBMC/system)