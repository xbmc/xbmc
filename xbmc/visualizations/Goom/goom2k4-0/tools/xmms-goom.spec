%define _xmmsoutputdir %(xmms-config --visualization-plugin-dir)

Summary: A neat visual plugin for XMMS.
Name: xmms-goom
Version: 1.9.0
Release: fr1
Copyright: GPL
Group: Applications/Multimedia
URL: http://goom.sourceforge.net/
Source: http://prdownloads.sourceforge.net/goom/xmms-goom_%{version}.tgz
BuildRoot: %{_tmppath}/%{name}-root
Requires: xmms >= 0.9.5.1
BuildPrereq: xmms-devel, gtk+-devel

%description
A great visual plugins for XMMS.

%prep
%setup -q -n %{name}_%{version}

%build
%configure --libdir=%{_xmmsoutputdir}
make

%install
rm -rf %{buildroot}
%makeinstall libdir=%{buildroot}/%{_xmmsoutputdir}
strip %{buildroot}/%{_xmmsoutputdir}/*.so

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog NEWS README doc
%{_xmmsoutputdir}/libgoom.*

%changelog
* Sun Jan  6 2002 Matthias Saou <matthias.saou@est.une.marmotte.net>
- Initial RPM release.

