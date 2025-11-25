/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GPUTypes.h"

#include <memory>
#include <vector>

// Forward declarations for ONNX Runtime types
namespace Ort
{
class SessionOptions;
}

namespace KODI
{
namespace SEMANTIC
{

/*!
 * \brief GPU Acceleration manager for embedding generation
 *
 * This class manages GPU resources and configuration for accelerating
 * ONNX Runtime inference on compatible hardware. It handles:
 * - Detection and enumeration of available GPU devices
 * - Configuration of ONNX Runtime execution providers
 * - Memory management and optimization
 * - Performance monitoring and metrics
 * - Automatic fallback to CPU when needed
 *
 * Supported backends:
 * - CUDA: NVIDIA GPUs on Windows/Linux (5-10x speedup)
 * - DirectML: All GPU vendors on Windows (3-7x speedup)
 * - OpenCL: AMD/Intel/NVIDIA on Linux/Windows (2-5x speedup)
 * - CoreML: Apple Silicon and Intel Macs (4-8x speedup)
 * - ROCm: AMD GPUs on Linux (5-10x speedup)
 *
 * Example usage:
 * \code
 * CGPUAccelerator gpu;
 * if (gpu.Initialize(config))
 * {
 *   // Configure ONNX session for GPU
 *   Ort::SessionOptions options;
 *   if (gpu.ConfigureSessionOptions(options))
 *   {
 *     // Session will use GPU acceleration
 *   }
 * }
 * \endcode
 */
class CGPUAccelerator
{
public:
  /*!
   * \brief Constructor
   */
  CGPUAccelerator();

  /*!
   * \brief Destructor
   */
  ~CGPUAccelerator();

  /*!
   * \brief Initialize GPU acceleration with configuration
   *
   * Detects available GPU devices, validates backend support,
   * and prepares for GPU-accelerated inference.
   *
   * \param config GPU configuration settings
   * \return true if initialization succeeded, false otherwise
   */
  bool Initialize(const GPUConfig& config);

  /*!
   * \brief Shutdown and release GPU resources
   */
  void Shutdown();

  /*!
   * \brief Check if GPU acceleration is available and ready
   *
   * \return true if GPU is ready for use, false otherwise
   */
  bool IsAvailable() const;

  /*!
   * \brief Get current GPU acceleration status
   *
   * \return Current status (Ready, Error, Disabled, etc.)
   */
  GPUStatus GetStatus() const;

  /*!
   * \brief Get active GPU backend type
   *
   * \return Backend type currently in use
   */
  GPUBackend GetBackend() const;

  /*!
   * \brief Enumerate all available GPU devices
   *
   * Scans the system for compatible GPU devices across all
   * supported backends.
   *
   * \return Vector of available GPU devices
   */
  std::vector<GPUDeviceInfo> EnumerateDevices() const;

  /*!
   * \brief Get information about the currently active device
   *
   * \return Device info, or empty info if no GPU active
   */
  GPUDeviceInfo GetActiveDevice() const;

  /*!
   * \brief Get current GPU performance metrics
   *
   * Returns runtime statistics about GPU utilization, memory,
   * temperature, etc. Not all metrics available on all platforms.
   *
   * \return Performance metrics
   */
  GPUMetrics GetMetrics() const;

  /*!
   * \brief Configure ONNX Runtime session options for GPU
   *
   * Adds the appropriate execution provider to the session options
   * based on the active backend and configuration.
   *
   * \param sessionOptions ONNX Runtime session options to configure
   * \return true if GPU provider added successfully, false otherwise
   */
  bool ConfigureSessionOptions(Ort::SessionOptions& sessionOptions) const;

  /*!
   * \brief Get optimal batch size for current GPU
   *
   * Returns recommended batch size based on GPU memory and model size.
   * Larger batches = better GPU utilization.
   *
   * \return Recommended batch size (typically 16-64 for GPU vs 1-8 for CPU)
   */
  int GetOptimalBatchSize() const;

  /*!
   * \brief Estimate memory usage for a batch of embeddings
   *
   * \param batchSize Number of items in batch
   * \param maxTokens Maximum sequence length in tokens
   * \return Estimated memory usage in megabytes
   */
  size_t EstimateMemoryUsage(int batchSize, int maxTokens) const;

  /*!
   * \brief Check if there's enough GPU memory for a batch
   *
   * \param batchSize Number of items in batch
   * \param maxTokens Maximum sequence length in tokens
   * \return true if batch will fit in GPU memory, false otherwise
   */
  bool CanFitBatch(int batchSize, int maxTokens) const;

  /*!
   * \brief Get the current configuration
   *
   * \return Active GPU configuration
   */
  GPUConfig GetConfig() const;

  /*!
   * \brief Update configuration at runtime
   *
   * Note: Some settings (like backend) require reinitialization.
   *
   * \param config New configuration
   * \return true if update succeeded, false if reinit needed
   */
  bool UpdateConfig(const GPUConfig& config);

  /*!
   * \brief Get a human-readable status message
   *
   * Returns detailed information about GPU status, including
   * any errors or warnings.
   *
   * \return Status message string
   */
  std::string GetStatusMessage() const;

  /*!
   * \brief Detect best available GPU backend for current system
   *
   * Auto-detects the optimal backend based on platform and
   * available hardware.
   *
   * \return Recommended backend, or NONE if no GPU available
   */
  static GPUBackend DetectBestBackend();

  /*!
   * \brief Check if a specific backend is available
   *
   * \param backend Backend type to check
   * \return true if backend is supported and available
   */
  static bool IsBackendAvailable(GPUBackend backend);

private:
  /*!
   * \brief Private implementation (PIMPL pattern)
   *
   * Hides platform-specific and ONNX Runtime implementation details.
   */
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

} // namespace SEMANTIC
} // namespace KODI
