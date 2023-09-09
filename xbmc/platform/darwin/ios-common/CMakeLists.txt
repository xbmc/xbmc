set(SOURCES AnnounceReceiver.mm
            CPUInfoDarwinEmbed.cpp
            DarwinEmbedKeyboard.mm
            DarwinEmbedKeyboardView.mm
            DarwinEmbedNowPlayingInfoManager.mm
            DarwinEmbedUtils.mm
            GPUInfoDarwinEmbed.cpp
            NSData+GZIP.m
            PlatformDarwinEmbedded.cpp)

set(HEADERS AnnounceReceiver.h
            CPUInfoDarwinEmbed.h
            DarwinEmbedKeyboard.h
            DarwinEmbedKeyboardView.h
            DarwinEmbedNowPlayingInfoManager.h
            DarwinEmbedUtils.h
            GPUInfoDarwinEmbed.h
            NSData+GZIP.h
            PlatformDarwinEmbedded.h)

core_add_library(platform_ios-common)
