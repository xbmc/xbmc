#pragma once
#include "StdString.h"

class CHttpApi
{
public:
  static CStdString MethodCall(CStdString &command, CStdString &parameter);
  static bool checkForFunctionTypeParas(CStdString &cmd, CStdString &paras);
  static int xbmcCommand(const CStdString &parameter);
};
