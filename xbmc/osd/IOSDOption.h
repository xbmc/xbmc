#pragma once
#include "key.h"
namespace OSD
{
	class IOSDOption
	{
	public:
		IOSDOption();
		virtual ~IOSDOption(void);
		virtual IOSDOption* Clone()const=0 ;
		virtual void Draw(int x, int y, bool bFocus=false)=0;
		virtual bool OnAction(const CAction& action)=0;
  protected:
    int m_iHeading;
	};
};