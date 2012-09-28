# CPLUFF
externalproject_add(cpluff
	LIST_SEPARATOR @
	URL ${root}/lib/cpluff
	CONFIGURE_COMMAND ${plexdir}/scripts/configure_cpluff.sh ${dependdir} ${CMAKE_BINARY_DIR}/lib/cpluff/cpluff-prefix/src/cpluff
	BUILD_COMMAND make -j4
	BUILD_IN_SOURCE 1
	INSTALL_COMMAND ""
)

set(ALIAS_LIST "")
add_library(cpluff-${ARCH} MODULE ${CMAKE_BINARY_DIR}/lib/cpluff/cpluff-prefix/src/cpluff/libcpluff/.libs/libcpluff.a ${expat_LIBRARY})
set_target_properties(cpluff-${ARCH} PROPERTIES LINKER_LANGUAGE C)
