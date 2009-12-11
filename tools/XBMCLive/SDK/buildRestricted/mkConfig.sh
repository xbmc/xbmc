
makeConfig()
{
	CATEGORIES="main restricted universe multiverse"

	configString=""
	configString="$configString --distribution karmic"
	configString="$configString --mirror-bootstrap $UBUNTUMIRROR_BASEURL"
	configString="$configString --mirror-binary $UBUNTUMIRROR_BASEURL"
	configString="$configString --mirror-chroot $UBUNTUMIRROR_BASEURL"
	configString="$configString --mirror-chroot-security http://security.ubuntu.com/ubuntu/"
	configString="$configString --mirror-binary-security http://security.ubuntu.com/ubuntu/"
	configString="$configString --packages-lists packages.txt"


	if [ -n "$APT_HTTP_PROXY" ]; then
		configString="$configString --apt-http-proxy $APT_HTTP_PROXY"
	fi
	if [ -n "$APT_FTP_PROXY" ]; then
		configString="$configString --apt-ftp-proxy $APT_FTP_PROXY"
	fi

	lh config --mode ubuntu --archive-areas "$CATEGORIES" $configString

	# Copy files for chroot
	cp -R "$THISDIR"/Files/chroot_* config
}
