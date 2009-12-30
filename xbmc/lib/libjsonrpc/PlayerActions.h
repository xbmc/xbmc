#pragma once

#include "StdString.h"
#include "JSONRPC.h"

class CPlayerActions
{
public:
  static JSON_STATUS GetActivePlayers(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS Pause(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS SkipNext(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS SkipPrevious(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
};
