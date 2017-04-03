set(SOURCES AddonsDirectory.cpp
            AudioBookFileDirectory.cpp
            CacheStrategy.cpp
            CDDADirectory.cpp
            CDDAFile.cpp
            CircularCache.cpp
            CurlFile.cpp
            DAVCommon.cpp
            DAVDirectory.cpp
            DAVFile.cpp
            DirectoryCache.cpp
            Directory.cpp
            DirectoryFactory.cpp
            DirectoryHistory.cpp
            DllLibCurl.cpp
            EventsDirectory.cpp
            FavouritesDirectory.cpp
            FileCache.cpp
            File.cpp
            FileDirectoryFactory.cpp
            FileFactory.cpp
            FTPDirectory.cpp
            FTPParse.cpp
            HTTPDirectory.cpp
            IDirectory.cpp
            IFile.cpp
            ImageFile.cpp
            iso9660.cpp
            ISO9660Directory.cpp
            ISOFile.cpp
            LibraryDirectory.cpp
            MultiPathDirectory.cpp
            MultiPathFile.cpp
            MusicDatabaseDirectory.cpp
            MusicDatabaseFile.cpp
            MusicFileDirectory.cpp
            MusicSearchDirectory.cpp
            NFSDirectory.cpp
            NFSFile.cpp
            OverrideDirectory.cpp
            OverrideFile.cpp
            PipeFile.cpp
            PipesManager.cpp
            PlaylistDirectory.cpp
            PlaylistFileDirectory.cpp
            PluginDirectory.cpp
            PVRDirectory.cpp
            RarDirectory.cpp
            RarFile.cpp
            RarManager.cpp
            ResourceDirectory.cpp
            ResourceFile.cpp
            RSSDirectory.cpp
            SFTPDirectory.cpp
            SFTPFile.cpp
            ShoutcastFile.cpp
            SmartPlaylistDirectory.cpp
            SourcesDirectory.cpp
            SpecialProtocol.cpp
            SpecialProtocolDirectory.cpp
            SpecialProtocolFile.cpp
            StackDirectory.cpp
            udf25.cpp
            UDFDirectory.cpp
            UDFFile.cpp
            VideoDatabaseDirectory.cpp
            VideoDatabaseFile.cpp
            VirtualDirectory.cpp
            XbtDirectory.cpp
            XbtFile.cpp
            XbtManager.cpp
            ZeroconfDirectory.cpp
            ZipDirectory.cpp
            ZipFile.cpp
            ZipManager.cpp)

set(HEADERS AddonsDirectory.h
            CDDADirectory.h
            CDDAFile.h
            CacheStrategy.h
            CircularCache.h
            CurlFile.h
            DAVCommon.h
            DAVDirectory.h
            DAVFile.h
            Directorization.h
            Directory.h
            DirectoryCache.h
            DirectoryFactory.h
            DirectoryHistory.h
            DllLibCurl.h
            DllLibNfs.h
            EventsDirectory.h
            FTPDirectory.h
            FTPParse.h
            FavouritesDirectory.h
            File.h
            FileCache.h
            FileDirectoryFactory.h
            FileFactory.h
            HTTPDirectory.h
            IDirectory.h
            IFile.h
            IFileDirectory.h
            IFileTypes.h
            ImageFile.h
            ISO9660Directory.h
            ISOFile.h
            iso9660.h
            LibraryDirectory.h
            MultiPathDirectory.h
            MultiPathFile.h
            MusicDatabaseDirectory.h
            MusicDatabaseFile.h
            MusicFileDirectory.h
            MusicSearchDirectory.h
            NFSDirectory.h
            NFSFile.h
            OverrideDirectory.h
            OverrideFile.h
            PVRDirectory.h
            PipeFile.h
            PipesManager.h
            PlaylistDirectory.h
            PlaylistFileDirectory.h
            PluginDirectory.h
            RSSDirectory.h
            RarDirectory.h
            RarFile.h
            RarManager.h
            ResourceDirectory.h
            ResourceFile.h
            SFTPDirectory.h
            SFTPFile.h
            ShoutcastFile.h
            SmartPlaylistDirectory.h
            SourcesDirectory.h
            SpecialProtocol.h
            SpecialProtocolDirectory.h
            SpecialProtocolFile.h
            StackDirectory.h
            udf25.h
            UDFDirectory.h
            UDFFile.h
            VideoDatabaseDirectory.h
            VirtualDirectory.h
            XbtDirectory.h
            XbtFile.h
            XbtManager.h
            ZeroconfDirectory.h
            ZipDirectory.h
            ZipFile.h
            ZipManager.h)

if(CORE_SYSTEM_NAME STREQUAL android)
  list(APPEND SOURCES APKDirectory.cpp
                      APKFile.cpp
                      AndroidAppDirectory.cpp
                      AndroidAppFile.cpp)
  list(APPEND HEADERS APKDirectory.h
                      APKFile.h
                      AndroidAppDirectory.h
                      AndroidAppFile.h)
endif()

if(NOT CORE_SYSTEM_NAME STREQUAL windows AND ENABLE_SMBCLIENT)
  list(APPEND SOURCES SMBDirectory.cpp
                      SMBFile.cpp)
  list(APPEND HEADERS SMBDirectory.h
                      SMBFile.h)
endif()

if(BLURAY_FOUND)
  list(APPEND SOURCES BlurayDirectory.cpp
                      BlurayFile.cpp)
  list(APPEND HEADERS BlurayDirectory.h
                      BlurayFile.h)
endif()

if(ENABLE_UPNP)
  list(APPEND SOURCES NptXbmcFile.cpp
                      UPnPDirectory.cpp
                      UPnPFile.cpp)
  list(APPEND HEADERS UPnPDirectory.h
                      UPnPFile.h)
endif()

core_add_library(filesystem)
if(ENABLE_STATIC_LIBS AND ENABLE_UPNP)
  target_link_libraries(filesystem PRIVATE upnp)
endif()
