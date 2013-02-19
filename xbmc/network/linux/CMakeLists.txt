find_all_sources(. network_linux_SRCS)

if(HAVE_LIBAVAHI)
	list(REMOVE_ITEM network_linux_SRCS ./ZeroconfAvahi.cpp)
	list(REMOVE_ITEM network_linux_SRCS ./ZeroconfBrowserAvahi.cpp)
endif(HAVE_LIBAVAHI)

add_sources(${network_linux_SRCS})
