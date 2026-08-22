/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "savestate_generated.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <flatbuffers/flatbuffers.h>

namespace KODI
{
namespace RETRO
{
/*!
 * \brief FlatBuffer offsets and metadata for a serialized savestate blob
 *
 * A blob is stored as either raw data or compressed data. The offsets are
 * owned by the FlatBufferBuilder used to create them.
 */
struct SavestateBlobOffsets
{
  //! \brief Offset of the raw blob data
  flatbuffers::Offset<flatbuffers::Vector<uint8_t>> raw;

  //! \brief Offset of the compressed blob data
  flatbuffers::Offset<flatbuffers::Vector<uint8_t>> compressed;

  //! \brief Compression applied to the compressed blob data
  SAVESTATE::CompressionType compressionType{SAVESTATE::CompressionType_None};

  //! \brief Size of the blob after decompression, or zero for raw data
  uint64_t uncompressedSize{0};
};

/*!
 * \brief Blob data retained while a savestate is being constructed
 *
 * This allows an existing compressed blob to be copied without decompressing
 * it, for example when changing savestate metadata.
 */
struct PendingSavestateBlob
{
  //! \brief Uncompressed blob data
  std::vector<uint8_t> raw;

  //! \brief Compressed blob data
  std::vector<uint8_t> compressed;

  //! \brief Size of the compressed blob after decompression
  uint64_t uncompressedSize{0};

  //! \brief Compression applied to the compressed blob data
  SAVESTATE::CompressionType compression{SAVESTATE::CompressionType_None};

  /*!
   * \brief Clear the blob data and reset its compression metadata
   */
  void Clear();

  /*!
   * \brief Check whether this object contains supported compressed data
   *
   * \return True if the blob contains non-empty zstd-compressed data, false otherwise
   */
  bool HasCompressedData() const;
};

/*!
 * \brief Helpers for serializing and validating savestate data blobs
 */
class CSavestateBlob
{
public:
  /*!
   * \brief Create FlatBuffer offsets for raw blob data
   *
   * \param builder The builder that will own the returned offsets
   * \param rawData The raw blob data to serialize
   * \param fieldName The blob field name used for diagnostic logging
   *
   * \return Offsets and metadata for the serialized blob
   */
  static SavestateBlobOffsets CreateWriteOffsets(flatbuffers::FlatBufferBuilder& builder,
                                                 const std::vector<uint8_t>& rawData,
                                                 const char* fieldName,
                                                 bool compress);

  /*!
   * \brief Create FlatBuffer offsets for pending raw or compressed blob data
   *
   * Valid compressed data is copied without decompression. Invalid compressed
   * metadata falls back to the pending raw data.
   *
   * \param builder The builder that will own the returned offsets
   * \param pending The pending blob data and compression metadata
   * \param fieldName The blob field name used for diagnostic logging
   *
   * \return Offsets and metadata for the serialized blob
   */
  static SavestateBlobOffsets CreateWriteOffsets(flatbuffers::FlatBufferBuilder& builder,
                                                 const PendingSavestateBlob& pending,
                                                 const char* fieldName,
                                                 bool compress);

  /*!
   * \brief Validate and prepare compressed memory data for reading
   *
   * The destination is cleared before validation. Compression is unsupported
   * by this version, so valid compressed data is rejected after validation.
   *
   * \param savestate The savestate containing the compressed memory data
   * \param expectedSize The expected decompressed memory size in bytes
   * \param decompressedMemoryData Receives the decompressed memory data
   *
   * \return True if the memory data was prepared, false on invalid or unsupported data
   */
  static bool PrepareMemoryData(const SAVESTATE::Savestate& savestate,
                                size_t expectedSize,
                                std::vector<uint8_t>& decompressedMemoryData);

  /*!
   * \brief Validate and prepare compressed video data for reading
   *
   * The destination is cleared before validation. Compression is unsupported
   * by this version, so valid compressed data is rejected after validation.
   *
   * \param savestate The savestate containing the compressed video data
   * \param decompressedVideoData Receives the decompressed video data
   *
   * \return True if the video data was prepared, false on invalid or unsupported data
   */
  static bool PrepareVideoData(const SAVESTATE::Savestate& savestate,
                               std::vector<uint8_t>& decompressedVideoData);

  /*!
   * \brief Check whether raw memory data has the expected supported size
   *
   * \param savestate The savestate containing the raw memory data
   * \param expectedSize The required memory size in bytes
   *
   * \return True if the raw memory data is valid, false otherwise
   */
  static bool IsValidRawMemoryData(const SAVESTATE::Savestate& savestate, size_t expectedSize);

  /*!
   * \brief Check whether a savestate contains non-empty raw video data
   *
   * \param savestate The savestate to inspect
   *
   * \return True if raw video data is present, false otherwise
   */
  static bool HasRawVideoData(const SAVESTATE::Savestate& savestate);

  /*!
   * \brief Validate compressed memory data before copying it unchanged
   *
   * \param savestate The savestate containing the compressed memory data
   *
   * \return True if the compressed and uncompressed sizes are valid, false otherwise
   */
  static bool IsValidCopiedCompressedMemoryData(const SAVESTATE::Savestate& savestate);

  /*!
   * \brief Check whether a memory data buffer is within the storage limit
   *
   * \param size The memory data size in bytes
   *
   * \return True if the size is within the limit, false otherwise
   */
  static bool IsValidMemoryDataSize(size_t size);

  /*!
   * \brief Check whether an expected memory size is non-zero and within the storage limit
   *
   * \param expectedSize The expected memory size in bytes
   *
   * \return True if the size is supported, false otherwise
   */
  static bool IsSupportedMemorySize(size_t expectedSize);
};
} // namespace RETRO
} // namespace KODI
