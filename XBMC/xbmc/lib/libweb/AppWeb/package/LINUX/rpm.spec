#
#	RPM spec file for the Mbedthis Appweb HTTP web server
#
Summary: !!BLD_NAME!! -- Embeddable HTTP Web Server
Name: !!BLD_PRODUCT!!
Version: !!BLD_VERSION!!
Release: !!BLD_NUMBER!!
License: Dual GPL/commercial
Group: Applications/Internet
URL: http://www.mbedthis.com/appweb
Distribution: Mbedthis
Vendor: Mbedthis Software
BuildRoot: !!ROOT_DIR!!/rpmDist
AutoReqProv: no

%description
Mbedthis Appweb is an embeddable HTTP Web Server

%prep

%build
    mkdir -p !!ROOT_DIR!!/rpmDist
    for suffix in "" -dev -doc -src ; do
        dir="!!BLD_PRODUCT!!${suffix}-!!BLD_VERSION!!"
        cp -r !!ROOT_DIR!!/${dir}/*  !!ROOT_DIR!!/rpmDist
    done

%install

%clean

%files -f binFiles.txt

%post
if [ -x /usr/bin/chcon ] ; then 
	if `sestatus | grep enabled` ; then
		for f in /usr/lib/!!BLD_PRODUCT!!/modules/*.so ; do
			chcon /usr/bin/chcon -t texrel_shlib_t $f
		done
	fi
fi

ldconfig /usr/lib/lib!!BLD_PRODUCT!!.so.?.?.?
ldconfig -n /usr/lib/!!BLD_PRODUCT!!
ldconfig -n /usr/lib/!!BLD_PRODUCT!!/modules

%preun

%postun

#
#	Dev package
#
%package dev
Summary: Mbedthis Appweb -- Development headers for Mbedthis Appweb
Group: Applications/Internet
Prefix: !!BLD_INC_PREFIX!!

%description dev
Development headers for the Mbedthis Appweb is an embedded HTTP web server.

%files dev -f devFiles.txt

#
#	Source package
#
%package src
Summary: Mbedthis Appweb -- Source code for Mbedthis Appweb
Group: Applications/Internet
Prefix: !!BLD_SRC_PREFIX!!

%description src
Source code for the Mbedthis Appweb, an embedded HTTP web server.

%files src -f srcFiles.txt

#
#	Documentation and Samples package
#
%package doc
Summary: Mbedthis Appweb -- Documentation and Samples for Mbedthis Appweb
Group: Applications/Internet
Prefix: !!BLD_DOC_PREFIX!!

%description doc
Documentation and samples for Mbedthis Appweb, an embedded HTTP web server.

%files doc -f docFiles.txt
