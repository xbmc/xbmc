# See: http://gcc.gnu.org/ml/gcc/2000-05/msg01141.html
AC_DEFUN([CHECK_PTHREAD],
[
        # first try without -pthread
        AC_TRY_LINK(
		[
			#include <pthread.h>
		],[
			pthread_create(0,0,0,0);
		],[
			AC_MSG_RESULT(yes)
			PTHREAD_CPPFLAGS=
			PTHREAD_LDFLAGS=
			PTHREAD_LIBS=
		],[
        # now with -pthread        
	AC_CHECK_LIB(pthread,pthread_create,
	[
		PTHREAD_CPPFLAGS=
		PTHREAD_LDFLAGS=
		PTHREAD_LIBS=-lpthread
	],[
		AC_MSG_CHECKING(if compiler supports -pthread)
		save_CPPFLAGS="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS -pthread"
		AC_TRY_LINK(
		[
			#include <pthread.h>
		],[
			pthread_create(0,0,0,0);
		],[
			AC_MSG_RESULT(yes)
			PTHREAD_CPPFLAGS=-pthread
			PTHREAD_LDFLAGS=-pthread
			PTHREAD_LIBS=
		],[
			AC_MSG_RESULT(no)
			AC_MSG_CHECKING(if compiler supports -pthreads)
			save_CPPFLAGS="$CPPFLAGS"
			CPPFLAGS="$save_CPPFLAGS -pthreads"
			AC_TRY_LINK(
			[
				#include <pthread.h>
			],[
				pthread_create(0,0,0,0);
			],[
				AC_MSG_RESULT(yes)
				PTHREAD_CPPFLAGS=-pthreads
				PTHREAD_LDFLAGS=-pthreads
				PTHREAD_LIBS=
			],[
				AC_MSG_RESULT(no)
				AC_MSG_CHECKING(if compiler supports -threads)
				save_CPPFLAGS="$CPPFLAGS"
				CPPFLAGS="$save_CPPFLAGS -threads"
				AC_TRY_LINK(
				[
					#include <pthread.h>
				],[
					pthread_create(0,0,0,0);
				],[
					AC_MSG_RESULT(yes)
					PTHREAD_CPPFLAGS=-threads
					PTHREAD_LDFLAGS=-threads
					PTHREAD_LIBS=
				],[
					AC_MSG_ERROR([Your system is not supporting pthreads!])
				])
			])
		])
		CPPFLAGS="$save_CPPFLAGS"
	])
         
        ])
])
