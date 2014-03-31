//
//  PlexAttributeParser.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-04-05.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#ifndef PLEXATTRIBUTEPARSER_H
#define PLEXATTRIBUTEPARSER_H

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "URL.h"
#include "plex/PlexUtils.h"

class CFileItem;

class CPlexAttributeParserBase
{
  public:
    CPlexAttributeParserBase() {}
    virtual void Process(const CURL& url, const CStdString& key, const CStdString& value, CFileItem *item);
};

class CPlexAttributeParserInt : public CPlexAttributeParserBase
{
  public:
    int64_t GetInt(const CStdString& value);
    virtual void Process(const CURL& url, const CStdString &key, const CStdString &value, CFileItem *item);
};

class CPlexAttributeParserBool : public CPlexAttributeParserInt
{
  public:
    virtual void Process(const CURL& url, const CStdString &key, const CStdString &value, CFileItem *item);
};

class CPlexAttributeParserKey : public CPlexAttributeParserBase
{
  public:
    virtual void Process(const CURL& url, const CStdString &key, const CStdString &value, CFileItem *item);
};

class CPlexAttributeParserMediaUrl : public CPlexAttributeParserBase
{
  public:
    virtual void Process(const CURL &url, const CStdString &key, const CStdString &value, CFileItem *item);
    static CStdString GetImageURL(const CURL &url, const CStdString &source, int height, int width);
};

class CPlexAttributeParserMediaFlag : public CPlexAttributeParserMediaUrl
{
  public:
    virtual void Process(const CURL &url, const CStdString &key, const CStdString &value, CFileItem *item);
};

class CPlexAttributeParserType : public CPlexAttributeParserInt
{
  public:
    virtual void Process(const CURL &url, const CStdString &key, const CStdString &value, CFileItem *item);
};

class CPlexAttributeParserLabel : public CPlexAttributeParserBase
{
  public:
    virtual void Process(const CURL &url, const CStdString &key, const CStdString &value, CFileItem *item);
};

class CPlexAttributeParserDateTime : public CPlexAttributeParserBase
{
  public:
    virtual void Process(const CURL &url, const CStdString &key, const CStdString &value, CFileItem *item);
};

class CPlexAttributeParserTitleSort : public CPlexAttributeParserBase
{
public:
  virtual void Process(const CURL &url, const CStdString &key, const CStdString &value, CFileItem *item);
};


#endif // PLEXATTRIBUTEPARSER_H
