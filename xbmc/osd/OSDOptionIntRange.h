#pragma once
#include "iosdoption.h"
#include "IExecutor.h" 
namespace OSD
{
	class COSDOptionIntRange :
		public IOSDOption
	{
	public:
		COSDOptionIntRange(int iAction,int iHeading);
		COSDOptionIntRange(int iAction,int iHeading,int iStart, int iEnd, int iInterval, int iValue);
		COSDOptionIntRange(const COSDOptionIntRange& option);
		const OSD::COSDOptionIntRange& operator = (const COSDOptionIntRange& option);


		virtual ~COSDOptionIntRange(void);
		virtual IOSDOption* Clone() const;
		virtual void Draw(int x, int y, bool bFocus=false, bool bSelected=false);
		virtual bool OnAction(IExecutor& executor,const CAction& action);
	private:
    int m_iAction;
		int	m_iMin;
		int	m_iMax;
    int m_iInterval;
		int	m_iValue;
	};
};
