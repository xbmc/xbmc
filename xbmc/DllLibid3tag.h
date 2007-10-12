#pragma once
#include "DynamicDll.h"
#include "lib/libid3tag/metadata.h"

class DllLibID3TagInterface
{
public:
    virtual ~DllLibID3TagInterface() {}
    virtual struct id3_file *id3_file_open(char const *, enum id3_file_mode)=0;
    virtual struct id3_file *id3_file_fdopen(int, enum id3_file_mode)=0;
    virtual int id3_file_close(struct id3_file *)=0;
    virtual struct id3_tag *id3_file_tag(struct id3_file const *)=0;
    virtual int id3_file_update(struct id3_file *)=0;
    virtual struct id3_tag *id3_tag_new(void)=0;
    virtual void id3_tag_delete(struct id3_tag *)=0;
    virtual unsigned int id3_tag_version(struct id3_tag const *)=0;
    virtual int id3_tag_options(struct id3_tag *, int, int)=0;
    virtual void id3_tag_setlength(struct id3_tag *, id3_length_t)=0;
    virtual void id3_tag_clearframes(struct id3_tag *)=0;
    virtual int id3_tag_attachframe(struct id3_tag *, struct id3_frame *)=0;
    virtual int id3_tag_detachframe(struct id3_tag *, struct id3_frame *)=0;
    virtual struct id3_frame *id3_tag_findframe(struct id3_tag const *, char const *, unsigned int)=0;
    virtual signed long id3_tag_query(id3_byte_t const *, id3_length_t)=0;
    virtual struct id3_tag *id3_tag_parse(id3_byte_t const *, id3_length_t)=0;
    virtual id3_length_t id3_tag_render(struct id3_tag const *, id3_byte_t *)=0;
    virtual struct id3_frame *id3_frame_new(char const *)=0;
    virtual void id3_frame_delete(struct id3_frame *)=0;
    virtual union id3_field *id3_frame_field(struct id3_frame const *, unsigned int)=0;
    virtual enum id3_field_type id3_field_type(union id3_field const *)=0;
    virtual int id3_field_setint(union id3_field *, signed long)=0;
    virtual int id3_field_settextencoding(union id3_field *, enum id3_field_textencoding)=0;
    virtual int id3_field_setstrings(union id3_field *, unsigned int, id3_ucs4_t **)=0;
    virtual int id3_field_addstring(union id3_field *, id3_ucs4_t const *)=0;
    virtual int id3_field_setlanguage(union id3_field *, char const *)=0;
    virtual int id3_field_setlatin1(union id3_field *, id3_latin1_t const *)=0;
    virtual int id3_field_setfulllatin1(union id3_field *, id3_latin1_t const *)=0;
    virtual int id3_field_setstring(union id3_field *, id3_ucs4_t const *)=0;
    virtual int id3_field_setfullstring(union id3_field *, id3_ucs4_t const *)=0;
    virtual int id3_field_setframeid(union id3_field *, char const *)=0;
    virtual int id3_field_setbinarydata(union id3_field *, id3_byte_t const *, id3_length_t)=0;
    virtual signed long id3_field_getint(union id3_field const *)=0;
    virtual enum id3_field_textencoding id3_field_gettextencoding(union id3_field const *)=0;
    virtual id3_latin1_t const *id3_field_getlatin1(union id3_field const *)=0;
    virtual id3_latin1_t const *id3_field_getfulllatin1(union id3_field const *)=0;
    virtual id3_ucs4_t const *id3_field_getstring(union id3_field const *)=0;
    virtual id3_ucs4_t const *id3_field_getfullstring(union id3_field const *)=0;
    virtual unsigned int id3_field_getnstrings(union id3_field const *)=0;
    virtual id3_ucs4_t const *id3_field_getstrings(union id3_field const *, unsigned int)=0;
    virtual char const *id3_field_getframeid(union id3_field const *)=0;
    virtual id3_byte_t const *id3_field_getbinarydata(union id3_field const *, id3_length_t *)=0;
    virtual id3_ucs4_t const *id3_genre_index(unsigned int)=0;
    virtual id3_ucs4_t const *id3_genre_name(id3_ucs4_t const *)=0;
    virtual int id3_genre_number(id3_ucs4_t const *)=0;
    virtual id3_latin1_t *id3_ucs4_latin1duplicate(id3_ucs4_t const *)=0;
    virtual id3_utf16_t *id3_ucs4_utf16duplicate(id3_ucs4_t const *)=0;
    virtual id3_utf8_t *id3_ucs4_utf8duplicate(id3_ucs4_t const *)=0;
    virtual void id3_ucs4_putnumber(id3_ucs4_t *, unsigned long)=0;
    virtual unsigned long id3_ucs4_getnumber(id3_ucs4_t const *)=0;
    virtual void id3_ucs4_free(id3_ucs4_t *)=0;
    virtual id3_ucs4_t *id3_latin1_ucs4duplicate(id3_latin1_t const *)=0;
    virtual id3_ucs4_t *id3_utf16_ucs4duplicate(id3_utf16_t const *)=0;
    virtual id3_ucs4_t *id3_utf8_ucs4duplicate(id3_utf8_t const *)=0;
    virtual void id3_latin1_free(id3_latin1_t *)=0;
    virtual void id3_utf16_free(id3_utf16_t *)=0;
    virtual void id3_utf8_free(id3_utf8_t *)=0;
    virtual void id3_ucs4_list_free(id3_ucs4_list_t *)=0;
    virtual const id3_ucs4_t* id3_metadata_getartist(const struct id3_tag*, enum id3_field_textencoding*)=0;
    virtual const id3_ucs4_t* id3_metadata_getalbum(const struct id3_tag*, enum id3_field_textencoding*)=0;
    virtual const id3_ucs4_t* id3_metadata_getalbumartist(const struct id3_tag*, enum id3_field_textencoding*)=0;
    virtual const id3_ucs4_t* id3_metadata_gettitle(const struct id3_tag*, enum id3_field_textencoding*)=0;
    virtual const id3_ucs4_t* id3_metadata_gettrack(const struct id3_tag*, enum id3_field_textencoding*)=0;
    virtual const id3_ucs4_t* id3_metadata_getpartofset(const struct id3_tag* tag, enum id3_field_textencoding*)=0;
    virtual const id3_ucs4_t* id3_metadata_getyear(const struct id3_tag*, enum id3_field_textencoding*)=0;
    virtual const id3_ucs4_t* id3_metadata_getgenre(const struct id3_tag*, enum id3_field_textencoding*)=0;
    virtual id3_ucs4_list_t* id3_metadata_getgenres(const struct id3_tag*, enum id3_field_textencoding*)=0;
    virtual const id3_ucs4_t* id3_metadata_getcomment(const struct id3_tag*, enum id3_field_textencoding*)=0;
    virtual const id3_ucs4_t* id3_metadata_getencodedby(const struct id3_tag* tag, enum id3_field_textencoding*)=0;
    virtual char id3_metadata_getrating(const struct id3_tag* tag)=0;
    virtual int id3_metadata_haspicture(const struct id3_tag*, enum id3_picture_type)=0;
    virtual const id3_latin1_t* id3_metadata_getpicturemimetype(const struct id3_tag*, enum id3_picture_type)=0;
    virtual id3_byte_t const *id3_metadata_getpicturedata(const struct id3_tag*, enum id3_picture_type, id3_length_t*)=0;
    virtual id3_byte_t const* id3_metadata_getuniquefileidentifier(const struct id3_tag*, const char* owner_identifier, id3_length_t*)=0;
    virtual const id3_ucs4_t* id3_metadata_getusertext(const struct id3_tag*, const char* description)=0;
    virtual int id3_metadata_getfirstnonstandardpictype(const struct id3_tag*, enum id3_picture_type*)=0;
    virtual int id3_metadata_setartist(struct id3_tag* tag, id3_ucs4_t* value)=0;
    virtual int id3_metadata_setalbum(struct id3_tag* tag, id3_ucs4_t* value)=0;
    virtual int id3_metadata_setalbumartist(struct id3_tag* tag, id3_ucs4_t* value)=0;
    virtual int id3_metadata_settitle(struct id3_tag* tag, id3_ucs4_t* value)=0;
    virtual int id3_metadata_settrack(struct id3_tag* tag, id3_ucs4_t* value)=0;
    virtual int id3_metadata_setpartofset(struct id3_tag* tag, id3_ucs4_t* value)=0;
    virtual int id3_metadata_setyear(struct id3_tag* tag, id3_ucs4_t* value)=0;
    virtual int id3_metadata_setgenre(struct id3_tag* tag, id3_ucs4_t* value)=0;
    virtual int id3_metadata_setencodedby(struct id3_tag* tag, id3_ucs4_t* value)=0;
    virtual int id3_metadata_setcomment(struct id3_tag* tag, id3_ucs4_t* value)=0;
    virtual int id3_metadata_setrating(struct id3_tag* tag, char value)=0;
};

class DllLibID3Tag : public DllDynamic, DllLibID3TagInterface
{
#ifndef _LINUX
  DECLARE_DLL_WRAPPER(DllLibID3Tag, Q:\\system\\libid3tag.dll)
#else
  DECLARE_DLL_WRAPPER(DllLibID3Tag, Q:\\system\\libid3tag-i486-linux.so)
#endif

  DEFINE_METHOD2(struct id3_file*, id3_file_open, (char const* p1, enum id3_file_mode p2))
  DEFINE_METHOD2(struct id3_file*, id3_file_fdopen, (int p1, enum id3_file_mode p2))
  DEFINE_METHOD1(int, id3_file_close, (struct id3_file* p1))
  DEFINE_METHOD1(struct id3_tag*, id3_file_tag, (struct id3_file const*p1 ))
  DEFINE_METHOD1(int, id3_file_update, (struct id3_file* p1))
  DEFINE_METHOD0(struct id3_tag*, id3_tag_new)
  DEFINE_METHOD1(void, id3_tag_delete, (struct id3_tag* p1))
  DEFINE_METHOD1(unsigned int, id3_tag_version, (struct id3_tag const* p1))
  DEFINE_METHOD3(int, id3_tag_options, (struct id3_tag* p1, int p2, int p3))
  DEFINE_METHOD2(void, id3_tag_setlength, (struct id3_tag* p1, id3_length_t p2))
  DEFINE_METHOD1(void, id3_tag_clearframes, (struct id3_tag* p1))
  DEFINE_METHOD2(int, id3_tag_attachframe, (struct id3_tag* p1, struct id3_frame* p2))
  DEFINE_METHOD2(int, id3_tag_detachframe, (struct id3_tag* p1, struct id3_frame* p2))
  DEFINE_METHOD3(struct id3_frame*, id3_tag_findframe, (struct id3_tag const* p1, char const* p2, unsigned int p3))
  DEFINE_METHOD2(signed long, id3_tag_query, (id3_byte_t const* p1, id3_length_t p2))
  DEFINE_METHOD2(struct id3_tag*, id3_tag_parse, (id3_byte_t const* p1, id3_length_t p2))
  DEFINE_METHOD2(id3_length_t, id3_tag_render, (struct id3_tag const* p1, id3_byte_t* p2))
  DEFINE_METHOD1(struct id3_frame*, id3_frame_new, (char const* p1))
  DEFINE_METHOD1(void, id3_frame_delete, (struct id3_frame* p1))
  DEFINE_METHOD2(union id3_field*, id3_frame_field, (struct id3_frame const* p1, unsigned int p2))
  DEFINE_METHOD1(enum id3_field_type, id3_field_type, (union id3_field const* p1))
  DEFINE_METHOD2(int, id3_field_setint, (union id3_field* p1, signed long p2))
  DEFINE_METHOD2(int, id3_field_settextencoding, (union id3_field*p1, enum id3_field_textencoding p2))
  DEFINE_METHOD3(int, id3_field_setstrings, (union id3_field* p1, unsigned int p2, id3_ucs4_t** p3))
  DEFINE_METHOD2(int, id3_field_addstring, (union id3_field* p1, id3_ucs4_t const* p2))
  DEFINE_METHOD2(int, id3_field_setlanguage, (union id3_field* p1, char const*p2))
  DEFINE_METHOD2(int, id3_field_setlatin1, (union id3_field* p1, id3_latin1_t const* p2))
  DEFINE_METHOD2(int, id3_field_setfulllatin1, (union id3_field*p1, id3_latin1_t const* p2))
  DEFINE_METHOD2(int, id3_field_setstring, (union id3_field* p1, id3_ucs4_t const* p2))
  DEFINE_METHOD2(int, id3_field_setfullstring, (union id3_field* p1, id3_ucs4_t const* p2))
  DEFINE_METHOD2(int, id3_field_setframeid, (union id3_field* p1, char const*p2))
  DEFINE_METHOD3(int, id3_field_setbinarydata, (union id3_field* p1, id3_byte_t const* p2, id3_length_t p3))
  DEFINE_METHOD1(signed long, id3_field_getint, (union id3_field const* p1))
  DEFINE_METHOD1(enum id3_field_textencoding, id3_field_gettextencoding, (union id3_field const* p1))
  DEFINE_METHOD1(id3_latin1_t const*, id3_field_getlatin1, (union id3_field const* p1))
  DEFINE_METHOD1(id3_latin1_t const*, id3_field_getfulllatin1, (union id3_field const* p1))
  DEFINE_METHOD1(id3_ucs4_t const*, id3_field_getstring, (union id3_field const* p1))
  DEFINE_METHOD1(id3_ucs4_t const*, id3_field_getfullstring, (union id3_field const* p1))
  DEFINE_METHOD1(unsigned int, id3_field_getnstrings, (union id3_field const* p1))
  DEFINE_METHOD2(id3_ucs4_t const*, id3_field_getstrings, (union id3_field const* p1, unsigned int p2))
  DEFINE_METHOD1(char const*, id3_field_getframeid, (union id3_field const* p1))
  DEFINE_METHOD2(id3_byte_t const*, id3_field_getbinarydata, (union id3_field const* p1, id3_length_t* p2))
  DEFINE_METHOD1(id3_ucs4_t const*, id3_genre_index, (unsigned int p1))
  DEFINE_METHOD1(id3_ucs4_t const*, id3_genre_name, (id3_ucs4_t const* p1))
  DEFINE_METHOD1(int, id3_genre_number, (id3_ucs4_t const*p1))
  DEFINE_METHOD1(id3_latin1_t*, id3_ucs4_latin1duplicate, (id3_ucs4_t const* p1))
  DEFINE_METHOD1(id3_utf16_t*, id3_ucs4_utf16duplicate, (id3_ucs4_t const* p1))
  DEFINE_METHOD1(id3_utf8_t*, id3_ucs4_utf8duplicate, (id3_ucs4_t const* p1))
  DEFINE_METHOD2(void, id3_ucs4_putnumber, (id3_ucs4_t* p1, unsigned long p2))
  DEFINE_METHOD1(unsigned long, id3_ucs4_getnumber, (id3_ucs4_t const* p1))
  DEFINE_METHOD1(void, id3_ucs4_free, (id3_ucs4_t* p1))
  DEFINE_METHOD1(id3_ucs4_t*, id3_latin1_ucs4duplicate, (id3_latin1_t const* p1))
  DEFINE_METHOD1(id3_ucs4_t*, id3_utf16_ucs4duplicate, (id3_utf16_t const* p1))
  DEFINE_METHOD1(id3_ucs4_t*, id3_utf8_ucs4duplicate, (id3_utf8_t const* p1))
  DEFINE_METHOD1(void, id3_latin1_free, (id3_latin1_t* p1))
  DEFINE_METHOD1(void, id3_utf16_free, (id3_utf16_t* p1))
  DEFINE_METHOD1(void, id3_utf8_free, (id3_utf8_t* p1))
  DEFINE_METHOD1(void, id3_ucs4_list_free, (id3_ucs4_list_t* p1))
  DEFINE_METHOD2(const id3_ucs4_t*, id3_metadata_getartist, (const struct id3_tag* p1, enum id3_field_textencoding* p2))
  DEFINE_METHOD2(const id3_ucs4_t*, id3_metadata_getalbum, (const struct id3_tag* p1, enum id3_field_textencoding* p2))
  DEFINE_METHOD2(const id3_ucs4_t*, id3_metadata_getalbumartist, (const struct id3_tag* p1, enum id3_field_textencoding* p2))
  DEFINE_METHOD2(const id3_ucs4_t*, id3_metadata_gettitle, (const struct id3_tag* p1, enum id3_field_textencoding* p2))
  DEFINE_METHOD2(const id3_ucs4_t*, id3_metadata_gettrack, (const struct id3_tag* p1, enum id3_field_textencoding* p2))
  DEFINE_METHOD2(const id3_ucs4_t*, id3_metadata_getpartofset, (const struct id3_tag* p1, enum id3_field_textencoding* p2))
  DEFINE_METHOD2(const id3_ucs4_t*, id3_metadata_getyear, (const struct id3_tag* p1, enum id3_field_textencoding* p2))
  DEFINE_METHOD2(const id3_ucs4_t*, id3_metadata_getgenre, (const struct id3_tag* p1, enum id3_field_textencoding* p2))
  DEFINE_METHOD2(id3_ucs4_list_t*, id3_metadata_getgenres, (const struct id3_tag* p1, enum id3_field_textencoding* p2))
  DEFINE_METHOD2(const id3_ucs4_t*, id3_metadata_getcomment, (const struct id3_tag* p1, enum id3_field_textencoding* p2))
  DEFINE_METHOD2(const id3_ucs4_t*, id3_metadata_getencodedby, (const struct id3_tag* p1, enum id3_field_textencoding* p2))
  DEFINE_METHOD1(char, id3_metadata_getrating, (const struct id3_tag* p1))
  DEFINE_METHOD2(int, id3_metadata_haspicture, (const struct id3_tag* p1, enum id3_picture_type p2))
  DEFINE_METHOD2(const id3_latin1_t*, id3_metadata_getpicturemimetype, (const struct id3_tag* p1, enum id3_picture_type p2))
  DEFINE_METHOD3(id3_byte_t const*, id3_metadata_getpicturedata, (const struct id3_tag* p1, enum id3_picture_type p2, id3_length_t* p3))
  DEFINE_METHOD3(id3_byte_t const*, id3_metadata_getuniquefileidentifier, (const struct id3_tag* p1, const char* p2, id3_length_t* p3))
  DEFINE_METHOD2(const id3_ucs4_t*, id3_metadata_getusertext, (const struct id3_tag* p1, const char* p2))
  DEFINE_METHOD2(int, id3_metadata_getfirstnonstandardpictype, (const struct id3_tag* p1, enum id3_picture_type* p2))
  DEFINE_METHOD2(int, id3_metadata_setartist, (struct id3_tag* p1, id3_ucs4_t* p2))
  DEFINE_METHOD2(int, id3_metadata_setalbum, (struct id3_tag* p1, id3_ucs4_t* p2))
  DEFINE_METHOD2(int, id3_metadata_setalbumartist, (struct id3_tag* p1, id3_ucs4_t* p2))
  DEFINE_METHOD2(int, id3_metadata_settitle, (struct id3_tag* p1, id3_ucs4_t* p2))
  DEFINE_METHOD2(int, id3_metadata_settrack, (struct id3_tag* p1, id3_ucs4_t* p2))
  DEFINE_METHOD2(int, id3_metadata_setpartofset, (struct id3_tag* p1, id3_ucs4_t* p2))
  DEFINE_METHOD2(int, id3_metadata_setyear, (struct id3_tag* p1, id3_ucs4_t* p2))
  DEFINE_METHOD2(int, id3_metadata_setgenre, (struct id3_tag* p1, id3_ucs4_t* p2))
  DEFINE_METHOD2(int, id3_metadata_setencodedby, (struct id3_tag* p1, id3_ucs4_t* p2))
  DEFINE_METHOD2(int, id3_metadata_setcomment, (struct id3_tag* p1, id3_ucs4_t* p2))
  DEFINE_METHOD2(int, id3_metadata_setrating, (struct id3_tag* p1, char p2))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(id3_file_open)
    RESOLVE_METHOD(id3_file_fdopen)
    RESOLVE_METHOD(id3_file_close)
    RESOLVE_METHOD(id3_file_tag)
    RESOLVE_METHOD(id3_file_update)
    RESOLVE_METHOD(id3_tag_new)
    RESOLVE_METHOD(id3_tag_delete)
    RESOLVE_METHOD(id3_tag_version)
    RESOLVE_METHOD(id3_tag_options)
    RESOLVE_METHOD(id3_tag_setlength)
    RESOLVE_METHOD(id3_tag_clearframes)
    RESOLVE_METHOD(id3_tag_attachframe)
    RESOLVE_METHOD(id3_tag_detachframe)
    RESOLVE_METHOD(id3_tag_findframe)
    RESOLVE_METHOD(id3_tag_query)
    RESOLVE_METHOD(id3_tag_parse)
    RESOLVE_METHOD(id3_tag_render)
    RESOLVE_METHOD(id3_frame_new)
    RESOLVE_METHOD(id3_frame_delete)
    RESOLVE_METHOD(id3_frame_field)
    RESOLVE_METHOD(id3_field_type)
    RESOLVE_METHOD(id3_field_setint)
    RESOLVE_METHOD(id3_field_settextencoding)
    RESOLVE_METHOD(id3_field_setstrings)
    RESOLVE_METHOD(id3_field_addstring)
    RESOLVE_METHOD(id3_field_setlanguage)
    RESOLVE_METHOD(id3_field_setlatin1)
    RESOLVE_METHOD(id3_field_setfulllatin1)
    RESOLVE_METHOD(id3_field_setstring)
    RESOLVE_METHOD(id3_field_setfullstring)
    RESOLVE_METHOD(id3_field_setframeid)
    RESOLVE_METHOD(id3_field_setbinarydata)
    RESOLVE_METHOD(id3_field_getint)
    RESOLVE_METHOD(id3_field_gettextencoding)
    RESOLVE_METHOD(id3_field_getlatin1)
    RESOLVE_METHOD(id3_field_getfulllatin1)
    RESOLVE_METHOD(id3_field_getstring)
    RESOLVE_METHOD(id3_field_getfullstring)
    RESOLVE_METHOD(id3_field_getnstrings)
    RESOLVE_METHOD(id3_field_getstrings)
    RESOLVE_METHOD(id3_field_getframeid)
    RESOLVE_METHOD(id3_field_getbinarydata)
    RESOLVE_METHOD(id3_genre_index)
    RESOLVE_METHOD(id3_genre_name)
    RESOLVE_METHOD(id3_genre_number)
    RESOLVE_METHOD(id3_ucs4_latin1duplicate)
    RESOLVE_METHOD(id3_ucs4_utf16duplicate)
    RESOLVE_METHOD(id3_ucs4_utf8duplicate)
    RESOLVE_METHOD(id3_ucs4_putnumber)
    RESOLVE_METHOD(id3_ucs4_getnumber)
    RESOLVE_METHOD(id3_ucs4_free)
    RESOLVE_METHOD(id3_latin1_ucs4duplicate)
    RESOLVE_METHOD(id3_utf16_ucs4duplicate)
    RESOLVE_METHOD(id3_utf8_ucs4duplicate)
    RESOLVE_METHOD(id3_latin1_free)
    RESOLVE_METHOD(id3_utf16_free)
    RESOLVE_METHOD(id3_utf8_free)
    RESOLVE_METHOD(id3_ucs4_list_free)
    RESOLVE_METHOD(id3_metadata_getartist)
    RESOLVE_METHOD(id3_metadata_getalbum)
    RESOLVE_METHOD(id3_metadata_getalbumartist)
    RESOLVE_METHOD(id3_metadata_gettitle)
    RESOLVE_METHOD(id3_metadata_gettrack)
    RESOLVE_METHOD(id3_metadata_getpartofset)
    RESOLVE_METHOD(id3_metadata_getyear)
    RESOLVE_METHOD(id3_metadata_getgenre)
    RESOLVE_METHOD(id3_metadata_getgenres)
    RESOLVE_METHOD(id3_metadata_getcomment)
    RESOLVE_METHOD(id3_metadata_getencodedby)
    RESOLVE_METHOD(id3_metadata_getrating)
    RESOLVE_METHOD(id3_metadata_haspicture)
    RESOLVE_METHOD(id3_metadata_getpicturemimetype)
    RESOLVE_METHOD(id3_metadata_getpicturedata)
    RESOLVE_METHOD(id3_metadata_getuniquefileidentifier)
    RESOLVE_METHOD(id3_metadata_getusertext)
    RESOLVE_METHOD(id3_metadata_getfirstnonstandardpictype)
    RESOLVE_METHOD(id3_metadata_setartist)
    RESOLVE_METHOD(id3_metadata_setalbum)
    RESOLVE_METHOD(id3_metadata_setalbumartist)
    RESOLVE_METHOD(id3_metadata_settitle)
    RESOLVE_METHOD(id3_metadata_settrack)
    RESOLVE_METHOD(id3_metadata_setpartofset)
    RESOLVE_METHOD(id3_metadata_setyear)
    RESOLVE_METHOD(id3_metadata_setgenre)
    RESOLVE_METHOD(id3_metadata_setencodedby)
    RESOLVE_METHOD(id3_metadata_setcomment)
    RESOLVE_METHOD(id3_metadata_setrating)
  END_METHOD_RESOLVE()
};
