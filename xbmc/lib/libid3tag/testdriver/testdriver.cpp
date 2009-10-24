// testdriver.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "testdriver.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "metadata.h"

// The one and only application object

CWinApp theApp;

using namespace std;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
  int nRetCode = 0;

  // initialize MFC and print and error on failure
  if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
  {
    // TODO: change error code to suit your needs
    _tprintf(_T("Fatal Error: MFC initialization failed\n"));
    nRetCode = 1;
  }
  else
  {
    // TODO: code your application's behavior here.
  }

  struct id3_file *id3file;
  struct id3_tag *tag;
  id3_ucs4_t const *ucs4;
  id3_latin1_t const *latin1;
  id3_utf16_t const *utf16;
  enum id3_field_textencoding encoding;

  if (1)
  {     //  Read tag
    id3file = id3_file_open("test.mp3", ID3_FILE_MODE_READWRITE);
    tag = id3_file_tag(id3file);

    char test2 = id3_metadata_getrating(tag);

    ucs4 = id3_metadata_getcomment(tag, &encoding);
    utf16 = id3_ucs4_utf16duplicate(ucs4);
    OutputDebugStringW((wchar_t*)utf16);
    OutputDebugString("\n");

    const id3_latin1_t *test = id3_metadata_getpicturemimetype(tag, ID3_PICTURE_TYPE_OTHER);
    ucs4=id3_metadata_getartist(tag, &encoding);
    utf16 = id3_ucs4_utf16duplicate(ucs4);
    OutputDebugStringW((wchar_t*)utf16);
    OutputDebugString("\n");

    ucs4=id3_metadata_getalbum(tag, &encoding);
    utf16 = id3_ucs4_utf16duplicate(ucs4);
    OutputDebugStringW((wchar_t*)utf16);
    OutputDebugString("\n");

    ucs4=id3_metadata_getalbumartist(tag, &encoding);
    utf16 = id3_ucs4_utf16duplicate(ucs4);
    OutputDebugStringW((wchar_t*)utf16);
    OutputDebugString("\n");

    ucs4=id3_metadata_gettitle(tag, &encoding);
    utf16 = id3_ucs4_utf16duplicate(ucs4);
    OutputDebugStringW((wchar_t*)utf16);
    OutputDebugString("\n");

    ucs4=id3_metadata_gettrack(tag, &encoding);
    latin1 = id3_ucs4_latin1duplicate(ucs4);
    OutputDebugString((LPCSTR)latin1);
    OutputDebugString("\n");

    ucs4=id3_metadata_getpartofset(tag, &encoding);
    latin1 = id3_ucs4_latin1duplicate(ucs4);
    OutputDebugString((LPCSTR)latin1);
    OutputDebugString("\n");

    ucs4=id3_metadata_getyear(tag, &encoding);
    latin1 = id3_ucs4_latin1duplicate(ucs4);
    OutputDebugString((LPCSTR)latin1);
    OutputDebugString("\n");

    ucs4=id3_metadata_getgenre(tag, &encoding);
    latin1 = id3_ucs4_latin1duplicate(ucs4);
    OutputDebugString((LPCSTR)latin1);
    OutputDebugString("\n");

    id3_ucs4_list_t *list = id3_metadata_getgenres(tag, &encoding);
    if (list)
    {
      for (unsigned int j = 0; j < list->nstrings; j++)
      {
        ucs4=list->strings[j];
        latin1 = id3_ucs4_latin1duplicate(ucs4);
        OutputDebugString((LPCSTR)latin1);
        OutputDebugString("\n");
      }
      id3_ucs4_list_free(list);
    }
    ucs4=id3_metadata_getcomment(tag, &encoding);
    latin1 = id3_ucs4_latin1duplicate(ucs4);
    OutputDebugString((LPCSTR)latin1);
    OutputDebugString("\n");

    id3_metadata_haspicture(tag, ID3_PICTURE_TYPE_COVERFRONT);

    latin1=id3_metadata_getpicturemimetype(tag, ID3_PICTURE_TYPE_COVERFRONT);
    OutputDebugString((LPCSTR)latin1);
    OutputDebugString("\n");

    id3_length_t length;
    const id3_byte_t* byte=id3_metadata_getpicturedata(tag, ID3_PICTURE_TYPE_COVERFRONT, &length);
    
    latin1=(id3_latin1_t*)id3_metadata_getuniquefileidentifier(tag, "http://musicbrainz.org", &length);
    OutputDebugString((LPCSTR)latin1);
    OutputDebugString("\n");

    ucs4=id3_metadata_getusertext(tag, "MusicBrainz TRM Id");
    latin1 = id3_ucs4_latin1duplicate(ucs4);
    OutputDebugString((LPCSTR)latin1);
    OutputDebugString("\n");


    id3_file_close(id3file);
  }
  else
  {
    const char* file="Writetest.mp3";

    OutputDebugString("Write tag\n");
    OutputDebugString("---------\n");

    id3file = id3_file_open(file, ID3_FILE_MODE_READWRITE);
    tag = id3_file_tag(id3file);

    const char* latin1="The little new artist";
    id3_ucs4_t* ucs4=id3_latin1_ucs4duplicate((const id3_latin1_t*)latin1);
    id3_metadata_setartist(tag, ucs4);
    OutputDebugString("Artist written: ");
    OutputDebugString(latin1);
    OutputDebugString("\n");
    id3_ucs4_free(ucs4);

    id3_tag_options(tag, ID3_TAG_OPTION_COMPRESSION, 0);
    id3_tag_options(tag, ID3_TAG_OPTION_CRC, 0);
    id3_tag_options(tag, ID3_TAG_OPTION_UNSYNCHRONISATION, 0);
    id3_tag_options(tag, ID3_TAG_OPTION_ID3V1, 1);

    id3_file_update(id3file);

    id3_file_close(id3file);

    id3file = id3_file_open(file, ID3_FILE_MODE_READWRITE);
    tag = id3_file_tag(id3file);

    const id3_ucs4_t* artist=id3_metadata_getartist(tag, &encoding);
    id3_latin1_t* artistl=id3_ucs4_latin1duplicate(artist);
    OutputDebugString("Artist read: ");
    OutputDebugString((LPCSTR)artistl);
    OutputDebugString("\n");
    id3_latin1_free(artistl);

    id3_file_close(id3file);
  }

  return nRetCode;
}
