set(SOURCES AddonsDirectory.cpp
            AudioBookFileDirectory.cpp
            CacheStrategy.cpp
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
            OverrideDirectory.cpp
            OverrideFile.cpp
            PipeFile.cpp
            PipesManager.cpp
            PlaylistDirectory.cpp
            PlaylistFileDirectory.cpp
            PluginDirectory.cpp
            PVRDirectory.cpp
            ResourceDirectory.cpp
            ResourceFile.cpp
            RSSDirectory.cpp
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
            OverrideDirectory.h
            OverrideFile.h
            PVRDirectory.h
            PipeFile.h
            PipesManager.h
            PlaylistDirectory.h
            PlaylistFileDirectory.h
            PluginDirectory.h
            RSSDirectory.h
            ResourceDirectory.h
            ResourceFile.h
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

if(BLURAY_FOUND)
  list(APPEND SOURCES BlurayCallback.cpp
                      BlurayDirectory.cpp
                      BlurayFile.cpp)
  list(APPEND HEADERS BlurayCallback.h
                      BlurayDirectory.h
                      BlurayFile.h)
endif()

if(ENABLE_OPTICAL)
  list(APPEND SOURCES CDDADirectory.cpp
                      CDDAFile.cpp)
  list(APPEND HEADERS CDDADirectory.h
                      CDDAFile.h)
endif()

if(NFS_FOUND)
  list(APPEND SOURCES NFSDirectory.cpp
                      NFSFile.cpp)
  list(APPEND HEADERS NFSDirectory.h
                      NFSFile.h)
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
