/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DarwinVoiceInput.h"

#if defined(TARGET_DARWIN)

#include "utils/log.h"

#ifdef __OBJC__
#import <Foundation/Foundation.h>
#if defined(TARGET_DARWIN_IOS) || defined(TARGET_DARWIN_TVOS)
#import <Speech/Speech.h>
#else
#import <AppKit/AppKit.h>
#endif

/*!
 * @brief Objective-C implementation class
 */
@interface DarwinVoiceInputImpl : NSObject
#if defined(TARGET_DARWIN_IOS) || defined(TARGET_DARWIN_TVOS)
  <SFSpeechRecognizerDelegate>
#else
  <NSSpeechRecognizerDelegate>
#endif

@property (nonatomic, assign) KODI::SEMANTIC::CDarwinVoiceInput* owner;

#if defined(TARGET_DARWIN_IOS) || defined(TARGET_DARWIN_TVOS)
@property (nonatomic, strong) SFSpeechRecognizer* speechRecognizer;
@property (nonatomic, strong) SFSpeechAudioBufferRecognitionRequest* recognitionRequest;
@property (nonatomic, strong) SFSpeechRecognitionTask* recognitionTask;
@property (nonatomic, strong) AVAudioEngine* audioEngine;
#else
@property (nonatomic, strong) NSSpeechRecognizer* speechRecognizer;
#endif

- (instancetype)initWithOwner:(KODI::SEMANTIC::CDarwinVoiceInput*)owner;
- (BOOL)initialize;
- (void)shutdown;
- (BOOL)startListening;
- (void)stopListening;
- (void)cancel;
- (BOOL)hasPermissions;
- (void)requestPermissions;
- (void)setLanguage:(NSString*)languageCode;

@end

@implementation DarwinVoiceInputImpl

- (instancetype)initWithOwner:(KODI::SEMANTIC::CDarwinVoiceInput*)owner
{
  self = [super init];
  if (self)
  {
    _owner = owner;
  }
  return self;
}

- (BOOL)initialize
{
#if defined(TARGET_DARWIN_IOS) || defined(TARGET_DARWIN_TVOS)
  // iOS/tvOS implementation using Speech framework
  NSLocale* locale = [NSLocale localeWithLocaleIdentifier:@"en-US"];
  _speechRecognizer = [[SFSpeechRecognizer alloc] initWithLocale:locale];
  _speechRecognizer.delegate = self;

  _audioEngine = [[AVAudioEngine alloc] init];

  if (!_speechRecognizer)
  {
    NSLog(@"DarwinVoiceInput: Failed to create speech recognizer");
    return NO;
  }

  return YES;
#else
  // macOS implementation using NSSpeechRecognizer
  _speechRecognizer = [[NSSpeechRecognizer alloc] init];
  [_speechRecognizer setDelegate:self];

  if (!_speechRecognizer)
  {
    NSLog(@"DarwinVoiceInput: Failed to create speech recognizer");
    return NO;
  }

  // Set commands to nil for continuous recognition
  [_speechRecognizer setCommands:nil];

  return YES;
#endif
}

- (void)shutdown
{
  [self stopListening];

#if defined(TARGET_DARWIN_IOS) || defined(TARGET_DARWIN_TVOS)
  _speechRecognizer = nil;
  _audioEngine = nil;
  _recognitionRequest = nil;
  _recognitionTask = nil;
#else
  _speechRecognizer = nil;
#endif
}

- (BOOL)startListening
{
#if defined(TARGET_DARWIN_IOS) || defined(TARGET_DARWIN_TVOS)
  // iOS/tvOS implementation
  if (_recognitionTask)
  {
    [_recognitionTask cancel];
    _recognitionTask = nil;
  }

  _recognitionRequest = [[SFSpeechAudioBufferRecognitionRequest alloc] init];
  _recognitionRequest.shouldReportPartialResults = YES;

  AVAudioInputNode* inputNode = _audioEngine.inputNode;
  AVAudioFormat* recordingFormat = [inputNode outputFormatForBus:0];

  [inputNode installTapOnBus:0 bufferSize:1024 format:recordingFormat
                       block:^(AVAudioPCMBuffer* buffer, AVAudioTime* when) {
    [self->_recognitionRequest appendAudioPCMBuffer:buffer];
  }];

  [_audioEngine prepare];
  NSError* error = nil;
  [_audioEngine startAndReturnError:&error];

  if (error)
  {
    NSLog(@"DarwinVoiceInput: Failed to start audio engine: %@", error);
    return NO;
  }

  __weak typeof(self) weakSelf = self;
  _recognitionTask = [_speechRecognizer recognitionTaskWithRequest:_recognitionRequest
                                                    resultHandler:^(SFSpeechRecognitionResult* result, NSError* error) {
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (!strongSelf || !strongSelf.owner)
      return;

    if (error)
    {
      NSLog(@"DarwinVoiceInput: Recognition error: %@", error);
      std::string errorMsg = [error.localizedDescription UTF8String];
      strongSelf.owner->OnError(static_cast<int>(error.code), errorMsg);
      return;
    }

    if (result)
    {
      std::string text = [result.bestTranscription.formattedString UTF8String];
      float confidence = result.bestTranscription.segments.count > 0 ?
                        result.bestTranscription.segments.firstObject.confidence : 0.5f;

      if (result.isFinal)
      {
        strongSelf.owner->OnFinalResults(text, confidence);
      }
      else
      {
        strongSelf.owner->OnPartialResults(text, confidence);
      }
    }
  }];

  return YES;
#else
  // macOS implementation
  [_speechRecognizer startListening];
  return YES;
#endif
}

- (void)stopListening
{
#if defined(TARGET_DARWIN_IOS) || defined(TARGET_DARWIN_TVOS)
  if (_audioEngine.isRunning)
  {
    [_audioEngine stop];
    [_recognitionRequest endAudio];
    [_audioEngine.inputNode removeTapOnBus:0];
  }

  if (_recognitionTask)
  {
    [_recognitionTask finish];
    _recognitionTask = nil;
  }
#else
  [_speechRecognizer stopListening];
#endif
}

- (void)cancel
{
#if defined(TARGET_DARWIN_IOS) || defined(TARGET_DARWIN_TVOS)
  if (_recognitionTask)
  {
    [_recognitionTask cancel];
    _recognitionTask = nil;
  }
  [self stopListening];
#else
  [_speechRecognizer stopListening];
#endif
}

- (BOOL)hasPermissions
{
#if defined(TARGET_DARWIN_IOS) || defined(TARGET_DARWIN_TVOS)
  SFSpeechRecognizerAuthorizationStatus status = [SFSpeechRecognizer authorizationStatus];
  return (status == SFSpeechRecognizerAuthorizationStatusAuthorized);
#else
  // macOS doesn't require explicit permission
  return YES;
#endif
}

- (void)requestPermissions
{
#if defined(TARGET_DARWIN_IOS) || defined(TARGET_DARWIN_TVOS)
  [SFSpeechRecognizer requestAuthorization:^(SFSpeechRecognizerAuthorizationStatus status) {
    dispatch_async(dispatch_get_main_queue(), ^{
      if (self.owner)
      {
        if (status == SFSpeechRecognizerAuthorizationStatusAuthorized)
        {
          self.owner->OnStatusChanged(KODI::SEMANTIC::VoiceInputStatus::Ready);
        }
        else
        {
          self.owner->OnError(9, "Speech recognition permission denied");
        }
      }
    });
  }];
#endif
}

- (void)setLanguage:(NSString*)languageCode
{
#if defined(TARGET_DARWIN_IOS) || defined(TARGET_DARWIN_TVOS)
  NSLocale* locale = [NSLocale localeWithLocaleIdentifier:languageCode];
  _speechRecognizer = [[SFSpeechRecognizer alloc] initWithLocale:locale];
  _speechRecognizer.delegate = self;
#else
  // macOS NSSpeechRecognizer doesn't support language change at runtime
#endif
}

#if !defined(TARGET_DARWIN_IOS) && !defined(TARGET_DARWIN_TVOS)
// macOS delegate methods
- (void)speechRecognizer:(NSSpeechRecognizer*)sender didRecognizeCommand:(id)command
{
  if (_owner && command)
  {
    std::string text = [[command description] UTF8String];
    _owner->OnFinalResults(text, 0.8f);
  }
}
#endif

@end
#endif // __OBJC__

namespace KODI
{
namespace SEMANTIC
{

CDarwinVoiceInput::CDarwinVoiceInput()
{
#ifdef __OBJC__
  m_impl = (__bridge_retained DarwinVoiceInputImpl*)[[DarwinVoiceInputImpl alloc] initWithOwner:this];
#endif
}

CDarwinVoiceInput::~CDarwinVoiceInput()
{
  Shutdown();
#ifdef __OBJC__
  if (m_impl)
  {
    CFBridgingRelease(m_impl);
    m_impl = nullptr;
  }
#endif
}

bool CDarwinVoiceInput::Initialize()
{
  CLog::Log(LOGINFO, "DarwinVoiceInput: Initializing");

#ifdef __OBJC__
  DarwinVoiceInputImpl* impl = (__bridge DarwinVoiceInputImpl*)m_impl;
  if (![impl initialize])
  {
    CLog::Log(LOGERROR, "DarwinVoiceInput: Failed to initialize");
    return false;
  }

  if (![impl hasPermissions])
  {
    CLog::Log(LOGWARNING, "DarwinVoiceInput: No permissions");
    m_status = VoiceInputStatus::Error;
    if (m_listener)
      m_listener->OnError(VoiceInputError::PermissionDenied, "Speech recognition permission required");
    return false;
  }

  m_status = VoiceInputStatus::Ready;
  CLog::Log(LOGINFO, "DarwinVoiceInput: Initialized successfully");
  return true;
#else
  return false;
#endif
}

void CDarwinVoiceInput::Shutdown()
{
  CLog::Log(LOGINFO, "DarwinVoiceInput: Shutting down");

#ifdef __OBJC__
  if (m_impl)
  {
    DarwinVoiceInputImpl* impl = (__bridge DarwinVoiceInputImpl*)m_impl;
    [impl shutdown];
  }
#endif

  m_status = VoiceInputStatus::Idle;
}

bool CDarwinVoiceInput::IsAvailable() const
{
  return m_status == VoiceInputStatus::Ready || m_status == VoiceInputStatus::Listening;
}

bool CDarwinVoiceInput::HasPermissions() const
{
#ifdef __OBJC__
  if (m_impl)
  {
    DarwinVoiceInputImpl* impl = (__bridge DarwinVoiceInputImpl*)m_impl;
    return [impl hasPermissions];
  }
#endif
  return false;
}

bool CDarwinVoiceInput::RequestPermissions()
{
  CLog::Log(LOGINFO, "DarwinVoiceInput: Requesting permissions");

#ifdef __OBJC__
  if (m_impl)
  {
    DarwinVoiceInputImpl* impl = (__bridge DarwinVoiceInputImpl*)m_impl;
    [impl requestPermissions];
    return true;
  }
#endif
  return false;
}

std::vector<VoiceLanguage> CDarwinVoiceInput::GetSupportedLanguages() const
{
  std::vector<VoiceLanguage> languages;

  // Common languages supported by Apple Speech framework
  languages.push_back({"en-US", "English (United States)", true});
  languages.push_back({"en-GB", "English (United Kingdom)", false});
  languages.push_back({"fr-FR", "French (France)", false});
  languages.push_back({"de-DE", "German (Germany)", false});
  languages.push_back({"es-ES", "Spanish (Spain)", false});
  languages.push_back({"it-IT", "Italian (Italy)", false});
  languages.push_back({"ja-JP", "Japanese (Japan)", false});
  languages.push_back({"zh-CN", "Chinese (Simplified)", false});
  languages.push_back({"ko-KR", "Korean (Korea)", false});

  return languages;
}

bool CDarwinVoiceInput::SetLanguage(const std::string& languageCode)
{
  m_languageCode = languageCode;

#ifdef __OBJC__
  if (m_impl)
  {
    DarwinVoiceInputImpl* impl = (__bridge DarwinVoiceInputImpl*)m_impl;
    [impl setLanguage:[NSString stringWithUTF8String:languageCode.c_str()]];
  }
#endif

  CLog::Log(LOGINFO, "DarwinVoiceInput: Language set to {}", languageCode);
  return true;
}

std::string CDarwinVoiceInput::GetLanguage() const
{
  return m_languageCode;
}

void CDarwinVoiceInput::SetMode(VoiceInputMode mode)
{
  m_mode = mode;
}

VoiceInputMode CDarwinVoiceInput::GetMode() const
{
  return m_mode;
}

bool CDarwinVoiceInput::StartListening()
{
  if (m_isListening)
    return false;

  CLog::Log(LOGINFO, "DarwinVoiceInput: Starting listening");

#ifdef __OBJC__
  if (m_impl)
  {
    DarwinVoiceInputImpl* impl = (__bridge DarwinVoiceInputImpl*)m_impl;
    if ([impl startListening])
    {
      m_isListening = true;
      m_status = VoiceInputStatus::Listening;

      if (m_listener)
        m_listener->OnVoiceStatusChanged(VoiceInputStatus::Listening);

      return true;
    }
  }
#endif

  return false;
}

void CDarwinVoiceInput::StopListening()
{
  if (!m_isListening)
    return;

  CLog::Log(LOGINFO, "DarwinVoiceInput: Stopping listening");

#ifdef __OBJC__
  if (m_impl)
  {
    DarwinVoiceInputImpl* impl = (__bridge DarwinVoiceInputImpl*)m_impl;
    [impl stopListening];
  }
#endif

  m_isListening = false;
  m_status = VoiceInputStatus::Ready;

  if (m_listener)
    m_listener->OnVoiceStatusChanged(VoiceInputStatus::Ready);
}

void CDarwinVoiceInput::Cancel()
{
  CLog::Log(LOGINFO, "DarwinVoiceInput: Cancelling");

#ifdef __OBJC__
  if (m_impl)
  {
    DarwinVoiceInputImpl* impl = (__bridge DarwinVoiceInputImpl*)m_impl;
    [impl cancel];
  }
#endif

  m_isListening = false;
  m_status = VoiceInputStatus::Ready;

  if (m_listener)
    m_listener->OnError(VoiceInputError::Cancelled, "Recognition cancelled");
}

bool CDarwinVoiceInput::IsListening() const
{
  return m_isListening;
}

VoiceInputStatus CDarwinVoiceInput::GetStatus() const
{
  return m_status;
}

void CDarwinVoiceInput::SetListener(IVoiceInputListener* listener)
{
  m_listener = listener;
}

std::string CDarwinVoiceInput::GetPlatformName() const
{
#if defined(TARGET_DARWIN_IOS)
  return "iOS Speech Framework";
#elif defined(TARGET_DARWIN_TVOS)
  return "tvOS Speech Framework";
#else
  return "macOS Speech Framework";
#endif
}

// Callbacks from Objective-C
void CDarwinVoiceInput::OnPartialResults(const std::string& text, float confidence)
{
  if (m_listener)
  {
    VoiceRecognitionResult result;
    result.text = text;
    result.confidence = confidence;
    result.languageCode = m_languageCode;
    result.isFinal = false;

    m_listener->OnPartialResult(result);
  }
}

void CDarwinVoiceInput::OnFinalResults(const std::string& text, float confidence)
{
  if (m_listener)
  {
    VoiceRecognitionResult result;
    result.text = text;
    result.confidence = confidence;
    result.languageCode = m_languageCode;
    result.isFinal = true;

    CLog::Log(LOGINFO, "DarwinVoiceInput: Final result: '{}' (confidence: {:.2f})",
              text, confidence);
    m_listener->OnFinalResult(result);
  }

  m_isListening = false;
  m_status = VoiceInputStatus::Ready;
}

void CDarwinVoiceInput::OnError(int errorCode, const std::string& message)
{
  CLog::Log(LOGERROR, "DarwinVoiceInput: Error {}: {}", errorCode, message);

  VoiceInputError error = VoiceInputError::RecognitionFailed;

  // Map error codes
  if (errorCode == 9)
    error = VoiceInputError::PermissionDenied;

  m_isListening = false;
  m_status = VoiceInputStatus::Error;

  if (m_listener)
  {
    m_listener->OnError(error, message);
    m_listener->OnVoiceStatusChanged(VoiceInputStatus::Error);
  }
}

void CDarwinVoiceInput::OnStatusChanged(VoiceInputStatus status)
{
  m_status = status;
  if (m_listener)
    m_listener->OnVoiceStatusChanged(status);
}

void CDarwinVoiceInput::OnVolumeChanged(float level)
{
  if (m_listener)
    m_listener->OnVolumeChanged(level);
}

} // namespace SEMANTIC
} // namespace KODI

#endif // TARGET_DARWIN
