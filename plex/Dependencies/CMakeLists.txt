if(APPLE)
  set(OS "darwin")
  set(ARCH "i686")
elseif(UNIX)
  set(OS "ubuntu")
  set(ARCH "x86_64")
endif()

set(BUILDNR 7)
set(DEPVERSION a987074)
set(FNAME ${DEPVERSION}-${OS}-${ARCH}.tar.bz2)
set(DEST_DIR ${CMAKE_BINARY_DIR}/Plex/Dependencies/deps/src/dependencies)

externalproject_add(
  dependencies
  LIST_SEPARATOR @
  URL http://nightlies.plexapp.com/plex-dependencies/laika-depends/${BUILDNR}/${FNAME}
  URL_MD5 f7f89b9960fc98289f9d5b245a75251f
  CONFIGURE_COMMAND ""
  BUILD_COMMAND rm -f ${plexdir}/Dependencies/${OS}-${ARCH}
  BUILD_IN_SOURCE 1
  INSTALL_COMMAND ln -s ${DEST_DIR} ${plexdir}/Dependencies/${OS}-${ARCH}
  PREFIX deps
)
