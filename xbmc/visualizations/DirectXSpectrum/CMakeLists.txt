add_library(Spectrum_win32dx MODULE directx_spectrum.cpp)
target_link_libraries(Spectrum_win32dx ${CONFIG_LIBRARY_D3DX9})
install(TARGETS Spectrum_win32dx LIBRARY DESTINATION ${RESOURCEPATH}/addons/visualization.dxspectrum COMPONENT RUNTIME)
