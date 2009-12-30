#pragma once

#include "StdString.h"
#include "JSONRPC.h"

class CFileActions
{
public:
  static JSON_STATUS GetRootDirectory(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS GetDirectory(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS Download(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
};
