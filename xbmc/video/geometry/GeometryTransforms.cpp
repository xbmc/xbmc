/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GeometryTransforms.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace KODI::VIDEO::GEOMETRY
{

int NormaliseRotation(int degrees)
{
  const int wrapped = ((degrees % 360) + 360) % 360;
  return wrapped % 90 == 0 ? wrapped : 0;
}

CRect Rotate(const CRect& rect, const CRect& frame, int orientation)
{
  const float width = frame.Width();
  const float height = frame.Height();

  switch (orientation)
  {
    case 90:
      return {height - rect.y2, rect.x1, height - rect.y1, rect.x2};
    case 180:
      return {width - rect.x2, height - rect.y2, width - rect.x1, height - rect.y1};
    case 270:
      return {rect.y1, width - rect.x2, rect.y2, width - rect.x1};
    default:
      return rect;
  }
}

float AspectOf(const CRect& rect)
{
  return rect.Height() > 0.0f ? rect.Width() / rect.Height() : 0.0f;
}

CRectInt OriginSizeRect(int x, int y, int width, int height)
{
  return {x, y, x + width, y + height};
}

int EdgeDistance(const CRectInt& a, const CRectInt& b)
{
  return std::max(
      {std::abs(a.x1 - b.x1), std::abs(a.y1 - b.y1), std::abs(a.x2 - b.x2), std::abs(a.y2 - b.y2)});
}

bool EdgesWithin(const CRectInt& a, const CRectInt& b, unsigned int tolerance)
{
  return EdgeDistance(a, b) <= static_cast<int>(tolerance);
}

CRect FractionOf(const CRect& rect, const CRect& frame)
{
  return {(rect.x1 - frame.x1) / frame.Width(), (rect.y1 - frame.y1) / frame.Height(),
          (rect.x2 - frame.x1) / frame.Width(), (rect.y2 - frame.y1) / frame.Height()};
}

CRect FromFraction(const CRect& fraction, const CRect& frame)
{
  return {frame.x1 + fraction.x1 * frame.Width(), frame.y1 + fraction.y1 * frame.Height(),
          frame.x1 + fraction.x2 * frame.Width(), frame.y1 + fraction.y2 * frame.Height()};
}

CRectInt StereoViewRect(const std::string& stereoMode, unsigned int width, unsigned int height)
{
  const int w = static_cast<int>(width);
  const int h = static_cast<int>(height);

  if (stereoMode == "left_right" || stereoMode == "right_left")
    return {0, 0, w / 2, h};

  if (stereoMode == "top_bottom" || stereoMode == "bottom_top")
    return {0, 0, w, h / 2};

  // Anything else - mono, empty, or a packing we do not recognise - is left whole: halving a
  // mono frame would report a pillarbox that is not there, and mask real picture.
  return {};
}

CRectInt MeasuredFrameRect(const std::string& stereoMode, unsigned int width, unsigned int height)
{
  const CRectInt view{StereoViewRect(stereoMode, width, height)};
  return view.IsEmpty() ? CRectInt{0, 0, static_cast<int>(width), static_cast<int>(height)} : view;
}

CRectInt ScaleRect(const CRectInt& rect,
                   unsigned int fromWidth,
                   unsigned int fromHeight,
                   unsigned int toWidth,
                   unsigned int toHeight)
{
  if (fromWidth == 0 || fromHeight == 0 || toWidth == 0 || toHeight == 0)
    return rect;

  const double scaleX = static_cast<double>(toWidth) / fromWidth;
  const double scaleY = static_cast<double>(toHeight) / fromHeight;
  return {static_cast<int>(std::lround(rect.x1 * scaleX)),
          static_cast<int>(std::lround(rect.y1 * scaleY)),
          static_cast<int>(std::lround(rect.x2 * scaleX)),
          static_cast<int>(std::lround(rect.y2 * scaleY))};
}

StreamGeometry MeasuredStreamGeometry(const std::string& stereoMode,
                                      unsigned int width,
                                      unsigned int height,
                                      float displayAspect)
{
  StreamGeometry stream;
  stream.coded = MeasuredFrameRect(stereoMode, width, height);
  stream.displayAspect = displayAspect;
  return stream;
}

float PixelAspectRatio(const StreamGeometry& stream)
{
  const float width = static_cast<float>(stream.coded.Width());
  const float height = static_cast<float>(stream.coded.Height());
  if (stream.displayAspect <= 0.0f || width <= 0.0f || height <= 0.0f)
    return 1.0f;

  const float par = stream.displayAspect * height / width;

  // A stream whose declared display ratio is its coded ratio has square pixels, and should come
  // back out at exactly the frame it went in as. The smallest pixel aspect any real anamorphic
  // coding uses is several percent from square, so nothing genuine is rounded away.
  return std::abs(par - 1.0f) < 0.001f ? 1.0f : par;
}

CRect ToSquarePixels(const CRectInt& coded, const StreamGeometry& stream)
{
  const float par = PixelAspectRatio(stream);

  // Relative to the frame, so that the result always has the frame at the origin. A
  // stereoscopic scan measures one view, whose frame is an offset region of the picture.
  return {static_cast<float>(coded.x1 - stream.coded.x1) * par,
          static_cast<float>(coded.y1 - stream.coded.y1),
          static_cast<float>(coded.x2 - stream.coded.x1) * par,
          static_cast<float>(coded.y2 - stream.coded.y1)};
}

CRect ToDisplaySpace(const CRectInt& coded, const StreamGeometry& stream)
{
  return Rotate(ToSquarePixels(coded, stream), ToSquarePixels(stream.coded, stream),
                NormaliseRotation(stream.orientation));
}

CRect FitAspect(float aspect, const CRect& frame)
{
  if (aspect <= 0.0f || frame.Width() <= 0.0f || frame.Height() <= 0.0f)
    return frame;

  float width = frame.Width();
  float height = width / aspect;
  if (height > frame.Height())
  {
    height = frame.Height();
    width = height * aspect;
  }

  // A ratio the frame is already in gives back the frame rather than a rectangle a fraction of
  // a pixel inside it. The vocabulary's entries are percent apart, so nothing real is snapped.
  constexpr float EXACT = 0.01f;
  if (frame.Width() - width < EXACT)
    width = frame.Width();
  if (frame.Height() - height < EXACT)
    height = frame.Height();

  const float x = frame.x1 + (frame.Width() - width) / 2.0f;
  const float y = frame.y1 + (frame.Height() - height) / 2.0f;
  return {x, y, x + width, y + height};
}

CRect MaintainedRect(float maintainAspect, float contentAspect, const CRect& operatingArea)
{
  // Both absent cases fall out of FitAspect(), which gives back the frame for a non-positive
  // ratio: no maintain leaves the operating area whole, unknown content fills the maintain.
  return FitAspect(contentAspect, FitAspect(maintainAspect, operatingArea));
}

bool DescribesFrame(const RenderGeometry& geometry, const CRect& frameSource)
{
  // The ratio first: it is zero until something has been resolved, and a default-constructed
  // geometry has a zero frame that would otherwise match a zero source.
  return geometry.aspect > 0.0f &&
         geometry.codedFrame.Width() == static_cast<int>(frameSource.Width()) &&
         geometry.codedFrame.Height() == static_cast<int>(frameSource.Height());
}

CRect SourceRect(const RenderGeometry& geometry, const CRect& frameSource)
{
  const CRect& frame = geometry.displayFrame;
  if (frame.Width() <= 0.0f || frame.Height() <= 0.0f || frameSource.Width() <= 0.0f ||
      frameSource.Height() <= 0.0f)
    return frameSource;

  // A quarter turn is undone by three more.
  const CRect unit{0.0f, 0.0f, 1.0f, 1.0f};
  const CRect fraction = Rotate(FractionOf(geometry.displayRect, frame), unit,
                                (360 - NormaliseRotation(geometry.orientation)) % 360);

  return FromFraction(fraction, frameSource);
}

ContentCut CutToContent(const RenderGeometry& geometry,
                        const CRectInt& content,
                        const CRect& frameSource)
{
  // Nothing was measured on this frame - live detection is off, or no reading yet - so the
  // source is cut to the rectangle the resolver settled on. Returning it uncut while reporting
  // the resolved ratio would leave the coded bars on screen.
  if (content.IsEmpty() || geometry.codedFrame.Width() <= 0 || geometry.codedFrame.Height() <= 0)
    return {SourceRect(geometry, frameSource), geometry.aspect};

  // Expressed against the coded frame rather than the display one: the two differ by a uniform
  // scale on x, and SourceRect works in fractions.
  const CRectInt& coded = geometry.codedFrame;

  // Left unrotated: content and coded are in the space frameSource is in, so there is no
  // display-space turn for SourceRect() to undo. The ratio below still answers as the rotation
  // leaves it, which is why only that half of the result carries the orientation.
  RenderGeometry cut;
  cut.displayFrame = CRect{coded};
  cut.displayRect = CRect{content};

  const float ratio =
      static_cast<float>(content.Width()) * geometry.par / static_cast<float>(content.Height());

  return {SourceRect(cut, frameSource),
          NormaliseRotation(geometry.orientation) % 180 == 0 ? ratio : 1.0f / ratio};
}

} // namespace KODI::VIDEO::GEOMETRY
