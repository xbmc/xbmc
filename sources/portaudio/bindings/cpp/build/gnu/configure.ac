#
# PortAudioCpp V19 autoconf input file
# Shamelessly ripped from the PortAudio one by Dominic Mazzoni
# Ludwig Schwardt
#

# Require autoconf >= 2.13
AC_PREREQ(2.13)

AC_INIT([PortAudioCpp], [12])
AC_CONFIG_SRCDIR([../../include/portaudiocpp/PortAudioCpp.hxx])

###### Top-level directory of pacpp
###### This makes it easy to shuffle the build directories
###### Also edit AC_CONFIG_SRCDIR above (wouldn't accept this variable)!
PACPP_ROOT="../.."

######
###### SET THIS TO PORTAUDIO DIRECTORY
######
PORTAUDIO="$PACPP_ROOT/../portaudio"

# Various other variables and flags

PACPP_INC="$PACPP_ROOT/include"
INCLUDES="-I$PACPP_INC -I$PORTAUDIO -I$PORTAUDIO/pa_common"
CFLAGS="-g -O2 -Wall -ansi -pedantic $INCLUDES"
CXXFLAGS="$CFLAGS"
PALIBDIR="$PORTAUDIO/lib"

# Checks for programs

AC_PROG_CC
AC_PROG_CXX
AC_PROG_LN_S
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PATH_PROG(AR, ar, no)
if [[ $AR = "no" ]] ; then
    AC_MSG_ERROR("Could not find ar - needed to create a library");
fi

# This must be one of the first tests we do or it will fail...
AC_C_BIGENDIAN

# Transfer these variables to the Makefile
AC_SUBST(PACPP_ROOT)
AC_SUBST(PORTAUDIO)
AC_SUBST(PADLL)
AC_SUBST(PACPP_DLL)
AC_SUBST(PACPP_INC)
AC_SUBST(SHARED_FLAGS)
AC_SUBST(DLL_LIBS)
AC_SUBST(CXXFLAGS)

##################### CHECK FOR INSTALLED PACKAGES ############################

# checks for various host APIs and arguments to configure that
# turn them on or off

AC_CHECK_LIB(asound, snd_pcm_open, have_alsa=yes, have_alsa=no)

# Determine the host description for the subsequent test.
# PKG_CHECK_MODULES seems to check and set the host variable also, but
# that then requires pkg-config availability which is not standard on
# MinGW systems and can be a pain to install.
AC_CANONICAL_HOST

PKG_CHECK_MODULES(JACK, jack, have_jack=yes, have_jack=no)

AC_ARG_WITH(alsa, 
            [  --with-alsa (default=auto)],
            with_alsa=$withval, with_alsa="yes")

AC_ARG_WITH(jack, 
            [  --with-jack (default=auto)],
            with_jack=$withval, with_jack="yes")

AC_ARG_WITH(oss, 
            [  --with-oss (default=yes)],
            with_oss=$withval, with_oss="yes")

AC_ARG_WITH(host_os, 
            [  --with-host_os (no default)],
            host_os=$withval)

AC_ARG_WITH(winapi,
            [  --with-winapi ((wmme/directx/asio) default=wmme)],
            with_winapi=$withval, with_winapi="wmme")

# Mac API added for ASIO, can have other api's listed
AC_ARG_WITH(macapi,
            [  --with-macapi (asio) default=asio)],
            with_macapi=$withval, with_macapi="asio")

AC_ARG_WITH(asiodir,
            [  --with-asiodir (default=/usr/local/asiosdk2)],
            with_asiodir=$withval, with_asiodir="/usr/local/asiosdk2")

AC_ARG_WITH(dxdir,
            [  --with-dxdir (default=/usr/local/dx7sdk)],
            with_dxdir=$withval, with_dxdir="/usr/local/dx7sdk")


##################### HOST-SPECIFIC LIBRARY SETTINGS ##########################

case "${host_os}" in
  darwin* )
	# Mac OS X configuration

	LIBS="-framework AudioUnit -framework AudioToolbox -framework CoreAudio";
	PADLL="libportaudio.dylib";
	PACPP_DLL="libportaudiocpp.dylib";
	SHARED_FLAGS="-framework AudioUnit -framework AudioToolbox";
	SHARED_FLAGS="$SHARED_FLAGS -framework CoreAudio -dynamiclib";
        if [[ $with_macapi = "asio" ]] ; then
            if [[ $with_asiodir ]] ; then
              ASIODIR="$with_asiodir";
            else
              ASIODIR="/usr/local/asiosdk2";
            fi
            echo "ASIODIR: $ASIODIR";
        fi
	;;

  mingw* )
        # MingW configuration

        echo "WINAPI: $with_winapi"
        if [[ $with_winapi = "directx" ]] ; then
            if [[ $with_dxdir ]] ; then
              DXDIR="$with_dxdir";
            else
              DXDIR="/usr/local/dx7sdk";
            fi
            echo "DXDIR: $DXDIR"
	    LIBS="-L$PALIBDIR -lportaudio"
            LIBS="$LIBS -lwinmm -lm -ldsound -lole32";
            PADLL="portaudio.dll";
            PACPP_DLL="portaudiocpp.dll";
            SHARED_FLAGS="-shared -mthreads";
            DLL_LIBS="-lwinmm -lm -L./dx7sdk/lib -ldsound -lole32";
            CFLAGS="$CFLAGS -DPA_NO_WMME -DPA_NO_ASIO";
	    CXXFLAGS="$CFLAGS"
        elif [[ $with_winapi = "asio" ]] ; then
            if [[ $with_asiodir ]] ; then
              ASIODIR="$with_asiodir";
            else
              ASIODIR="/usr/local/asiosdk2";
            fi
            echo "ASIODIR: $ASIODIR"

	    LIBS="-L$PALIBDIR -lportaudio"
            LIBS="$LIBS -lwinmm -lm -lstdc++ -lole32 -luuid";
            PADLL="portaudio.dll";
            PACPP_DLL="portaudiocpp.dll";
            SHARED_FLAGS="-shared -mthreads";
            DLL_LIBS="-lwinmm -lm -lstdc++ -lole32 -luuid";
            CFLAGS="$CFLAGS -ffast-math -fomit-frame-pointer -DPA_NO_WMME -DPA_NO_DS -DWINDOWS";
            CXXFLAGS="$CFLAGS";
        else   # WMME default
	    LIBS="-L$PALIBDIR -lportaudio"
            LIBS="$LIBS -lwinmm -lm -lstdc++ -lole32 -luuid";
            PADLL="portaudio.dll";
            PACPP_DLL="portaudiocpp.dll";
            SHARED_FLAGS="-shared -mthreads";
            DLL_LIBS="-lwinmm";
            CFLAGS="$CFLAGS -DPA_NO_DS -DPA_NO_ASIO";
            CXXFLAGS="$CFLAGS";
        fi
        ;;

  cygwin* )
	# Cygwin configuration

	LIBS="-L$PALIBDIR -lportaudio"
	LIBS="$LIBS -lwinmm -lm";
	PADLL="portaudio.dll";
	PACPP_DLL="portaudiocpp.dll";
	SHARED_FLAGS="-shared -mthreads";
	DLL_LIBS="-lwinmm";
	;;

  *)
	# Unix OSS configuration

   AC_CHECK_LIB(pthread, pthread_create,
                ,
                AC_MSG_ERROR([libpthread not found!]))
	
	LIBS="$LIBS -L$PALIBDIR -lportaudio"
	
	if [[ $have_jack = "yes" ] && [ $with_jack != "no" ]] ; then
   	      	LIBS="$LIBS $JACK_LIBS"
		CFLAGS="$CFLAGS $JACK_CFLAGS"
                AC_DEFINE(PA_USE_JACK)
	fi

	if [[ $have_alsa = "yes" ] && [ $with_alsa != "no" ]] ; then
                LIBS="$LIBS -lasound"
                AC_DEFINE(PA_USE_ALSA)
	fi

	if [[ $with_oss != "no" ]] ; then
		AC_DEFINE(PA_USE_OSS)
	fi
	LIBS="$LIBS -lm -lpthread";
	PADLL="libportaudio.so";
	PACPP_DLL="libportaudiocpp.so";
	SHARED_FLAGS="-shared";
esac

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
