#include "HttpApi.h"
#include "log.h"
#include "XBMChttp.h"
#include "Application.h"
#include "ApplicationMessenger.h"

#define MAX_PARAS 20

CStdString CHttpApi::WebMethodCall(CStdString &command, CStdString &parameter)
{
  CStdString response = MethodCall(command, parameter);
  response.Format("%s%s%s", m_pXbmcHttp->incWebHeader ? "<html>\n" : "", response.c_str(), m_pXbmcHttp->incWebFooter ? "\n</html>\n" : "");
  return response;
}

CStdString CHttpApi::MethodCall(CStdString &command, CStdString &parameter)
{
  if (parameter.IsEmpty())
    checkForFunctionTypeParas(command, parameter);

  int cnt=0;

  if (!parameter.IsEmpty())
    g_application.getApplicationMessenger().HttpApi(command + "; " + parameter, true);
  else
    g_application.getApplicationMessenger().HttpApi(command, true);

  //wait for response - max 20s
  Sleep(0);
  CStdString response = g_application.getApplicationMessenger().GetResponse();

  while (response=="[No response yet]" && cnt++<200 && !g_application.m_bStop)
  {
    response=g_application.getApplicationMessenger().GetResponse();
    CLog::Log(LOGDEBUG, "HttpApi: waiting %d", cnt);
    Sleep(100);
  }

  return m_pXbmcHttp->userHeader + response + m_pXbmcHttp->userFooter;
}

bool CHttpApi::checkForFunctionTypeParas(CStdString &cmd, CStdString &paras)
{
  int open, close;
  open = cmd.Find("(");
  if (open>0)
  {
    close=cmd.length();
    while (close>open && cmd.Mid(close,1)!=")")
      close--;
    if (close>open)
    {
      paras = cmd.Mid(open + 1, close - open - 1);
      cmd = cmd.Left(open);
      return (close-open)>1;
    }
  }
  return false;
}

int CHttpApi::xbmcCommand(const CStdString &parameter)
{
  return m_pXbmcHttp->xbmcCommand(parameter);
}
