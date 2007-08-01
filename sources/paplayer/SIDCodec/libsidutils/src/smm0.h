/***************************************************************************
                          smm0.h  -  sidusage file support
                             -------------------
    begin                : Tues Nov 19 2002
    copyright            : (C) 2002-2004 by Simon White
    email                : sidplay2@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _smm0_h_
#define _smm0_h_

#include <stdio.h>
#include <string.h>
#include <sidplay/sidtypes.h>

// IFF IDs
#define BUILD_ID(a, b, c, d) ((uint) a << 24 | \
                              (uint) b << 16 | \
                              (uint) c << 8  | \
                              (uint) d)

#define FORM_ID BUILD_ID('F','O','R','M')
#define SMM0_ID BUILD_ID('S','M','M','0')
#define INF0_ID BUILD_ID('I','N','F','0')
#define ERR0_ID BUILD_ID('E','R','R','0')
#define TIME_ID BUILD_ID('T','I','M','E')
#define MD5_ID  BUILD_ID('M','D','5',' ')
#define BODY_ID BUILD_ID('B','O','D','Y')
#define BXF_ID  BUILD_ID('B','X','F',' ') /* Body extended flags */

// Future Versions
#define SMM1_ID BUILD_ID('S','M','M','1')
#define SMM2_ID BUILD_ID('S','M','M','2')
#define SMM3_ID BUILD_ID('S','M','M','3')
#define SMM4_ID BUILD_ID('S','M','M','4')
#define SMM5_ID BUILD_ID('S','M','M','5')
#define SMM6_ID BUILD_ID('S','M','M','6')
#define SMM7_ID BUILD_ID('S','M','M','7')
#define SMM8_ID BUILD_ID('S','M','M','8')
#define SMM9_ID BUILD_ID('S','M','M','9')
#define SMM_EX_FLAGS (sizeof(sid_usage_t::memflags_t)-1)


class Chunk
{
private:
    const uint_least32_t m_id;
    const bool m_compulsory;
    Chunk * const m_sub;
    Chunk * const m_next;
    bool  m_used;

private:
    Chunk *match (uint_least32_t id);

protected:
    bool _read  (FILE *file, uint8_t *data, uint_least32_t length, uint_least32_t &remaining);
    bool _write (FILE *file, const uint8_t *data, uint_least32_t length, uint_least32_t &count);

    virtual void init (sid2_usage_t &usage) = 0;
    virtual bool used (const sid2_usage_t &) { return true; }

public:
    Chunk(uint_least32_t id, bool compulsory, Chunk *next, Chunk *sub)
        : m_id(id), m_compulsory(compulsory), m_next(next), m_sub(sub) {;}
    virtual bool read  (FILE *file, sid2_usage_t &usage, uint_least32_t length);
    virtual bool write (FILE *file, const sid2_usage_t &usage, uint_least32_t &length);
};

class Inf_v0: public Chunk
{
protected:
    void init  (sid2_usage_t &usage);
public:
    Inf_v0(Chunk *next): Chunk(INF0_ID, true, next, 0) {;}
    bool read  (FILE *file, sid2_usage_t &usage, uint_least32_t length);
    bool write (FILE *file, const sid2_usage_t &usage, uint_least32_t &length);
};

class Err_v0: public Chunk
{
protected:
    void init  (sid2_usage_t &usage);
public:
    Err_v0(Chunk *next): Chunk(ERR0_ID, false, next, 0) {;}
    bool read  (FILE *file, sid2_usage_t &usage, uint_least32_t length);
    bool write (FILE *file, const sid2_usage_t &usage, uint_least32_t &length);
    bool used  (const sid2_usage_t &usage);
};

class Md5: public Chunk
{
protected:
    void init  (sid2_usage_t &usage);
public:
    Md5(Chunk *next): Chunk(MD5_ID, false, next, 0) {;}
    bool read  (FILE *file, sid2_usage_t &usage, uint_least32_t length);
    bool write (FILE *file, const sid2_usage_t &usage, uint_least32_t &length);
    bool used  (const sid2_usage_t &usage);
};

class Time: public Chunk
{
protected:
    void init  (sid2_usage_t &usage);
public:
    Time(Chunk *next): Chunk(TIME_ID, false, next, 0) {;}
    bool read  (FILE *file, sid2_usage_t &usage, uint_least32_t length);
    bool write (FILE *file, const sid2_usage_t &usage, uint_least32_t &length);
    bool used  (const sid2_usage_t &usage);
};

class Body;
class Body_extended_flags: public Chunk
{
private:
    Body   &m_body;
    uint8_t m_flags[0x100 * SMM_EX_FLAGS + 1];

protected:
    void init   (sid2_usage_t &) {;}    
    bool recall (FILE *file, int &count, int &extension, uint_least32_t &length);
    bool store  (FILE *file, int count, int extension, uint_least32_t &length);

public:
    Body_extended_flags(Chunk *next, Body *body)
      :Chunk(BXF_ID, false, next, 0),
       m_body(*body) {;}
    bool read  (FILE *file, sid2_usage_t &usage, uint_least32_t length);
    bool write (FILE *file, const sid2_usage_t &usage, uint_least32_t &length);
    bool used  (const sid2_usage_t &usage);
};

class Body: public Chunk
{
    friend class Body_extended_flags;

private:
    Body_extended_flags m_exflags;

    uint8_t m_pages;
    struct usage_t
    {
      uint8_t page;
      uint8_t flags[256];
      bool    extended;
    } m_usage[256];

protected:
    void init  (sid2_usage_t &usage);

public:
    Body (Chunk *next)
      :Chunk(BODY_ID, true, next, &m_exflags),
       m_exflags(0, this), m_pages(0) {;}
    bool read  (FILE *file, sid2_usage_t &usage, uint_least32_t length);
    bool write (FILE *file, const sid2_usage_t &usage, uint_least32_t &length);
};

// SMM0 chunk description
class Smm_v0: public Chunk
{
private:
    Inf_v0    m_info;
    Err_v0    m_error;
    Md5       m_md5;
    Time      m_time;
    Body      m_body;

    void init (sid2_usage_t &) {;}

public:
    Smm_v0 (Chunk *next)
    :Chunk(SMM0_ID, true, next, &m_info),
     m_info(&m_error),
     m_error(&m_md5),
     m_md5(&m_time),
     m_time(&m_body),
     m_body(0) {;}
};

#endif // _smm0_h_
