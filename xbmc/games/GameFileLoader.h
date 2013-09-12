/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#pragma once

#include <set>
#include <stdint.h>
#include <string>
#include <vector>

// Forward declarations
class CFileItem;
namespace LIBRETRO { struct retro_game_info; }
namespace GAMES { class CGameClient; }

namespace GAMES
{
  /**
   * Game clients can load files in one of two ways: by path, or by a buffer of
   * the file's data. This class is a container that allows both ways, and also
   * stores a copy of the game file's data.
   */
  class CGameFile
  {
  public:
    enum TYPE
    {
      TYPE_INVALID,
      TYPE_PATH,
      TYPE_DATA
    };

    CGameFile() { Reset(); }
    CGameFile(TYPE type, const std::string &originalPath, const std::string &translatedPath = "");
    void Reset();

    TYPE Type() const { return m_type; }

    // The path to be sent to the game client.
    const std::string &Path() const { return m_strTranslatedPath; }

    /**
     * Returns the game file loaded into memory. CGameFile implements lazy
     * loading, so the data isn't loaded until the first call to this function.
     */
    const std::vector<uint8_t> &Buffer();

    /**
     * Returns the CRC of the file's data. If the type is TYPE_PATH and
     * CGameFileLoaderUseParentZip was chosen, we take the CRC of the original
     * path instead of the translated path. We don't want to CRC a .zip file if
     * we can help it. This allows users to zip a game file without loosing
     * their save states.
     */
    const std::string &CRC();

    /**
     * Dump game file info to a retro_game_info struct. Libretro cores will
     * know whether to load using path or data.
     */
    bool ToInfo(LIBRETRO::retro_game_info &info);

  private:
    /**
     * Load the file specified by m_path into the buffer. Subsequent calls to
     * Read() are ignored. If this succeeds, m_data will have a non-zero size().
     */
    static void Read(const std::string &path, std::vector<uint8_t> &data);

    TYPE                 m_type;
    std::string          m_strOriginalPath;
    std::string          m_strTranslatedPath;
    std::vector<uint8_t> m_data;
    bool                 m_bIsLoaded;
    std::string          m_strCRC;
    bool                 m_bIsCRCed;
  };

  /**
    * Loading a file in libretro cores is a complicated process. Game clients
    * support different extensions, some support loading from the VFS, and
    * some have the ability to load ROMs from within zips. Game clients have
    * a tendency to lie about their capabilities. Furthermore, different ROMs
    * can have different results, so it is desirable to try different
    * strategies upon failure.
    */
  class CGameFileLoader
  {
  public:
    virtual ~CGameFileLoader() { }

    /**
      * Returns true if this strategy is a viable option. In this case, result
      * is a valid CGameFile, representing the original file or a preferred
      * substitute file.
      */
    virtual bool CanLoad(const CGameClient &gc, const CFileItem& file, CGameFile &result) = 0;

    /**
      * Perform the gamut of checks on the file: "gameclient" property, platform,
      * extension, and a positive match on at least one of the CGameFileLoader
      * strategies.
      */
    static bool CanOpen(const CGameClient &gc, const CFileItem &file);

    /**
      * HELPER FUNCTION: If zipPath is a zip file, this will enumerate its contents
      * and return the first file inside with a valid extension. If this returns
      * false, effectivePath will be set to zipPath.
      */
    static bool GetEffectiveRomPath(const std::string &zipPath, const std::set<std::string> &validExts, std::string &effectivePath);

    /**
      * HELPER FUNCTION: If the game client was a bad boy and provided no
      * extensions, this will optimistically return true.
      */
    static bool IsExtensionValid(const std::string &ext, const std::set<std::string> &setExts);
  };

  /**
    * Load the file from the local hard disk.
    */
  class CGameFileLoaderUseHD : public CGameFileLoader
  {
  public:
    virtual bool CanLoad(const CGameClient &gc, const CFileItem& file, CGameFile &result);
  };

  /**
    * Use the VFS to load the file.
    */
  class CGameFileLoaderUseVFS : public CGameFileLoader
  {
  public:
    virtual bool CanLoad(const CGameClient &gc, const CFileItem& file, CGameFile &result);
  };

  /**
    * If the game client blocks extracting, we don't want to load a file from
    * within a zip. In this case, we try to use the container zip (parent
    * folder on the vfs).
    */
  class CGameFileLoaderUseParentZip : public CGameFileLoader
  {
  public:
    virtual bool CanLoad(const CGameClient &gc, const CFileItem& file, CGameFile &result);
  };

  /**
    * If a zip fails to load, try loading the ROM inside from the zip:// vfs.
    * Try to avoid recursion clashes with the above strategy.
    */
  class CGameFileLoaderEnterZip : public CGameFileLoader
  {
  public:
    virtual bool CanLoad(const CGameClient &gc, const CFileItem& file, CGameFile &result);
  };
} // namespace GAMES
