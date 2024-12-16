# OpenGL Roadmap for RetroPlayer

OpenGL support in RetroPlayer will enable hardware rendering for cores that require OpenGL. This roadmap outlines the key tasks needed to complete its integration in Kodi.

The OpenGL tracking issue is: https://github.com/xbmc/xbmc/issues/17743

## Current Status

The foundational work for OpenGL support in RetroPlayer has been merged, including:

- **Plumbing APIs:** RetroPlayer APIs now support OpenGL procedures and framebuffer streams ([PR #26091](https://github.com/xbmc/xbmc/pull/26091))
- **Integration with Game API:** The Game API has been extended to connect libretro cores with OpenGL rendering functionality.
- **Core Compatibility:** Initial testing has verified that OpenGL cores, such as Mupen64Plus and Beetle PSX HW, can interface with RetroPlayer without crashing.

## Remaining Work

### VAOs and Rendering Pipeline

The use of Vertex Array Objects (VAOs) is a critical step in modernizing RetroPlayer's rendering pipeline. VAOs streamline the management of vertex attributes, making the OpenGL rendering process more efficient and less error-prone.

- **Current Progress:** The removal of global VAOs has been addressed in [PR #22211: Remove Global VAOs](https://github.com/xbmc/xbmc/pull/22211), which simplifies the rendering pipeline.
- **Next Step:** Review and merge [PR #22211](https://github.com/xbmc/xbmc/pull/22211) to finalize VAO updates.
- The PR has been rebased here: https://github.com/garbear/xbmc/commits/retro-gl-v3

Once the VAO work is completed, RetroPlayer will benefit from a cleaner and more maintainable rendering architecture.

### Frame Buffer Object (FBO) Renderbuffers

Mostly done. Original work [here](https://github.com/lrusak/xbmc/commits/retro-gl-v3) and rebased work [here](https://github.com/garbear/xbmc/commits/retro-gl-v3).

### Synchronization

The main synchronization challenges for OpenGL support in RetroPlayer stem from the separation of the game loop (which updates game state) and the rendering thread (which processes graphics).

- **GL Context Management:**
  OpenGL contexts are bound to threads, requiring careful management to ensure that rendering operations occur exclusively on  the rendering thread. Context sharing between threads needs to be avoided or strictly synchronized.

- **Dynamic Resolution Changes:**
  Dynamic resolution adjustments during gameplay add complexity. Both threads must coordinate to resize and reallocate frame buffers seamlessly, without causing crashes or visual glitches.

#### GL Context Management

Managing OpenGL contexts in RetroPlayer presents unique challenges due to its multithreaded architecture. Key considerations include:

- **Thread Binding:**
  OpenGL contexts are thread-specific. RetroPlayer must ensure that rendering occurs on a dedicated rendering thread without interfering with the game loop.

- **Context Resetting:**
  When switching contexts (e.g., during resolution changes), RetroPlayer must reinitialize OpenGL state while avoiding disruptions to ongoing operations.

- **Multiple Platforms:**
  Context management strategies must accommodate platform-specific OpenGL implementations, such as OpenGL ES for ARM-based devices or Metal wrappers for macOS.

To address these challenges, RetroPlayer can leverage context creation and management best practices, such as ensuring all rendering occurs on the same thread and using thread-safe proxy functions for OpenGL operations initiated by the game loop.

#### Dynamic Resolution Changes

Needed to accommodate gameplay scenarios where resolution changes are required mid-session, such as switching between fullscreen and windowed modes or adapting to performance constraints. This requires:

- **Real-Time Buffer Resizing:**
  Ensure frame buffers are resized and reallocated without disrupting gameplay or introducing visual artifacts.

- **Synchronization with Rendering Thread:**
  Both the game loop and the rendering thread must coordinate to apply resolution changes seamlessly.

---

## Conclusion

RetroPlayer's OpenGL support is well underway, with foundational work completed and key APIs in place. The remaining tasks include finalizing VAO updates, completing FBO renderbuffers, addressing synchronization challenges, and implementing dynamic resolution support. With these enhancements, RetroPlayer will achieve robust OpenGL integration, paving the way for future advancements like Vulkan support and broader hardware compatibility.
