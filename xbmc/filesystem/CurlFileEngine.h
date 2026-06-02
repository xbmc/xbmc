/*
 *  Copyright (C) 2025 Team CoreELEC
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFile.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

typedef void CURL_HANDLE;
typedef void CURLM;
struct curl_slist;

class CURL;

namespace XFILE
{

/**
 * High-performance HTTP streaming engine for ISO disc images.
 *
 * Features:
 * - Background worker thread with RingBuffer for continuous prefetch
 * - Global LRU block cache (CCurlFileLRUCache) shared across file instances
 * - ISO-first-read tail-block parallel prefetch (UDF filesystem table)
 * - Deferred close: worker stays alive ~200ms for rapid open/close cycles
 * - Transparent 302 redirect handling with effective URL tracking
 * - Cross-domain credential protection for redirected URLs
 */
class CCurlFileEngine
{
public:
  CCurlFileEngine();
  ~CCurlFileEngine();

  // --- Lifecycle ---
  bool Open(const CURL& url);
  void Close();

  // --- I/O ---
  ssize_t Read(void* buffer, size_t size);
  int64_t Seek(int64_t position, int whence);

  // --- Queries ---
  int64_t GetPosition() const { return m_logicalPos; }
  int64_t GetLength() const { return m_totalSize; }
  bool IsRangeSupported() const { return m_supportRange; }
  bool IsIsoFile() const { return m_isIso; }
  time_t GetModTime() const { return m_modTime; }
  const std::string& GetFileUrl() const { return m_fileUrl; }

  // --- ISO deferred close ---
  void ResetForReuse();

  // --- Configuration ---
  void SetBufferSize(size_t bytes) { m_ringBufferSize = bytes; }

  // --- Stat (HEAD-only info fetch, no worker) ---
  int Stat(const CURL& url, struct __stat64* buffer);

private:
  // --- Worker ---
  void StartWorker();
  void WorkerLoop();
  void StopWorker();

  // --- HTTP helpers ---
  static std::string GetExtensionFromUrl(const std::string& url);
  static std::string FixDavProtocol(const std::string& url);
  static std::string ExtractHost(const std::string& url);
  static bool EqualsNoCase(const std::string& a, const std::string& b);

  void UpdateEffectiveUrl(CURL_HANDLE* curl, const std::string& originalUrl, const char* ctx);
  void SetupBaseCurlOptions(CURL_HANDLE* curl, const std::string& targetUrl);
  void SetupStatHeadOptions(CURL_HANDLE* curl, const std::string& targetUrl);
  void SetupWorkerDownloadOptions(CURL_HANDLE* curl, const std::string& targetUrl, int64_t start);
  bool DownloadRange(CURL_HANDLE* curl, int64_t start, int64_t length, std::vector<uint8_t>& buf);

  // --- Parse Kodi URL protocol options (after '|') ---
  void ParseProtocolOptions(const CURL& url);

  // --- Callbacks ---
  static size_t WorkerWriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
  size_t HandleWorkerWrite(void* contents, size_t size);
  static size_t CacheWriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

  // ------------------------------------------------------------------
  // State
  // ------------------------------------------------------------------
  std::string m_fileUrl;
  std::string m_effectiveUrl;
  std::string m_userName;
  std::string m_password;
  std::string m_userAgent;
  std::string m_referer;
  std::string m_cookie;
  std::string m_httpAuth;
  std::string m_cipherList;
  std::string m_customRequest;
  curl_slist* m_customHeaders = nullptr;

  int m_redirectLimit = 5;
  int m_connectTimeout = 0;
  bool m_seekable = true;
  bool m_failOnError = false;
  bool m_verifyPeer = true;

  bool m_isIso = false;
  bool m_isDirectory = false;
  bool m_supportRange = true;
  bool m_isFirstRead = true;
  int64_t m_totalSize = 0;
  int64_t m_logicalPos = 0;
  time_t m_modTime = 0;

  std::thread m_workerThread;
  std::atomic<bool> m_running{false};
  std::atomic<bool> m_eof{false};
  std::atomic<bool> m_hasError{false};
  std::atomic<bool> m_abortTransfer{false};
  std::atomic<bool> m_triggerReset{false};
  std::atomic<int64_t> m_resetTargetPos{0};
  std::atomic<int64_t> m_downloadPos{0};
  std::atomic<int> m_cdnFallbackCount{0};

  std::vector<uint8_t> m_ringBuffer;
  size_t m_ringBufferSize = 64 * 1024 * 1024;
  size_t m_rbHead = 0;
  size_t m_rbTail = 0;
  size_t m_rbAvailable = 0;
  std::atomic<int64_t> m_rbLogicalStart{0};
  std::mutex m_rbMutex;
  std::condition_variable m_rbCvReader;
  std::condition_variable m_rbCvWriter;

  long m_netConnectTimeoutSec = 10;
  long m_netLowSpeedTimeSec = 15;
  long m_netWorkerLowSpeedTimeSec = 15;
  long m_netReadTimeoutSec = 20;
  long m_netRangeTotalTimeoutSec = 20;
  int m_netMaxRetries = 5;
};

} // namespace XFILE

bool TryStatCache(const std::string& origUrl, std::string& cdnUrl, int64_t& fileSize);
void ClearStatCache(const std::string& origUrl);
std::unique_ptr<XFILE::CCurlFileEngine> TryReuseEngine(const std::string& urlKey);
void CacheEngineForReuse(const std::string& urlKey, std::unique_ptr<XFILE::CCurlFileEngine> engine);
