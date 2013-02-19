if(TARGET_COMMON_DARWIN)
  set(OS "darwin")
  set(ARCH "i686")
elseif(TARGET_COMMON_LINUX)
  set(OS "linux-ubuntu")
  set(ARCH "x86_64")
endif(TARGET_COMMON_DARWIN)

execute_process(
    COMMAND ${plexdir}/scripts/fetch-binaries.py -c dependencies.ini -o ${plexdir}/Dependencies -p ${OS} -a ${ARCH}
    WORKING_DIRECTORY ${plexdir}/Dependencies
)

file(COPY ${root}/tools/darwin/depends/libcrystalhd/libcrystalhd DESTINATION ${dependdir}/include)