#pragma once
#include "iosdoption.h"
#include "IExecutor.h" 

namespace OSD
{
	class COSDOptionButton :
		public IOSDOption
	{
	public:
		COSDOptionButton(int iAction,int iHeading);
		COSDOptionButton(const COSDOptionButton& option);
		const OSD::COSDOptionButton& operator = (const COSDOptionButton& option);

		virtual ~COSDOptionButton(void);
		virtual IOSDOption* Clone() const;
		virtual void Draw(int x, int y, bool bFocus=false, bool bSelected=false);
		virtual bool OnAction(IExecutor& executor, const CAction& action);
    virtual int  GetMessage() const {return m_iAction;};
    virtual void SetLabel(const CStdString& strValue);
    virtual void SetValue(int iValue){};
	private:
    int          m_iAction;
		CStdString   m_strValue;
	};
};
