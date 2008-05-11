#pragma once

#include "DllLibass.h"

extern "C"{
  #include "libass/ass.h"
}

/** Wrapper for Libass **/

class CDVDSubtitlesLibass
{
public:
  CDVDSubtitlesLibass();
  ~CDVDSubtitlesLibass();

  ass_image_t* RenderImage(int imageWidth, int imageHeight, double pts);
  ass_event_t* GetEvents();

  int GetNrOfEvents();

  bool DecodeHeader(char* data, int size);
  bool DecodeDemuxPkt(char* data, int size, double start, double duration);
  bool ReadFile(const std::string& strFile);

  long GetNrOfReferences();
  long Acquire();
  long Release();

private:
  bool hasHeader;
  DllLibass m_dll;
  long m_references;
  ass_library_t* m_library;
  ass_track_t* m_track;
  ass_renderer_t* m_renderer;
};

