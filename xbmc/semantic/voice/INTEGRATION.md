# Voice Search Integration Guide

This document describes how to integrate voice search into the Kodi Semantic Search dialog.

## Files Created

### Core Voice Input System
1. **IVoiceInput.h** - Voice input interface with listener callbacks
2. **VoiceInputManager.h/.cpp** - Factory for creating platform-specific implementations
3. **VoiceSearchIntegration.h/.cpp** - Helper class for easy integration into dialogs
4. **CMakeLists.txt** - Build configuration

### Platform Implementations
1. **WindowsVoiceInput.h/.cpp** - Windows SAPI 5.4 implementation
2. **AndroidVoiceInput.h/.cpp** - Android SpeechRecognizer via JNI
3. **LinuxVoiceInput.h/.cpp** - Linux PocketSphinx implementation
4. **DarwinVoiceInput.h/.cpp** - macOS/iOS Speech Framework (Objective-C++)
5. **StubVoiceInput.h/.cpp** - Fallback for unsupported platforms

## Integration into CGUIDialogSemanticSearch

### 1. Header Changes (CGUIDialogSemanticSearch.h)

Add include:
```cpp
#include "semantic/voice/VoiceSearchIntegration.h"
```

Update class declaration:
```cpp
class CGUIDialogSemanticSearch : public CGUIDialog, public CThread
{
  // ... existing members ...

  /*!
   * @brief Toggle voice input on/off
   */
  void ToggleVoiceInput();

  /*!
   * @brief Check if voice input is available
   * @return true if voice input is supported and ready
   */
  bool IsVoiceInputAvailable() const;

  /*!
   * @brief Update voice button state
   */
  void UpdateVoiceButton();

private:
  // ... existing members ...

  // Voice input
  std::unique_ptr<VoiceSearchIntegration> m_voiceIntegration;
  bool m_isListeningForVoice{false};
};
```

Update control IDs comment to include:
```cpp
 * - 27: Voice search button
 * - 28: Voice status indicator
```

### 2. Implementation Changes (CGUIDialogSemanticSearch.cpp)

In `Initialize()`:
```cpp
bool CGUIDialogSemanticSearch::Initialize(...)
{
  // ... existing initialization ...

  // Initialize voice input
  m_voiceIntegration = std::make_unique<VoiceSearchIntegration>();
  m_voiceIntegration->Initialize(
      [this](const std::string& query) {
        // Voice query callback
        SetQueryFromVoice(query);
      },
      [this](VoiceInputStatus status, const std::string& message) {
        // Voice status callback
        UpdateVoiceButton();
        if (!message.empty())
        {
          SET_CONTROL_LABEL(28, message);
          if (status == VoiceInputStatus::Listening ||
              status == VoiceInputStatus::Processing)
          {
            SET_CONTROL_VISIBLE(28);
          }
          else
          {
            SET_CONTROL_HIDDEN(28);
          }
        }
      });

  return true;
}
```

In `Shutdown()`:
```cpp
void CGUIDialogSemanticSearch::Shutdown()
{
  if (m_voiceIntegration)
  {
    m_voiceIntegration->Shutdown();
  }
}
```

In `OnMessage()`, handle voice button click:
```cpp
bool CGUIDialogSemanticSearch::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      int controlId = message.GetSenderId();

      // ... existing button handling ...

      if (controlId == 27) // Voice button
      {
        ToggleVoiceInput();
        return true;
      }
      break;
    }
  }
  return CGUIDialog::OnMessage(message);
}
```

Add new methods:
```cpp
void CGUIDialogSemanticSearch::ToggleVoiceInput()
{
  if (!m_voiceIntegration || !m_voiceIntegration->IsAvailable())
  {
    UpdateSearchStatus("Voice input not available");
    return;
  }

  if (m_voiceIntegration->ToggleListening())
  {
    m_isListeningForVoice = m_voiceIntegration->IsListening();
    UpdateVoiceButton();
  }
}

bool CGUIDialogSemanticSearch::IsVoiceInputAvailable() const
{
  return m_voiceIntegration && m_voiceIntegration->IsAvailable();
}

void CGUIDialogSemanticSearch::UpdateVoiceButton()
{
  if (!m_voiceIntegration)
    return;

  if (m_voiceIntegration->IsListening())
  {
    // Update button to show "listening" state
    SET_CONTROL_LABEL(27, "🎤 Listening...");
  }
  else
  {
    SET_CONTROL_LABEL(27, "🎤");
  }
}

void CGUIDialogSemanticSearch::SetQueryFromVoice(const std::string& query)
{
  m_currentQuery = query;

  // Set the query in the search input
  CGUIEditControl* editControl =
      dynamic_cast<CGUIEditControl*>(GetControl(CONTROL_SEARCH_INPUT));
  if (editControl)
  {
    editControl->SetLabel2(query);
  }

  // Trigger search
  m_needsUpdate = true;
  if (!IsRunning())
  {
    Create();
  }

  // Stop listening
  if (m_voiceIntegration)
  {
    m_voiceIntegration->StopListening();
  }

  m_isListeningForVoice = false;
  UpdateVoiceButton();
}
```

### 3. XML Skin Changes (DialogSemanticSearch.xml)

After the search input field (control id="2"), add:

```xml
<!-- Voice search button -->
<control type="button" id="27">
	<left>940</left>
	<top>70</top>
	<width>50</width>
	<height>50</height>
	<font>font12</font>
	<label>🎤</label>
	<texturefocus border="5">buttons/button-focus.png</texturefocus>
	<texturenofocus border="5">buttons/button-nofocus.png</texturenofocus>
	<onclick>-</onclick>
	<onup>10</onup>
	<ondown>3</ondown>
	<onleft>2</onleft>
	<onright>10</onright>
	<visible>System.HasVoiceInput</visible>
</control>

<!-- Voice status indicator -->
<control type="label" id="28">
	<left>30</left>
	<top>125</top>
	<width>900</width>
	<height>30</height>
	<font>font10</font>
	<textcolor>blue</textcolor>
	<align>center</align>
	<label></label>
	<visible>false</visible>
</control>
```

Update Clear button (control id="10") positions:
```xml
<left>1000</left>
<width>60</width>
<onleft>27</onleft>
```

Update Search input (control id="2") navigation:
```xml
<onright>27</onright>
```

### 4. Build Integration

Add to parent CMakeLists.txt:
```cmake
add_subdirectory(voice)
```

Link against the voice library in the semantic library target:
```cmake
target_link_libraries(semantic_search
  PRIVATE
    semantic_voice
)
```

## Platform-Specific Notes

### Windows
- Uses SAPI 5.4+ (built into Windows Vista and later)
- No additional permissions required
- Supports free-form dictation
- Real-time partial results

### Android
- Requires RECORD_AUDIO permission in AndroidManifest.xml
- Needs Java helper class (VoiceInputHelper.java) in Android app
- Uses Google Speech Recognition service
- May require internet connection for cloud processing

### Linux
- Requires PocketSphinx library (optional dependency)
- Offline recognition (no internet required)
- Lower accuracy than cloud-based solutions
- Install: `sudo apt-get install pocketsphinx pocketsphinx-en-us`

### macOS/iOS
- Uses Apple Speech Framework
- iOS requires microphone permission in Info.plist
- High accuracy, on-device processing
- iOS 10+ required

### Unsupported Platforms
- Stub implementation returns "not supported" errors
- Voice button is hidden via `System.HasVoiceInput` visibility condition

## Testing

1. **Check availability**: Verify voice button appears on supported platforms
2. **Permissions**: Test permission request flow on Android/iOS
3. **Recognition**: Speak test queries and verify transcription
4. **Partial results**: Check real-time transcription feedback
5. **Error handling**: Test with no microphone, denied permissions
6. **UI feedback**: Verify status indicators update correctly
7. **Integration**: Confirm voice query triggers search correctly

## Future Enhancements

- Language selection UI
- Voice activity detection for hands-free mode
- Offline support on more platforms
- Custom wake word support
- Volume level visualization
- Alternative transcription services (Google, Azure, etc.)

## Control ID Reference

- **27**: Voice search button
- **28**: Voice status label (shows "Listening...", "Processing...", etc.)

## Dependencies

### Required
- Kodi core libraries
- Platform SDKs (Windows SDK, Android NDK, iOS SDK, etc.)

### Optional
- PocketSphinx (Linux)
- Google Play Services (Android - for speech recognition)

## Troubleshooting

### Voice button not showing
- Check platform support in VoiceInputManager
- Verify CMake detected platform correctly
- Check skin visibility condition

### No microphone detected
- Verify hardware permissions
- Check system audio settings
- Review platform-specific logs

### Poor recognition accuracy
- Check microphone quality/positioning
- Verify correct language is selected
- Consider switching to cloud-based service (Android/iOS)

### Crashes on startup
- Verify all platform-specific libraries are linked
- Check COM initialization on Windows
- Review JNI setup on Android
