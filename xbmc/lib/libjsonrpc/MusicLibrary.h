#pragma once
#include "StdString.h"
#include "JSONRPC.h"

class CMusicLibrary
{
public:
  static JSON_STATUS GetArtists(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS GetAlbums(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS GetSongs(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
  static JSON_STATUS GetSongInfo(const CStdString &method, const Json::Value& parameterObject, Json::Value &result);
};
