CPPFLAGS	+=	-I$(top_srcdir)/xbmc -I$(top_srcdir)/xbmc/linux 
CPPFLAGS	+=	-I$(top_srcdir)/guilib -I$(top_srcdir)/xbmc/lib/UnrarXLib 
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/utils -I$(top_srcdir)/xbmc/lib/libPython/Python
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/lib/libshout
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/lib/libUPnP/Platinum/ThirdParty/Neptune/Source/Core
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/lib/libUPnP/Platinum/Source/Core
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/lib/libUPnP/Platinum/Source/Devices/MediaServer
CPPFLAGS	+=	-I$(top_srcdir)/xbmc/lib/libUPnP/Platinum/ThirdParty/Neptune/Source/System/Posix
#CPPFLAGS	+=	-I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include
# !!FIXME!!
CPPFLAGS	+=	@HAL_CFLAGS@ -D__STDC_FORMAT_MACROS
SUBDIRS		=	MusicDatabaseDirectory VideoDatabaseDirectory
noinst_LIBRARIES	=	libFileSystem.a
libFileSystem_a_SOURCES	=	ASAPFileDirectory.cpp \
				CacheMemBuffer.cpp \
				CacheStrategy.cpp \
				CDDADirectory.cpp \
				cddb.cpp \
				cdioSupport.cpp \
				CMythDirectory.cpp \
				CMythFile.cpp \
				CMythSession.cpp \
				DAAPDirectory.cpp \
				DirectoryCache.cpp \
				Directory.cpp \
				DirectoryHistory.cpp \
				DirectoryTuxBox.cpp \
				DllLibCurl.cpp \
				FactoryDirectory.cpp \
				FactoryFileDirectory.cpp \
				FileCache.cpp \
				FileCDDA.cpp \
				File.cpp \
				FileCurl.cpp \
				FileDAAP.cpp \
				FileFactory.cpp \
				FileFileReader.cpp \
				FileHD.cpp \
				FileISO.cpp \
				FileLastFM.cpp \
				FileMMS.cpp \
				FileMusicDatabase.cpp \
				FileRar.cpp \
				FileRTV.cpp \
				FileShoutcast.cpp \
				FileSpecialProtocol.cpp \
				FileSmb.cpp \
				FileTuxBox.cpp \
				FileXBMSP.cpp \
				FileZip.cpp \
				FTPDirectory.cpp \
				FTPParse.cpp \
				HTTPDirectory.cpp \
				HDDirectory.cpp \
				HDHomeRun.cpp \
				IDirectory.cpp \
				IFile.cpp \
				iso9660.cpp \
				ISO9660Directory.cpp \
				LastFMDirectory.cpp \
				MultiPathDirectory.cpp \
				MultiPathFile.cpp \
				MusicDatabaseDirectory.cpp \
				MusicFileDirectory.cpp \
				MusicSearchDirectory.cpp \
				NptXbmcFile.cpp \
				NSFFileDirectory.cpp \
				OGGFileDirectory.cpp \
				PlaylistDirectory.cpp \
				PlaylistFileDirectory.cpp \
				PluginDirectory.cpp \
				RarDirectory.cpp \
				RarManager.cpp \
				RSSDirectory.cpp \
				RTVDirectory.cpp \
				ShoutcastDirectory.cpp \
				ShoutcastRipFile.cpp \
				SIDFileDirectory.cpp \
				SmartPlaylistDirectory.cpp \
				SMBDirectory.cpp \
				SpecialProtocol.cpp \
				SpecialProtocolDirectory.cpp \
				StackDirectory.cpp \
				UPnPDirectory.cpp \
				UPnPVirtualPathDirectory.cpp \
				VideoDatabaseDirectory.cpp \
				VirtualDirectory.cpp \
				VirtualPathDirectory.cpp \
				VTPDirectory.cpp \
				VTPSession.cpp \
				VTPFile.cpp \
				XBMSDirectory.cpp \
				ZipDirectory.cpp \
				ZipManager.cpp

# vim: ts=8:sw=8:noexpandtab
