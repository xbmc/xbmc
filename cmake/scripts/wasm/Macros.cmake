# WASM has no dlopen-based shared DVD wrapper; libdvd is linked statically.
function(core_link_library lib wraplib)
  message(STATUS "WASM: skipping core_link_library(${lib} ${wraplib}) — no .so wrapper in browser")
endfunction()
