add_subdirectory(bus)
add_subdirectory(devices)
add_subdirectory(dialogs)

find_all_sources(. peripherals_SRCS)
add_sources(${peripherals_SRCS})
