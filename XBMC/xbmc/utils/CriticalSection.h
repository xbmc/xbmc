// CriticalSection.h: interface for the CCriticalSection class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CRITICALSECTION_H__FC7E6698_33C9_42D6_81EA_F7869FEE11B7__INCLUDED_)
#define AFX_CRITICALSECTION_H__FC7E6698_33C9_42D6_81EA_F7869FEE11B7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CCriticalSection  
{
public:
	CCriticalSection();
  CCriticalSection(const CCriticalSection& src);
  CCriticalSection& operator=(const CCriticalSection& src);
	
  // Conversion operator
  operator LPCRITICAL_SECTION();
	virtual ~CCriticalSection();
private:
	
	CRITICAL_SECTION			m_critSection;
};

#endif // !defined(AFX_CRITICALSECTION_H__FC7E6698_33C9_42D6_81EA_F7869FEE11B7__INCLUDED_)
