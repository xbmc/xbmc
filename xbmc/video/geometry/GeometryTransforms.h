/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Geometry.h"

#include <string>
#include <string_view>

namespace KODI::VIDEO::GEOMETRY
{

//! \brief What is known about the stream being played, as coded.
struct StreamGeometry
{
  //! The coded frame, before pixel-aspect correction and before rotation.
  CRectInt coded;

  float displayAspect{0.0f}; //!< the displayed ratio, not the coded one; zero for unknown
  int orientation{0}; //!< clockwise rotation applied for display, in degrees
};

//! \brief \p degrees reduced to a quarter turn; anything else is upright.
int NormaliseRotation(int degrees);

//! \brief Turn \p rect clockwise within \p frame, both upright and at the origin.
CRect Rotate(const CRect& rect, const CRect& frame, int orientation);

float AspectOf(const CRect& rect);

//! \brief The corner-to-corner rectangle an origin and a size describe.
CRectInt OriginSizeRect(int x, int y, int width, int height);

//! \brief Chebyshev distance between two rectangles: the furthest any single edge has moved.
int EdgeDistance(const CRectInt& a, const CRectInt& b);

//! \brief Are these the same boundary within \p tolerance, edge for edge?
bool EdgesWithin(const CRectInt& a, const CRectInt& b, unsigned int tolerance);

//! \brief \p rect as fractions of \p frame.
CRect FractionOf(const CRect& rect, const CRect& frame);

//! \brief The rectangle \p fraction describes within \p frame - FractionOf()'s inverse.
CRect FromFraction(const CRect& fraction, const CRect& frame);

//! \brief The first view of a packed stereoscopic frame, or an empty rectangle for mono.
CRectInt StereoViewRect(std::string_view stereoMode, unsigned int width, unsigned int height);

//! \brief The region a measurement describes: one view of a packed frame, else the whole.
CRectInt MeasuredFrameRect(std::string_view stereoMode, unsigned int width, unsigned int height);

//! \brief \p rect read in one coordinate space, expressed in another of the same shape.
CRectInt ScaleRect(const CRectInt& rect,
                   unsigned int fromWidth,
                   unsigned int fromHeight,
                   unsigned int toWidth,
                   unsigned int toHeight);

//! \brief What a measurement of a playing stream is expressed against, \p displayAspect being
//! the ratio the whole packed frame is displayed at.
StreamGeometry MeasuredStreamGeometry(std::string_view stereoMode,
                                      unsigned int width,
                                      unsigned int height,
                                      float displayAspect);

//! \brief Width of a coded pixel in display pixels; 1 when the stream declares no ratio.
float PixelAspectRatio(const StreamGeometry& stream);

//! \brief Undo the coding's pixel aspect, leaving the picture oriented as it was coded.
CRect ToSquarePixels(const CRectInt& coded, const StreamGeometry& stream);

//! \brief A coded rectangle carried to the screen: pixel-aspect correction, then rotation.
CRect ToDisplaySpace(const CRectInt& coded, const StreamGeometry& stream);

//! \brief The largest rectangle of ratio \p aspect that fits centred inside \p frame.
CRect FitAspect(float aspect, const CRect& frame);

//! \brief Where content of \p contentAspect is drawn inside the \p maintainAspect area of
//! \p operatingArea. Either ratio may be zero, meaning the enclosing rectangle is filled.
CRect MaintainedRect(float maintainAspect, float contentAspect, const CRect& operatingArea);

//! \brief The resolved geometry reduced to what a render path reads.
struct RenderGeometry
{
  CRectInt codedFrame;
  CRect displayFrame; //!< the resolved frame, which the rectangle below is a region of
  CRect displayRect; //!< the content rectangle in force for the title
  float aspect{0.0f}; //!< zero until something has been resolved
  float par{1.0f};
  int orientation{0};
};

//! \brief Whether \p geometry was resolved from a frame of \p frameSource's shape.
bool DescribesFrame(const RenderGeometry& geometry, const CRect& frameSource);

//! \brief The part of \p frameSource holding content, in its own coordinates.
CRect SourceRect(const RenderGeometry& geometry, const CRect& frameSource);

//! \brief What a renderer needs to cut its source to one frame's own measurement.
struct ContentCut
{
  CRect source; //!< the region of the source the content occupies
  float aspect{0.0f}; //!< that content's ratio, as the rotation leaves it
};

//! \brief \p frameSource cut to \p content, a rectangle measured from one frame in the space
//! of RenderGeometry::codedFrame. The pixel aspect and rotation come from \p geometry.
ContentCut CutToContent(const RenderGeometry& geometry,
                        const CRectInt& content,
                        const CRect& frameSource);

} // namespace KODI::VIDEO::GEOMETRY
