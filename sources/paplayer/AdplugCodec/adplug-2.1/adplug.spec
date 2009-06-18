%define name adplug
%define version 2.1
%define release 1

Summary: AdLib sound player library
Name: %{name}
Version: %{version}
Release: %{release}
Source0: %{name}-%{version}.tar.bz2
URL: http://adplug.sourceforge.net/
License: LGPL
Group: Applications/Multimedia
BuildRoot: %{_tmppath}/%{name}-buildroot
Prefix: %{_prefix}
BuildRequires: binio-devel >= 1.4
Requires: binio >= 1.4

%description
AdPlug is a free, multi-platform, hardware independent AdLib sound player
library, mainly written in C++. AdPlug plays sound data, originally created
for the AdLib (OPL2) audio board, on top of an OPL2 emulator or by using the
real hardware. No OPL2 chip is required for playback.

It supports various audio formats from MS-DOS AdLib trackers.

%package devel
Group: Development/Libraries
Summary: Development files of AdPlug
Requires: %name = %version-%release
Requires: binio-devel >= 1.4

%description devel
AdPlug is a free, multi-platform, hardware independent AdLib sound player
library, mainly written in C++. AdPlug plays sound data, originally created
for the AdLib (OPL2) audio board, on top of an OPL2 emulator or by using the
real hardware. No OPL2 chip is required for playback.

It supports various audio formats from MS-DOS AdLib trackers.

This package contains the C++ headers and documentation required for
building programs based on AdPlug.

%package static-devel
Group: Development/Libraries
Summary: Static library of AdPlug
Requires: %name-devel = %version-%release

%description static-devel
AdPlug is a free, multi-platform, hardware independent AdLib sound player
library, mainly written in C++. AdPlug plays sound data, originally created
for the AdLib (OPL2) audio board, on top of an OPL2 emulator or by using the
real hardware. No OPL2 chip is required for playback.

It supports various audio formats from MS-DOS AdLib trackers.

This package contains the static library required for statically
linking applications based on AdPlug.

%prep
%setup -q

%build
%configure
make 

%install
rm -rf $RPM_BUILD_ROOT
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%doc README AUTHORS NEWS TODO
%_bindir/adplugdb
%_mandir/man1/adplugdb.1*
%_libdir/*.so.*

%files devel
%defattr(-,root,root)
%doc doc/*.txt doc/*.ps
%_includedir/adplug/*
%_libdir/*.so
%_libdir/*.la

%files static-devel
%defattr(-,root,root)
%_libdir/*.a

%changelog
* Tue Mar  4 2003 Götz Waschk <waschk@linux-mandrake.com> 1.4-1
- requires binio library
- fix groups for RH standard
- remove patches
- add adplugdb
- new version

* Tue Nov 26 2002 Götz Waschk <waschk@linux-mandrake.com> 1.3-1
- initial package
