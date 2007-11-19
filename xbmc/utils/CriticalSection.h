// CriticalSection.h: interface for the CCriticalSection class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CRITICALSECTION_H__FC7E6698_33C9_42D6_81EA_F7869FEE11B7__INCLUDED_)
#define AFX_CRITICALSECTION_H__FC7E6698_33C9_42D6_81EA_F7869FEE11B7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _LINUX
#include "linux/XSyncUtils.h"
#endif

class CCriticalSection
{
public:
  CCriticalSection();

  // Conversion operator
  operator LPCRITICAL_SECTION();
  virtual ~CCriticalSection();
private:

  CRITICAL_SECTION m_critSection;
};

BOOL  NTAPI OwningCriticalSection(LPCRITICAL_SECTION section);               /* checks if current thread owns the critical section */
DWORD NTAPI ExitCriticalSection(LPCRITICAL_SECTION section);                 /* leaves critical section fully, and returns count */ 
VOID  NTAPI RestoreCriticalSection(LPCRITICAL_SECTION section, DWORD count); /* restores critical section count */

#endif // !defined(AFX_CRITICALSECTION_H__FC7E6698_33C9_42D6_81EA_F7869FEE11B7__INCLUDED_)

