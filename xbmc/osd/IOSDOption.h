#pragma once

namespace OSD
{
  class IExecutor;
	class IOSDOption
	{
	public:
		IOSDOption();
		virtual ~IOSDOption(void);
		virtual IOSDOption* Clone()const=0 ;
		virtual void Draw(int x, int y, bool bFocus=false, bool bSelected=false)=0;
		virtual bool OnAction(IExecutor& executor, const CAction& action)=0;
    virtual int  GetMessage() const=0;
    virtual void SetValue(int iValue)=0;
    virtual void SetLabel(const CStdString& strLabel)=0;
  protected:
    int m_iHeading;
	};
};