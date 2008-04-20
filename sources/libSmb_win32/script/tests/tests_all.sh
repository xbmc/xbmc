
$SCRIPTDIR/test_smbtorture_s3.sh //$SERVER_IP/tmp $USERNAME $PASSWORD "" || failed=`expr $failed + $?`
$SCRIPTDIR/test_smbclient_s3.sh $SERVER $SERVER_IP || failed=`expr $failed + $?`

SMBTORTURE4VERSION=`$SMBTORTURE4 --version`
if [ -n "$SMBTORTURE4" -a -n "$SMBTORTURE4VERSION" ];then
	echo "Running Tests with Samba4's smbtorture"
	echo $SMBTORTURE4VERSION
	$SCRIPTDIR/test_posix_s3.sh //$SERVER_IP/tmp $USERNAME $PASSWORD "" || failed=`expr $failed + $?`
else
	echo "Skip Tests with Samba4's smbtorture"
fi
