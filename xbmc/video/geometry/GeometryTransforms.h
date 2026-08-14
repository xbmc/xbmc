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

/*!
 * \file GeometryTransforms.h
 * \brief The pure rectangle arithmetic of content geometry, which render paths run per frame.
 */

namespace KODI::VIDEO::GEOMETRY
{

//! \brief What is known about the stream being played, as coded.
struct StreamGeometry
{
  //! The coded frame, before pixel-aspect correction and before rotation.
  CRectInt coded;

  //! \brief The ratio the stream is displayed at, which is not its ratio in coded pixels. Zero
  //! or negative means unknown, and is taken as square pixels.
  float displayAspect{0.0f};

  //! \brief Clockwise rotation applied to the picture for display, in degrees.
  int orientation{0};
};

//! \brief Reduce a stated rotation to the quarter turn a rectangle can carry. Anything else is
//! treated as upright.
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

//! \brief \p rect as fractions of \p frame, so it can be re-expressed against another frame
//! whatever scale or pixel aspect separates the two.
CRect FractionOf(const CRect& rect, const CRect& frame);

//! \brief The rectangle \p fraction describes within \p frame - FractionOf()'s inverse.
CRect FromFraction(const CRect& fraction, const CRect& frame);

/*!
 * \brief The region of a packed stereoscopic frame holding a single view. Both views carry the
 *        same geometry, so this is always the first.
 *
 * \return the view's rectangle, or an empty rectangle for mono content, meaning the whole
 *         frame
 */
CRectInt StereoViewRect(const std::string& stereoMode, unsigned int width, unsigned int height);

//! \brief The region of a coded frame a measurement describes: one view when the frame packs
//! two, the whole frame otherwise.
CRectInt MeasuredFrameRect(const std::string& stereoMode, unsigned int width, unsigned int height);

/*!
 * \brief A rectangle read in one coordinate space, expressed in another of the same shape.
 *
 * \return the scaled rectangle, or \p rect unchanged when either space is degenerate
 */
CRectInt ScaleRect(const CRectInt& rect,
                   unsigned int fromWidth,
                   unsigned int fromHeight,
                   unsigned int toWidth,
                   unsigned int toHeight);

/*!
 * \brief What a measurement of a playing stream is expressed against: the measured region and
 *        the pixel aspect that region implies, established together.
 *
 * \param displayAspect the ratio the whole packed frame is displayed at, or zero when the
 *        stream declares none
 */
StreamGeometry MeasuredStreamGeometry(const std::string& stereoMode,
                                      unsigned int width,
                                      unsigned int height,
                                      float displayAspect);

/*!
 * \brief Width of a coded pixel in display pixels.
 *
 * \return 1 when the stream declares no display aspect, so that content with no anamorphic
 *         coding is unaffected rather than undefined
 */
float PixelAspectRatio(const StreamGeometry& stream);

//! \brief Undo the coding's pixel aspect, leaving the picture oriented as it was coded.
CRect ToSquarePixels(const CRectInt& coded, const StreamGeometry& stream);

//! \brief Carry a coded rectangle all the way to what is on the screen: pixel-aspect correction
//! first, then rotation.
CRect ToDisplaySpace(const CRectInt& coded, const StreamGeometry& stream);

//! \brief The largest rectangle of ratio \p aspect that fits centred inside \p frame.
CRect FitAspect(float aspect, const CRect& frame);

/*!
 * \brief Where content of ratio \p contentAspect is drawn when this playback has been told to
 *        occupy the \p maintainAspect area of \p operatingArea.
 *
 * The target nests inside the operating area rather than replacing it, so everything stays
 * contained by the raster.
 *
 * \param maintainAspect the ratio to render into, or zero to use the whole operating area
 * \param contentAspect the content's own ratio, or zero when it is not known, in which case
 *        the maintained target is filled rather than guessed at
 */
CRect MaintainedRect(float maintainAspect, float contentAspect, const CRect& operatingArea);

//! \brief The resolved geometry reduced to what a render path reads, taken per rendered frame.
struct RenderGeometry
{
  CRectInt codedFrame;
  CRect displayFrame; //!< the resolved frame, which the rectangle below is a region of
  CRect displayRect; //!< what the resolver settled on for the title
  float aspect{0.0f}; //!< zero until something has been resolved
  float par{1.0f};
  int orientation{0};
};

/*!
 * \brief Does \p geometry describe the region being rendered? A resolved geometry outlives the
 *        frame it was resolved from, so a render path asks before cutting to it.
 *
 * \param frameSource the region of the coded frame being rendered - the whole frame, or one
 *        view of a stereoscopic frame
 */
bool DescribesFrame(const RenderGeometry& geometry, const CRect& frameSource);

/*!
 * \brief The content as a region of what the renderer samples, so bars baked into the coding are
 *        left out of the drawn picture rather than scaled with it.
 *
 * \param frameSource the region of the coded frame being sampled - the whole frame, or one
 *        view of a stereoscopic frame
 * \return the part of \p frameSource holding content, in its coordinates; \p frameSource
 *         itself when nothing is known
 */
CRect SourceRect(const RenderGeometry& geometry, const CRect& frameSource);

//! \brief What a renderer needs to cut its source to one frame's own measurement.
struct ContentCut
{
  CRect source; //!< the region of the source the content occupies
  float aspect{0.0f}; //!< the ratio that content is, named as the rotation leaves it
};

/*!
 * \brief Cut \p frameSource to a content rectangle measured from one particular frame. The pixel
 *        aspect, the rotation and the frame are carried over from \p geometry untouched.
 *
 * \param content the rectangle in coded pixels, in the same space as RenderGeometry::codedFrame
 * \return \p frameSource unchanged, and the geometry's own ratio, when there is no content
 *         rectangle or no frame to express it against
 */
ContentCut CutToContent(const RenderGeometry& geometry,
                        const CRectInt& content,
                        const CRect& frameSource);

} // namespace KODI::VIDEO::GEOMETRY
