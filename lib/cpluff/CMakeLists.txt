# CPLUFF
externalproject_add(cpluff
	LIST_SEPARATOR @
	URL ${root}/lib/cpluff
	CONFIGURE_COMMAND ${plexdir}/scripts/configure_cpluff.sh ${dependdir} ${CMAKE_BINARY_DIR}/lib/cpluff/cpluff-prefix/src/cpluff
	BUILD_COMMAND make -j4
	BUILD_IN_SOURCE 1
	INSTALL_COMMAND ""
)

add_custom_target(wrap_cpluff COMMAND ${CMAKE_CXX_COMPILER} -arch i386 -all_load -bundle -flat_namespace -undefined dynamic_lookup -L${dependdir}/lib -lexpat -o ${CMAKE_BINARY_DIR}/lib/libcpluff-${ARCH}.so ${CMAKE_BINARY_DIR}/lib/cpluff/cpluff-prefix/src/cpluff/libcpluff/.libs/libcpluff.a DEPENDS cpluff)
