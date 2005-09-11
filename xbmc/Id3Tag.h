#include "tag.h"
#include "lib/libid3tag/metadata.h"

namespace MUSIC_INFO
{

#pragma once

  class CID3Tag : public CTag
{
  struct ID3dll
  {
    /* file interface */
    struct id3_file *(_cdecl* id3_file_open)(char const *, enum id3_file_mode);
    struct id3_file *(_cdecl* id3_file_fdopen)(int, enum id3_file_mode);
    int (_cdecl* id3_file_close)(struct id3_file *);
    struct id3_tag *(_cdecl* id3_file_tag)(struct id3_file const *);
    int (_cdecl* id3_file_update)(struct id3_file *);

    /* tag interface */
    struct id3_tag *(_cdecl* id3_tag_new)(void);
    void (_cdecl* id3_tag_delete)(struct id3_tag *);
    unsigned int (_cdecl* id3_tag_version)(struct id3_tag const *);
    int (_cdecl* id3_tag_options)(struct id3_tag *, int, int);
    void (_cdecl* id3_tag_setlength)(struct id3_tag *, id3_length_t);
    void (_cdecl* id3_tag_clearframes)(struct id3_tag *);
    int (_cdecl* id3_tag_attachframe)(struct id3_tag *, struct id3_frame *);
    int (_cdecl* id3_tag_detachframe)(struct id3_tag *, struct id3_frame *);
    struct id3_frame *(_cdecl* id3_tag_findframe)(struct id3_tag const *,
				        char const *, unsigned int);
    signed long (_cdecl* id3_tag_query)(id3_byte_t const *, id3_length_t);
    struct id3_tag *(_cdecl* id3_tag_parse)(id3_byte_t const *, id3_length_t);
    id3_length_t (_cdecl* id3_tag_render)(struct id3_tag const *, id3_byte_t *);

    /* frame interface */
    struct id3_frame *(_cdecl* id3_frame_new)(char const *);
    void (_cdecl* id3_frame_delete)(struct id3_frame *);
    union id3_field *(_cdecl* id3_frame_field)(struct id3_frame const *, unsigned int);

    /* field interface */
    enum id3_field_type (_cdecl* id3_field_type)(union id3_field const *);
    int (_cdecl* id3_field_setint)(union id3_field *, signed long);
    int (_cdecl* id3_field_settextencoding)(union id3_field *, enum id3_field_textencoding);
    int (_cdecl* id3_field_setstrings)(union id3_field *, unsigned int, id3_ucs4_t **);
    int (_cdecl* id3_field_addstring)(union id3_field *, id3_ucs4_t const *);
    int (_cdecl* id3_field_setlanguage)(union id3_field *, char const *);
    int (_cdecl* id3_field_setlatin1)(union id3_field *, id3_latin1_t const *);
    int (_cdecl* id3_field_setfulllatin1)(union id3_field *, id3_latin1_t const *);
    int (_cdecl* id3_field_setstring)(union id3_field *, id3_ucs4_t const *);
    int (_cdecl* id3_field_setfullstring)(union id3_field *, id3_ucs4_t const *);
    int (_cdecl* id3_field_setframeid)(union id3_field *, char const *);
    int (_cdecl* id3_field_setbinarydata)(union id3_field *,
			        id3_byte_t const *, id3_length_t);

    signed long (_cdecl* id3_field_getint)(union id3_field const *);
    enum id3_field_textencoding (_cdecl* id3_field_gettextencoding)(union id3_field const *);
    id3_latin1_t const *(_cdecl* id3_field_getlatin1)(union id3_field const *);
    id3_latin1_t const *(_cdecl* id3_field_getfulllatin1)(union id3_field const *);
    id3_ucs4_t const *(_cdecl* id3_field_getstring)(union id3_field const *);
    id3_ucs4_t const *(_cdecl* id3_field_getfullstring)(union id3_field const *);
    unsigned int (_cdecl* id3_field_getnstrings)(union id3_field const *);
    id3_ucs4_t const *(_cdecl* id3_field_getstrings)(union id3_field const *,
				          unsigned int);
    char const *(_cdecl* id3_field_getframeid)(union id3_field const *);
    id3_byte_t const *(_cdecl* id3_field_getbinarydata)(union id3_field const *,
					      id3_length_t *);

    /* genre interface */
    id3_ucs4_t const *(_cdecl* id3_genre_index)(unsigned int);
    id3_ucs4_t const *(_cdecl* id3_genre_name)(id3_ucs4_t const *);
    int (_cdecl* id3_genre_number)(id3_ucs4_t const *);

    /* ucs4 interface */
    id3_latin1_t *(_cdecl* id3_ucs4_latin1duplicate)(id3_ucs4_t const *);
    id3_utf16_t *(_cdecl* id3_ucs4_utf16duplicate)(id3_ucs4_t const *);
    id3_utf8_t *(_cdecl* id3_ucs4_utf8duplicate)(id3_ucs4_t const *);
    void (_cdecl* id3_ucs4_putnumber)(id3_ucs4_t *, unsigned long);
    unsigned long (_cdecl* id3_ucs4_getnumber)(id3_ucs4_t const *);
    void (_cdecl* id3_ucs4_free)(id3_ucs4_t *);

    /* latin1/utf16/utf8 interfaces */
    id3_ucs4_t *(_cdecl* id3_latin1_ucs4duplicate)(id3_latin1_t const *);
    id3_ucs4_t *(_cdecl* id3_utf16_ucs4duplicate)(id3_utf16_t const *);
    id3_ucs4_t *(_cdecl* id3_utf8_ucs4duplicate)(id3_utf8_t const *);
    void (_cdecl* id3_latin1_free)(id3_latin1_t *);
    void (_cdecl* id3_utf16_free)(id3_utf16_t *);
    void (_cdecl* id3_utf8_free)(id3_utf8_t *);

    /* metadata interface */
    const id3_ucs4_t* (_cdecl* id3_metadata_getartist)(const struct id3_tag*);
    const id3_ucs4_t* (_cdecl* id3_metadata_getalbum)(const struct id3_tag*);
    const id3_ucs4_t* (_cdecl* id3_metadata_gettitle)(const struct id3_tag*);
    const id3_ucs4_t* (_cdecl* id3_metadata_gettrack)(const struct id3_tag*);
    const id3_ucs4_t* (_cdecl* id3_metadata_getpartofset)(const struct id3_tag* tag);
    const id3_ucs4_t* (_cdecl* id3_metadata_getyear)(const struct id3_tag*);
    const id3_ucs4_t* (_cdecl* id3_metadata_getgenre)(const struct id3_tag*);
    const id3_ucs4_t* (_cdecl* id3_metadata_getcomment)(const struct id3_tag*);
    const id3_ucs4_t* (_cdecl* id3_metadata_getencodedby)(const struct id3_tag* tag);
    int (_cdecl* id3_metadata_haspicture)(const struct id3_tag*, enum id3_picture_type);
    const id3_latin1_t* (_cdecl* id3_metadata_getpicturemimetype)(const struct id3_tag*, enum id3_picture_type);
    id3_byte_t const *(_cdecl* id3_metadata_getpicturedata)(const struct id3_tag*, enum id3_picture_type, id3_length_t*);
    id3_byte_t const* (_cdecl* id3_metadata_getuniquefileidentifier)(const struct id3_tag*, const char* owner_identifier, id3_length_t*);
    const id3_ucs4_t* (_cdecl* id3_metadata_getusertext)(const struct id3_tag*, const char* description);
    int (_cdecl* id3_metadata_getfirstnonstandardpictype)(const struct id3_tag*, enum id3_picture_type*);
    int (_cdecl* id3_metadata_setartist)(struct id3_tag* tag, id3_ucs4_t* value);
    int (_cdecl* id3_metadata_setalbum)(struct id3_tag* tag, id3_ucs4_t* value);
    int (_cdecl* id3_metadata_settitle)(struct id3_tag* tag, id3_ucs4_t* value);
    int (_cdecl* id3_metadata_settrack)(struct id3_tag* tag, id3_ucs4_t* value);
    int (_cdecl* id3_metadata_setpartofset)(struct id3_tag* tag, id3_ucs4_t* value);
    int (_cdecl* id3_metadata_setyear)(struct id3_tag* tag, id3_ucs4_t* value);
    int (_cdecl* id3_metadata_setgenre)(struct id3_tag* tag, id3_ucs4_t* value);
    int (_cdecl* id3_metadata_setencodedby)(struct id3_tag* tag, id3_ucs4_t* value);
  };

public:
  CID3Tag(void);
  virtual ~CID3Tag(void);
  virtual bool Read(const CStdString& strFile);
  virtual bool Write(const CStdString& strFile);

  CStdString ParseMP3Genre(const CStdString& str) const;

protected:
  bool Parse();
  void ParseReplayGainInfo();

  CStdString GetArtist() const;
  CStdString GetAlbum() const;
  CStdString GetTitle() const;
  int GetTrack() const;
  int GetPartOfSet() const;
  CStdString GetYear() const;
  CStdString GetGenre() const;
  CStdString GetComment() const;
  CStdString GetEncodedBy() const;

  bool HasPicture(id3_picture_type pictype) const;
  CStdString GetPictureMimeType(id3_picture_type pictype) const;
  const BYTE* GetPictureData(id3_picture_type pictype, id3_length_t* length) const;
  const BYTE* GetUniqueFileIdentifier(const CStdString& strOwnerIdentifier, id3_length_t* lenght) const;
  CStdString GetUserText(const CStdString& strDescription) const;
  bool GetFirstNonStandardPictype(id3_picture_type* pictype) const;

  void SetArtist(const CStdString& strValue);
  void SetAlbum(const CStdString& strValue);
  void SetTitle(const CStdString& strValue);
  void SetTrack(int n);
  void SetPartOfSet(int n);
  void SetYear(const CStdString& strValue);
  void SetGenre(const CStdString& strValue);
  void SetEncodedBy(const CStdString& strValue);

  CStdString Ucs4ToStringCharset(const id3_ucs4_t* ucs4) const;
  id3_ucs4_t* StringCharsetToUcs4(const CStdString& str) const;

  bool LoadDLL();                     // load the DLL in question
  bool m_bDllLoaded;                  // whether our dll is loaded
  ID3dll m_dll;

  id3_tag* m_tag;
};
};
