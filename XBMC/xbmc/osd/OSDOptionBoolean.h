#pragma once
#include "iosdoption.h"
namespace OSD
{
	class COSDOptionBoolean :
		public IOSDOption
	{
	public:
		COSDOptionBoolean(void);
		COSDOptionBoolean(bool bValue);
		COSDOptionBoolean(const COSDOptionBoolean& option);
		const OSD::COSDOptionBoolean& operator = (const COSDOptionBoolean& option);


		virtual ~COSDOptionBoolean(void);
		virtual IOSDOption* Clone() const;
		virtual void Draw(int x, int y, bool bFocus=false);
		virtual bool OnAction(const CAction& action);
	private:
		bool	m_bValue;
	};
};
