#!/bin/sh

# this runs the file serving tests that are expected to pass with samba3

if [ $# != 2 ]; then
cat <<EOF
Usage: test_smbclient_s3.sh SERVER SERVER_IP
EOF
exit 1;
fi

SERVER="$1"
SERVER_IP="$2"
SMBCLIENT="$VALGRIND ${SMBCLIENT:-$SRCDIR/bin/smbclient} $CONFIGURATION"

incdir=`dirname $0`
. $incdir/test_functions.sh

failed=0

# Test that a noninteractive smbclient does not prompt
test_noninteractive_no_prompt()
{
    prompt="smb"

    echo du | \
	$SMBCLIENT "$@" -U$USERNAME%$PASSWORD //$SERVER/tmp 2>&1 | \
    grep $prompt

    if [ $? = 0 ] ; then
	# got a prompt .. fail
	echo matched interactive prompt in non-interactive mode
	false
    else
	true
    fi
}

# Test that an interactive smbclient prompts to stdout
test_interactive_prompt_stdout()
{
    prompt="smb"
    tmpfile=/tmp/smbclient.in.$$

    cat > $tmpfile <<EOF
du
quit
EOF

    CLI_FORCE_INTERACTIVE=yes \
    $SMBCLIENT "$@" -U$USERNAME%$PASSWORD //$SERVER/tmp \
	< $tmpfile 2>/dev/null | \
    grep $prompt

    if [ $? = 0 ] ; then
	# got a prompt .. succeed
	rm -f $tmpfile
	true
    else
	echo failed to match interactive prompt on stdout
	rm -f $tmpfile
	false
    fi
}

testit "smbclient -L $SERVER_IP" $SMBCLIENT -L $SERVER_IP -N -p 139 || failed=`expr $failed + 1`
testit "smbclient -L $SERVER" $SMBCLIENT -L $SERVER -N -p 139 || failed=`expr $failed + 1`

testit "noninteractive smbclient does not prompt" \
    test_noninteractive_no_prompt || \
    failed=`expr $failed + 1`

testit "noninteractive smbclient -l does not prompt" \
   test_noninteractive_no_prompt -l /tmp || \
    failed=`expr $failed + 1`

testit "interactive smbclient prompts on stdout" \
   test_interactive_prompt_stdout || \
    failed=`expr $failed + 1`

testit "interactive smbclient -l prompts on stdout" \
   test_interactive_prompt_stdout -l /tmp || \
    failed=`expr $failed + 1`

testok $0 $failed
