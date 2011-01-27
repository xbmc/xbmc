#ifndef __COMPAT_H__
#define __COMPAT_H__

#if WIN32
#include <direct.h>
#include <windows.h>
#include <process.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#endif

//
// This file handles all portablity issues with streamripper
//

// File Routines
////////////////////////////////////////// 

#if WIN32
#define FHANDLE	HANDLE
#define OpenFile(_filename_)	CreateFile(_filename_, GENERIC_READ,  	\
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 	\
		FILE_ATTRIBUTE_NORMAL, NULL)
#define CloseFile(_fhandle_) 	CloseHandle(_fhandle_)
//#define MoveFile(_oldfile_, _newfile_)     MoveFile(_oldfile_, _newfile_)
//#define CloseFile(_file_)   	CloseHandle(file)
#define INVALID_FHANDLE 	INVALID_HANDLE_VALUE
#elif __UNIX__

#define FHANDLE	int
#define OpenFile(_filename_)	open(_filename_, O_RDWR | O_CREAT, S_IRWXU | S_IRGRP | S_IROTH)
#define CloseFile(_fhandle_) 	close(_fhandle_)
#define MoveFile(_oldfile_, _newfile_)     rename(_oldfile_, _newfile_)
#define DeleteFile(_file_)  	(!unlink(_file_))
#define INVALID_FHANDLE 	-1

#endif 

// Thread Routines
////////////////////////////////////////// 
//#if WIN32
#if WIN32
#define THANDLE	HANDLE
#define BeginThread(_thandle_, callback) \
               {_thandle_ = (THANDLE)_beginthread((void *)callback, 0, (void *)NULL);}
#define WaitForThread(_thandle_)	WaitForSingleObject(_thandle_, INFINITE);
#define DestroyThread(_thandle_)	CloseHandle(_thandle_)

#define HSEM			HANDLE
#define	SemInit(_s_)	{_s_ = CreateEvent(NULL, TRUE, FALSE, NULL);}
#define	SemWait(_s_)	{WaitForSingleObject(_s_, INFINITE); ResetEvent(_s_);}
#define	SemPost(_s_)	SetEvent(_s_)
#define	SemDestroy(_s_)	CloseHandle(_s_)


#elif __UNIX__

#define THANDLE		pthread_t
#define BeginThread(_thandle_, callback) pthread_create(&_thandle_, NULL, \
                          (void *)callback, (void *)NULL)
#define WaitForThread(_thandle_)	pthread_join(_thandle_, NULL)
#define DestroyThread(_thandle_)	// is there one for unix?
#define HSEM		sem_t
#define	SemInit(_s_)	sem_init(&(_s_), 0, 0)
#define	SemWait(_s_)	sem_wait(&(_s_))
#define	SemPost(_s_)	sem_post(&(_s_))
#define	SemDestroy(_s_)	sem_destroy(&(_s_))
#define Sleep(x) 	usleep(x)	
#define SOCKET socket_t

#endif

// Socket Routines
////////////////////////////////////////// 

#if WIN32
//#define EAGAIN          WSAEWOULDBLOCK
#define EWOULDBLOCK     WSAEWOULDBLOCK
#elif __UNIX__
#define closesocket     close
#define SOCKET_ERROR	-1
#define WSACleanup()
#endif

#endif // __COMPAT_H__
