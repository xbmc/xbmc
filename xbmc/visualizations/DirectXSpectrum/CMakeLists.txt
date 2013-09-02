add_library(Spectrum_win32dx MODULE directx_spectrum.cpp)
target_link_libraries(Spectrum_win32dx ${DirectX_LIBRARIES})
install(TARGETS Spectrum_win32dx LIBRARY DESTINATION ${RESOURCEPATH}/addons/visualization.dxspectrum COMPONENT RUNTIME)
