#pragma once
#include "iosdoption.h"
#include "IExecutor.h" 
#include "GUICheckMarkControl.h"
namespace OSD
{
	class COSDOptionBoolean :
		public IOSDOption
	{
	public:
		COSDOptionBoolean(int iAction,int iHeading);
		COSDOptionBoolean(int iAction,int iHeading,bool bValue);
		COSDOptionBoolean(const COSDOptionBoolean& option);
		const OSD::COSDOptionBoolean& operator = (const COSDOptionBoolean& option);


		virtual ~COSDOptionBoolean(void);
		virtual IOSDOption* Clone() const;
		virtual void Draw(int x, int y, bool bFocus=false, bool bSelected=false);
		virtual bool OnAction(IExecutor& executor, const CAction& action);
    bool         GetValue() const;
    virtual int  GetMessage() const {return m_iAction;};
    virtual void SetValue(int iValue){};
    virtual void SetLabel(const CStdString& strLabel){};
	private:
    int                    m_iAction;
		bool	                 m_bValue;
    CGUICheckMarkControl   m_image;
	};
};
