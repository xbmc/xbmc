#ifndef __X_HANDLE__
#define __X_HANDLE__

#ifndef _WIN32

#include "PlatformDefs.h"

struct CXHandle {

public:
	typedef enum { HND_NULL = 0, HND_FILE, HND_EVENT, HND_MUTEX, HND_THREAD } HandleType;
	
	HandleType  m_type;
	SDL_sem		*m_hSem;
	SDL_Thread  *m_hThread;

	// simulate mutex and critical section
	SDL_mutex	*m_hMutex;
	int			RecursionCount;  // for mutex - for compatibility with WIN32 critical section
    DWORD		OwningThread;

	int		fd;
	bool	m_bManualEvent;

	CXHandle() :	fd(0), 
					m_type(HND_NULL), 
					m_hSem(NULL), 
					m_hMutex(NULL), 
					m_hThread(NULL), 
					RecursionCount(0),
					OwningThread(0),
					m_bManualEvent(FALSE) { };
	
	CXHandle(HandleType nType) :	fd(0), 
									m_type(nType), 
									m_hSem(NULL), 
									m_hMutex(NULL), 
									m_hThread(NULL), 
									RecursionCount(0),
									OwningThread(0),
									m_bManualEvent(FALSE) { };
	
	virtual ~CXHandle() {

		//TODO: issue error to log if recursion count is bigger than 0
		if (RecursionCount > 0) {
			//ERROR
		}

		if (m_hSem) {
			SDL_DestroySemaphore(m_hSem);
		}

		if (m_hMutex) {
			SDL_DestroyMutex(m_hMutex);
		}

		if ( fd != 0 ) {
			close(fd);
		}

	}

};

#define HANDLE CXHandle*

bool CloseHandle(HANDLE hObject);

#endif

#endif
