#pragma once
#include "iosdoption.h"
#include "IExecutor.h" 
namespace OSD
{
	class COSDOptionFloatRange :
		public IOSDOption
	{
	public:
		COSDOptionFloatRange(int iAction, int iHeading);
		COSDOptionFloatRange(int iAction, int iHeading,float fStart, float fEnd, float fInterval, float fValue);
		COSDOptionFloatRange(const COSDOptionFloatRange& option);
		const OSD::COSDOptionFloatRange& operator = (const COSDOptionFloatRange& option);


		virtual ~COSDOptionFloatRange(void);
		virtual IOSDOption* Clone() const;
		virtual void Draw(int x, int y, bool bFocus=false, bool bSelected=false);
		virtual bool OnAction(IExecutor& executor,const CAction& action);
	private:
    int m_iAction;
		float	m_fMin;
		float	m_fMax;
    float m_fInterval;
		float	m_fValue;
	};
};
