Name:		libvorbis
Version:	1.0
Release:	1
Summary:	The Vorbis General Audio Compression Codec

Group:		System Environment/Libraries
License:	BSD
URL:		http://www.xiph.org/
Vendor:		Xiph.org Foundation <team@xiph.org>
Source:		http://www.xiph.org/pub/ogg/vorbis/download/%{name}-%{version}.tar.gz
Prefix:		%{_prefix}
BuildRoot:	%{_tmppath}/%{name}-root

# We're forced to use an epoch since both Red Hat and Ximian use it in their
# rc packages
Epoch:          2
# Dirty trick to tell rpm that this package actually provides what the
# last rc and beta was offering
Provides:       %{name} = %{epoch}:1.0rc3-%{release}
Provides:       %{name} = %{epoch}:1.0beta4-%{release}

Requires:	libogg >= 1.0
BuildRequires:	libogg-devel >= 1.0

%description
Ogg Vorbis is a fully open, non-proprietary, patent-and-royalty-free,
general-purpose compressed audio format for audio and music at fixed 
and variable bitrates from 16 to 128 kbps/channel.

%package devel
Summary: 	Vorbis Library Development
Group: 		Development/Libraries
Requires:	libogg-devel >= 1.0
Requires:	libvorbis = %{version}
# Dirty trick to tell rpm that this package actually provides what the
# last rc and beta was offering
Provides:       %{name}-devel = %{epoch}:1.0rc3-%{release}
Provides:       %{name}-devel = %{epoch}:1.0beta4-%{release}

%description devel
The libvorbis-devel package contains the header files, static libraries 
and documentation needed to develop applications with libvorbis.

%prep
%setup -q -n %{name}-%{version}

%build
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{_prefix} --enable-static
make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

make DESTDIR=$RPM_BUILD_ROOT install

%clean 
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%defattr(-,root,root)
%doc COPYING
%doc AUTHORS
%doc README
%{_libdir}/libvorbis.so.*
%{_libdir}/libvorbisfile.so.*
%{_libdir}/libvorbisenc.so.*

%files devel
%doc doc/*.html
%doc doc/*.png
%doc doc/*.txt
%doc doc/vorbisfile
%doc doc/vorbisenc
%{_datadir}/aclocal/vorbis.m4
%{_includedir}/vorbis/codec.h
%{_includedir}/vorbis/vorbisfile.h
%{_includedir}/vorbis/vorbisenc.h
%{_libdir}/libvorbis.a
%{_libdir}/libvorbis.so
%{_libdir}/libvorbisfile.a
%{_libdir}/libvorbisfile.so
%{_libdir}/libvorbisenc.a
%{_libdir}/libvorbisenc.so

%changelog
* Sun Jul 14 2002 Thomas Vander Stichele <thomas@apestaart.org>
- Added BuildRequires:
- updated for 1.0 release

* Sat May 25 2002 Michael Smith <msmith@icecast.org>
- Fixed requires, copyright string.
* Sun Dec 31 2001 Jack Moffitt <jack@xiph.org>
- Updated for rc3 release.

* Sun Oct 07 2001 Jack Moffitt <jack@xiph.org>
- Updated for configurable prefixes

* Sat Oct 21 2000 Jack Moffitt <jack@icecast.org>
- initial spec file created
