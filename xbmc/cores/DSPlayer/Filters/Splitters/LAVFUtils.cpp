/*
*      Copyright (C) 2010 Team XBMC
*      http://www.xbmc.org
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/

#include "lavfutils.h"

extern "C"
{
#include "libavcodec/audioconvert.h"
}

#include <sstream>

static int get_bit_rate(AVCodecContext *ctx)
{
  DllAvCodec dllAvCodec;
  dllAvCodec.Load();
  int bit_rate;
  int bits_per_sample;

  switch (ctx->codec_type)
  {
  case AVMEDIA_TYPE_VIDEO:
  case AVMEDIA_TYPE_DATA:
  case AVMEDIA_TYPE_SUBTITLE:
  case AVMEDIA_TYPE_ATTACHMENT:
    bit_rate = ctx->bit_rate;
    break;
  case AVMEDIA_TYPE_AUDIO:
    bits_per_sample = dllAvCodec.av_get_bits_per_sample(ctx->codec_id);
    bit_rate = bits_per_sample ? ctx->sample_rate * ctx->channels * bits_per_sample : ctx->bit_rate;
    break;
  default:
    bit_rate = 0;
    break;
  }
  return bit_rate;
}

const char *get_stream_language(AVStream *pStream)
{
  char *lang = NULL;
  DllAvFormat dllAvFormat;
  dllAvFormat.Load();
  if (dllAvFormat.av_metadata_get(pStream->metadata, "language", NULL, 0))
  {
    lang = dllAvFormat.av_metadata_get(pStream->metadata, "language", NULL, 0)->value;
  }
  else if (pStream->language && strlen(pStream->language) > 0)
  {
    lang = pStream->language;
  }
  return lang;
}

HRESULT lavf_describe_stream(AVStream *pStream, WCHAR **ppszName)
{
  CheckPointer(pStream, E_POINTER);
  CheckPointer(ppszName, E_POINTER);

  DllAvFormat dllAvFormat;
  DllAvCodec dllAvCodec;
  dllAvFormat.Load();
  dllAvCodec.Load();
  AVCodecContext *enc = pStream->codec;

  char tmpbuf1[32];

  // Grab the codec
  const char *codec_name;
  AVCodec *p = dllAvCodec.avcodec_find_decoder(enc->codec_id);

  if (p)
  {
    codec_name = p->name;
  }
  else if (enc->codec_name[0] != '\0')
  {
    codec_name = enc->codec_name;
  }
  else
  {
    /* output avi tags */
    char tag_buf[32];
    dllAvCodec.av_get_codec_tag_string(tag_buf, sizeof(tag_buf), enc->codec_tag);
    sprintf_s(tmpbuf1, "%s / 0x%04X", tag_buf, enc->codec_tag);
    codec_name = tmpbuf1;
  }

  const char *lang = get_stream_language(pStream);
  std::string sLanguage;

  if (lang)
  {
    sLanguage = ProbeLangForLanguage(lang);
    if (sLanguage.empty())
    {
      sLanguage = lang;
    }
  }

  char *title = NULL;

  if (dllAvFormat.av_metadata_get(pStream->metadata, "title", NULL, 0))
  {
    title = dllAvFormat.av_metadata_get(pStream->metadata, "title", NULL, 0)->value;
  }

  int bitrate = get_bit_rate(enc);


  std::ostringstream buf;
  switch (enc->codec_type)
  {
  case AVMEDIA_TYPE_VIDEO:
    buf << "V: ";
    // Title/Language
    if (title && lang)
    {
      buf << title << " [" << lang << "] (";
    }
    else if (title || lang)
    {
      // Print either title or lang
      buf << (title ? title : sLanguage) << " (";
    }
    // Codec
    buf << codec_name;
    // Pixel Format
    if (enc->pix_fmt != PIX_FMT_NONE)
    {
      buf << ", " << dllAvCodec.avcodec_get_pix_fmt_name(enc->pix_fmt);
    }
    // Dimensions
    if (enc->width)
    {
      buf << ", " << enc->width << "x" << enc->height;
    }
    // Bitrate
    if (bitrate > 0)
    {
      buf << ", " << (bitrate / 1000) << " kb/s";
    }
    // Closing tag
    if (title || lang)
    {
      buf << ")";
    }
    break;
  case AVMEDIA_TYPE_AUDIO:
    buf << "A: ";
    // Title/Language
    if (title && lang)
    {
      buf << title << " [" << lang << "] (";
    }
    else if (title || lang)
    {
      // Print either title or lang
      buf << (title ? title : sLanguage) << " (";
    }
    // Codec
    buf << codec_name;
    // Sample Rate
    if (enc->sample_rate)
    {
      buf << ", " << enc->sample_rate << " Hz";
    }
    // Get channel layout
    char channel[32];
    dllAvCodec.avcodec_get_channel_layout_string(channel, 32, enc->channels, enc->channel_layout);
    buf << ", " << channel;
    // Sample Format
    if (enc->sample_fmt != SAMPLE_FMT_NONE)
    {
      buf << ", " << dllAvCodec.avcodec_get_sample_fmt_name(enc->sample_fmt);
    }
    // Bitrate
    if (bitrate > 0)
    {
      buf << ", " << (bitrate / 1000) << " kb/s";
    }
    // Closing tag
    if (title || lang)
    {
      buf << ")";
    }
    break;
  case AVMEDIA_TYPE_SUBTITLE:
    buf << "S: ";
    // Title/Language
    if (title && lang)
    {
      buf << title << " [" << lang << "]";
    }
    else if (title || lang)
    {
      // Print either title or lang
      buf << (title ? title : sLanguage);
    }
    else
    {
      buf << "Stream #" << pStream->index;
    }
    break;
  default:
    buf << "Unknown: Stream " << pStream->index;
    break;
  }

  std::string info = buf.str();
  size_t len = info.size() + 1;
  *ppszName = (WCHAR *)CoTaskMemAlloc(len * sizeof(WCHAR));
  MultiByteToWideChar(CP_UTF8, 0, info.c_str(), -1, *ppszName, (int)len);

  return S_OK;
}
