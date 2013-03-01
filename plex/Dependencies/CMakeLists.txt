execute_process(
  COMMAND ${plexdir}/scripts/fetch-depends.sh
  WORKING_DIRECTORY ${root}
)

file(COPY ${root}/tools/darwin/depends/libcrystalhd/libcrystalhd DESTINATION ${dependdir}/include)