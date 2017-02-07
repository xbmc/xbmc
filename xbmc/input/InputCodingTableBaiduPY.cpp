/*
*      Copyright (C) 2005-2013 Team Kodi
*      http://kodi.tv
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#include "InputCodingTableBaiduPY.h"

#include <stdlib.h>
#include <utility>

#include "filesystem/CurlFile.h"
#include "utils/StringUtils.h"
#include "utils/RegExp.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/helpers/GUIMessageHelper.h"

CInputCodingTableBaiduPY::CInputCodingTableBaiduPY(const std::string& strUrl) :
  CThread("BaiduPYApi"),
  m_messageCounter{ 0 },
  m_api_begin{ 0 },
  m_api_end{ 20 },
  m_api_nomore{ false },
  m_initialized{ false }
{
  m_url = strUrl;
  m_codechars = "abcdefghijklmnopqrstuvwxyz";
  m_code = "";
}

void CInputCodingTableBaiduPY::Process()
{
  m_initialized = true;
  while (!m_bStop) //Make sure we don't exit the thread
  {
    AbortableWait(m_Event, -1); //Wait for work to appear
    while (!m_bStop) //Process all queued work before going back to wait on the event
    {
      CSingleLock lock(m_CS);
      if (m_work.empty())
        break;

      auto work = m_work.front();
      m_work.pop_front();
      lock.Leave();

      std::string data;
      XFILE::CCurlFile http;
      std::string strUrl;
      strUrl = StringUtils::Format(m_url.c_str(), work.c_str(), m_api_begin, m_api_end);

      if (http.Get(strUrl, data))
        HandleResponse(work, data);
    }
  }
}

void CInputCodingTableBaiduPY::HandleResponse(const std::string& strCode, const std::string& response)
{
  using KODI::MESSAGING::HELPERS::PostGUIMessage;

  if (strCode != m_code) // don't handle obsolete response
    return;

  std::vector<std::wstring> words;
  CRegExp reg;
  reg.RegComp("\\[\"(.+?)\",[^\\]]+\\]");
  int pos = 0;
  int num = 0;
  while ((pos = reg.RegFind(response.c_str(), pos)) >= 0)
  {
    num++;
    std::string full = reg.GetMatch(0);
    std::string word = reg.GetMatch(1);
    pos += full.length();
    words.push_back(UnicodeToWString(word));
  }
  if (words.size() < 20)
    m_api_nomore = true;
  else
  {
    m_api_begin += 20;
    m_api_end += 20;
  }
  CSingleLock lock(m_CS);
  m_responses.insert(std::make_pair(++m_messageCounter, words));
  CGUIMessage msg(GUI_MSG_CODINGTABLE_LOOKUP_COMPLETED, 0, 0, m_messageCounter);
  msg.SetStringParam(strCode);
  lock.Leave();
  PostGUIMessage(msg, g_windowManager.GetActiveWindowID());
}

std::wstring CInputCodingTableBaiduPY::UnicodeToWString(const std::string& unicode)
{
  std::wstring result = L"";
  for (unsigned int i = 0; i < unicode.length(); i += 6)
  {
    int c;
    sscanf(unicode.c_str() + i, "\\u%x", &c);
    result += (wchar_t)c;
  }
  return result;
}

std::vector<std::wstring> CInputCodingTableBaiduPY::GetResponse(int response)
{
  CSingleLock lock(m_CS);
  auto words = m_responses.at(response);
  m_responses.erase(response);
  return words;
}

void CInputCodingTableBaiduPY::Initialize()
{
  CSingleLock lock(m_CS);
  if (!IsRunning())
    Create();
}

void CInputCodingTableBaiduPY::Deinitialize()
{
  m_Event.Set();
  StopThread(true);
  m_initialized = false;
}

bool CInputCodingTableBaiduPY::IsInitialized() const
{
  return m_initialized;
}

bool CInputCodingTableBaiduPY::GetWordListPage(const std::string& strCode, bool isFirstPage)
{
  if (strCode.empty())
    return false;
  if (isFirstPage || m_code != strCode)
  {
    m_api_begin = 0;
    m_api_end = 20;
    m_code = strCode;
    m_api_nomore = false;
  }
  else
  {
    if (m_api_nomore)
      return false;
  }

  CSingleLock lock(m_CS);
  m_work.push_back(strCode);
  m_Event.Set();
  return true;
}
