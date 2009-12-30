#pragma once
#include "StdString.h"
#include "JSONRPC.h"
#include "../FileItem.h"
#include "../VideoInfoTag.h"
#include "../MusicInfoTag.h"

using namespace MUSIC_INFO;

class CLibraryBase
{
protected:
  static void FillVideoDetails(const CVideoInfoTag *videoInfo, const Json::Value& parameterObject, Json::Value &result);
  static void FillMusicDetails(const CMusicInfoTag *musicInfo, const Json::Value& parameterObject, Json::Value &result);
  static void HandleFileItemList(const CStdString &id, const CStdString &resultname, CFileItemList &items, unsigned int &start, unsigned int &end, const Json::Value& parameterObject, Json::Value &result);
private:
  static bool ParseSortMethods(const CStdString &method, const CStdString &order, SORT_METHOD &sortmethod, SORT_ORDER &sortorder);
  static void Sort(CFileItemList &items, const Json::Value& parameterObject);
};
