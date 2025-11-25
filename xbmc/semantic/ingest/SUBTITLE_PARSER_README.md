# Subtitle Parser Implementation

## Overview

The `CSubtitleParser` class provides comprehensive support for parsing subtitle files in multiple formats (SRT, ASS/SSA, VTT) for Kodi's semantic search functionality. It extracts dialogue text, timing information, and speaker identification where available.

## Supported Formats

### 1. SRT (SubRip)
- **Extension**: `.srt`
- **Timestamp Format**: `HH:MM:SS,mmm`
- **Features**:
  - Simple text-based format
  - Multi-line entries supported
  - HTML tags are stripped
  - Sequence numbers are ignored

**Example**:
```
1
00:00:01,000 --> 00:00:04,500
Hello, world!

2
00:00:05,000 --> 00:00:08,200
This is a <i>test</i> subtitle.
```

### 2. ASS/SSA (Advanced SubStation Alpha)
- **Extensions**: `.ass`, `.ssa`
- **Timestamp Format**: `H:MM:SS.cc` (centiseconds)
- **Features**:
  - Advanced formatting with styles
  - Speaker identification via Style field
  - ASS formatting codes stripped (`{\an8}`, `{\pos(x,y)}`, etc.)
  - Multi-layer support

**Example**:
```
[Events]
Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
Dialogue: 0,0:00:01.00,0:00:04.50,Default,,0,0,0,,Hello, world!
Dialogue: 0,0:00:05.00,0:00:08.20,Speaker1,,0,0,0,,{\an8}This is a test.
```

### 3. VTT (WebVTT)
- **Extension**: `.vtt`
- **Timestamp Format**: `HH:MM:SS.mmm` or `MM:SS.mmm`
- **Features**:
  - Web-standard format
  - Cue identifiers supported
  - Cue settings (positioning, alignment) ignored
  - NOTE blocks skipped
  - HTML tags stripped

**Example**:
```
WEBVTT

00:00:01.000 --> 00:00:04.500
Hello, world!

cue-2
00:00:05.000 --> 00:00:08.200 align:start
This is a test.

NOTE This is a comment
```

## Key Features

### Text Cleaning
All parsers utilize `ContentParserBase` utilities for consistent text processing:
- **`StripHTMLTags()`**: Removes HTML tags like `<b>`, `<i>`, `<font>`
- **`NormalizeWhitespace()`**: Removes duplicate spaces, normalizes line endings
- **`DetectCharset()`**: Auto-detects and converts encodings (UTF-8, UTF-16, Latin-1)
- **`IsNonDialogue()`**: Filters non-dialogue content

### Non-Dialogue Filtering
The parser automatically filters out common non-dialogue content:
- Music indicators: `[music]`, `♪`, `♫`
- Sound effects: `[door opens]`, `[phone rings]`, `[laughs]`
- Atmospheric: `[silence]`, `[static]`, `[inaudible]`
- Parenthetical sounds: `(door opens)`, `(phone rings)`

### Encoding Support
- UTF-8 with or without BOM
- UTF-16 LE/BE with BOM
- Latin-1 and other common subtitle encodings
- Automatic charset detection and conversion

### Subtitle Discovery
`FindSubtitleForMedia()` automatically locates subtitle files for a video:

**Search Order**:
1. Same directory as media file:
   - `movie.srt`, `movie.ass`, `movie.ssa`, `movie.vtt`
   - With language codes: `movie.en.srt`, `movie.eng.srt`, etc.
2. `Subs/` subdirectory with same patterns

**Supported Language Codes**:
- en, eng (English)
- es, spa (Spanish)
- fr, fre (French)
- de, ger (German)
- it, ita (Italian)
- pt, por (Portuguese)
- ru, rus (Russian)
- ja, jpn (Japanese)
- zh, chi (Chinese)
- ko, kor (Korean)
- And 40+ more...

## Usage Examples

### Basic Parsing

```cpp
#include "semantic/ingest/SubtitleParser.h"

using namespace KODI::SEMANTIC;

CSubtitleParser parser;

// Check if file is supported
if (parser.CanParse("/path/to/movie.srt"))
{
    // Parse subtitle file
    std::vector<ParsedEntry> entries = parser.Parse("/path/to/movie.srt");

    for (const auto& entry : entries)
    {
        std::cout << "Time: " << entry.startMs << " - " << entry.endMs << "ms\n";
        std::cout << "Text: " << entry.text << "\n";
        if (!entry.speaker.empty())
            std::cout << "Speaker: " << entry.speaker << "\n";
        std::cout << "Confidence: " << entry.confidence << "\n\n";
    }
}
```

### Subtitle Discovery

```cpp
#include "semantic/ingest/SubtitleParser.h"

using namespace KODI::SEMANTIC;

std::string mediaPath = "/movies/example/movie.mkv";
std::string subtitlePath = CSubtitleParser::FindSubtitleForMedia(mediaPath);

if (!subtitlePath.empty())
{
    CSubtitleParser parser;
    auto entries = parser.Parse(subtitlePath);
    // Process entries...
}
else
{
    std::cout << "No subtitle file found\n";
}
```

### Format Detection

```cpp
#include "semantic/ingest/SubtitleParser.h"

using namespace KODI::SEMANTIC;

CSubtitleParser parser;

// Get all supported extensions
auto extensions = parser.GetSupportedExtensions();
// Returns: {"srt", "ass", "ssa", "vtt"}

// Check specific formats
bool canParseSRT = parser.CanParse("movie.srt");   // true
bool canParseASS = parser.CanParse("movie.ass");   // true
bool canParseTXT = parser.CanParse("movie.txt");   // false
```

## Implementation Details

### Timestamp Conversion

All timestamps are converted to milliseconds for consistency:

**SRT**: `01:23:45,678` → `5025678 ms`
```cpp
int64_t ParseSRTTimestamp(const std::string& timestamp);
// Format: HH:MM:SS,mmm
```

**ASS**: `1:23:45.67` → `5025670 ms`
```cpp
int64_t ParseASSTimestamp(const std::string& timestamp);
// Format: H:MM:SS.cc (centiseconds × 10)
```

**VTT**: `01:23:45.678` → `5025678 ms`
```cpp
int64_t ParseVTTTimestamp(const std::string& timestamp);
// Format: HH:MM:SS.mmm or MM:SS.mmm
```

### ASS Code Stripping

The parser removes ASS/SSA formatting codes:
- `{\an8}` - Alignment codes
- `{\pos(x,y)}` - Positioning
- `{\fad(in,out)}` - Fade timing
- `{\i1}`, `{\b1}` - Italic, bold toggles
- `{\p1}...{\p0}` - Drawing commands
- `{\fs30}` - Font size
- And all other ASS override tags

### Error Handling

```cpp
try {
    auto entries = parser.Parse("/path/to/subtitle.srt");
}
catch (const std::runtime_error& e) {
    // Handle errors:
    // - File not found
    // - File too large (>5MB)
    // - Read errors
    // - Invalid format
    std::cerr << "Parse error: " << e.what() << "\n";
}
```

## ParsedEntry Structure

```cpp
struct ParsedEntry
{
    int64_t startMs;        // Start time in milliseconds
    int64_t endMs;          // End time in milliseconds
    std::string text;       // Cleaned dialogue text
    std::string speaker;    // Speaker name (ASS only, if available)
    float confidence;       // Always 1.0 for subtitles
};
```

## Integration with ContentParserBase

The `CSubtitleParser` inherits from `ContentParserBase` and uses its utilities:

```cpp
class CSubtitleParser : public ContentParserBase
{
    // Inherited utilities:
    // - StripHTMLTags(text)
    // - NormalizeWhitespace(text)
    // - DetectCharset(content)
    // - IsNonDialogue(text)
    // - HasSupportedExtension(path, extensions)
};
```

## Performance Characteristics

- **File Size Limit**: 5 MB maximum to prevent memory issues
- **Encoding Detection**: Automatic with BOM detection and fallback
- **Memory Efficient**: Processes files line-by-line where possible
- **Filtering**: Non-dialogue content filtered during parsing (zero overhead)

## Test Data

Example subtitle files are provided in `/home/user/xbmc/xbmc/semantic/ingest/test/data/`:
- `example.srt` - SRT format with various features
- `example.ass` - ASS format with styles and formatting
- `example.vtt` - VTT format with cues and comments

## Future Enhancements

Potential improvements for future waves:
1. **More Formats**: Support for MicroDVD, MPL, SUB
2. **Language Detection**: Auto-detect subtitle language
3. **OCR Support**: Parse image-based subtitles (PGS, VobSub)
4. **Timing Adjustment**: Handle subtitle sync offsets
5. **Speaker Detection**: Improved speaker identification from text patterns
6. **Confidence Scoring**: Quality assessment based on formatting, completeness

## References

- Kodi's existing subtitle parsers: `xbmc/cores/VideoPlayer/DVDSubtitles/`
- SubRip format: https://en.wikipedia.org/wiki/SubRip
- ASS format: http://www.tcax.org/docs/ass-specs.htm
- WebVTT format: https://www.w3.org/TR/webvtt1/
