add_subdirectory(bus)
add_subdirectory(devices)
add_subdirectory(dialogs)

aux_source_directory(. peripherals_SRCS)
add_sources(${peripherals_SRCS})
