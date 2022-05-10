/*
 *  Copyright (C) 2012-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SpeechRecognitionDarwin.h"

#include "LangInfo.h"
#include "speech/ISpeechRecognitionListener.h"
#include "speech/SpeechRecognitionErrors.h"
#include "threads/CriticalSection.h"
#include "utils/log.h"

#include <algorithm>
#include <mutex>
#include <vector>

#import <AVFoundation/AVFoundation.h>
#import <Speech/Speech.h>

API_AVAILABLE(macos(10.15), ios(10.0))
API_UNAVAILABLE(tvos) @interface SpeechRecognitionImpl : NSObject<SFSpeechRecognizerDelegate>

@property(nonatomic, strong) SFSpeechRecognizer* speechRecognizer;
@property(nonatomic, strong) SFSpeechAudioBufferRecognitionRequest* recognitionRequest;
@property(nonatomic, strong) SFSpeechRecognitionTask* recognitionTask;
@property(nonatomic, strong) AVAudioEngine* audioEngine;
@property(nonatomic, strong) NSTimer* talkTimeoutTimer;
@property(nonatomic, copy) NSString* text;

// C++ members
@property(nonatomic) speech::ISpeechRecognitionListener* listener;
@property(nonatomic) CSpeechRecognitionDarwin* owner;

@end

@implementation SpeechRecognitionImpl

- (void)startSpeechRecognition:(speech::ISpeechRecognitionListener*)listener
                         owner:(CSpeechRecognitionDarwin*)owner
{
  self.listener = listener;
  self.owner = owner;

  // Get current Kodi GUI locale and use it for speech recognition.
  std::string kodiLocale = g_langInfo.GetLocale().ToShortString();
  std::replace(kodiLocale.begin(), kodiLocale.end(), '_', '-');
  NSString* locale = @(kodiLocale.c_str());

  self.speechRecognizer =
      [[SFSpeechRecognizer alloc] initWithLocale:[NSLocale localeWithLocaleIdentifier:locale]];
  if (self.speechRecognizer == nil)
  {
    CLog::LogF(LOGWARNING,
               "Speech recognizer not available for user's current locale. Trying en-US");
    self.speechRecognizer =
        [[SFSpeechRecognizer alloc] initWithLocale:[NSLocale localeWithLocaleIdentifier:@"en-US"]];
  }
  if (self.speechRecognizer == nil)
  {
    [self onError:speech::RecognitionError::SERVICE_NOT_AVAILABLE
        logMessage:@"Unable to create an SFSpeechRecognizer instance"];
    return;
  }

  [self.speechRecognizer setDelegate:self];

  self.audioEngine = [[AVAudioEngine alloc] init];
  if (self.audioEngine == nil)
  {
    [self onError:speech::RecognitionError::SERVICE_NOT_AVAILABLE
        logMessage:@"Unable to create an AVAudioEngine instance"];
    return;
  }

  [SFSpeechRecognizer requestAuthorization:^(SFSpeechRecognizerAuthorizationStatus authStatus) {
    switch (authStatus)
    {
      case SFSpeechRecognizerAuthorizationStatusAuthorized: // User gave access to speech recognition
        break;

      case SFSpeechRecognizerAuthorizationStatusDenied: // User denied access to speech recognition
      case SFSpeechRecognizerAuthorizationStatusRestricted: // Speech recognition restricted on this device
      case SFSpeechRecognizerAuthorizationStatusNotDetermined: // Speech recognition not yet authorized
      default:
        [self onError:speech::RecognitionError::INSUFFICIENT_PERMISSIONS
            logMessage:@"Insufficient permissions"];
        break;
    }
  }];

  listener->OnReadyForSpeech();

  self.recognitionRequest = [[SFSpeechAudioBufferRecognitionRequest alloc] init];
  if (self.recognitionRequest == nil)
  {
    [self onError:speech::RecognitionError::SERVICE_NOT_AVAILABLE
        logMessage:@"Unable to create an SFSpeechAudioBufferRecognitionRequest instance"];
    return;
  }

  self.recognitionRequest.shouldReportPartialResults = YES;

  AVAudioNode* inputNode = [self.audioEngine inputNode];
  if (inputNode == nil)
  {
    [self onError:speech::RecognitionError::AUDIO
        logMessage:@"Audio engine instance has no input node"];
    return;
  }

  [self.recognitionTask cancel];
  self.recognitionTask = nil;

  // stop recognition after 10 secs if the user did not start talking
  self.talkTimeoutTimer = [NSTimer scheduledTimerWithTimeInterval:10.0
                                                           target:self
                                                         selector:@selector(onTalkTimeout:)
                                                         userInfo:nil
                                                          repeats:NO];

  __typeof__(self) __weak welf = self;
  self.recognitionTask = [self.speechRecognizer
      recognitionTaskWithRequest:self.recognitionRequest
                   resultHandler:^(SFSpeechRecognitionResult* _Nullable result,
                                   NSError* _Nullable error) {
                     __typeof__(self) sself = welf;
                     if (!sself) // the object (self) is dead; it makes no sense to continue
                       return;

                     BOOL isFinal = NO;

                     // reset talk timeout timer to fire 3 secs after user stopped talking
                     [sself.talkTimeoutTimer invalidate];
                     sself.talkTimeoutTimer =
                         [NSTimer scheduledTimerWithTimeInterval:3.0
                                                          target:sself
                                                        selector:@selector(onTalkTimeout:)
                                                        userInfo:nil
                                                         repeats:NO];
                     if (result != nil)
                     {
                       isFinal = result.isFinal;
                       sself.text = result.bestTranscription.formattedString;
                       listener->OnResults({[sself.text UTF8String]});
                     }

                     if (error == nil && !isFinal)
                       return;

                     [sself.audioEngine stop];
                     [inputNode removeTapOnBus:0];

                     if (error != nil && !isFinal)
                     {
                       int recognitionError = speech::RecognitionError::UNKNOWN;

                       if (sself.text == nil)
                       {
                         recognitionError = speech::RecognitionError::NO_MATCH;
                       }
                       else if ([error.domain isEqualToString:@"kLSRErrorDomain"])
                       {
                         switch (error.code)
                         {
                           case 102: // Assets are not installed
                           case 201: // Siri or Dictation is disabled
                           case 300: // Failed to initialize recognizer
                             recognitionError = speech::RecognitionError::SERVICE_NOT_AVAILABLE;
                             break;

                           case 301: // Request was canceled
                             break;
                         }
                       }
                       else if ([error.domain isEqualToString:@"kAFAssistantErrorDomain"])
                       {
                         switch (error.code)
                         {
                           case 1100: // Trying to start recognition while an earlier instance is still active
                             recognitionError = speech::RecognitionError::RECOGNIZER_BUSY;
                             break;

                           case 1110: // Failed to recognize any speech
                             recognitionError = speech::RecognitionError::NO_MATCH;
                             break;

                           case 1700: // Request is not authorized
                             recognitionError = speech::RecognitionError::INSUFFICIENT_PERMISSIONS;
                             break;

                           case 203: // Failure occurred during speech recognition
                           case 1101: // Connection to speech process was invalidated
                           case 1107: // Connection to speech process was interrupted
                             break;
                         }
                       }

                       NSString* logMsg =
                           [NSString stringWithFormat:@"code='%ld' description='%@'",
                                                      (long)error.code, error.localizedDescription];
                       [sself onError:recognitionError logMessage:logMsg];
                     }
                     else
                     {
                       owner->OnRecognitionDone(listener);
                     }

                     sself.recognitionRequest = nil;
                     sself.recognitionTask = nil;
                     sself.text = nil;
                     [sself.talkTimeoutTimer invalidate];
                     sself.talkTimeoutTimer = nil;
                   }];

  [inputNode installTapOnBus:0
                  bufferSize:4096
                      format:[inputNode outputFormatForBus:0]
                       block:^(AVAudioPCMBuffer* buffer, AVAudioTime* when) {
                         [self.recognitionRequest appendAudioPCMBuffer:buffer];
                       }];

  [self.audioEngine prepare];

  NSError* outError;
  [self.audioEngine startAndReturnError:&outError];

  if (outError != nil)
  {
    [self onError:speech::RecognitionError::AUDIO
        logMessage:[NSString stringWithFormat:
                                 @"Audio engine couldn't start because of an error. code='%ld'",
                                 outError.code]];
  }
}

- (void)speechRecognizer:(SFSpeechRecognizer*)speechRecognizer availabilityDidChange:(BOOL)available
{
  if (available)
    return;

  [self.recognitionTask cancel];
  self.recognitionTask = nil;

  if (self.listener && self.owner)
  {
    [self onError:speech::RecognitionError::SERVICE_NOT_AVAILABLE
        logMessage:@"Service currently not available. Try again later"];
  }
}

- (void)onTalkTimeout:(NSTimer*)timer
{
  [self.recognitionRequest endAudio];
}

- (void)onError:(int)recognitionError logMessage:(NSString*)logMessage
{
  CLog::Log(LOGERROR, "Speech recognition error: {}", [logMessage UTF8String]);
  self.listener->OnError(recognitionError);
  self.owner->OnRecognitionDone(self.listener);
}

@end

std::shared_ptr<speech::ISpeechRecognition> speech::ISpeechRecognition::CreateInstance()
{
  return std::make_shared<CSpeechRecognitionDarwin>();
}

struct API_AVAILABLE(macos(10.15), ios(10.0)) API_UNAVAILABLE(tvos) SpeechRecognitionDarwinImpl
{
  CCriticalSection m_listenersMutex;
  std::vector<std::shared_ptr<speech::ISpeechRecognitionListener>> m_listeners;
  SpeechRecognitionImpl* m_recognizer{nil};
};

CSpeechRecognitionDarwin::CSpeechRecognitionDarwin() : m_impl(new SpeechRecognitionDarwinImpl)
{
}

void CSpeechRecognitionDarwin::StartSpeechRecognition(
    const std::shared_ptr<speech::ISpeechRecognitionListener>& listener)
{
  // Speech: macOS 10.15+ iOS 10.0+. Currently not available on tvOS!
  if (@available(macOS 10.15, iOS 10.0, *))
  {
    if (!m_impl->m_recognizer)
      m_impl->m_recognizer = [[SpeechRecognitionImpl alloc] init];

    if (m_impl->m_recognizer == nil)
    {
      CLog::LogF(LOGERROR, "Unable to create a SpeechRecognitionImpl instance");
      return;
    }

    std::unique_lock<CCriticalSection> lock(m_impl->m_listenersMutex);
    m_impl->m_listeners.emplace_back(
        listener); // we need to ensure the listener lives as long as we do
    lock.unlock();

    [m_impl->m_recognizer startSpeechRecognition:listener.get() owner:this];
  }
  else
  {
    CLog::LogF(LOGERROR, "Operating system does not match the minimum required version");
    listener->OnError(speech::RecognitionError::SERVICE_NOT_AVAILABLE);
  }
}

CSpeechRecognitionDarwin::~CSpeechRecognitionDarwin()
{
}

void CSpeechRecognitionDarwin::OnRecognitionDone(speech::ISpeechRecognitionListener* listener)
{
  std::unique_lock<CCriticalSection> lock(m_impl->m_listenersMutex);
  for (auto it = m_impl->m_listeners.begin(); it != m_impl->m_listeners.end(); ++it)
  {
    if ((*it).get() == listener)
    {
      m_impl->m_listeners.erase(it);
      break;
    }
  }
}
