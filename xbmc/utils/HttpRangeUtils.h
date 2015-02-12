#pragma once
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

#include <stdint.h>

#include <string>
#include <vector>

class CHttpRange
{
public:
  CHttpRange();
  CHttpRange(uint64_t firstPosition, uint64_t lastPosition);
  virtual ~CHttpRange() { }

  bool operator<(const CHttpRange &other) const;
  bool operator==(const CHttpRange &other) const;
  bool operator!=(const CHttpRange &other) const;

  virtual uint64_t GetFirstPosition() const { return m_first; }
  virtual void SetFirstPosition(uint64_t firstPosition) { m_first = firstPosition; }
  virtual uint64_t GetLastPosition() const { return m_last; }
  virtual void SetLastPosition(uint64_t lastPosition) { m_last = lastPosition; }

  virtual uint64_t GetLength() const;
  virtual void SetLength(uint64_t length);

  virtual bool IsValid() const;

protected:
  uint64_t m_first;
  uint64_t m_last;
};

typedef std::vector<CHttpRange> HttpRanges;

class CHttpResponseRange : public CHttpRange
{
public:
  CHttpResponseRange();
  CHttpResponseRange(uint64_t firstPosition, uint64_t lastPosition);
  CHttpResponseRange(const void* data, uint64_t firstPosition, uint64_t lastPosition);
  CHttpResponseRange(const void* data, uint64_t length);
  virtual ~CHttpResponseRange() { }

  bool operator==(const CHttpResponseRange &other) const;
  bool operator!=(const CHttpResponseRange &other) const;

  const void* GetData() const { return m_data; }
  void SetData(const void* data) { m_data = data; }
  void SetData(const void* data, uint64_t length);
  void SetData(const void* data, uint64_t firstPosition, uint64_t lastPosition);

  virtual bool IsValid() const;

protected:
  const void* m_data;
};

typedef std::vector<CHttpResponseRange> HttpResponseRanges;

class CHttpRanges
{
public:
  CHttpRanges();
  CHttpRanges(const HttpRanges& httpRanges);
  virtual ~CHttpRanges() { }

  const HttpRanges& Get() const { return m_ranges; }
  bool Get(size_t index, CHttpRange& range) const;
  bool GetFirst(CHttpRange& range) const;
  bool GetLast(CHttpRange& range) const;
  size_t Size() const { return m_ranges.size(); }
  bool IsEmpty() const { return m_ranges.empty(); }

  bool GetFirstPosition(uint64_t& position) const;
  bool GetLastPosition(uint64_t& position) const;
  uint64_t GetLength() const;

  bool GetTotalRange(CHttpRange& range) const;

  void Add(const CHttpRange& range);
  void Remove(size_t index);
  void Clear();

  HttpRanges::const_iterator Begin() const { return m_ranges.begin(); }
  HttpRanges::const_iterator End() const { return m_ranges.end(); }

  bool Parse(const std::string& header);
  bool Parse(const std::string& header, uint64_t totalLength);

protected:
  void SortAndCleanup();

  HttpRanges m_ranges;
};

class HttpRangeUtils
{
public:
  /*!
  * \brief Generates a valid Content-Range HTTP header value for the given HTTP
  * range definition.
  *
  * \param range HTTP range definition used to generate the Content-Range HTTP header
  * \return Content-Range HTTP header value
  */
  static std::string GenerateContentRangeHeaderValue(const CHttpRange* range);

  /*!
  * \brief Generates a valid Content-Range HTTP header value for the given HTTP
  * range properties.
  *
  * \param start Start position of the HTTP range
  * \param end Last/End position of the HTTP range
  * \param total Total length of original content (not just the range)
  * \return Content-Range HTTP header value
  */
  static std::string GenerateContentRangeHeaderValue(uint64_t start, uint64_t end, uint64_t total);

  /*!
   * \brief Generates a multipart boundary that can be used in ranged HTTP
   * responses.
   *
   * \return Multipart boundary that can be used in ranged HTTP responses
   */
  static std::string GenerateMultipartBoundary();

  /*!
  * \brief Generates the multipart/byteranges Content-Type HTTP header value
  * containing the given multipart boundary for a ranged HTTP response.
  *
  * \param multipartBoundary Multipart boundary to be used in the ranged HTTP response
  * \return multipart/byteranges Content-Type HTTP header value
  */
  static std::string GenerateMultipartBoundaryContentType(const std::string& multipartBoundary);

  /*!
  * \brief Generates a multipart boundary including the Content-Type HTTP
  * header value with the (actual) given content type of the original
  * content.
  *
  * \param multipartBoundary Multipart boundary to be used in the ranged HTTP response
  * \param contentType (Actual) Content type of the original content
  * \return Multipart boundary (including the Content-Type HTTP header) value that can be used in ranged HTTP responses
  */
  static std::string GenerateMultipartBoundaryWithHeader(const std::string& multipartBoundary, const std::string& contentType);

  /*!
  * \brief Generates a multipart boundary including the Content-Type HTTP
  * header value with the (actual) given content type of the original
  * content and the Content-Range HTTP header value for the given range.
  *
  * \param multipartBoundary Multipart boundary to be used in the ranged HTTP response
  * \param contentType (Actual) Content type of the original content
  * \param range HTTP range definition used to generate the Content-Range HTTP header
  * \return Multipart boundary (including the Content-Type and Content-Range HTTP headers) value that can be used in ranged HTTP responses
  */
  static std::string GenerateMultipartBoundaryWithHeader(const std::string& multipartBoundary, const std::string& contentType, const CHttpRange* range);

  /*!
  * \brief Generates a multipart boundary including the Content-Type HTTP
  * header value with the (actual) given content type of the original
  * content and the Content-Range HTTP header value for the given range.
  *
  * \param multipartBoundaryWithContentType Multipart boundary (already including the Content-Type HTTP header value) to be used in the ranged HTTP response
  * \param range HTTP range definition used to generate the Content-Range HTTP header
  * \return Multipart boundary (including the Content-Type and Content-Range HTTP headers) value that can be used in ranged HTTP responses
  */
  static std::string GenerateMultipartBoundaryWithHeader(const std::string& multipartBoundaryWithContentType, const CHttpRange* range);

  /*!
  * \brief Generates a multipart boundary end that can be used in ranged HTTP
   * responses.
  *
  * \param multipartBoundary Multipart boundary to be used in the ranged HTTP response
  * \return Multipart boundary end that can be used in a ranged HTTP response
  */
  static std::string GenerateMultipartBoundaryEnd(const std::string& multipartBoundary);
};

