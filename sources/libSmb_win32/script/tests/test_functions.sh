
samba3_stop_sig_term() {
	RET=0
	kill -USR1 `cat $PIDDIR/timelimit.nmbd.pid` >/dev/null 2>&1 || \
		kill -ALRM `cat $PIDDIR/timelimit.nmbd.pid` || RET=$?

	kill -USR1 `cat $PIDDIR/timelimit.smbd.pid` >/dev/null 2>&1 || \
		kill -ALRM `cat $PIDDIR/timelimit.smbd.pid` || RET=$?

	return $RET;
}

samba3_stop_sig_kill() {
	kill -ALRM `cat $PIDDIR/timelimit.nmbd.pid` >/dev/null 2>&1
	kill -ALRM `cat $PIDDIR/timelimit.smbd.pid` >/dev/null 2>&1
	return 0;
}

samba3_check_or_start() {
	if [ -n "$SERVER_TEST_FIFO" ];then

		trap samba3_stop_sig_kill INT QUIT
		trap samba3_stop_sig_kill TERM

		if [ -p "$SERVER_TEST_FIFO" ];then
			return 0;
		fi

		if [ -n "$SOCKET_WRAPPER_DIR" ];then
			if [ -d "$SOCKET_WRAPPER_DIR" ]; then
				rm -f $SOCKET_WRAPPER_DIR/*
			else
				mkdir -p $SOCKET_WRAPPER_DIR
			fi
		fi

		rm -f $SERVER_TEST_FIFO
		mkfifo $SERVER_TEST_FIFO

		rm -f $NMBD_TEST_LOG
		echo -n "STARTING NMBD..."
		((
			if test x"$NMBD_MAXTIME" = x; then
			    NMBD_MAXTIME=2700
			fi
			timelimit $NMBD_MAXTIME $NMBD_VALGRIND $SRCDIR/bin/nmbd -F -S --no-process-group -d0 -s $SERVERCONFFILE > $NMBD_TEST_LOG 2>&1 &
			TIMELIMIT_NMBD_PID=$!
			echo $TIMELIMIT_NMBD_PID > $PIDDIR/timelimit.nmbd.pid
			wait $TIMELIMIT_NMBD_PID
			ret=$?;
			rm -f $SERVER_TEST_FIFO
			if [ -n "$SOCKET_WRAPPER_DIR" -a -d "$SOCKET_WRAPPER_DIR" ]; then
				rm -f $SOCKET_WRAPPER_DIR/*
			fi
			if [ x"$ret" = x"0" ];then
				echo "nmbd exits with status $ret";
				echo "nmbd exits with status $ret" >>$NMBD_TEST_LOG;
			elif [ x"$ret" = x"137" ];then
				echo "nmbd got SIGXCPU and exits with status $ret!"
				echo "nmbd got SIGXCPU and exits with status $ret!">>$NMBD_TEST_LOG;
			else
				echo "nmbd failed with status $ret!"
				echo "nmbd failed with status $ret!">>$NMBD_TEST_LOG;
			fi
			exit $ret;
		) || exit $? &) 2>/dev/null || exit $?
		echo  "DONE"

		rm -f $SMBD_TEST_LOG
		echo -n "STARTING SMBD..."
		((
			if test x"$SMBD_MAXTIME" = x; then
			    SMBD_MAXTIME=2700
			fi
			timelimit $SMBD_MAXTIME $SMBD_VALGRIND $SRCDIR/bin/smbd -F -S --no-process-group -d0 -s $SERVERCONFFILE > $SMBD_TEST_LOG 2>&1 &
			TIMELIMIT_SMBD_PID=$!
			echo $TIMELIMIT_SMBD_PID > $PIDDIR/timelimit.smbd.pid
			wait $TIMELIMIT_SMBD_PID
			ret=$?;
			rm -f $SERVER_TEST_FIFO
			if [ -n "$SOCKET_WRAPPER_DIR" -a -d "$SOCKET_WRAPPER_DIR" ]; then
				rm -f $SOCKET_WRAPPER_DIR/*
			fi
			if [ x"$ret" = x"0" ];then
				echo "smbd exits with status $ret";
				echo "smbd exits with status $ret" >>$SMBD_TEST_LOG;
			elif [ x"$ret" = x"137" ];then
				echo "smbd got SIGXCPU and exits with status $ret!"
				echo "smbd got SIGXCPU and exits with status $ret!">>$SMBD_TEST_LOG;
			else
				echo "smbd failed with status $ret!"
				echo "smbd failed with status $ret!">>$SMBD_TEST_LOG;
			fi
			exit $ret;
		) || exit $? &) 2>/dev/null || exit $?
		echo  "DONE"
	fi
	return 0;
}

samba3_nmbd_test_log() {
	if [ -n "$NMBD_TEST_LOG" ];then
		if [ -r "$NMBD_TEST_LOG" ];then
			return 0;
		fi
	fi
	return 1;
}

samba3_smbd_test_log() {
	if [ -n "$SMBD_TEST_LOG" ];then
		if [ -r "$SMBD_TEST_LOG" ];then
			return 0;
		fi
	fi
	return 1;
}

samba3_check_only() {
	if [ -n "$SERVER_TEST_FIFO" ];then
		if [ -p "$SERVER_TEST_FIFO" ];then
			return 0;
		fi
		return 1;
	fi
	return 0;
}

testit() {
	if [ -z "$PREFIX" ]; then
	    PREFIX=test_prefix
	    mkdir -p $PREFIX
	fi
	name=$1
	shift 1
	SERVERS_ARE_UP="no"
	TEST_LOG="$PREFIX/test_log.$$"
	trap "rm -f $TEST_LOG" EXIT
	cmdline="$*"

	if [ x"$RUN_FROM_BUILD_FARM" = x"yes" ];then
		echo "--==--==--==--==--==--==--==--==--==--==--"
		echo "Running test $name (level 0 stdout)"
		echo "--==--==--==--==--==--==--==--==--==--==--"
		date
		echo "Testing $name"
	else
		echo "Testing $name ($failed)"
	fi

	samba3_check_only && SERVERS_ARE_UP="yes"
	if [ x"$SERVERS_ARE_UP" != x"yes" ];then
		if [ x"$RUN_FROM_BUILD_FARM" = x"yes" ];then
			echo "SERVERS are down! Skipping: $cmdline"
			echo "=========================================="
			echo "TEST SKIPPED: $name (reason SERVERS are down)"
			echo "=========================================="
   		else
			echo "TEST SKIPPED: $name (reason SERVERS are down)"
		fi
		return 1
	fi
	
	( $cmdline > $TEST_LOG 2>&1 )
	status=$?
	if [ x"$status" != x"0" ]; then
		echo "TEST OUTPUT:"
		cat $TEST_LOG;
		samba3_nmbd_test_log && echo "NMBD OUTPUT:";
		samba3_nmbd_test_log && cat $NMBD_TEST_LOG;
		samba3_smbd_test_log && echo "SMBD OUTPUT:";
		samba3_smbd_test_log && cat $SMBD_TEST_LOG;
		rm -f $TEST_LOG;
		if [ x"$RUN_FROM_BUILD_FARM" = x"yes" ];then
			echo "=========================================="
			echo "TEST FAILED: $name (status $status)"
			echo "=========================================="
   		else
			echo "TEST FAILED: $cmdline (status $status)"
		fi
		return 1;
	fi
	rm -f $TEST_LOG;
	if [ x"$RUN_FROM_BUILD_FARM" = x"yes" ];then
		echo "ALL OK: $cmdline"
		echo "=========================================="
		echo "TEST PASSED: $name"
		echo "=========================================="
	fi
	return 0;
}

testok() {
	name=`basename $1`
	failed=$2

	if [ x"$failed" = x"0" ];then
		:
	else
		echo "$failed TESTS FAILED or SKIPPED ($name)";
	fi
	exit $failed
}

teststatus() {
	name=`basename $1`
	failed=$2

	if [ x"$failed" = x"0" ];then
		echo "TEST STATUS: $failed";
	else
		echo "TEST STATUS: $failed";
	fi
	exit $failed
}

if [ -z "$VALGRIND" ]; then
    MALLOC_CHECK_=2
    export MALLOC_CHECK_
fi

