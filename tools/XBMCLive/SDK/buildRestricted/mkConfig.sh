
makeConfig()
{
	mkdir -p "$THISDIR/$WORKDIR"
	cd "$THISDIR/$WORKDIR"


	configString=""
	configString="$configString --distribution karmic"
	configString="$configString --mirror-bootstrap $UBUNTUMIRROR_BASEURL"
	configString="$configString --mirror-binary $UBUNTUMIRROR_BASEURL"
	configString="$configString --mirror-chroot $UBUNTUMIRROR_BASEURL"
	configString="$configString --mirror-chroot-security http://security.ubuntu.com/ubuntu/"
	configString="$configString --mirror-binary-security http://security.ubuntu.com/ubuntu/"
	configString="$configString --packages-lists packages.txt"

	configString="$configString --apt-http-proxy http://127.0.0.1:3142"
	configString="$configString --apt-ftp-proxy http://127.0.0.1:3142"

	configString="$configString --interactive shell"

	lh config --mode ubuntu --categories "main restricted universe multiverse" $configString

	# Copy files for chroot
	cp -R "$THISDIR"/Files/chroot_* config
}
