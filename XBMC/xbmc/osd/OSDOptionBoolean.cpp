
#include ".\osdoptionboolean.h"
using namespace OSD;
COSDOptionBoolean::COSDOptionBoolean(void)
{
	m_bValue=false;
}

COSDOptionBoolean::COSDOptionBoolean(bool bValue)
{
	m_bValue=bValue;
}

COSDOptionBoolean::COSDOptionBoolean(const COSDOptionBoolean& option)
{
	*this=option;
}

const OSD::COSDOptionBoolean& COSDOptionBoolean::operator = (const COSDOptionBoolean& option)
{
	if (this==&option) return *this;
	m_bValue=option.m_bValue;
	return *this;
}

COSDOptionBoolean::~COSDOptionBoolean(void)
{
}

IOSDOption* COSDOptionBoolean::Clone() const
{
	return new COSDOptionBoolean(*this);
}


void COSDOptionBoolean::Draw(int x, int y, bool bFocus)
{
}

bool COSDOptionBoolean::OnAction(const CAction& action)
{
	if (action.wID==ACTION_SELECT_ITEM)
	{
		m_bValue=!m_bValue;
		return true;
	}
  return false;
}