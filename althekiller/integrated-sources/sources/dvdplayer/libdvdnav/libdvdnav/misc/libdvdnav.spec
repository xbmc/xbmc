%define	prefix  /usr/local
%define name    libdvdnav
%define ver     0.2.0cvs
%define rel     0

Name:		%{name}
Summary:	DVD Navigation library
Version:	%{ver}
Release:	%{rel}
Group:		Development/Libraries
Copyright:	GPL
Url:		http://dvd.sourceforge.net/
Source:		%{name}-%{version}.tar.gz
Buildroot:	%{_tmppath}/%{name}-%{version}-%{release}-root

%description
libdvdnav provides support to applications wishing to make use of advanced
DVD navigation features.

%prep
%setup

%build
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{prefix} 
make

%install
rm -rf $RPM_BUILD_ROOT
make install-strip DESTDIR=$RPM_BUILD_ROOT

%clean
rm -r $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc AUTHORS ChangeLog COPYING INSTALL README
%{prefix}/bin/dvdnav-config
%{prefix}/lib/libdvdnav*.la
%{prefix}/lib/libdvdnav*.so.*
%{prefix}/lib/libdvdnav*.so
%{prefix}/include/dvdnav/*
#/dvdnav.m4

%changelog
* Sun Mar 18 2002 Daniel Caujolle-Bert <f1rmb@users.sourceforge.net>
- Add missing files. Fix rpm generation.
* Tue Mar 12 2002 Rich Wareham <richwareham@users.sourceforge.net>
- Canabalisation to form libdvdnav spec file.
* Sun Sep 09 2001 Thomas Vander Stichele <thomas@apestaart.org>
- first spec file
