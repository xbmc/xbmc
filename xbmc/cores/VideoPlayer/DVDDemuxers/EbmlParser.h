#pragma once

#include <functional>
#include <limits>
#include <list>
#include <map>
#include <string>
#include <type_traits>

#include "DVDInputStreams/DVDInputStream.h"

using EbmlId = uint32_t;

/* top-level master-IDs */
static const EbmlId EBML_ID_HEADER = 0x1A45DFA3;

/* IDs in the HEADER master */
static const EbmlId EBML_ID_EBMLVERSION = 0x4286;
static const EbmlId EBML_ID_EBMLREADVERSION = 0x42F7;
static const EbmlId EBML_ID_EBMLMAXIDLENGTH = 0x42F2;
static const EbmlId EBML_ID_EBMLMAXSIZELENGTH = 0x42F3;
static const EbmlId EBML_ID_DOCTYPE = 0x4282;
static const EbmlId EBML_ID_DOCTYPEVERSION = 0x4287;
static const EbmlId EBML_ID_DOCTYPEREADVERSION = 0x4285;

/* general EBML types */
static const EbmlId EBML_ID_VOID = 0xEC;
static const EbmlId EBML_ID_CRC32 = 0xBF;

static const EbmlId EBML_ID_RESERVED1 = 0xFF;
static const EbmlId EBML_ID_RESERVED2 = 0x7FFF;
static const EbmlId EBML_ID_RESERVED3 = 0x3FFFFF;
static const EbmlId EBML_ID_RESERVED4 = 0x1FFFFFFF;
static const EbmlId EBML_ID_INVALID = 0xFFFFFFFF;

static const uint64_t INVALID_EBML_TAG_LENGTH = 0xFFFFFFFFFFFFFF;

EbmlId EbmlReadId(CDVDInputStream *input);
bool EbmlReadId(CDVDInputStream *input, EbmlId *output);
uint64_t EbmlReadLen(CDVDInputStream *input);
bool EbmlReadLen(CDVDInputStream *input, uint64_t *output);

uint64_t EbmlReadUint(CDVDInputStream *input, uint64_t len);
bool EbmlReadUint(CDVDInputStream *input, uint64_t *output, uint64_t len);

std::string EbmlReadString(CDVDInputStream *input, uint64_t len);
bool EbmlReadString(CDVDInputStream *input, std::string *output, uint64_t len);
std::string EbmlReadRaw(CDVDInputStream *input, uint64_t len);
bool EbmlReadRaw(CDVDInputStream *input, std::string *output, uint64_t len);


typedef std::function<bool(CDVDInputStream* input, uint64_t tagLen)> EbmlParserFunctor;
typedef std::map<EbmlId,EbmlParserFunctor> ParserMap;
bool EbmlParseMaster(CDVDInputStream *input, const ParserMap &parser, uint64_t tagLen, bool stopOnError = false);

template <typename DataType>
EbmlParserFunctor BindEbmlUintParser(DataType *output);
EbmlParserFunctor BindEbmlStringParser(std::string *output, uint64_t maxLen = std::numeric_limits<uint64_t>::max());
EbmlParserFunctor BindEbmlRawParser(std::string *output, uint64_t maxLen = std::numeric_limits<uint64_t>::max());
template <typename Container, typename FunctorGenerator, typename ...Args>
EbmlParserFunctor BindEbmlListParser(Container *lst, FunctorGenerator functorGenerator, Args... args);

struct EbmlMasterParser
{
  ParserMap parser;
  std::function<bool()> parsed;
  bool stopOnError = false;
  bool operator()(CDVDInputStream *input, uint64_t tagLen);
};

struct EbmlHeader
{
  uint32_t version = 1;
  std::string doctype = "matroska";
  bool Parse(CDVDInputStream *input);
  int64_t offsetBegin;
  int64_t offsetEnd;
};

EbmlMasterParser BindEbmlHeaderParser(EbmlHeader *ebmlHeader);

template <typename DataType>
EbmlParserFunctor BindEbmlUintParser(DataType *output)
{
  typedef typename std::common_type<DataType,uint64_t>::type common;
  return [output](CDVDInputStream *input, uint64_t tagLen)
  {
    uint64_t result;
    if (!EbmlReadUint(input, &result, tagLen))
      return false;
    if (std::is_same<DataType,bool>::value)
      (*output) = result;
    else if (common(result) < common(std::numeric_limits<DataType>::max()))
      (*output) = result;
    else
      return false;
    return true;
  };
}

template <typename Container, typename FunctorGenerator, typename ...Args>
EbmlParserFunctor BindEbmlListParser(Container *lst, FunctorGenerator functorGenerator, Args... args)
{
  return [lst,functorGenerator,args...](CDVDInputStream *input, uint64_t tagLen)
  {
    auto it = lst->emplace(lst->end());
    if (it == lst->end())
      return false;
    if (functorGenerator(&(*it), args...)(input, tagLen))
      return true;
    lst->erase(it);
    return false;
  };
}

// vim: ts=2 sw=2 expandtab
