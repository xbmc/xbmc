/*
 *  Copyright (C) 2022-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_SHADER_PRESET_H
#define C_API_ADDONINSTANCE_SHADER_PRESET_H

#include "../addon_base.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
  //============================================================================
  /// @ingroup cpp_kodi_addon_shader_preset_Defs
  /// @brief **The data system used for shader presets**
  ///
  ///@{
  typedef void* preset_file;

  /*!
   * \brief Scale types
   *
   * If no scale type is specified, it is assumed that the scale type is
   * relative to the input with a scaling factor of 1.0.
   *
   * Exceptions: If no scale type is set for the last pass, it is assumed to
   * output at the full resolution rather than assuming of scale of 1.0, and
   * bypasses any frame-buffer object rendering.
   */
  typedef enum SHADER_SCALE_TYPE
  {
    /*!
     * \brief Use the source size
     *
     * Output size of the shader pass is relative to the input size. Value is
     * float.
     */
    SHADER_SCALE_TYPE_INPUT,

    /*!
     * \brief Use the window viewport size
     *
     * Output size of the shader pass is relative to the size of the window
     * viewport. Value is float. This value can change over time if the user
     * resizes his/her window!
     */
    SHADER_SCALE_TYPE_ABSOLUTE,

    /*!
     * \brief Use a statically defined size
     *
     * Output size is statically defined to a certain size. Useful for hi-res
     * blenders or similar.
     */
    SHADER_SCALE_TYPE_VIEWPORT
  } SHADER_SCALE_TYPE;

  typedef enum SHADER_FILTER_TYPE
  {
    SHADER_FILTER_TYPE_UNSPEC,
    SHADER_FILTER_TYPE_LINEAR,
    SHADER_FILTER_TYPE_NEAREST
  } SHADER_FILTER_TYPE;

  /*!
   * \brief Texture wrapping mode
   */
  typedef enum SHADER_WRAP_TYPE
  {
    SHADER_WRAP_TYPE_BORDER, /* Deprecated, will be translated to EDGE in GLES */
    SHADER_WRAP_TYPE_EDGE,
    SHADER_WRAP_TYPE_REPEAT,
    SHADER_WRAP_TYPE_MIRRORED_REPEAT
  } SHADER_WRAP_TYPE;

  /*!
   * \brief FBO scaling parameters for a single axis
   */
  typedef struct fbo_scale_axis
  {
    SHADER_SCALE_TYPE type;
    union
    {
      float scale;
      unsigned abs;
    };
  } fbo_scale_axis;

  /*!
   * \brief FBO parameters
   */
  typedef struct fbo_scale
  {
    /*!
     * \brief sRGB framebuffer
     */
    bool srgb_fbo;

    /*!
     * \brief Float framebuffer
     *
     * This parameter defines if the pass should be rendered to a 32-bit
     * floating point buffer. This only takes effect if the pass is actually
     * rendered to an FBO. This is useful for shaders which have to store FBO
     * values outside the range [0, 1].
     */
    bool fp_fbo;

    /*!
     * \brief Scaling parameters for X axis
     */
    fbo_scale_axis scale_x;

    /*!
     * \brief Scaling parameters for Y axis
     */
    fbo_scale_axis scale_y;
  } fbo_scale;

  typedef struct video_shader_parameter
  {
    char* id;
    char* desc;
    float current;
    float minimum;
    float initial;
    float maximum;
    float step;
  } video_shader_parameter;

  typedef struct video_shader_pass
  {
    /*!
     * \brief Path to the shader pass source
     */
    char* source_path;

    /*!
     * \brief The vertex shader source
     */
    char* vertex_source;

    /*!
     * \brief The fragment shader source, if separate from the vertex source, or
     * NULL otherwise
     */
    char* fragment_source;

    /*!
     * \brief FBO parameters
     */
    fbo_scale fbo;

    /*!
     * \brief Defines how the result of this pass will be filtered
     *
     * @todo Define behavior for unspecified filter
     */
    SHADER_FILTER_TYPE filter;

    /*!
     * \brief Wrapping mode
     */
    SHADER_WRAP_TYPE wrap;

    /*!
     * \brief Frame count mod
     */
    unsigned frame_count_mod;

    /*!
     * \brief Mipmapping
     */
    bool mipmap;

    /*!
     * \brief Aliased pass name
     */
    char* alias;
  } video_shader_pass;

  typedef struct video_shader_lut
  {
    /*!
     * \brief Name of the sampler uniform, e.g. `uniform sampler2D foo`.
     */
    char* id;

    /*!
     * \brief Path of the texture
     */
    char* path;

    /*!
     * \brief Filtering for the texture
     */
    SHADER_FILTER_TYPE filter;

    /*!
     * \brief Texture wrapping mode
     */
    SHADER_WRAP_TYPE wrap;

    /*!
     * \brief Use mipmapping for the texture
     */
    bool mipmap;
  } video_shader_lut;

  typedef struct video_shader
  {
    unsigned pass_count;
    video_shader_pass* passes;

    unsigned lut_count;
    video_shader_lut* luts;

    unsigned parameter_count;
    video_shader_parameter* parameters;
  } video_shader;
  //----------------------------------------------------------------------------

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--

  /*!
   * @brief ShaderPreset properties
   *
   * Not to be used outside this header.
   */
  typedef struct AddonProps_ShaderPreset
  {
    /*!
     * \brief The path to the user profile
     */
    const char* user_path;

    /*!
     * \brief The path to this add-on
     */
    const char* addon_path;
  } AddonProps_ShaderPreset;

  /*!
   * \brief Structure to transfer the methods from kodi_shader_preset_dll.h to
   * Kodi
   */
  struct AddonInstance_ShaderPreset;

  /*!
   * @brief ShaderPreset callbacks
   *
   * Not to be used outside this header.
   */
  typedef struct AddonToKodiFuncTable_ShaderPreset
  {
    KODI_HANDLE kodiInstance;
  } AddonToKodiFuncTable_ShaderPreset;

  /*!
   * @brief ShaderPreset function hooks
   *
   * Not to be used outside this header.
   */
  typedef struct KodiToAddonFuncTable_ShaderPreset
  {
    KODI_HANDLE addonInstance;

    preset_file(__cdecl* PresetFileNew)(const struct AddonInstance_ShaderPreset*, const char*);
    void(__cdecl* PresetFileFree)(const struct AddonInstance_ShaderPreset*, preset_file);
    bool(__cdecl* VideoShaderRead)(const struct AddonInstance_ShaderPreset*,
                                   preset_file,
                                   struct video_shader*);
    bool(__cdecl* VideoShaderWrite)(const struct AddonInstance_ShaderPreset*,
                                    preset_file,
                                    const struct video_shader*);
    bool(__cdecl* VideoShaderResolveParameters)(const struct AddonInstance_ShaderPreset*,
                                                preset_file,
                                                struct video_shader*);
    void(__cdecl* VideoShaderFree)(const struct AddonInstance_ShaderPreset*, struct video_shader*);
  } KodiToAddonFuncTable_ShaderPreset;

  /*!
   * @brief ShaderPreset instance
   *
   * Not to be used outside this header.
   */
  typedef struct AddonInstance_ShaderPreset
  {
    struct AddonProps_ShaderPreset* props;
    struct AddonToKodiFuncTable_ShaderPreset* toKodi;
    struct KodiToAddonFuncTable_ShaderPreset* toAddon;
  } AddonInstance_ShaderPreset;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_SHADER_PRESET_H */
