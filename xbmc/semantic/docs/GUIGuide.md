# Semantic Search GUI Guide

## Overview

The Semantic Search dialog (`CGUIDialogSemanticSearch`) provides a powerful and intuitive interface for searching your media library using natural language queries.

---

## Opening the Search Dialog

### Methods

**1. Keyboard Shortcut** (Default: `Ctrl+F`)
- Press the configured keyboard shortcut from anywhere in Kodi

**2. Context Menu**
- Navigate to any media item
- Open context menu
- Select "Semantic Search"

**3. Voice Command** (if voice search enabled)
- Press voice input button/key
- Speak your query
- Dialog opens automatically with results

**4. Programmatic** (for skin developers)
```xml
<onclick>ActivateWindow(SemanticSearch)</onclick>
```

---

## Dialog Layout

```
┌─────────────────────────────────────────────────────┐
│  Semantic Search                          [Mode][X] │
├─────────────────────────────────────────────────────┤
│  🔍 [Search Input Field________________] [🎤] [⌫]  │
│                                                     │
│  Filters: [Type] [Genres] [Year] [Rating] [Clear]  │
│  Active: 🏷️ Action, 2020-2024, PG-13               │
├─────────────────────────────────────────────────────┤
│  Results (42 found) - 38ms                         │
│  ┌─────────────────────────────────────────────┐   │
│  │ ▶ The Dark Knight (2008)                    │   │
│  │   "Batman must fight the Joker..."          │   │
│  │   📍 01:23:45 | ⭐ 0.95 | 💬 Subtitle        │   │
│  ├─────────────────────────────────────────────┤   │
│  │ ▶ Batman Begins (2005)                      │   │
│  │   "Bruce Wayne becomes Batman to fight..."  │   │
│  │   📍 00:45:12 | ⭐ 0.87 | 🎤 Transcription   │   │
│  ├─────────────────────────────────────────────┤   │
│  │ ...                                          │   │
│  └─────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────┤
│  Context Preview: The Dark Knight - 01:23:45       │
│  ┌─────────────────────────────────────────────┐   │
│  │ [01:23:30] Batman: "I'm not a hero."        │   │
│  │ [01:23:45] Joker: "You're just a freak..."  │ ← │
│  │ [01:24:00] Batman: "This city needs me."    │   │
│  └─────────────────────────────────────────────┘   │
│                                                     │
│  [Play at Timestamp] [Similar Content] [Done]      │
└─────────────────────────────────────────────────────┘
```

### Control IDs Reference

| ID | Element | Description |
|----|---------|-------------|
| 1  | Heading | Dialog title |
| 2  | Search Input | Text edit control for queries |
| 3  | Results List | Scrollable results |
| 4  | Details Panel | Selected result details |
| 5  | Context Preview | Surrounding content |
| 6  | Mode Toggle | Hybrid/Keyword/Semantic |
| 7  | Media Filter | Movies/TV/Music Videos/All |
| 8  | Progress | Search progress indicator |
| 9  | Close Button | Close dialog |
| 10 | Clear Button | Clear search input |
| 11 | Status Label | Search status/errors |
| 12 | Result Count | Number of results |
| 13-26 | Filter Controls | Various filter options |

---

## Basic Search

### Text Search

1. **Open** the dialog
2. **Type** your query in the search field
3. **Wait** 500ms (debounce delay)
4. **View** results automatically

**Example Queries**:
- `action scene with explosion`
- `romantic conversation between two characters`
- `detective solving mystery`
- `final battle in science fiction movie`

### Search Tips

✅ **Natural Language**: Use complete phrases
- Good: "detective interrogating suspect in dark room"
- Okay: "detective interrogation"
- Poor: "detective"

✅ **Be Descriptive**: Add context
- Good: "car chase through city streets"
- Better: "fast car chase at night in rainy city"

✅ **Conceptual Search**: Think about meaning
- "someone making a difficult choice" ← finds dramatic scenes
- "happy reunion after long time" ← finds emotional moments

❌ **Avoid**:
- Single words (use filters instead)
- Overly generic queries
- Technical jargon (unless in technical content)

---

## Voice Search

### Platform Setup

Voice search requires platform-specific setup:

#### Windows

**Requirements**:
- Windows 10+ with speech recognition enabled
- Microphone configured in Windows Settings

**Setup**:
1. Go to Windows Settings → Privacy → Microphone
2. Enable microphone access for Kodi
3. Test microphone in Windows Speech Recognition

**Usage**:
- Click microphone button or press configured hotkey
- Speak clearly when prompted
- Wait for recognition (typically 1-3 seconds)

#### Android

**Requirements**:
- Android 5.0+ with Google Speech Services
- Microphone permission granted to Kodi

**Setup**:
1. Open Kodi → Settings → System → Semantic Search
2. Enable "Voice Search"
3. Grant microphone permission when prompted

**Usage**:
- Tap microphone icon
- Speak your query
- Results appear automatically

#### Linux

**Requirements**:
- PulseAudio or PipeWire audio system
- `pocketsphinx` installed (offline) OR
- Network connection (Google Speech API)

**Setup (Offline)**:
```bash
# Install PocketSphinx
sudo apt-get install pocketsphinx

# Configure in Kodi
Settings → System → Semantic Search → Voice Input
Provider: PocketSphinx (Offline)
```

**Setup (Online)**:
```bash
# No installation needed
Settings → System → Semantic Search → Voice Input
Provider: Google Speech (Online)
Language: English (US)
```

#### macOS / iOS

**Requirements**:
- macOS 10.14+ or iOS 13+
- Microphone permission granted

**Setup**:
1. System Preferences → Security & Privacy → Microphone
2. Enable for Kodi
3. Configure in Kodi settings

**Usage**:
- Click microphone button
- Speak query (Siri speech recognition)
- High accuracy, fast results

### Voice Search Settings

**Kodi Settings → System → Semantic Search → Voice**:

```
Enable Voice Search: ✓
Voice Provider: [Auto-detect]
Language: English (US)
Input Mode: Push-to-Talk  [HandsFree]
Confidence Threshold: 0.7
Show Partial Results: ✓
Continuous Listening: ✗
```

**Input Modes**:

- **Push-to-Talk** (Default): Click to start, click to stop
  - More reliable
  - Better for noisy environments
  - Lower battery usage

- **HandsFree**: Always listening for wake word
  - Convenient but experimental
  - May have false activations
  - Higher battery/CPU usage

### Voice Search Tips

✅ **Speak Clearly**: Enunciate words
✅ **Moderate Pace**: Not too fast or slow
✅ **Quiet Environment**: Reduce background noise
✅ **Short Queries**: 5-10 words work best

❌ **Avoid**:
- Whispering
- Speaking while far from microphone
- Complex punctuation (say "comma", "period")

### Troubleshooting Voice

**Problem**: Microphone not detected
- Check system permissions
- Verify microphone works in other apps
- Restart Kodi

**Problem**: Poor recognition accuracy
- Switch to different provider (online vs offline)
- Adjust confidence threshold (lower for easier matching)
- Check language setting matches your speech

**Problem**: Slow response
- Online providers need internet connection
- Try offline provider (PocketSphinx on Linux)
- Check network latency

---

## Search Filters

### Media Type Filter

**Purpose**: Limit search to specific media types

**Options**:
- 🎬 **Movies**: Feature films
- 📺 **TV Shows**: Episodes
- 🎵 **Music Videos**: Music videos
- 🌐 **All**: No filter (default)

**Usage**:
- Click "Type" button
- Select desired type
- Results update automatically

**Keyboard**: `Ctrl+T` to cycle through types

---

### Genre Filter

**Purpose**: Filter by one or more genres

**Usage**:
1. Click "Genres" button
2. Check/uncheck genres from list
3. Multiple genres use OR logic (any match)
4. Click "Apply" or press Enter

**Example**:
- Selected: `Action, Thriller`
- Matches: Movies/episodes with Action OR Thriller

**Available Genres** (loaded from video database):
- Action, Adventure, Animation, Comedy, Crime
- Documentary, Drama, Family, Fantasy, History
- Horror, Music, Mystery, Romance, Sci-Fi
- Thriller, War, Western, etc.

**Keyboard**: `Ctrl+G` to open genre selector

---

### Year Range Filter

**Purpose**: Filter by release/air year

**Interface**:
```
Year Range
Min: [1900] ────────●──── [2024] :Max
     1980                  2024

Presets:
[Last Year] [Last 5 Years] [Last 10 Years] [Custom]
```

**Usage**:
- Drag sliders to set range
- Or type year values directly
- Or use preset buttons

**Presets**:
- **Last Year**: 2024-2024
- **Last 5 Years**: 2020-2024
- **Last 10 Years**: 2015-2024
- **Custom**: Manual range

**Keyboard**: `Ctrl+Y` to open year filter

---

### Rating Filter

**Purpose**: Filter by content rating (MPAA/age)

**Options**:
- **All**: No rating filter
- **G**: General audiences
- **PG**: Parental guidance
- **PG-13**: Parents strongly cautioned
- **R**: Restricted (17+)
- **NC-17**: Adults only
- **Unrated**: No rating assigned

**Note**: Matches both MPAA ratings and equivalent age ratings (e.g., PG-13 = 12+)

**Keyboard**: `Ctrl+R` to cycle ratings

---

### Duration Filter

**Purpose**: Filter by video length

**Options**:
- **All**: No duration filter
- **Short**: Less than 30 minutes (clips, short episodes)
- **Medium**: 30-90 minutes (episodes, short films)
- **Long**: More than 90 minutes (movies, specials)

**Custom Duration**:
- Click "Custom" for minute-level control
- Set min/max duration in minutes

**Keyboard**: `Ctrl+D` to cycle duration filters

---

### Source Filter

**Purpose**: Filter by content source type

**Options** (multi-select):
- ☑️ **Subtitles**: Search subtitle text
- ☑️ **Transcription**: Search audio transcriptions
- ☑️ **Metadata**: Search titles, descriptions, cast, etc.

**Default**: All sources enabled

**Use Cases**:
- Subtitle only: Find dialogue/spoken content
- Transcription only: Find actual audio content
- Metadata only: Find by title/description/cast

**Example**:
- Query: "Morgan Freeman"
- Subtitle+Transcription: Scenes where he speaks
- Metadata: Movies/episodes he's in

**Keyboard**: `Ctrl+S` to toggle sources

---

### Filter Presets

**Purpose**: Save and load common filter combinations

**Usage**:
1. Configure filters as desired
2. Click "Save Preset" button
3. Enter preset name (e.g., "Recent Action")
4. Preset saved for future use

**Loading Presets**:
1. Click "Presets" button
2. Select from saved presets
3. All filters applied instantly

**Built-in Presets**:
- **Recent Releases**: Last 2 years, all types
- **Classic Movies**: Movies before 2000
- **Family Content**: G/PG ratings, all years
- **Recent TV**: TV shows, last 5 years

**Managing Presets**:
- Right-click preset → Rename
- Right-click preset → Delete
- Drag to reorder

**Keyboard**: `Ctrl+P` for preset menu

---

### Clearing Filters

**Methods**:
1. Click "Clear Filters" button (clears all)
2. Click individual filter badges (clears that filter)
3. Press `Ctrl+Shift+X` keyboard shortcut

**Filter Badges**:
```
Active Filters: [Action][Thriller][2020-2024][PG-13] [Clear All]
                  ×       ×         ×          ×
```
Click × on any badge to remove that filter.

---

## Search Modes

### Hybrid Mode (Default)

**Icon**: 🔀 Hybrid

**How it works**:
- Combines FTS5 keyword search + vector semantic search
- Uses Reciprocal Rank Fusion (RRF) to merge results
- Best overall relevance

**When to use**:
- General queries
- Unknown media
- Exploratory search

**Example**:
- Query: "detective solving mystery"
- Finds: Both exact phrase matches AND conceptually similar scenes

**Configuration**:
- Keyword weight: 40%
- Vector weight: 60%
- Can be adjusted in settings

---

### Keyword Mode

**Icon**: 🔤 Keyword

**How it works**:
- Traditional FTS5 full-text search
- BM25 ranking algorithm
- Fast, exact matches

**When to use**:
- Known exact phrases
- Quotes or dialogue search
- Very specific terms

**Example**:
- Query: "may the force be with you"
- Finds: Exact subtitle/transcription matches

**Advantages**:
- Fastest (no embedding overhead)
- Predictable results
- Best for exact quotes

---

### Semantic Mode

**Icon**: 🧠 Semantic

**How it works**:
- Pure vector similarity search
- Cosine similarity ranking
- Conceptual matching

**When to use**:
- Vague/exploratory queries
- Concept-based search
- No known keywords

**Example**:
- Query: "emotional goodbye scene"
- Finds: Farewell scenes even without those exact words

**Advantages**:
- Finds conceptually similar content
- Handles synonyms and paraphrasing
- Great for discovery

**Note**: Slower than keyword mode due to embedding generation

---

### Switching Modes

**Methods**:
1. Click mode button in top-right
2. Press `Ctrl+M` keyboard shortcut
3. Cycle: Hybrid → Keyword → Semantic → Hybrid

**Mode Indicator**:
```
[🔀 Hybrid ▼]  ← Click to change
```

**Auto-mode** (experimental):
- Analyzes query complexity
- Automatically selects best mode
- Enable in settings

---

## Search Results

### Result Display

Each result shows:

```
┌─────────────────────────────────────────────────────┐
│ ▶ Movie/Episode Title (Year)                  [⋮]  │
│   "Matched text snippet with highlight..."         │
│   📍 Timestamp | ⭐ Score | 💬 Source | 🎬 Type     │
└─────────────────────────────────────────────────────┘
```

**Components**:
- **▶ Play Icon**: Click to play at timestamp
- **Title**: Media item name with year
- **Snippet**: Context around match (with `<b>` highlights)
- **Timestamp**: Position in media (HH:MM:SS)
- **Score**: Relevance score (0.0-1.0, higher = better)
- **Source**: Subtitle/Transcription/Metadata
- **Type**: Movie/Episode/Music Video
- **⋮ Menu**: Actions (play, similar, info, etc.)

### Result Actions

**Click Result**:
- Opens context preview panel
- Shows surrounding content (±30 seconds)
- Highlights matched chunk

**Double-Click Result**:
- Plays media at timestamp
- Resumes from exact match point

**Right-Click Menu**:
- **Play at Timestamp**: Start playback at this point
- **Play from Beginning**: Start media from 00:00
- **Find Similar**: Search for similar content (vector search)
- **Show Context**: Display ±60 seconds around match
- **Media Information**: Open full media info dialog
- **Add to Playlist**: Add to current playlist
- **Queue**: Queue for later playback

**Keyboard Navigation**:
- `↑/↓`: Navigate results
- `Enter`: Play at timestamp
- `Space`: Preview context
- `S`: Find similar
- `I`: Media info
- `Q`: Queue

---

### Context Preview

**Purpose**: See surrounding content for better context

**Display**:
```
Context: The Dark Knight - 01:23:45 (±30s)

[01:23:15] Batman: "Where is he?"
[01:23:30] Joker: "You want to know..."
[01:23:45] Batman: "Tell me now!"      ← Matched chunk
[01:24:00] Joker: "Why so serious?"
[01:24:15] Batman: "This ends tonight."
```

**Features**:
- Shows chunks before and after match
- Highlights matched chunk with background
- Displays timestamp for each chunk
- Scrollable for longer context

**Adjusting Window**:
- Settings → Context Window: 15s / 30s / 60s / 120s
- Larger window = more context but slower

**Keyboard**: `Ctrl+K` to toggle context panel

---

### Sorting Results

**Default Sort**: Relevance (descending score)

**Sort Options**:
1. **Relevance**: Best matches first (default)
2. **Timestamp**: Chronological within each media
3. **Media Title**: Alphabetical by title
4. **Year**: Newest/oldest first
5. **Score**: Explicit score sorting

**Changing Sort**:
- Click column header to sort
- Click again to reverse order
- `Ctrl+Click` for secondary sort

**Keyboard**:
- `Ctrl+1-5`: Quick sort by option 1-5

---

## Search History

### Viewing History

**Access**:
- Click search input field
- Dropdown shows recent searches
- Click any entry to re-run search

**Display**:
```
Recent Searches:
  detective solving mystery (42 results)
  action scene explosion (18 results)
  romantic conversation (7 results)
  ...
```

**Limit**: Last 100 searches (configurable)

**Keyboard**: `↓` in empty search field to see history

---

### Search Suggestions

**Auto-complete** based on:
- Search history (your past searches)
- Popular queries (anonymized, if sharing enabled)
- Common phrases in indexed content

**Display**:
```
de█
  detective solving mystery
  detective interrogation
  detective noir style
```

**Usage**:
- Type to filter suggestions
- `↓/↑` to navigate
- `Enter` to select

**Settings**:
- Enable/disable suggestions
- Suggestion count (5/10/20)
- Include popular queries (privacy setting)

---

### Managing History

**Clear All History**:
1. Settings → Semantic Search → History
2. Click "Clear History"
3. Confirm deletion

**Or**: `Ctrl+Shift+H` in search dialog

**Privacy Mode**:
- Settings → Privacy Mode: ON
- Disables history recording
- No suggestions based on history
- Previous history preserved until manually cleared

**Per-Profile**:
- History is per-profile by default
- Each Kodi profile has separate history
- Master profile can view all histories (admin setting)

**Auto-cleanup**:
- Old entries automatically removed after limit reached
- Configurable: 50/100/500/unlimited
- Default: 100 most recent

---

## Keyboard Shortcuts

### Global Shortcuts

| Key | Action |
|-----|--------|
| `Ctrl+F` | Open semantic search dialog |
| `Ctrl+Shift+F` | Open with voice input |
| `Escape` | Close dialog |
| `Ctrl+W` | Close dialog |

### Search Input

| Key | Action |
|-----|--------|
| `Enter` | Execute search / select suggestion |
| `Ctrl+A` | Select all text |
| `Ctrl+X` | Cut |
| `Ctrl+C` | Copy |
| `Ctrl+V` | Paste |
| `Ctrl+Backspace` | Delete word |
| `↓` | Show history/suggestions |

### Result Navigation

| Key | Action |
|-----|--------|
| `↑/↓` | Navigate results |
| `Page Up/Down` | Scroll page |
| `Home` | First result |
| `End` | Last result |
| `Enter` | Play at timestamp |
| `Space` | Toggle context preview |

### Modes & Filters

| Key | Action |
|-----|--------|
| `Ctrl+M` | Cycle search mode |
| `Ctrl+T` | Cycle media type filter |
| `Ctrl+G` | Open genre filter |
| `Ctrl+Y` | Open year filter |
| `Ctrl+R` | Cycle rating filter |
| `Ctrl+D` | Cycle duration filter |
| `Ctrl+S` | Toggle source filters |
| `Ctrl+P` | Open filter presets |
| `Ctrl+Shift+X` | Clear all filters |

### Actions

| Key | Action |
|-----|--------|
| `S` | Find similar to selected |
| `I` | Media information |
| `Q` | Queue selected |
| `P` | Add to playlist |
| `Ctrl+K` | Toggle context panel |
| `F5` | Refresh results |

### Advanced

| Key | Action |
|-----|--------|
| `Ctrl+Shift+D` | Debug mode (show raw scores) |
| `Ctrl+Shift+P` | Performance metrics |
| `Ctrl+Shift+H` | Clear search history |

---

## Accessibility

### Screen Reader Support

**Announcements**:
- Search status ("Searching...", "42 results found")
- Result selection ("Result 1 of 42: The Dark Knight")
- Filter changes ("Action filter applied")
- Errors ("No results found")

**ARIA Labels**:
- All controls properly labeled
- Result metadata announced
- Context preview readable

**Keyboard Navigation**:
- Full keyboard support (no mouse required)
- Logical tab order
- Skip links for faster navigation

---

### High Contrast Mode

**Automatic Detection**:
- Detects system high contrast settings
- Adjusts colors automatically
- Maintains readability

**Manual Override**:
- Settings → Accessibility → High Contrast
- Choose theme: Light / Dark / Custom

---

### Font Size

**Adjustable Text**:
- Settings → Accessibility → Font Size
- Options: Small / Normal / Large / Extra Large
- Affects all text in dialog

**Zoom**:
- `Ctrl++` / `Ctrl+-`: Zoom in/out (if supported by skin)

---

## Tips & Best Practices

### Search Strategy

1. **Start Broad**: Use general terms, then filter
2. **Use Natural Language**: Complete sentences work better
3. **Iterate**: Refine based on initial results
4. **Try Different Modes**: Hybrid → Semantic → Keyword
5. **Use Filters**: Narrow down by type, genre, year

### Performance Tips

1. **Enable Caching**: Faster repeat searches
2. **Limit Results**: Set max results to 20-50 (faster)
3. **Keyword Mode**: Fastest for simple queries
4. **Close Other Apps**: Free up memory for better performance

### Privacy Tips

1. **Privacy Mode**: Disable history if shared device
2. **Clear History**: Periodically clear old searches
3. **Offline Voice**: Use PocketSphinx (Linux) for no cloud
4. **Disable Analytics**: Turn off usage sharing in settings

---

## Troubleshooting

### No Results Found

**Possible Causes**:
1. Media not indexed yet
2. Filters too restrictive
3. Query too specific

**Solutions**:
- Check indexing status (Settings → System → Semantic Search → Status)
- Clear filters and try again
- Simplify query
- Try different search mode

### Slow Search

**Possible Causes**:
1. First query after startup (model loading)
2. Large library (>100K chunks)
3. Low memory

**Solutions**:
- Wait for model to load (one-time delay)
- Enable caching
- Reduce topK values in settings
- Use keyword mode for simple queries

### Unexpected Results

**Possible Causes**:
1. Wrong search mode for query type
2. Stale embeddings (old index)
3. Generic query matching too much

**Solutions**:
- Try different search mode
- Reindex media (Settings → Reindex All)
- Make query more specific
- Use filters to narrow scope

---

## Advanced Usage

### Power User Features

**Batch Actions**:
- Select multiple results (`Ctrl+Click`)
- Right-click → "Add all to playlist"
- Useful for creating themed playlists

**Smart Filters**:
- Combine multiple filter types for precision
- Example: Action + 2020-2024 + PG-13 + >90min
- Save as preset for quick reuse

**Similar Content Discovery**:
- Find result you like
- Right-click → "Find Similar"
- Discovers related content via vector similarity

**Cross-referencing**:
- Search for actor/director name
- Filter by type (episodes vs movies)
- Find all their content in one query

---

*For API integration and development, see [APIReference.md](APIReference.md)*
*For performance optimization, see [PerformanceTuning.md](PerformanceTuning.md)*

---

*Last Updated: 2025-11-25*
*Phase 2 GUI Implementation Complete*
