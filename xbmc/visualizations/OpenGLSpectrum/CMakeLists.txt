find_all_sources(. OGL_SRC)
add_library(opengl_spectrum MODULE ${OGL_SRC})
target_link_libraries(opengl_spectrum ${CONFIG_LIBRARY_OPENGL})
install(TARGETS opengl_spectrum DESTINATION ${RESOURCEPATH}/addons/visualization.glspectrum)