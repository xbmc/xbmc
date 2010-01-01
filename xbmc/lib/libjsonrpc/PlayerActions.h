#pragma once

#include "StdString.h"
#include "JSONRPC.h"

class CPlayerActions
{
public:
  static JSON_STATUS GetActivePlayers(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS GetAvailablePlayers(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS PlayPause(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS Stop(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS SkipPrevious(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS SkipNext(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);

  static JSON_STATUS BigSkipBackward(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS BigSkipForward(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS SmallSkipBackward(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS SmallSkipForward(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);

  static JSON_STATUS Rewind(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS Forward(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);

  static JSON_STATUS Record(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
};
