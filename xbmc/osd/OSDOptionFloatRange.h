#pragma once
#include "iosdoption.h"
namespace OSD
{
	class COSDOptionFloatRange :
		public IOSDOption
	{
	public:
		COSDOptionFloatRange(void);
		COSDOptionFloatRange(float fStart, float fEnd, float fInterval, float fValue);
		COSDOptionFloatRange(const COSDOptionFloatRange& option);
		OSD::COSDOptionFloatRange operator = (const COSDOptionFloatRange& option);


		virtual ~COSDOptionFloatRange(void);
		virtual IOSDOption* Clone() const;
		virtual void Draw(int x, int y, bool bFocus=false);
		virtual bool OnAction(const CAction& action);
	private:
		float	m_fMin;
		float	m_fMax;
    float m_fInterval;
		float	m_fValue;
	};
};
