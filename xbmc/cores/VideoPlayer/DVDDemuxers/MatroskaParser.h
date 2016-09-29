#pragma once

#include "EbmlParser.h"

using MatroskaSegmentUID = std::string;
//using MatroskaChapterDisplayMap = std::map<std::string,std::string>;
struct MatroskaChapterDisplayMap : std::map<std::string,std::string>
{
  std::string GetDefault();
};

struct MatroskaChapterAtom
{
  uint64_t uid = 0;
  uint64_t timeStart = 0;
  uint64_t timeEnd = 0;
  bool flagHidden = false;
  bool flagEnabled = false;
  MatroskaSegmentUID segUid;
  uint64_t segEditionUid = 0;
  MatroskaChapterDisplayMap displays;
  std::list<MatroskaChapterAtom> subChapters;
};

struct MatroskaEdition
{
  uint64_t uid = 0;
  bool flagHidden = false;
  bool flagDefault = false;
  bool flagOrdered = false;
  std::list<MatroskaChapterAtom> chapterAtoms;
};

struct MatroskaChapters
{
  std::list<MatroskaEdition> editions;
};

typedef std::multimap<EbmlId,uint64_t> MatroskaSeekMap;

struct MatroskaSegmentInfo
{
  MatroskaSegmentUID uid;
  uint64_t timecodeScale = 1000000;
};

struct MatroskaSegment
{
  int64_t offsetBegin;
  int64_t offsetEnd;
  MatroskaSegmentInfo infos;
  MatroskaSeekMap seekMap;
  MatroskaChapters chapters;

  bool Parse(CDVDInputStream *input);
};

struct MatroskaFile
{
  int64_t offsetBegin;
  int64_t offsetEnd;
  EbmlHeader ebmlHeader;
  MatroskaSegment segment;
  bool Parse(CDVDInputStream *input);
};

// vim: ts=2 sw=2 expandtab
