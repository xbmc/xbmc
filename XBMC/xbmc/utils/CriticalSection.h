//////////////////////////////////////////////////////////////////////
//
// CriticalSection.h: interface for the CCriticalSection class.
//
//////////////////////////////////////////////////////////////////////
#ifndef _CRITICAL_SECTION_H_
#define _CRITICAL_SECTION_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#ifdef _LINUX
#include "PlatformDefs.h"
#include "linux/XSyncUtils.h"
#include "XCriticalSection.h"
#endif

class CCriticalSection
{
public:

	// Constructor/destructor.
	CCriticalSection();
	virtual ~CCriticalSection();
	
	XCriticalSection& getCriticalSection() { return m_criticalSection; }

private:
	XCriticalSection m_criticalSection;
};

// The CCritical section overloads.
void InitializeCriticalSection(CCriticalSection* section);
void DeleteCriticalSection(CCriticalSection* section);
BOOL OwningCriticalSection(CCriticalSection* section);
DWORD ExitCriticalSection(CCriticalSection* section);
void RestoreCriticalSection(CCriticalSection* section, DWORD count);
void EnterCriticalSection(CCriticalSection* section);
void LeaveCriticalSection(CCriticalSection* section);

// And a few special ones.
void EnterCriticalSection(CCriticalSection& section);
void LeaveCriticalSection(CCriticalSection& section);
BOOL OwningCriticalSection(CCriticalSection& section);
DWORD ExitCriticalSection(CCriticalSection& section);
void RestoreCriticalSection(CCriticalSection& section, DWORD count);

#endif
