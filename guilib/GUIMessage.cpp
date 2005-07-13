#include "include.h"
#include "GUIMessage.h"
#include "LocalizeStrings.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CGUIMessage::CGUIMessage(DWORD dwMsg, DWORD dwSenderID, DWORD dwControlID, DWORD dwParam1, DWORD dwParam2, void* lpVoid)
{
  m_dwMessage = dwMsg;
  m_dwSenderID = dwSenderID;
  m_dwControlID = dwControlID;
  m_dwParam1 = dwParam1;
  m_dwParam2 = dwParam2;
  m_lpVoid = lpVoid;
  m_strLabel = L"";
  m_strParam = "";
}

CGUIMessage::CGUIMessage(const CGUIMessage& msg)
{
  *this = msg;
}

CGUIMessage::~CGUIMessage(void)
{}


DWORD CGUIMessage::GetControlId() const
{
  return m_dwControlID;
}

DWORD CGUIMessage::GetMessage() const
{
  return m_dwMessage;
}

void* CGUIMessage::GetLPVOID() const
{
  return m_lpVoid;
}

DWORD CGUIMessage::GetParam1() const
{
  return m_dwParam1;
}

DWORD CGUIMessage::GetParam2() const
{
  return m_dwParam2;
}

DWORD CGUIMessage::GetSenderId() const
{
  return m_dwSenderID;
}


const CGUIMessage& CGUIMessage::operator = (const CGUIMessage& msg)
{
  if (this == &msg) return * this;

  m_dwMessage = msg.m_dwMessage;
  m_dwControlID = msg.m_dwControlID;
  m_dwParam1 = msg.m_dwParam1;
  m_dwParam2 = msg.m_dwParam2;
  m_lpVoid = msg.m_lpVoid;
  m_strLabel = msg.m_strLabel;
  m_dwSenderID = msg.m_dwSenderID;
  m_strParam = msg.m_strParam;
  return *this;
}


void CGUIMessage::SetParam1(DWORD dwParam1)
{
  m_dwParam1 = dwParam1;
}

void CGUIMessage::SetParam2(DWORD dwParam2)
{
  m_dwParam2 = dwParam2;
}

void CGUIMessage::SetLPVOID(void* lpVoid)
{
  m_lpVoid = lpVoid;
}

void CGUIMessage::SetLabel(const wstring& wstrLabel)
{
  m_strLabel = wstrLabel;
}
const wstring& CGUIMessage::GetLabel() const
{
  return m_strLabel;
}
void CGUIMessage::SetLabel(const string& strLabel)
{
  if (!strLabel.size()) return ;
  WCHAR* wszLabel = new WCHAR[strLabel.size() + 1];
  swprintf(wszLabel, L"%S", strLabel.c_str());
  m_strLabel = wszLabel;
  delete [] wszLabel;
}
void CGUIMessage::SetLabel(int iString)
{
  const WCHAR* pszString = g_localizeStrings.Get(iString).c_str();
  m_strLabel = pszString;
}

void CGUIMessage::SetStringParam(const string& strParam)
{
  m_strParam = strParam;
}

const string& CGUIMessage::GetStringParam() const
{
  return m_strParam;
}
