%define name 	libdvdcss
%define version	1.2.9
%define release	1

%define major  	2
%define libname %{name}%{major}

%define buildfor_rh9 %([[ -e /etc/mandrake-release ]] && echo 0 || echo 1)

%if %buildfor_rh9
# some mdk macros that do not exist in rh
%define configure2_5x %configure
%define make %__make
%define makeinstall_std %makeinstall
# adjust define for Red Hat.
%endif


Name:		%{name}
Version:	%{version}
Release:	%{release}
Summary:        Library for accessing DVDs like block devices with transparent decryption
Source:		%{name}-%{version}.tar.bz2
License:	GPL
Group:		System/Libraries
URL:		http://www.videolan.org/libdvdcss/
Packager:	Yves Duret <yves@zarb.org>
BuildRoot:	%_tmppath/%name-%version-%release-root
Conflicts:	libdvdcss0.0.1, libdvdcss0.0.2

%description
libdvdcss is a simple library designed for accessing DVDs like a block device
without having to bother about the decryption. The important features are:
 * Portability: currently supported platforms are GNU/Linux, FreeBSD, NetBSD,
   OpenBSD, BeOS, Windows 98/ME, Windows NT/2000/XP, Mac OS X, Solaris,
   HP-UX and OS/2.
 * Adaptability: unlike most similar projects, libdvdcss doesn't require the
   region of your drive to be set and will try its best to read from the disc
   even in the case of a region mismatch.
 * Simplicity: a DVD player can be built around the libdvdcss API using no
   more than 6 library calls.

%package -n %{libname}
Summary:        Library for accessing DVDs like block devices with transparent decryption
Group:          System/Libraries
Provides:       %name = %version-%release

%description -n %{libname}
libdvdcss is a simple library designed for accessing DVDs like a block device
without having to bother about the decryption. The important features are:
 * Portability: currently supported platforms are GNU/Linux, FreeBSD, NetBSD,
   OpenBSD, BeOS, Windows 98/ME, Windows NT/2000/XP, Mac OS X, Solaris,
   HP-UX and OS/2.
 * Adaptability: unlike most similar projects, libdvdcss doesn't require the
   region of your drive to be set and will try its best to read from the disc
   even in the case of a region mismatch.
 * Simplicity: a DVD player can be built around the libdvdcss API using no
   more than 6 library calls.

%package -n %{libname}-devel
Summary:        Development tools for programs which will use the %{name} library
Group:          Development/C
Requires:	%{libname} = %version-%release
Provides:       %{name}-devel = %version-%release
 
%description -n %{libname}-devel
The %{name}-devel package includes the header files and static libraries
necessary for developing programs which will manipulate DVDs files using
the %{name} library.
 
If you are going to develop programs which will manipulate DVDs, you
should install %{name}-devel.  You'll also need to have the %{name}
package installed.

%prep
%setup -q

%build
%configure2_5x
%make

%install
%makeinstall_std

%clean
[ %buildroot != "/" ] && rm -Rf %buildroot

%post -n %{libname} -p /sbin/ldconfig
 
%postun -n %{libname} -p /sbin/ldconfig

%files -n %{libname}
%defattr(-,root,root)
%doc AUTHORS COPYING NEWS
%{_libdir}/*.so.*

%files -n %{libname}-devel
%defattr(-,root,root)
%doc ChangeLog COPYING
%{_libdir}/*.a
%{_libdir}/*.so
%{_libdir}/*.la
%{_includedir}/*

%changelog
* Mon Jul 11 2005 Sam Hocevar <sam@zoy.org> 1.2.9-1
- new upstream release

* Tue Jul 29 2003 Sam Hocevar <sam@zoy.org> 1.2.8-1
- new upstream release

* Fri Jun 13 2003 Sam Hocevar <sam@zoy.org> 1.2.7-1
- new upstream release
- key cache activated by default

* Mon Mar 10 2003 Alexis de Lattre <alexis@videolan.org> 1.2.6-1
- new upstream release
- small bug fixes

* Tue Jan 28 2003 Sam Hocevar <sam@zoy.org> 1.2.5-1
- new upstream release
- improved robustness in case of read errors
- key cache support
- added more macros to fix Red Hat build

* Mon Nov 18 2002 Alexis de Lattre <alexis@videolan.org> 1.2.4-2
- Changes in .spec file for Red Hat and RPM 4.1

* Thu Nov 14 2002 Alexis de Lattre <alexis@videolan.org> 1.2.4-1
- new upstream release
- fixes for Win32

* Sun Oct 13 2002 Sam Hocevar <sam@zoy.org> 1.2.3-1
- new upstream release
- fix for drives not allowing to read their disc key

* Sat Aug 10 2002 Sam Hocevar <sam@zoy.org> 1.2.2-1
- new upstream release
- even more fixes for the disc/drive region mismatch problem

* Sun Jun 02 2002 Sam Hocevar <sam@zoy.org> 1.2.1-1
- new upstream release
- fix for a crash on disc/drive region mismatch

* Mon May 20 2002 Sam Hocevar <sam@zoy.org> 1.2.0-1
- new upstream release
- weird libxalf dependency is gone

* Sun Apr 07 2002 Yves Duret <yduret@mandrakesoft.com> 1.1.1-2plf
- major version is 2 (aka guillaume sux).
- spec clean up: do not rm in %%prep, %%buildroot, %%makeinstall_std, %%provides %%version-%%release
- added doc in devel
- sync with CVS's one (%%description,%%files, conflicts).
- fix URL

* Sat Apr 06 2002 Guillaume Rousse <rousse@ccr.jussieu.fr> 1.1.1-1plf
- 1.1.1

* Wed Jan 30 2002 Guillaume Rousse <rousse@ccr.jussieu.fr> 1.0.0-3plf 
- new plf extension

* Wed Dec 05 2001 Guillaume Rousse <g.rousse@linux-mandrake.com> 1.0.0-3mdk
- removed conflict

* Tue Dec 04 2001 Guillaume Rousse <g.rousse@linux-mandrake.com> 1.0.0-2mdk
- contributed to PLF by Yves Duret <yduret@mandrakesoft.com>
- Conflicts: libdvdcss-ogle
- more doc files
- no doc file for devel package

* Fri Nov 30 2001 Yves Duret <yduret@mandrakesoft.com> 1.0.0-1mdk
- version 1.0.0

* Thu Aug 23 2001 Yves Duret <yduret@mandrakesoft.com> 0.0.3-1mdk
- version 0.0.3

* Mon Aug 13 2001 Yves Duret <yduret@mandrakesoft.com> 0.0.2-1mdk
- version 0.0.2

* Tue Jun 19 2001 Yves Duret <yduret@mandrakesoft.com> 0.0.1-1mdk
- first release and first mdk release
