find_all_sources(. windowing_SRCS)

if(TARGET_COMMON_DARWIN)
	find_all_sources(osx windowing_os_SRCS)
	list(REMOVE_ITEM windowing_os_SRCS osx/WinEventsIOS.mm)
	list(REMOVE_ITEM windowing_os_SRCS osx/WinSystemIOS.mm)
elseif(TARGET_COMMON_LINUX)
	find_all_sources(X11 windowing_os_SRCS)
elseif(TARGET_WIN32)
	find_all_sources(windows windowing_os_SRCS)
  list(REMOVE_ITEM windowing_os_SRCS windows/WinSystemWin32GL.cpp)
endif(TARGET_COMMON_DARWIN)

add_sources(${windowing_SRCS} ${windowing_os_SRCS})
