add_library(WaveForm MODULE Main.cpp)
target_link_libraries(WaveForm ${CONFIG_LIBRARY_OPENGL})
install(TARGETS WaveForm DESTINATION ${RESOURCEPATH}/addons/visualization.waveform)