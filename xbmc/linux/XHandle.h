#ifndef __X_HANDLE__
#define __X_HANDLE__

#ifndef _WIN32

#include "../../guilib/StdString.h"
#include <SDL/SDL_mutex.h>
#include <SDL/SDL_thread.h>

#include "PlatformDefs.h"
#include "StringUtils.h"

struct CXHandle {

public:
	typedef enum { HND_NULL = 0, HND_FILE, HND_EVENT, HND_MUTEX, HND_THREAD, HND_FIND_FILE } HandleType;
	
	HandleType  m_type;
	SDL_sem		*m_hSem;
	SDL_Thread  *m_hThread;

	// simulate mutex and critical section
	SDL_mutex	*m_hMutex;
	int			RecursionCount;  // for mutex - for compatibility with WIN32 critical section

   	DWORD		OwningThread;

	int			fd;
	bool		m_bManualEvent;
	
	time_t		m_tmCreation;

	CStdStringArray	m_FindFileResults;
	int 		m_nFindFileIterator;	
        CStdString      m_FindFileDir;
        off64_t         m_iOffset;
        bool            m_bCDROM;
	CXHandle() :	fd(0), 
					m_type(HND_NULL), 
					m_hSem(NULL), 
					m_hMutex(NULL), 
					m_hThread(NULL), 
					RecursionCount(0),
					OwningThread(0),
					m_bManualEvent(FALSE),
                                        m_iOffset(0),
                                        m_bCDROM(false),
					m_nFindFileIterator(0) { 
		m_tmCreation = time(NULL);
	};
	
	CXHandle(HandleType nType) :	fd(0), 
									m_type(nType), 
									m_hSem(NULL), 
									m_hMutex(NULL), 
									m_hThread(NULL), 
									RecursionCount(0),
									OwningThread(0),
									m_bManualEvent(FALSE),
									m_nFindFileIterator(0) { 
		m_tmCreation = time(NULL);
	};
	
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
