#pragma once
#include "IOSDOption.h"
#include "IExecutor.h"
#include "GUISliderControl.h"
namespace OSD
{
class COSDOptionIntRange :
      public IOSDOption
{
public:
  COSDOptionIntRange(int iAction, int iHeading, bool bPercent);
  COSDOptionIntRange(int iAction, int iHeading, bool bPercent, int iStart, int iEnd, int iInterval, int iValue);
  COSDOptionIntRange(const COSDOptionIntRange& option);
  const OSD::COSDOptionIntRange& operator = (const COSDOptionIntRange& option);


  virtual ~COSDOptionIntRange(void);
  virtual IOSDOption* Clone() const;
  virtual void Draw(int x, int y, bool bFocus = false, bool bSelected = false);
  virtual bool OnAction(IExecutor& executor, const CAction& action);

  int GetValue() const;
  virtual int GetMessage() const { return m_iAction;};
  virtual void SetValue(int iValue){m_iValue = iValue;};
  virtual void SetLabel(const CStdString& strLabel){};
private:
  CGUISliderControl m_slider;
  bool m_bPercent;
  int m_iAction;
  int m_iMin;
  int m_iMax;
  int m_iInterval;
  int m_iValue;
};
};
