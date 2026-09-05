/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContentBarDetector.h"

#include "ContentBarHistogram.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <optional>

namespace KODI::VIDEO::GEOMETRY
{

namespace
{

//! \brief A strided run of samples through one plane, in 8-bit equivalent buckets. Instantiated
//! once for 8-bit planes and once for deeper ones, so the per-sample path has no depth branch.
template<typename T>
class CSampleLine
{
public:
  CSampleLine() = default;

  CSampleLine(const PlaneRef& plane,
              unsigned int shift,
              std::ptrdiff_t origin,
              std::ptrdiff_t step,
              unsigned int count)
    : m_base(plane.data),
      m_shift(shift),
      m_origin(origin),
      m_step(step),
      m_count(count)
  {
  }

  unsigned int Count() const { return m_count; }

  unsigned int operator[](unsigned int index) const
  {
    const T* sample =
        reinterpret_cast<const T*>(m_base + m_origin + static_cast<std::ptrdiff_t>(index) * m_step);
    return static_cast<unsigned int>(*sample) >> m_shift;
  }

private:
  const uint8_t* m_base{nullptr};
  unsigned int m_shift{0};
  std::ptrdiff_t m_origin{0};
  std::ptrdiff_t m_step{0};
  unsigned int m_count{0};
};

//! \brief Identifies one line of the frame: a row, or a column, and the span walked along it.
struct LineSpec
{
  bool row{true};
  unsigned int index{0}; //!< row number, or column number
  unsigned int begin{0}; //!< inclusive start along the line
  unsigned int end{0}; //!< exclusive end along the line
};

enum class LineClass
{
  Bar,
  Overlay,
  Picture,
};

struct LineOutcome
{
  LineClass classification{LineClass::Picture};
  unsigned int high{0}; //!< p99.5 of the bar portion
  unsigned int low{0}; //!< p0.5 of the bar portion
  unsigned int spread{0}; //!< interquartile range of the bar portion
  bool suspectedOverlay{false};
};

//! \brief The robust statistics a line is judged dark-and-flat by.
struct LineStats
{
  unsigned int low{0}; //!< p0.5
  unsigned int spread{0}; //!< interquartile range
  unsigned int high{0}; //!< p99.5
};

LineStats RobustStats(const CHistogram& histogram)
{
  const std::array<unsigned int, 4> quantiles{
      histogram.Percentiles(std::array<double, 4>{0.005, 0.25, 0.75, 0.995})};
  return {quantiles[0], quantiles[2] - quantiles[1], quantiles[3]};
}

template<typename T>
class CDetector
{
public:
  CDetector(const FrameRef& frame, const DetectorParams& params);

  DetectionResult Run();

private:
  CSampleLine<T> Luma(const LineSpec& line) const;
  CSampleLine<T> Chroma(const PlaneRef& plane, const LineSpec& line) const;

  LineOutcome Classify(const LineSpec& line);
  bool ChromaIsNeutral(const LineSpec& line);
  EdgeMetrics MeasureEdge(const LineSpec& bar, const LineSpec& picture, unsigned int thickness);

  LineSpec Row(unsigned int y, unsigned int x0, unsigned int x1) const { return {true, y, x0, x1}; }
  LineSpec Column(unsigned int x, unsigned int y0, unsigned int y1) const
  {
    return {false, x, y0, y1};
  }

  //! \brief Fold one classified bar line into the running quality figures.
  void Accumulate(const LineOutcome& outcome);

  //! \brief Fold the accumulated evidence into the result's quality figures and its confidence.
  //! Evidence about the rect, never a correction to it.
  void ScoreResult(DetectionResult& result);

  /*!
   * \brief Walk lines inward from one edge until the first picture line, accumulating the bar
   * lines. \p step is +1 walking up from \p from, -1 walking down.
   * \return where the walk rested, which is the content boundary on that edge
   */
  template<typename TLine>
  unsigned int Walk(unsigned int from, unsigned int limit, int step, const TLine& line)
  {
    unsigned int edge = from;
    while (step > 0 ? edge < limit : edge > limit)
    {
      const LineOutcome outcome = Classify(line(step > 0 ? edge : edge - 1));
      m_overlaySuspected |= outcome.suspectedOverlay;
      if (outcome.classification == LineClass::Picture)
        break;
      Accumulate(outcome);
      edge = step > 0 ? edge + 1 : edge - 1;
    }
    return edge;
  }

  const FrameRef& m_frame;
  const DetectorParams& m_params;

  CRectInt m_roi;
  unsigned int m_shift{0};
  unsigned int m_bytes{1};
  unsigned int m_threshold{0}; //!< 8-bit equivalent bar threshold
  unsigned int m_fullScale{219}; //!< 8-bit equivalent nominal signal excursion
  unsigned int m_chromaShiftX{1};
  unsigned int m_chromaShiftY{1};
  bool m_hasChroma{false};
  bool m_rangeAssumed{false};

  CHistogram m_histogram;
  CHistogram m_remainder;
  CHistogram m_chromaU;
  CHistogram m_chromaV;

  unsigned int m_barLines{0};
  unsigned int m_overlayLines{0};
  bool m_overlaySuspected{false};
  //! Distributions across bar lines rather than maxima: the marginal line at the transition is
  //! where ringing lives and says nothing about the bar as a whole.
  CHistogram m_lineHighs;
  CHistogram m_lineSpreads;
  unsigned int m_lowestSeen{BUCKET_COUNT - 1};
};

template<typename T>
CDetector<T>::CDetector(const FrameRef& frame, const DetectorParams& params)
  : m_frame(frame),
    m_params(params)
{
  m_shift = frame.bitDepth > 8 ? frame.bitDepth - 8 : 0;
  m_bytes = sizeof(T);

  ColorRange range = frame.range;
  if (range == ColorRange::Unspecified)
  {
    range = ColorRange::Limited;
    m_rangeAssumed = true;
  }

  const unsigned int black = range == ColorRange::Full ? 0u : 16u;
  m_threshold = black + m_params.margin;
  m_fullScale = range == ColorRange::Full ? 255u : 219u;

  m_chromaShiftX = frame.subsampling == ChromaSubsampling::YUV444 ? 0u : 1u;
  m_chromaShiftY = frame.subsampling == ChromaSubsampling::YUV420 ? 1u : 0u;
  m_hasChroma = frame.u.data != nullptr && frame.v.data != nullptr;
}

template<typename T>
CSampleLine<T> CDetector<T>::Luma(const LineSpec& line) const
{
  const std::ptrdiff_t stride = m_frame.y.strideBytes;
  const std::ptrdiff_t bytes = static_cast<std::ptrdiff_t>(m_bytes);

  if (line.row)
  {
    return {m_frame.y, m_shift,
            static_cast<std::ptrdiff_t>(line.index) * stride + line.begin * bytes, bytes,
            line.end - line.begin};
  }
  return {m_frame.y, m_shift, static_cast<std::ptrdiff_t>(line.begin) * stride + line.index * bytes,
          stride, line.end - line.begin};
}

template<typename T>
CSampleLine<T> CDetector<T>::Chroma(const PlaneRef& plane, const LineSpec& line) const
{
  const std::ptrdiff_t stride = plane.strideBytes;
  const std::ptrdiff_t bytes = static_cast<std::ptrdiff_t>(m_bytes);

  if (line.row)
  {
    const unsigned int cy = line.index >> m_chromaShiftY;
    const unsigned int begin = line.begin >> m_chromaShiftX;
    const unsigned int end = (line.end + (1u << m_chromaShiftX) - 1) >> m_chromaShiftX;
    return {plane, m_shift, static_cast<std::ptrdiff_t>(cy) * stride + begin * bytes, bytes,
            end > begin ? end - begin : 0};
  }

  const unsigned int cx = line.index >> m_chromaShiftX;
  const unsigned int begin = line.begin >> m_chromaShiftY;
  const unsigned int end = (line.end + (1u << m_chromaShiftY) - 1) >> m_chromaShiftY;
  return {plane, m_shift, static_cast<std::ptrdiff_t>(begin) * stride + cx * bytes, stride,
          end > begin ? end - begin : 0};
}

/*!
 * \brief Is the median chroma of this line achromatic? A true bar carries no colour, where dark
 *        saturated picture can be both dark and flat in luma - on PQ especially.
 *
 * The median rather than a mean, so a bright overlay cannot drag it.
 */
template<typename T>
bool CDetector<T>::ChromaIsNeutral(const LineSpec& line)
{
  if (!m_hasChroma)
    return true;

  const CSampleLine<T> u = Chroma(m_frame.u, line);
  const CSampleLine<T> v = Chroma(m_frame.v, line);
  if (u.Count() == 0)
    return true;

  m_chromaU.Reset();
  m_chromaV.Reset();
  for (unsigned int i = 0; i < u.Count(); ++i)
  {
    m_chromaU.Add(u[i]);
    m_chromaV.Add(v[i]);
  }

  const int neutral = 128;
  const int tolerance = static_cast<int>(m_params.chromaTolerance);
  const int medianU = static_cast<int>(m_chromaU.Percentile(0.5));
  const int medianV = static_cast<int>(m_chromaV.Percentile(0.5));

  return std::abs(medianU - neutral) <= tolerance && std::abs(medianV - neutral) <= tolerance;
}

template<typename T>
LineOutcome CDetector<T>::Classify(const LineSpec& line)
{
  LineOutcome outcome;

  const CSampleLine<T> luma = Luma(line);
  const unsigned int count = luma.Count();
  if (count == 0)
    return outcome;

  //! \brief The outermost above-threshold runs, admitting only runs of minLength - a
  //! shorter bright run is noise, not an overlay.
  struct BrightRuns
  {
    unsigned int minLength;
    unsigned int lowest;
    unsigned int highest{0};
    unsigned int start{0};
    unsigned int length{0};

    void Extend(unsigned int at)
    {
      if (length == 0)
        start = at;
      ++length;
    }

    void Flush()
    {
      if (length >= minLength)
      {
        lowest = std::min(lowest, start);
        highest = std::max(highest, start + length - 1);
      }
      length = 0;
    }
  };

  // One pass builds the histogram and locates the outermost above-threshold runs, so the
  // overlay test costs nothing unless the line fails outright.
  m_histogram.Reset();
  BrightRuns runs{m_params.minRunLength, count};

  for (unsigned int i = 0; i < count; ++i)
  {
    const unsigned int value = luma[i];
    m_histogram.Add(value);

    if (value > m_threshold)
      runs.Extend(i);
    else
      runs.Flush();
  }
  runs.Flush();

  const LineStats stats = RobustStats(m_histogram);

  outcome.high = stats.high;
  outcome.low = stats.low;
  outcome.spread = stats.spread;

  // Cached: each call walks U and V and clears two histograms, and two branches below reach it.
  std::optional<bool> neutral;
  const auto chromaIsNeutral = [this, &neutral, &line]()
  {
    if (!neutral)
      neutral = ChromaIsNeutral(line);
    return *neutral;
  };

  // Dark and flat, judged by robust statistics: the high percentile tolerates half a percent of
  // the line being an outlier, where a maximum-deviation test over a 4K row would not, and the
  // interquartile range separates a uniform bar from a dark scene below the threshold.
  if (stats.high <= m_threshold && stats.spread <= m_params.flatBand && chromaIsNeutral())
  {
    outcome.classification = LineClass::Bar;
    return outcome;
  }

  if (runs.lowest > runs.highest)
    return outcome; // nothing bright enough to be an overlay

  // Only the last branch raises suspectedOverlay, where a line that would have been a bar
  // carries something too wide to admit; the rest return lines that are simply not bars.
  const unsigned int extent = runs.highest - runs.lowest + 1;
  if (static_cast<float>(extent) >= m_params.overlaySuspectExtent * static_cast<float>(count))
    return outcome;

  m_remainder.Reset();
  for (unsigned int i = 0; i < count; ++i)
  {
    if (i < runs.lowest || i > runs.highest)
      m_remainder.Add(luma[i]);
  }

  if (m_remainder.Total() == 0)
    return outcome;

  const LineStats remainder = RobustStats(m_remainder);

  if (remainder.high > m_threshold || remainder.spread > m_params.flatBand || !chromaIsNeutral())
    return outcome;

  outcome.high = remainder.high;
  outcome.low = remainder.low;
  outcome.spread = remainder.spread;

  // Subtitles, corner logos and timecode burn-ins are all horizontally localised where real
  // picture is not, so one width rule covers all three. Beyond the bound the line is kept as
  // picture, losing the bar in the wider direction, and the caller is told why.
  if (static_cast<float>(extent) < m_params.overlayMaxExtent * static_cast<float>(count))
    outcome.classification = LineClass::Overlay;
  else
    outcome.suspectedOverlay = true;

  return outcome;
}

template<typename T>
void CDetector<T>::Accumulate(const LineOutcome& outcome)
{
  ++m_barLines;
  m_lineHighs.Add(outcome.high);
  m_lineSpreads.Add(outcome.spread);
  m_lowestSeen = std::min(m_lowestSeen, outcome.low);
  if (outcome.classification == LineClass::Overlay)
    ++m_overlayLines;
}

//! \brief Measure the boundary between the last bar line and the first picture line. See
//! EdgeMetrics for what the two numbers separate.
template<typename T>
EdgeMetrics CDetector<T>::MeasureEdge(const LineSpec& bar,
                                      const LineSpec& picture,
                                      unsigned int thickness)
{
  EdgeMetrics metrics;
  metrics.thickness = thickness;
  if (thickness == 0)
    return metrics;

  const CSampleLine<T> barLine = Luma(bar);
  const CSampleLine<T> pictureLine = Luma(picture);
  const unsigned int count = std::min(barLine.Count(), pictureLine.Count());
  if (count == 0)
    return metrics;

  m_histogram.Reset();
  m_remainder.Reset();
  unsigned int stepped = 0;
  for (unsigned int i = 0; i < count; ++i)
  {
    const unsigned int barValue = barLine[i];
    const unsigned int pictureValue = pictureLine[i];
    m_histogram.Add(barValue);
    m_remainder.Add(pictureValue);
    if (pictureValue > barValue + m_params.margin)
      ++stepped;
  }

  // A ceiling for the bar, a high level for the picture: one specular pixel must not report a
  // step the line does not have.
  const int barCeiling = static_cast<int>(m_histogram.Percentile(0.995));
  const int pictureLevel = static_cast<int>(m_remainder.Percentile(0.9));
  const int step = pictureLevel - barCeiling;

  metrics.measured = true;
  metrics.step = step > 0 ? static_cast<float>(step) / static_cast<float>(m_fullScale) : 0.0f;
  metrics.coverage = static_cast<float>(stepped) / static_cast<float>(count);
  return metrics;
}

template<typename T>
DetectionResult CDetector<T>::Run()
{
  DetectionResult result;
  result.rangeAssumed = m_rangeAssumed;
  result.chromaTested = m_hasChroma;
  result.thresholdUsed = m_threshold << m_shift;

  m_roi = m_frame.roi;
  if (m_roi.IsEmpty())
    m_roi = CRectInt(0, 0, static_cast<int>(m_frame.width), static_cast<int>(m_frame.height));

  m_roi.x1 = std::max(0, m_roi.x1);
  m_roi.y1 = std::max(0, m_roi.y1);
  m_roi.x2 = std::min(static_cast<int>(m_frame.width), m_roi.x2);
  m_roi.y2 = std::min(static_cast<int>(m_frame.height), m_roi.y2);

  result.rect = m_roi;

  if (m_frame.y.data == nullptr || m_roi.Width() <= 0 || m_roi.Height() <= 0)
  {
    result.degenerate = true;
    return result;
  }

  const unsigned int x0 = static_cast<unsigned int>(m_roi.x1);
  const unsigned int x1 = static_cast<unsigned int>(m_roi.x2);
  const unsigned int y0 = static_cast<unsigned int>(m_roi.y1);
  const unsigned int y1 = static_cast<unsigned int>(m_roi.y2);

  // A walk that left no believable content span ends the reading the same way from either
  // axis.
  const auto noContent = [&](unsigned int low, unsigned int high)
  {
    if (high > low && high - low >= m_params.minContentExtent)
      return false;
    result.degenerate = true;
    result.rect = m_roi;
    return true;
  };

  // Rows first: on a letterboxed frame every column contains the bars and would be dark and
  // flat all the way across.
  const auto row = [&](unsigned int y) { return Row(y, x0, x1); };
  unsigned int top = Walk(y0, y1, 1, row);
  unsigned int bottom = Walk(y1, top, -1, row);

  if (noContent(top, bottom))
    return result;

  // A finding thinner than the floor is not a bar, and is discarded before the column walk so
  // the columns are judged over the rows that survived.
  const auto discardSubFloor = [&](unsigned int& edge, unsigned int limit, bool low)
  {
    const unsigned int thickness = low ? edge - limit : limit - edge;
    if (thickness == 0 || thickness >= m_params.edgeThicknessFloor)
      return;
    edge = limit;
    result.subFloorEdgesIgnored = true;
  };
  discardSubFloor(top, y0, true);
  discardSubFloor(bottom, y1, false);

  const auto column = [&](unsigned int x) { return Column(x, top, bottom); };
  unsigned int left = Walk(x0, x1, 1, column);
  unsigned int right = Walk(x1, left, -1, column);

  if (noContent(left, right))
    return result;

  discardSubFloor(left, x0, true);
  discardSubFloor(right, x1, false);

  result.rect = CRectInt(static_cast<int>(left), static_cast<int>(top), static_cast<int>(right),
                         static_cast<int>(bottom));

  const unsigned int topBar = top - y0;
  const unsigned int bottomBar = y1 - bottom;
  const unsigned int leftBar = left - x0;
  const unsigned int rightBar = x1 - right;

  result.top = MeasureEdge(Row(top - 1, left, right), Row(top, left, right), topBar);
  result.bottom = MeasureEdge(Row(bottom, left, right), Row(bottom - 1, left, right), bottomBar);
  result.left = MeasureEdge(Column(left - 1, top, bottom), Column(left, top, bottom), leftBar);
  result.right = MeasureEdge(Column(right, top, bottom), Column(right - 1, top, bottom), rightBar);

  result.overlayLines = m_overlayLines;
  result.overlaySuspected = m_overlaySuspected;
  result.blackFloorObserved = m_barLines > 0 ? (m_lowestSeen << m_shift) : 0;

  ScoreResult(result);
  return result;
}

template<typename T>
void CDetector<T>::ScoreResult(DetectionResult& result)
{
  const float margin = static_cast<float>(m_params.margin);
  const float band = static_cast<float>(m_params.flatBand);

  result.separation = 1.0f;
  result.flatness = 1.0f;
  if (m_barLines > 0)
  {
    // Across the bar's lines, not within one: the transition row is where ringing lives.
    const float highs = static_cast<float>(m_lineHighs.Percentile(0.9));
    const float spreads = static_cast<float>(m_lineSpreads.Percentile(0.9));
    result.separation = std::clamp((static_cast<float>(m_threshold) - highs) / margin, 0.0f, 1.0f);
    // Full marks until half the permitted dispersion is used; a bar with a little dither
    // is still plainly a bar.
    result.flatness = std::clamp((band - spreads) / (band * 0.5f), 0.0f, 1.0f);
  }

  // Scaled against the thicker bar, or the thickness floor when both are slight, so that a
  // nought-versus-two-pixel difference does not read as total disagreement. Off-centre bars are
  // real, so this may cost confidence but must never move an edge.
  const auto asymmetry = [this](unsigned int a, unsigned int b)
  {
    const unsigned int reference = std::max({a, b, m_params.edgeThicknessFloor});
    const unsigned int difference = a > b ? a - b : b - a;
    return std::clamp(static_cast<float>(difference) / static_cast<float>(reference), 0.0f, 1.0f);
  };
  const float worstAsymmetry = std::max(asymmetry(result.top.thickness, result.bottom.thickness),
                                        asymmetry(result.left.thickness, result.right.thickness));
  result.symmetry = 1.0f - m_params.symmetryPenalty * worstAsymmetry;

  const auto score = [this](const EdgeMetrics& edge)
  { return std::clamp(edge.step / m_params.edgeStepReference, 0.0f, 1.0f) * edge.coverage; };

  // The weakest evidence, not the average: averaging lets a single soft boundary be outvoted,
  // and a soft boundary is how a mask closes onto picture.
  float confidence = 1.0f;
  bool haveEvidence = false;
  if (m_barLines > 0)
  {
    confidence = std::min(result.separation, result.flatness);
    haveEvidence = true;
  }
  for (const EdgeMetrics* edge : {&result.top, &result.bottom, &result.left, &result.right})
  {
    if (!edge->measured)
      continue;
    confidence = haveEvidence ? std::min(confidence, score(*edge)) : score(*edge);
    haveEvidence = true;
  }

  confidence *= result.symmetry;
  if (result.rangeAssumed)
    confidence *= m_params.rangeAssumedPenalty;
  if (!result.chromaTested)
    confidence *= m_params.chromaAbsentPenalty;

  // overlaySuspected carries no penalty: it means the walk stopped on a bounded bright run it
  // would not admit, so the rect may be too wide, and confidence guards against readings that
  // are too narrow. Reported instead, for the sampler to weigh.

  result.confidence = std::clamp(confidence, 0.0f, 1.0f);
}

} // unnamed namespace

DetectionResult DetectContentRect(const FrameRef& frame, const DetectorParams& params)
{
  if (frame.width == 0 || frame.height == 0 || frame.bitDepth < 8 || frame.bitDepth > 16)
  {
    DetectionResult rejected;
    rejected.degenerate = true;
    return rejected;
  }

  if (frame.bitDepth == 8)
    return CDetector<uint8_t>(frame, params).Run();

  return CDetector<uint16_t>(frame, params).Run();
}

} // namespace KODI::VIDEO::GEOMETRY
