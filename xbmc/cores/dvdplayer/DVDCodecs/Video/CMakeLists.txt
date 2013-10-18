find_all_sources(. video_SRCS)
list(REMOVE_ITEM video_SRCS ./DVDVideoCodecOpenMax.cpp)
list(REMOVE_ITEM video_SRCS ./DVDVideoCodecVideoToolBox.cpp)
list(REMOVE_ITEM video_SRCS ./OpenMaxVideo.cpp)
list(REMOVE_ITEM video_SRCS ./OpenMax.cpp)

if(NOT TARGET_WIN32)
	list(REMOVE_ITEM video_SRCS ./DXVA.cpp)
endif()

if (NOT TARGET_LINUX)
	list(REMOVE_ITEM video_SRCS ./VDPAU.cpp)
	list(REMOVE_ITEM video_SRCS ./VAAPI.cpp)
endif()

add_sources(${video_SRCS})