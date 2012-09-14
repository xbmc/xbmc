/* xbmc/config.h.  Generated from config.h.in by configure.  */
/* xbmc/config.h.in.  Generated from configure.in by autoheader.  */

#pragma once

/* Define if building universal (internal helper macro) */
#define AC_APPLE_UNIVERSAL_BUILD 1

/* Whether AVPacket is in libavformat. */
/* #undef AVPACKET_IN_AVFORMAT */

/* Define to 1 if the `closedir' function returns void instead of `int'. */
/* #undef CLOSEDIR_VOID */

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* Define to 1 to have optical drive support */
#define HAS_DVD_DRIVE 1

/* Define to 1 if you have HAL installed */
/* #undef HAS_HAL */

/* Whether to use libRTMP library. */
#define HAS_LIBRTMP 1

/* Whether to build skin touched. */
#define HAS_SKIN_TOUCHED 1

/* Define to 1 if you have the <afpfs-ng/libafpclient.h> header file. */
/* #undef HAVE_AFPFS_NG_LIBAFPCLIENT_H */

/* Define to 1 if you have the `alarm' function. */
#define HAVE_ALARM 1

/* Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
#define HAVE_ALLOCA_H 1

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to 1 if you have the `atexit' function. */
#define HAVE_ATEXIT 1

/* Define to 1 if AVFilterBufferRefVideoProps has member sample_aspect_ratio.
   */
/* #undef HAVE_AVFILTERBUFFERREFVIDEOPROPS_SAMPLE_ASPECT_RATIO */

/* Define to 1 if you have the <cdio/iso9660.h> header file. */
#define HAVE_CDIO_ISO9660_H 1

/* Define to 1 if your system has a working `chown' function. */
#define HAVE_CHOWN 1

/* "Define to 1 if dbus is installed" */
/* #undef HAVE_DBUS */

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.
   */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
/* #undef HAVE_DOPRNT */

/* Define to 1 if you have the `dup2' function. */
#define HAVE_DUP2 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `fdatasync' function. */
#define HAVE_FDATASYNC 1

/* Define to 1 if you have the <ffmpeg/avcodec.h> header file. */
/* #undef HAVE_FFMPEG_AVCODEC_H */

/* Define to 1 if you have the <ffmpeg/avfilter.h> header file. */
/* #undef HAVE_FFMPEG_AVFILTER_H */

/* Define to 1 if you have the <ffmpeg/avformat.h> header file. */
/* #undef HAVE_FFMPEG_AVFORMAT_H */

/* Define to 1 if you have the <ffmpeg/avutil.h> header file. */
/* #undef HAVE_FFMPEG_AVUTIL_H */

/* Define to 1 if you have the <ffmpeg/rgb2rgb.h> header file. */
/* #undef HAVE_FFMPEG_RGB2RGB_H */

/* Define to 1 if you have the <ffmpeg/swscale.h> header file. */
/* #undef HAVE_FFMPEG_SWSCALE_H */

/* Define to 1 if you have the <float.h> header file. */
#define HAVE_FLOAT_H 1

/* Define to 1 if you have the `floor' function. */
#define HAVE_FLOOR 1

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#define HAVE_FSEEKO 1

/* Define to 1 if you have the `fs_stat_dev' function. */
/* #undef HAVE_FS_STAT_DEV */

/* Define to 1 if you have the `ftime' function. */
#define HAVE_FTIME 1

/* Define to 1 if you have the `ftruncate' function. */
#define HAVE_FTRUNCATE 1

/* Define if we have gcrypt */
//#define HAVE_GCRYPT 0

/* Define to 1 if you have the `getcwd' function. */
#define HAVE_GETCWD 1

/* Define to 1 if you have the `gethostbyaddr' function. */
#define HAVE_GETHOSTBYADDR 1

/* Define to 1 if you have the `gethostbyname' function. */
#define HAVE_GETHOSTBYNAME 1

/* Define to 1 if you have the `gethostname' function. */
#define HAVE_GETHOSTNAME 1

/* Define to 1 if you have the `getpagesize' function. */
#define HAVE_GETPAGESIZE 1

/* Define to 1 if you have the `getpass' function. */
#define HAVE_GETPASS 1

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the `inet_ntoa' function. */
#define HAVE_INET_NTOA 1

/* Define if we have inotify */
/* #undef HAVE_INOTIFY */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `lchown' function. */
#define HAVE_LCHOWN 1

/* Whether to use libafpclient library. */
/* #undef HAVE_LIBAFPCLIENT */

/* Define to 1 if you have the `avahi-client' library (-lavahi-client). */
/* #undef HAVE_LIBAVAHI_CLIENT */

/* Define to 1 if you have the `avahi-common' library (-lavahi-common). */
/* #undef HAVE_LIBAVAHI_COMMON */

/* Define to 1 if you have the `avcodec' library (-lavcodec). */
/* #undef HAVE_LIBAVCODEC */

/* Define to 1 if you have the <libavcodec/avcodec.h> header file. */
/* #undef HAVE_LIBAVCODEC_AVCODEC_H */

/* Define to 1 if you have the <libavcore/avcore.h> header file. */
/* #undef HAVE_LIBAVCORE_AVCORE_H */

/* Define to 1 if you have the <libavcore/samplefmt.h> header file. */
/* #undef HAVE_LIBAVCORE_SAMPLEFMT_H */

/* Define to 1 if you have the <libavfilter/avfilter.h> header file. */
/* #undef HAVE_LIBAVFILTER_AVFILTER_H */

/* Define to 1 if you have the <libavformat/avformat.h> header file. */
/* #undef HAVE_LIBAVFORMAT_AVFORMAT_H */

/* Define to 1 if you have the <libavutil/avutil.h> header file. */
/* #undef HAVE_LIBAVUTIL_AVUTIL_H */

/* Define to 1 if you have the <libavutil/mem.h> header file. */
/* #undef HAVE_LIBAVUTIL_MEM_H */

/* Define to 1 if you have the <libavutil/opt.h> header file. */
/* #undef HAVE_LIBAVUTIL_OPT_H */

/* Define to 1 if you have the <libavutil/samplefmt.h> header file. */
/* #undef HAVE_LIBAVUTIL_SAMPLEFMT_H */

/* Define to 1 if you have the `bluetooth' library (-lbluetooth). */
/* #undef HAVE_LIBBLUETOOTH */

/* System has libbluray library */
#define HAVE_LIBBLURAY 1

/* System has an old api libbluray without angle support */
/* #undef HAVE_LIBBLURAY_NOANGLE */

/* System has an old api libbluray without log support */
/* #undef HAVE_LIBBLURAY_NOLOGCONTROL */

/* Define to 1 if you have the `bz2' library (-lbz2). */
#define HAVE_LIBBZ2 1

/* "Define to 1 if libcec is installed" */
#define HAVE_LIBCEC 1

/* Define to 1 if you have the `crypto' library (-lcrypto). */
#define HAVE_LIBCRYPTO 1

/* Define to 1 if you have the 'Old Broadcom Crystal HD' library. */
#define HAVE_LIBCRYSTALHD 2

/* Define to 1 if you have the `dl' library (-ldl). */
/* #undef HAVE_LIBDL */

/* Define to 1 if you have the `EGL' library (-lEGL). */
/* #undef HAVE_LIBEGL */

/* Define to 1 if you have the `gcrypt' library (-lgcrypt). */
#define HAVE_LIBGCRYPT 0

/* Define to 1 if you have the `GL' library (-lGL). */
#define HAVE_LIBGL 1

/* Define to 1 if you have the `GLESv2' library (-lGLESv2). */
/* #undef HAVE_LIBGLESV2 */

/* Define to 1 if you have the `GLEW' library (-lGLEW). */
#define HAVE_LIBGLEW 1

/* Define to 1 if you have the `GLU' library (-lGLU). */
#define HAVE_LIBGLU 1

/* Define to 1 if you have the `iconv' library (-liconv). */
#define HAVE_LIBICONV 1

/* Define to 1 if you have the `jasper' library (-ljasper). */
/* #undef HAVE_LIBJASPER */

/* Define to 1 if you have the `jpeg' library (-ljpeg). */
#define HAVE_LIBJPEG 1

/* Define to 1 if you have the `lzo2' library (-llzo2). */
#define HAVE_LIBLZO2 1

/* Define to 1 if you have the `microhttpd' library (-lmicrohttpd). */
#define HAVE_LIBMICROHTTPD 1

/* Whether to use libnfs library. */
/* #undef HAVE_LIBNFS */

/* "Define to 1 if libpcre is installed" */
#define HAVE_LIBPCRE 1

/* "Define to 1 if libpcrecpp is installed" */
#define HAVE_LIBPCRECPP 1

/* Define to 1 if you have the `plist' library (-lplist). */
#define HAVE_LIBPLIST 1

/* Define to 1 if you have the <libpostproc/postprocess.h> header file. */
/* #undef HAVE_LIBPOSTPROC_POSTPROCESS_H */

/* Define to 1 if you have the `pthread' library (-lpthread). */
#define HAVE_LIBPTHREAD 1

/* Define to 1 if you have the `pulse' library (-lpulse). */
/* #undef HAVE_LIBPULSE */

/* Define to 1 if you have the `resolv' library (-lresolv). */
/* #undef HAVE_LIBRESOLV */

/* Define to 1 if you have the `rt' library (-lrt). */
/* #undef HAVE_LIBRT */

/* Define to 1 if you have the <librtmp/amf.h> header file. */
#define HAVE_LIBRTMP_AMF_H 1

/* Define to 1 if you have the <librtmp/log.h> header file. */
#define HAVE_LIBRTMP_LOG_H 1

/* Define to 1 if you have the <librtmp/rtmp.h> header file. */
#define HAVE_LIBRTMP_RTMP_H 1

/* Define to 1 if you have the `SDL' library (-lSDL). */
#define HAVE_LIBSDL 1

/* Define to 1 if you have the `SDL_gfx' library (-lSDL_gfx). */
/* #undef HAVE_LIBSDL_GFX */

/* Define to 1 if you have the `SDL_image' library (-lSDL_image). */
/* #undef HAVE_LIBSDL_IMAGE */

/* Define to 1 if you have the `SDL_mixer' library (-lSDL_mixer). */
#define HAVE_LIBSDL_MIXER 1

/* Define to 1 if you have the `shairport' library (-lshairport). */
#define HAVE_LIBSHAIRPORT 1

/* Define to 1 if you have Samba installed */
/* #undef HAVE_LIBSMBCLIENT */

/* Define to 1 if you have the `ssh' library (-lssh). */
#define HAVE_LIBSSH 1

/* Define to 1 if you have the `ssl' library (-lssl). */
#define HAVE_LIBSSL 1

/* Define to 1 if you have the <libswscale/rgb2rgb.h> header file. */
/* #undef HAVE_LIBSWSCALE_RGB2RGB_H */

/* Define to 1 if you have the <libswscale/swscale.h> header file. */
/* #undef HAVE_LIBSWSCALE_SWSCALE_H */

/* Define to 1 if you have the `tiff' library (-ltiff). */
#define HAVE_LIBTIFF 1

/* "Define to 1 if libudev is installed" */
/* #undef HAVE_LIBUDEV */

/* "Define to 1 if libusb is installed" */
/* #undef HAVE_LIBUSB */

/* Define to 1 if you have the 'vaapi' libraries (-lva AND -lva-glx) */
/* #undef HAVE_LIBVA */

/* Define to 1 if you have the 'VDADecoder' library. */
#define HAVE_LIBVDADECODER 1

/* Define to 1 if you have the 'vdpau' library (-lvdpau). */
/* #undef HAVE_LIBVDPAU */

/* Define to 1 if you have the `Xrandr' library (-lXrandr). */
/* #undef HAVE_LIBXRANDR */

/* Define to 1 if you have the `yajl' library (-lyajl). */
#define HAVE_LIBYAJL 1

/* Define to 1 if you have the `z' library (-lz). */
#define HAVE_LIBZ 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the `localeconv' function. */
#define HAVE_LOCALECONV 1

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if `lstat' has the bug that it succeeds when given the
   zero-length file name argument. */
/* #undef HAVE_LSTAT_EMPTY_STRING_BUG */

/* Define to 1 if you have the <malloc.h> header file. */
/* #undef HAVE_MALLOC_H */

/* Define to 1 if you have the `memchr' function. */
#define HAVE_MEMCHR 1

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have the `mkdir' function. */
#define HAVE_MKDIR 1

/* Define to 1 if you have a working `mmap' system call. */
#define HAVE_MMAP 1

/* Define to 1 if you have the `modf' function. */
#define HAVE_MODF 1

/* Define to 1 if you have the `munmap' function. */
#define HAVE_MUNMAP 1

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_NDIR_H */

/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define to 1 if you have the <nfsc/libnfs.h> header file. */
/* #undef HAVE_NFSC_LIBNFS_H */

/* Define if we have openssl */
#define HAVE_OPENSSL 1

/* Define to 1 if you have the <postproc/postprocess.h> header file. */
/* #undef HAVE_POSTPROC_POSTPROCESS_H */

/* Define to 1 if you have the `pow' function. */
#define HAVE_POW 1

/* If available, contains the Python version number currently in use. */
#define HAVE_PYTHON "2.7"

/* Define to 1 if you have the `rmdir' function. */
#define HAVE_RMDIR 1

/* "Define to 1 if using sdl" */
#define HAVE_SDL 1

/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1

/* Define to 1 if you have the `setenv' function. */
#define HAVE_SETENV 1

/* Define to 1 if you have the `setlocale' function. */
#define HAVE_SETLOCALE 1

/* Define to 1 if you have the `socket' function. */
#define HAVE_SOCKET 1

/* Define to 1 if you have the `sqrt' function. */
#define HAVE_SQRT 1

/* Define to 1 if `stat' has the bug that it succeeds when given the
   zero-length file name argument. */
/* #undef HAVE_STAT_EMPTY_STRING_BUG */

/* Define to 1 if stdbool.h conforms to C99. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stddef.h> header file. */
#define HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define to 1 if you have the `strcoll' function and it is properly defined.
   */
#define HAVE_STRCOLL 1

/* Define to 1 if you have the `strcspn' function. */
#define HAVE_STRCSPN 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the `strftime' function. */
#define HAVE_STRFTIME 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strncasecmp' function. */
#define HAVE_STRNCASECMP 1

/* Define to 1 if you have the `strpbrk' function. */
#define HAVE_STRPBRK 1

/* Define to 1 if you have the `strrchr' function. */
#define HAVE_STRRCHR 1

/* Define to 1 if you have the `strspn' function. */
#define HAVE_STRSPN 1

/* Define to 1 if you have the `strstr' function. */
#define HAVE_STRSTR 1

/* Define to 1 if you have the `strtol' function. */
#define HAVE_STRTOL 1

/* Define to 1 if you have the `strtoul' function. */
#define HAVE_STRTOUL 1

/* Define to 1 if `st_rdev' is member of `struct stat'. */
#define HAVE_STRUCT_STAT_ST_RDEV 1

/* Define to 1 if you have the `sysinfo' function. */
/* #undef HAVE_SYSINFO */

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_DIR_H */

/* Define to 1 if you have the <sys/file.h> header file. */
#define HAVE_SYS_FILE_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/mount.h> header file. */
#define HAVE_SYS_MOUNT_H 1

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_NDIR_H */

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/timeb.h> header file. */
#define HAVE_SYS_TIMEB_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/vfs.h> header file. */
/* #undef HAVE_SYS_VFS_H */

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
#define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <termios.h> header file. */
#define HAVE_TERMIOS_H 1

/* Define to 1 if you have the `tzset' function. */
#define HAVE_TZSET 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `utime' function. */
#define HAVE_UTIME 1

/* Define to 1 if you have the <utime.h> header file. */
#define HAVE_UTIME_H 1

/* Define to 1 if `utime(file, NULL)' sets file's timestamp to the present. */
#define HAVE_UTIME_NULL 1

/* Define to 1 if you have the 'VTBDecoder' library. */
/* #undef HAVE_VIDEOTOOLBOXDECODER */

/* Define to 1 if you have the `vprintf' function. */
#define HAVE_VPRINTF 1

/* Define to 1 if you have the <wchar.h> header file. */
#define HAVE_WCHAR_H 1

/* Define to 1 if you have the <wctype.h> header file. */
#define HAVE_WCTYPE_H 1

/* Define to 1 if you have X11 libs installed. */
/* #undef HAVE_X11 */

/* Define to 1 to enable non-free components. */
#define HAVE_XBMC_NONFREE 1

/* Define to 1 if you have the <yajl/yajl_version.h> header file. */
#define HAVE_YAJL_YAJL_VERSION_H 1

/* Define to 1 if the system has the type `_Bool'. */
#define HAVE__BOOL 1

/* Default LIRC device */
#define LIRC_DEVICE "/dev/lircd"

/* Define to 1 if `lstat' dereferences a symlink specified with a trailing
   slash. */
/* #undef LSTAT_FOLLOWS_SLASHED_SYMLINK */

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "xbmc"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "http://trac.xbmc.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "xbmc"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "xbmc 11.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "xbmc"

/* Define to the version of this package. */
#define PACKAGE_VERSION "11.0"

/* Whether AVUtil defines PIX_FMT_VDPAU_MPEG4. */
#define PIX_FMT_VDPAU_MPEG4_IN_AVUTIL 1

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* Define to the type of arg 1 for `select'. */
#define SELECT_TYPE_ARG1 int

/* Define to the type of args 2, 3 and 4 for `select'. */
#define SELECT_TYPE_ARG234 (fd_set *)

/* Define to the type of arg 5 for `select'. */
#define SELECT_TYPE_ARG5 (struct timeval *)

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `size_t', as computed by sizeof. */
#define SIZEOF_SIZE_T 4

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at runtime.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define to 1 if the `S_IS*' macros in <sys/stat.h> do not work properly. */
/* #undef STAT_MACROS_BROKEN */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
/* #undef TM_IN_SYS_TIME */

/* "Define to 1 if alsa is installed" */
/* #undef USE_ALSA */

/* Define to 1 to enable ASAP codec. */
/* #undef USE_ASAP_CODEC */

/* Whether to use external FFmpeg libraries. */
/* #undef USE_EXTERNAL_FFMPEG */

/* Version number of package */
#define VERSION "11.0"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* yajl version 1 */
/* #undef YAJL_MAJOR */

/* Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2). */
/* #undef _LARGEFILE_SOURCE */

/* Define for Solaris 2.5.1 so the uint32_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT32_T */

/* Define for Solaris 2.5.1 so the uint64_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT64_T */

/* Define for Solaris 2.5.1 so the uint8_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT8_T */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef gid_t */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to the type of a signed integer type of width exactly 16 bits if
   such a type exists and the standard includes do not define it. */
/* #undef int16_t */

/* Define to the type of a signed integer type of width exactly 32 bits if
   such a type exists and the standard includes do not define it. */
/* #undef int32_t */

/* Define to the type of a signed integer type of width exactly 64 bits if
   such a type exists and the standard includes do not define it. */
/* #undef int64_t */

/* Define to the type of a signed integer type of width exactly 8 bits if such
   a type exists and the standard includes do not define it. */
/* #undef int8_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef mode_t */

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to the equivalent of the C99 'restrict' keyword, or to
   nothing if this is not supported.  Do not define if restrict is
   supported directly.  */
#define restrict __restrict
/* Work around a bug in Sun C++: it does not support _Restrict, even
   though the corresponding Sun C compiler does, which causes
   "#define restrict _Restrict" in the previous line.  Perhaps some future
   version of Sun C++ will work with _Restrict; if so, it'll probably
   define __RESTRICT, just as Sun C does.  */
#if defined __SUNPRO_CC && !defined __RESTRICT
# define _Restrict
#endif

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef ssize_t */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef uid_t */

/* Define to the type of an unsigned integer type of width exactly 16 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint16_t */

/* Define to the type of an unsigned integer type of width exactly 32 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint32_t */

/* Define to the type of an unsigned integer type of width exactly 64 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint64_t */

/* Define to the type of an unsigned integer type of width exactly 8 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint8_t */
