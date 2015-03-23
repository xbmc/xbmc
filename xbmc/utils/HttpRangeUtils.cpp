/*
 *      Copyright (C) 2015 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "HttpRangeUtils.h"
#include "Util.h"
#include "network/httprequesthandler/IHTTPRequestHandler.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <algorithm>

#define HEADER_NEWLINE        "\r\n"
#define HEADER_SEPARATOR      HEADER_NEWLINE HEADER_NEWLINE
#define HEADER_BOUNDARY       "--"

#define HEADER_CONTENT_RANGE_VALUE          "%" PRIu64
#define HEADER_CONTENT_RANGE_VALUE_UNKNOWN  "*"
#define HEADER_CONTENT_RANGE_FORMAT_BYTES   "bytes " HEADER_CONTENT_RANGE_VALUE "-" HEADER_CONTENT_RANGE_VALUE "/"
#define CONTENT_RANGE_FORMAT_TOTAL          HEADER_CONTENT_RANGE_FORMAT_BYTES HEADER_CONTENT_RANGE_VALUE
#define CONTENT_RANGE_FORMAT_TOTAL_UNKNOWN  HEADER_CONTENT_RANGE_FORMAT_BYTES HEADER_CONTENT_RANGE_VALUE_UNKNOWN

CHttpRange::CHttpRange()
  : m_first(1),
    m_last(0)
{ }

CHttpRange::CHttpRange(uint64_t firstPosition, uint64_t lastPosition)
  : m_first(firstPosition),
    m_last(lastPosition)
{ }

bool CHttpRange::operator<(const CHttpRange &other) const
{
  return (m_first < other.m_first) ||
         (m_first == other.m_first && m_last < other.m_last);
}

bool CHttpRange::operator==(const CHttpRange &other) const
{
  return m_first == other.m_first && m_last == other.m_last;
}

bool CHttpRange::operator!=(const CHttpRange &other) const
{
  return !(*this == other);
}

uint64_t CHttpRange::GetLength() const
{
  if (!IsValid())
    return 0;

  return m_last - m_first + 1;
}

void CHttpRange::SetLength(uint64_t length)
{
  m_last = m_first + length - 1;
}

bool CHttpRange::IsValid() const
{
  return m_last >= m_first;
}

CHttpResponseRange::CHttpResponseRange()
  : CHttpRange(),
    m_data(NULL)
{ }

CHttpResponseRange::CHttpResponseRange(uint64_t firstPosition, uint64_t lastPosition)
  : CHttpRange(firstPosition, lastPosition),
    m_data(NULL)
{ }

CHttpResponseRange::CHttpResponseRange(const void* data, uint64_t firstPosition, uint64_t lastPosition)
  : CHttpRange(firstPosition, lastPosition),
    m_data(data)
{ }

CHttpResponseRange::CHttpResponseRange(const void* data, uint64_t length)
  : CHttpRange(0, length - 1),
    m_data(data)
{ }

bool CHttpResponseRange::operator==(const CHttpResponseRange &other) const
{
  if (!CHttpRange::operator==(other))
    return false;

  return m_data == other.m_data;
}

bool CHttpResponseRange::operator!=(const CHttpResponseRange &other) const
{
  return !(*this == other);
}

void CHttpResponseRange::SetData(const void* data, uint64_t length)
{
  if (length == 0)
    return;

  m_first = 0;

  SetData(data);
  SetLength(length);
}

void CHttpResponseRange::SetData(const void* data, uint64_t firstPosition, uint64_t lastPosition)
{
  SetData(data);
  SetFirstPosition(firstPosition);
  SetLastPosition(lastPosition);
}

bool CHttpResponseRange::IsValid() const
{
  if (!CHttpRange::IsValid())
    return false;

  return m_data != NULL;
}

CHttpRanges::CHttpRanges()
: m_ranges()
{ }

CHttpRanges::CHttpRanges(const HttpRanges& httpRanges)
: m_ranges(httpRanges)
{
  SortAndCleanup();
}

bool CHttpRanges::Get(size_t index, CHttpRange& range) const
{
  if (index >= Size())
    return false;

  range = m_ranges.at(index);
  return true;
}

bool CHttpRanges::GetFirst(CHttpRange& range) const
{
  if (m_ranges.empty())
    return false;

  range = m_ranges.front();
  return true;
}

bool CHttpRanges::GetLast(CHttpRange& range) const
{
  if (m_ranges.empty())
    return false;

  range = m_ranges.back();
  return true;
}

bool CHttpRanges::GetFirstPosition(uint64_t& position) const
{
  if (m_ranges.empty())
    return false;

  position = m_ranges.front().GetFirstPosition();
  return true;
}

bool CHttpRanges::GetLastPosition(uint64_t& position) const
{
  if (m_ranges.empty())
    return false;

  position = m_ranges.back().GetLastPosition();
  return true;
}

uint64_t CHttpRanges::GetLength() const
{
  uint64_t length = 0;
  for (HttpRanges::const_iterator range = m_ranges.begin(); range != m_ranges.end(); ++range)
    length += range->GetLength();

  return length;
}

bool CHttpRanges::GetTotalRange(CHttpRange& range) const
{
  if (m_ranges.empty())
    return false;

  uint64_t firstPosition, lastPosition;
  if (!GetFirstPosition(firstPosition) || !GetLastPosition(lastPosition))
    return false;

  range.SetFirstPosition(firstPosition);
  range.SetLastPosition(lastPosition);

  return range.IsValid();
}

void CHttpRanges::Add(const CHttpRange& range)
{
  if (!range.IsValid())
    return;

  m_ranges.push_back(range);

  SortAndCleanup();
}

void CHttpRanges::Remove(size_t index)
{
  if (index >= Size())
    return;

  m_ranges.erase(m_ranges.begin() + index);
}

void CHttpRanges::Clear()
{
  m_ranges.clear();
}

bool CHttpRanges::Parse(const std::string& header)
{
  return Parse(header, std::numeric_limits<uint64_t>::max());
}

bool CHttpRanges::Parse(const std::string& header, uint64_t totalLength)
{
  m_ranges.clear();

  if (header.empty() || totalLength == 0 || !StringUtils::StartsWithNoCase(header, "bytes="))
    return false;

  uint64_t lastPossiblePosition = totalLength - 1;

  // remove "bytes=" from the beginning
  std::string rangesValue = header.substr(6);

  // split the value of the "Range" header by ","
  std::vector<std::string> rangeValues = StringUtils::Split(rangesValue, ",");

  for (std::vector<std::string>::const_iterator range = rangeValues.begin(); range != rangeValues.end(); ++range)
  {
    // there must be a "-" in the range definition
    if (range->find("-") == std::string::npos)
      return false;

    std::vector<std::string> positions = StringUtils::Split(*range, "-");
    if (positions.size() != 2)
      return false;

    bool hasStart = false;
    uint64_t start = 0;
    bool hasEnd = false;
    uint64_t end = 0;

    // parse the start and end positions
    if (!positions.front().empty())
    {
      if (!StringUtils::IsNaturalNumber(positions.front()))
        return false;

      start = str2uint64(positions.front(), 0);
      hasStart = true;
    }
    if (!positions.back().empty())
    {
      if (!StringUtils::IsNaturalNumber(positions.back()))
        return false;

      end = str2uint64(positions.back(), 0);
      hasEnd = true;
    }

    // nothing defined at all
    if (!hasStart && !hasEnd)
      return false;

    // make sure that the end position makes sense
    if (hasEnd)
      end = std::min(end, lastPossiblePosition);

    if (!hasStart && hasEnd)
    {
      // the range is defined as the number of bytes from the end
      start = totalLength - end;
      end = lastPossiblePosition;
    }
    else if (hasStart && !hasEnd)
      end = lastPossiblePosition;

    // make sure the start position makes sense
    if (start > lastPossiblePosition)
      return false;

    // make sure that the start position is smaller or equal to the end position
    if (end < start)
      return false;

    m_ranges.push_back(CHttpRange(start, end));
  }

  if (m_ranges.empty())
    return false;

  SortAndCleanup();
  return !m_ranges.empty();
}

void CHttpRanges::SortAndCleanup()
{
  // sort the ranges by their first position
  std::sort(m_ranges.begin(), m_ranges.end());

  // check for overlapping ranges
  for (HttpRanges::iterator range = m_ranges.begin() + 1; range != m_ranges.end();)
  {
    HttpRanges::iterator previous = range - 1;

    // check if the current and the previous range overlap
    if (previous->GetLastPosition() + 1 >= range->GetFirstPosition())
    {
      // combine the previous and the current ranges by setting the last position of the previous range
      // to the last position of the current range
      previous->SetLastPosition(range->GetLastPosition());

      // then remove the current range which is not needed anymore
      range = m_ranges.erase(range);
    }
    else
      ++range;
  }
}

std::string HttpRangeUtils::GenerateContentRangeHeaderValue(const CHttpRange* range)
{
  if (range == NULL)
    return "";

  return StringUtils::Format(CONTENT_RANGE_FORMAT_TOTAL, range->GetFirstPosition(), range->GetLastPosition(), range->GetLength());
}

std::string HttpRangeUtils::GenerateContentRangeHeaderValue(uint64_t start, uint64_t end, uint64_t total)
{
  if (total > 0)
    return StringUtils::Format(CONTENT_RANGE_FORMAT_TOTAL, start, end, total);

  return StringUtils::Format(CONTENT_RANGE_FORMAT_TOTAL_UNKNOWN, start, end);
}

std::string HttpRangeUtils::GenerateMultipartBoundary()
{
  static char chars[] = "-_1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

  // create a string of length 30 to 40 and pre-fill it with "-"
  size_t count = static_cast<size_t>(CUtil::GetRandomNumber()) % 11 + 30;
  std::string boundary(count, '-');

  for (size_t i = static_cast<size_t>(CUtil::GetRandomNumber()) % 5 + 8; i < count; i++)
    boundary.replace(i, 1, 1, chars[static_cast<size_t>(CUtil::GetRandomNumber()) % 64]);

  return boundary;
}

std::string HttpRangeUtils::GenerateMultipartBoundaryContentType(const std::string& multipartBoundary)
{
  if (multipartBoundary.empty())
    return "";

  return "multipart/byteranges; boundary=" + multipartBoundary;
}

std::string HttpRangeUtils::GenerateMultipartBoundaryWithHeader(const std::string& multipartBoundary, const std::string& contentType)
{
  if (multipartBoundary.empty())
    return "";

  std::string boundaryWithHeader = HEADER_BOUNDARY + multipartBoundary + HEADER_NEWLINE;
  if (!contentType.empty())
    boundaryWithHeader += MHD_HTTP_HEADER_CONTENT_TYPE ": " + contentType + HEADER_NEWLINE;

  return boundaryWithHeader;
}

std::string HttpRangeUtils::GenerateMultipartBoundaryWithHeader(const std::string& multipartBoundary, const std::string& contentType, const CHttpRange* range)
{
  if (multipartBoundary.empty() || range == NULL)
    return "";

  return GenerateMultipartBoundaryWithHeader(GenerateMultipartBoundaryWithHeader(multipartBoundary, contentType), range);
}

std::string HttpRangeUtils::GenerateMultipartBoundaryWithHeader(const std::string& multipartBoundaryWithContentType, const CHttpRange* range)
{
  if (multipartBoundaryWithContentType.empty() || range == NULL)
    return "";

  std::string boundaryWithHeader = multipartBoundaryWithContentType;
  boundaryWithHeader += MHD_HTTP_HEADER_CONTENT_RANGE ": " + GenerateContentRangeHeaderValue(range);
  boundaryWithHeader += HEADER_SEPARATOR;

  return boundaryWithHeader;
}

std::string HttpRangeUtils::GenerateMultipartBoundaryEnd(const std::string& multipartBoundary)
{
  if (multipartBoundary.empty())
    return "";

  return HEADER_NEWLINE HEADER_BOUNDARY + multipartBoundary + HEADER_BOUNDARY;
}
