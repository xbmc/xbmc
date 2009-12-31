#pragma once
#include <features.h>
//#include "system.h"
#include "StdString.h"
#include <map>
#include <stdio.h>
#include <string>
#include <iostream>
#include "../libjsoncpp/json.h"

enum JSON_STATUS
{
  OK = 0,
  MethodNotFound = -32601,
  InvalidParams = -32602,
  InternalError = -32603,
  ParseError = -32700,
};

/* The method call needs to be perfectly threadsafe
   The method will only be called if parameterObject contains data as specified. So if method doesn't support parameters it won't be called with parameters and vice versa.
*/
typedef JSON_STATUS (*MethodCall) (const CStdString &method, const Json::Value& parameterObject, Json::Value &result);

enum ParameterNeeds
{
  NoParameters = 0,
  MayHaveParameters = 1,
  MustHaveParameters = 2,
};

typedef struct
{
  const char* command;
  ParameterNeeds parameters;
  MethodCall method;
  const char* description;
} JSON_ACTION;

typedef std::map<CStdString, JSON_ACTION> ActionMap;

class CJSONRPC
{
public:
  static void Initialize();
  static CStdString MethodCall(const CStdString &inputString);

  static JSON_STATUS Introspect(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS Version(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
private:
  static JSON_STATUS InternalMethodCall(const CStdString& method, Json::Value& o, Json::Value &result);
  static ActionMap m_actionMap;
};
