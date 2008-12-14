#!/bin/bash
#
#	remove: Removal script
#
#	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#
#	Usage: remove [configFile]
#
################################################################################
#
#	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#	The latest version of this code is available at http://www.mbedthis.com
#
#	This software is open source; you can redistribute it and/or modify it 
#	under the terms of the GNU General Public License as published by the 
#	Free Software Foundation; either version 2 of the License, or (at your 
#	option) any later version.
#
#	This program is distributed WITHOUT ANY WARRANTY; without even the 
#	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
#	See the GNU General Public License for more details at:
#	http://www.mbedthis.com/downloads/gplLicense.html
#	
#	This General Public License does NOT permit incorporating this software 
#	into proprietary programs. If you are unable to comply with the GPL, a 
#	commercial license for this software and support services are available
#	from Mbedthis Software at http://www.mbedthis.com
#
################################################################################
#
#	NOTE: We require a saved setup file exist in /etc/appwebInstall.conf
#	This is created by install.
#

FMT=

BLD_PRODUCT="!!BLD_PRODUCT!!"
BLD_NAME="!!BLD_NAME!!"
BLD_VERSION="!!BLD_VERSION!!"
BLD_NUMBER="!!BLD_NUMBER!!"
BLD_HOST_OS="!!BLD_HOST_OS!!"
BLD_HOST_CPU="!!BLD_HOST_CPU!!"

BLD_PREFIX="!!BLD_PREFIX!!"				# Fixed and can't be relocated
BLD_DOC_PREFIX="!!BLD_DOC_PREFIX!!"
BLD_INC_PREFIX="!!BLD_INC_PREFIX!!"
BLD_LIB_PREFIX="!!BLD_LIB_PREFIX!!"
BLD_MAN_PREFIX="!!BLD_MAN_PREFIX!!"
BLD_SAM_PREFIX="!!BLD_SAM_PREFIX!!"
BLD_SBIN_PREFIX="!!BLD_SBIN_PREFIX!!"
BLD_SRC_PREFIX="!!BLD_SRC_PREFIX!!"
BLD_WEB_PREFIX="!!BLD_WEB_PREFIX!!"

removebin=Y
removedoc=Y
removedev=Y
removesrc=N

PATH=$PATH:/sbin:/usr/sbin

###############################################################################
# 
#	Get a yes/no answer from the user. Usage: ans=`yesno "prompt" "default"`
#

yesno() {
	if [ "$!!BLD_PRODUCT!!_HEADLESS" = 1 ] ; then
		echo "Y"
		return
	fi
	echo -n "$1 [$2] : " 1>&2
	while [ 1 ] 
	do
		read ans
		if [ "$ans" = "" ] ; then
			echo $2 ; break
		elif [ "$ans" = "Y" -o "$ans" = "y" ] ; then
			echo "Y" ; break
		elif [ "$ans" = "N" -o "$ans" = "n" ] ; then
			echo "N" ; break
		fi
		echo -e "\nMust enter a 'y' or 'n'\n " 1>&1
	done
}

###############################################################################
#
#	Modify service
#	Usage:	configureService start|stop|install
#

configureService() {
	local action=$1

	case $action in
	start|stop)
		if [ $BLD_HOST_OS = WIN ] ; then
			if [ -x "$BLD_SBIN_PREFIX/$BLD_PRODUCT" ] ; then
				"$BLD_SBIN_PREFIX/$BLD_PRODUCT" -s
			fi
		elif which service >/dev/null 2>&1 ; then
			/sbin/service $BLD_PRODUCT $action >/dev/null 2>&1
		elif which invoke-rc.d >/dev/null 2>&1 ; then
			if [ -f /etc/init.d/$BLD_PRODUCT ] ; then
				invoke-rc.d $BLD_PRODUCT $action
			fi
		fi
		;;

	install)
		if [ $BLD_HOST_OS = WIN ] ; then
			if [ -x "$BLD_SBIN_PREFIX/$BLD_PRODUCT" ] ; then
				"$BLD_SBIN_PREFIX/$BLD_PRODUCT" -i default
			fi
		elif which chkconfig >/dev/null 2>&1 ; then
			/sbin/chkconfig --add $BLD_PRODUCT >/dev/null
			/sbin/chkconfig --level 5 $BLD_PRODUCT on >/dev/null

		elif which update-rc.d >/dev/null 2>&1 ; then
			update-rc.d $BLD_PRODUCT defaults >/dev/null
		fi
		;;

	remove)
		if [ $BLD_HOST_OS = WIN ] ; then
			if [ -x "$BLD_SBIN_PREFIX/$BLD_PRODUCT" ] ; then
				"$BLD_SBIN_PREFIX/$BLD_PRODUCT" -u
			fi
		elif which chkconfig >/dev/null 2>&1 ; then
			/sbin/chkconfig --del $BLD_PRODUCT >/dev/null 2>&1
		elif which update-rc.d >/dev/null 2>&1 ; then
			update-rc.d -f $BLD_PRODUCT remove >/dev/null
		fi
		;;
	esac
}


###############################################################################

deconfigureService() {

	if [ "$removebin" = "Y" ] ; then
		echo -e "\nStopping $BLD_NAME service"
		configureService stop
		echo -e "\nRemoving $BLD_NAME service"
		configureService remove
	fi
	if [ -f /usr/sbin/$BLD_PRODUCT ] 
	then
		pid=`pidof /usr/sbin/$BLD_PRODUCT`
		[ "$pid" != "" ] && kill -9 $pid
	fi
} 

###############################################################################

removeFiles() {
	local pkg doins name

	for pkg in bin dev doc src ; do
		
		doins=`eval echo \\$install${pkg}`
		if [ "$doins" = Y ] ; then
			if [ "$pkg" = "bin" ] ; then
				suffix=""
			else 
				suffix="-${pkg}"
			fi
			name="${BLD_PRODUCT}${suffix}"
			if [ "$FMT" = "rpm" ] ; then
				echo -e "\nRunning \"rpm -e $name\""
				rpm -e $name
			elif [ "$FMT" = "deb" ] ; then
				echo -e "\nRunning \"dpkg -r $name\""
				dpkg -r $name >/dev/null
			else
				removeTarFiles $pkg
			fi
		fi
	done
}

###############################################################################

removeTarFiles() {
	local pkg prefix
	local cdir=`pwd`

	pkg=$1

	[ $pkg = bin ] && prefix=$BLD_PREFIX
	[ $pkg = dev ] && prefix=$BLD_SAM_PREFIX
	[ $pkg = doc ] && prefix=$BLD_DOC_PREFIX
	[ $pkg = src ] && prefix=$BLD_SRC_PREFIX

	if [ ! $prefix/fileList.txt ] ; then
		echo "Can't find required file list $prefix/fileList.txt"
		echo "Can't remove the package"
		exit 255
	fi

	cd /
	removeFileList $prefix/fileList.txt
	cd $cdir
}

###############################################################################

preClean() {
	local cdir=`pwd`

	rm -f /var/lock/subsys/$BLD_PRODUCT /var/lock/$BLD_PRODUCT
	rm -fr /var/log/$BLD_PRODUCT
	rm -rf /var/run/$BLD_PRODUCT

	if [ "$removebin" = "Y" ] ; then
		cd $BLD_PREFIX
		removeIntermediateFiles access.log error.log '*.log.old' .dummy \
			$BLD_PRODUCT.conf .appweb_pid.log '.httpClient_pid.log' make.log
		rm -f $BLD_LIB_PREFIX/modules/*.so.*
		cd $cdir
	fi

	if [ "$removedev" = "Y" ] ; then
		if [ -d $BLD_SAM_PREFIX ] ; then
			cd $BLD_SAM_PREFIX
			make clean >/dev/null 2>&1 || true
			removeIntermediateFiles '*.o' '*.lo' '*.so' '*.a' make.rules \
				.config.h.sav make.log .changes
			cd $cdir
		fi
	fi

	if [ "$removesrc" = "Y" ] ; then
		if [ -d $BLD_SRC_PREFIX ] ; then
			cd $BLD_SRC_PREFIX
			make clean >/dev/null 2>&1 || true
			removeIntermediateFiles '*.o' '*.lo' '*.so' '*.a' make.rules \
				.config.h.sav make.log .changes access.log error.log \
				'*.log.old' .dummy $BLD_PRODUCT.conf .appweb_pid.log \
				.httpClient_pid.log \
				make.log
			rm -f build/.buildConfig*
			cd $cdir
		fi
	fi
}

###############################################################################

postClean() {
	local cdir=`pwd`

	echo
	if [ "$removebin" = "Y" ] ; then
		cleanDir $BLD_PREFIX

		if [ -d $BLD_MAN_PREFIX ] ; then
			cd $BLD_MAN_PREFIX
			rm -f appweb.1.gz httpComp.1.gz httpPassword.1.gz httpClient.1.gz
			cd $cdir
		fi

		cleanDir $BLD_WEB_PREFIX
		cleanDir $BLD_LIB_PREFIX
	fi

	if [ "$removedev" = "Y" ] ; then
		cleanDir $BLD_SAM_PREFIX
		cleanDir $BLD_INC_PREFIX
	fi

	if [ "$removedoc" = "Y" ] ; then
		cleanDir $BLD_DOC_PREFIX
	fi

	if [ "$removesrc" = "Y" ] ; then
		cleanDir $BLD_SRC_PREFIX
	fi

	if [ -x /usr/share/$BLD_PRODUCT ] ; then
		cleanDir /usr/share/$BLD_PRODUCT
	fi

	if [ -d /var/$BLD_PRODUCT ]
	then
		cleanDir /var/$BLD_PRODUCT
	fi

	rm -f /etc/${BLD_PRODUCT}Install.conf
}

###############################################################################
#
#	Clean a directory. Usage: removeFileList fileList
#

removeFileList() {

	if [ ! -f "$1" ]
	then
		echo "Can't find file list: $1, continuing ..."
		return
	fi
	echo -e "\nRemoving files in file list \"$1\" ..."
	cat "$1" | while read f
	do
		rm -f "$f"
	done
}

###############################################################################
#
#	Cleanup empty directories. Usage: cleanDir directory
#

cleanDir() {
	local dir
	local cdir=`pwd`

	dir="$1"

	if [ "$dir" = "" ] ; then
		echo "WARNING: clean directory is empty"
	fi

	[ ! -d $dir ] && return

	cd $dir
	echo "Cleaning `pwd` ..."
	if [ `pwd` = "/" ]
	then
		echo "Configuration error: clean directory was '/'"
		cd $cdir
		return
	fi
	find . -type d -print | sort -r | grep -v '^\.$' | while read d
	do
		count=`ls "$d" | wc -l | sed -e 's/ *//'`
		[ "$count" = "0" ] && rmdir "$d"
		if [ "$count" != "0" ] ; then
			f=`echo "$d" | sed -e 's/\.\///'`
			echo "Directory `pwd`/${f}, still has user data"
		fi
	done 

	cd $cdir
	count=`ls "$dir" | wc -l | sed -e 's/ *//'`
	[ "$count" = "0" ] && rmdir "$dir"
	if [ "$count" != "0" ] ; then
		echo "Directory ${dir}, still has user data"
	fi
	rmdir "$dir" 2>/dev/null || true
}

###############################################################################
#
#	Cleanup intermediate files
#
removeIntermediateFiles() {
	local cdir=`pwd`

	find `pwd` -type d -print | while read d
	do
		cd "${d}"
		eval rm -f $*
		cd "${cdir}"
	done
}

###############################################################################

setup() {

	if [ `id -u` != "0" ]
	then
		echo "You must be root to remove this product."
		exit 255
	fi
	
	#
	#	Headless removal. Expect an argument that supplies a config file.
	#
	if [ $# -ge 1 ]
	then
		if [ ! -f $1 ]
		then
			echo "Could not find config file \"$1\""
			exit 255
		else
			. $1 
			removeFiles $FMT
		fi
		exit 0
	fi
	
	#
	#	Get defaults from the installation configuration file
	#
	if [ ! -f /etc/${BLD_PRODUCT}Install.conf ]
	then
		echo "remove: Can't locate the /etc/${BLD_PRODUCT}Install.conf setup file"
		echo 
		echo "If you installed using the bare RPM images, use:"
		echo "    rpm -e $BLD_PRODUCT"
		echo 
		echo "If you installed using the bare Debian packages, use:"
		echo "    dpkg -r $BLD_PRODUCT"
		echo 
		echo "If you installed using make install, use:"
		echo "    make uninstall-all"
		echo 
		echo "If all else fails, try re-installing to repair the missing file"
		echo "and then run the removal script again".
		exit 255
	else
		.  /etc/${BLD_PRODUCT}Install.conf
	fi
	
	binDir=${binDir:-$BLD_PREFIX}
	devDir=${devDir:-$BLD_SAM_PREFIX}
	docDir=${docDir:-$BLD_DOC_PREFIX}
	srcDir=${srcDir:-$BLD_SRC_PREFIX}

	echo -e "\n$BLD_NAME !!BLD_VERSION!!-!!BLD_NUMBER!! Removal\n"
}


###############################################################################

askUser() {
	local finished

	echo "Enter requested information or press <ENTER> to accept the defaults. "

	#
	#	Confirm the configuration
	#
	finished=N
	while [ "$finished" = "N" ]
	do
		echo
		if [ -d $binDir ] ; then
			removebin=`yesno "Remove binary package" "$removebin"`
		else
			removebin=N
		fi
		if [ -d $devDir ] ; then
			removedev=`yesno "Remove development headers and samples package" \
				"$removedev"`
		else
			removedev=N
		fi
		if [ -d $docDir ] ; then
			removedoc=`yesno "Remove documentation package" "$removedoc"`
		else
			removedoc=N
		fi
#		if [ -d $srcDir ] ; then
#			removesrc=`yesno "Remove source code package" "$removesrc"`
#		else
#			removesrc=N
#		fi

		echo -e "\nProceed removing with these instructions:" 
		[ $removebin = Y ] && echo -e "  Remove binary package: $removebin"
		[ $removedev = Y ] && echo -e "  Remove development package: $removedev"
		[ $removedoc = Y ] && echo -e "  Remove documentation package: $removedoc"
#		[ $removesrc = Y ] && echo -e "  Remove source package: $removesrc"

		echo
		finished=`yesno "Accept these instructions" "Y"`

		if [ "$finished" != "Y" ] ; then
			exit 0
		fi
	done
}

################################################################################
#
#	Main program
#

setup $*
askUser
deconfigureService
preClean
removeFiles $FMT
postClean

echo -e "\n$BLD_NAME removal successful.\n"

##
##  Local variables:
##  tab-width: 4
##  c-basic-offset: 4
##  End:
##  vim: sw=4 ts=4 
##
