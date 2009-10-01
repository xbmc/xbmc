/*
 * /home/ms/files/source/libsidtune/RCS/SidTune.h,v
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SIDTUNE_H
#define SIDTUNE_H


#include "sidtypes.h"
#include "Buffer.h"
#include "SmartPtr.h"

#include <fstream>


const uint_least16_t SIDTUNE_MAX_SONGS = 256;
// Also PSID file format limit.

const uint_least16_t SIDTUNE_MAX_CREDIT_STRINGS = 10;
const uint_least16_t SIDTUNE_MAX_CREDIT_STRLEN = 80+1;
// 80 characters plus terminating zero.

const uint_least32_t SIDTUNE_MAX_MEMORY = 65536;
const uint_least32_t SIDTUNE_MAX_FILELEN = 65536+2+0x7C;
// C64KB+LOAD+PSID

const int SIDTUNE_SPEED_VBI = 0;        // Vertical-Blanking-Interrupt
const int SIDTUNE_SPEED_CIA_1A = 60;    // CIA 1 Timer A

const int SIDTUNE_CLOCK_UNKNOWN = 0x00;
const int SIDTUNE_CLOCK_PAL     = 0x01; // These are also used in the
const int SIDTUNE_CLOCK_NTSC    = 0x02; // emulator engine!
const int SIDTUNE_CLOCK_ANY     = (SIDTUNE_CLOCK_PAL | SIDTUNE_CLOCK_NTSC);

const int SIDTUNE_SIDMODEL_UNKNOWN = 0x00;
const int SIDTUNE_SIDMODEL_6581    = 0x01; // These are also used in the
const int SIDTUNE_SIDMODEL_8580    = 0x02; // emulator engine!
const int SIDTUNE_SIDMODEL_ANY     = (SIDTUNE_SIDMODEL_6581 | SIDTUNE_SIDMODEL_8580);

const int SIDTUNE_COMPATIBILITY_C64   = 0x00; // File is C64 compatible
const int SIDTUNE_COMPATIBILITY_PSID  = 0x01; // File is PSID specific
const int SIDTUNE_COMPATIBILITY_R64   = 0x02; // File is Real C64 only
const int SIDTUNE_COMPATIBILITY_BASIC = 0x03; // File requires C64 Basic


// Required to export template
#ifndef _SidTune_cpp_
extern
#endif
template class SID_EXTERN Buffer_sidtt<const uint_least8_t>;

struct SidTuneInfo
{
    // An instance of this structure is used to transport values to
    // and from SidTune objects.

    // You must read (i.e. activate) sub-song specific information
    // via:
    //        const SidTuneInfo& tuneInfo = SidTune[songNumber];
    //        const SidTuneInfo& tuneInfo = SidTune.getInfo();
    //        void SidTune.getInfo(tuneInfo&);
    
    // Consider the following fields as read-only, because the SidTune class
    // does not provide an implementation of: bool setInfo(const SidTuneInfo&).
    // Currently, the only way to get the class to accept values which
    // are written to these fields is by creating a derived class.

    const char* formatString;   // the name of the identified file format
    const char* statusString;   // error/status message of last operation
    
    const char* speedString;    // describing the speed a song is running at
    
    uint_least16_t loadAddr;
    uint_least16_t initAddr;
    uint_least16_t playAddr;
    
    uint_least16_t songs;
    uint_least16_t startSong;
    
    // The SID chip base address(es) used by the sidtune.
    uint_least16_t sidChipBase1;    // 0xD400 (normal, 1st SID)
    uint_least16_t sidChipBase2;    // 0xD?00 (2nd SID) or 0 (no 2nd SID)

    // Available after song initialization.
    //
    uint_least16_t currentSong;    // the one that has been initialized
    uint_least8_t songSpeed;       // intended speed, see top
    uint_least8_t clockSpeed;      // -"-
    uint_least8_t relocStartPage;  // First available page for relocation
    uint_least8_t relocPages;      // Number of pages available for relocation
    bool musPlayer;                // whether Sidplayer routine has been installed
    int  sidModel;                 // Sid Model required for this sid
    int  compatibility;            // compatibility requirements
    bool fixLoad;                  // whether load address might be duplicate
    uint_least16_t songLength;     // --- not yet supported ---
    //
    // Song title, credits, ...
    // 0 = Title, 1 = Author, 2 = Copyright/Publisher
    //
    uint_least8_t numberOfInfoStrings;  // the number of available text info lines
    char* infoString[SIDTUNE_MAX_CREDIT_STRINGS];
    //
    uint_least16_t numberOfCommentStrings;    // --- not yet supported ---
    char ** commentString;                // --- not yet supported ---
    //
    uint_least32_t dataFileLen;    // length of single-file sidtune file
    uint_least32_t c64dataLen;     // length of raw C64 data without load address
    char* path;                    // path to sidtune files; "", if cwd
    char* dataFileName;            // a first file: e.g. "foo.c64"; "", if none
    char* infoFileName;            // a second file: e.g. "foo.sid"; "", if none
    //
};


class SID_EXTERN SidTune
{
 private:
    typedef enum 
    {
        LOAD_NOT_MINE = 0,
        LOAD_OK,
        LOAD_ERROR
    } LoadStatus;
    
 public:  // ----------------------------------------------------------------

    // If your opendir() and readdir()->d_name return path names
    // that contain the forward slash (/) as file separator, but
    // your operating system uses a different character, there are
    // extra functions that can deal with this special case. Set
    // separatorIsSlash to true if you like path names to be split
    // correctly.
    // You do not need these extra functions if your systems file
    // separator is the forward slash.
    //
    // Load a sidtune from a file.
    //
    // To retrieve data from standard input pass in filename "-".
    // If you want to override the default filename extensions use this
    // contructor. Please note, that if the specified ``sidTuneFileName''
    // does exist and the loader is able to determine its file format,
    // this function does not try to append any file name extension.
    // See ``sidtune.cpp'' for the default list of file name extensions.
    // You can specific ``sidTuneFileName = 0'', if you do not want to
    // load a sidtune. You can later load one with open().
   SidTune();

   SidTune(const char* fileName, const char **fileNameExt = 0,
            const bool separatorIsSlash = false);

    // Load a single-file sidtune from a memory buffer.
    // Currently supported: PSID format
    SidTune(const uint_least8_t* oneFileFormatSidtune, const uint_least32_t sidtuneLength);

    virtual ~SidTune();

    // The sidTune class does not copy the list of file name extensions,
    // so make sure you keep it. If the provided pointer is 0, the
    // default list will be activated. This is a static list which
    // is used by all SidTune objects.
    void setFileNameExtensions(const char **fileNameExt);
    
    // Load a sidtune into an existing object.
    // From a file.
    bool load(const char* fileName, const bool separatorIsSlash = false);
    
    // From a buffer.
    bool read(const uint_least8_t* sourceBuffer, const uint_least32_t bufferLen);

    // Select sub-song (0 = default starting song)
    // and retrieve active song information.
    const SidTuneInfo& operator[](const uint_least16_t songNum);

    // Select sub-song (0 = default starting song)
    // and return active song number out of [1,2,..,SIDTUNE_MAX_SONGS].
    uint_least16_t selectSong(const uint_least16_t songNum);
    
    // Retrieve sub-song specific information.
    // Beware! Still member-wise copy!
    const SidTuneInfo& getInfo();
    
    // Get a copy of sub-song specific information.
    // Beware! Still member-wise copy!
    void getInfo(SidTuneInfo&);

    // Determine current state of object (true = okay, false = error).
    // Upon error condition use ``getInfo'' to get a descriptive
    // text string in ``SidTuneInfo.statusString''.
    operator bool()  { return status; }
    bool getStatus()  { return status; }

    // Whether sidtune uses two SID chips.
    bool isStereo()
    {
        return (info.sidChipBase1!=0 && info.sidChipBase2!=0);
    }
    
    // Copy sidtune into C64 memory (64 KB).
    bool placeSidTuneInC64mem(uint_least8_t* c64buf);

    // --- file save & format conversion ---

    // These functions work for any successfully created object.
    // overWriteFlag: true  = Overwrite existing file.
    //                  false = Default, return error when file already
    //                          exists.
    // One could imagine an "Are you sure ?"-checkbox before overwriting
    // any file.
    // returns: true = Successful, false = Error condition.
    bool saveC64dataFile( const char* destFileName, const bool overWriteFlag = false );
    bool saveSIDfile( const char* destFileName, const bool overWriteFlag = false );
    bool savePSIDfile( const char* destFileName, const bool overWriteFlag = false );

    // This function can be used to remove a duplicate C64 load address in
    // the C64 data (example: FE 0F 00 10 4C ...). A duplicate load address
    // of offset 0x02 is indicated by the ``fixLoad'' flag in the SidTuneInfo
    // structure.
    //
    // The ``force'' flag here can be used to remove the first load address
    // and set new INIT/PLAY addresses regardless of whether a duplicate
    // load address has been detected and indicated by ``fixLoad''.
    // For instance, some position independent sidtunes contain a load address
    // of 0xE000, but are loaded to 0x0FFE and call the player code at 0x1000.
    //
    // Do not forget to save the sidtune file.
    void fixLoadAddress(const bool force = false, uint_least16_t initAddr = 0,
                        uint_least16_t playAddr = 0);

    // Does not affect status of object, and therefore can be used
    // to load files. Error string is put into info.statusString, though.
    bool loadFile(const char* fileName, Buffer_sidtt<const uint_least8_t>& bufferRef);
    
    bool saveToOpenFile( std::ofstream& toFile, const uint_least8_t* buffer, uint_least32_t bufLen );
    
 protected:  // -------------------------------------------------------------

    SidTuneInfo info;
    bool status;

    uint_least8_t songSpeed[SIDTUNE_MAX_SONGS];
    uint_least8_t clockSpeed[SIDTUNE_MAX_SONGS];
    uint_least16_t songLength[SIDTUNE_MAX_SONGS];

    // holds text info from the format headers etc.
    char infoString[SIDTUNE_MAX_CREDIT_STRINGS][SIDTUNE_MAX_CREDIT_STRLEN];

    // See instructions at top.
    bool isSlashedFileName;

    // For files with header: offset to real data
    uint_least32_t fileOffset;

    // Needed for MUS/STR player installation.
    uint_least16_t musDataLen;

    Buffer_sidtt<const uint_least8_t> cache;

    // Filename extensions to append for various file types.
    static const char** fileNameExtensions;

    // --- protected member functions ---

    // Convert 32-bit PSID-style speed word to internal tables.
    void convertOldStyleSpeedToTables(uint_least32_t speed,
         int clock = SIDTUNE_CLOCK_PAL);

    virtual int convertPetsciiToAscii (SmartPtr_sidtt<const uint_least8_t>&, char*);

    // Check compatibility details are sensible
    bool checkCompatibility(void);
    // Check for valid relocation information
    bool checkRelocInfo(void);
    // Common address resolution procedure
    bool resolveAddrs(const uint_least8_t* c64data);

    // Support for various file formats.

    virtual LoadStatus PSID_fileSupport    (Buffer_sidtt<const uint_least8_t>& dataBuf);
    virtual bool       PSID_fileSupportSave(std::ofstream& toFile, const uint_least8_t* dataBuffer);

    virtual LoadStatus SID_fileSupport     (Buffer_sidtt<const uint_least8_t>& dataBuf,
                                            Buffer_sidtt<const uint_least8_t>& sidBuf);
    virtual bool       SID_fileSupportSave (std::ofstream& toFile);

    virtual LoadStatus MUS_fileSupport     (Buffer_sidtt<const uint_least8_t>& musBuf,
                                            Buffer_sidtt<const uint_least8_t>& strBuf);
    LoadStatus         MUS_load            (Buffer_sidtt<const uint_least8_t>& musBuf,
                                            bool init = false);
    LoadStatus         MUS_load            (Buffer_sidtt<const uint_least8_t>& musBuf,
                                            Buffer_sidtt<const uint_least8_t>& strBuf,
                                            bool init = false);
    virtual bool       MUS_detect          (const void* buffer, const uint_least32_t bufLen,
                                            uint_least32_t& voice3Index);
    virtual bool       MUS_mergeParts      (Buffer_sidtt<const uint_least8_t>& musBuf,
                                            Buffer_sidtt<const uint_least8_t>& strBuf);
    virtual void       MUS_setPlayerAddress();
    virtual void       MUS_installPlayer   (uint_least8_t *c64buf);

    virtual LoadStatus INFO_fileSupport    (Buffer_sidtt<const uint_least8_t>& dataBuf,
                                            Buffer_sidtt<const uint_least8_t>& infoBuf);
    virtual LoadStatus PRG_fileSupport     (const char* fileName,
                                            Buffer_sidtt<const uint_least8_t>& dataBuf);
    virtual LoadStatus X00_fileSupport     (const char* fileName,
                                            Buffer_sidtt<const uint_least8_t>& dataBuf);

    // Error and status message strings.
    static const char* txt_songNumberExceed;
    static const char* txt_empty;
    static const char* txt_unrecognizedFormat;
    static const char* txt_noDataFile;
    static const char* txt_notEnoughMemory;
    static const char* txt_cantLoadFile;
    static const char* txt_cantOpenFile;
    static const char* txt_fileTooLong;
    static const char* txt_dataTooLong;
    static const char* txt_cantCreateFile;
    static const char* txt_fileIoError;
    static const char* txt_VBI;
    static const char* txt_CIA;
    static const char* txt_noErrors;
    static const char* txt_na;
    static const char* txt_badAddr;
    static const char* txt_badReloc;
    static const char* txt_corrupt;

 private:  // ---------------------------------------------------------------
    
    void init();
    void cleanup();
#if !defined(SIDTUNE_NO_STDIN_LOADER)
    void getFromStdIn();
#endif
    void getFromFiles(const char* name);
    
    void deleteFileNameCopies();
    
    // Try to retrieve single-file sidtune from specified buffer.
    void getFromBuffer(const uint_least8_t* const buffer, const uint_least32_t bufferLen);
    
    // Cache the data of a single-file or two-file sidtune and its
    // corresponding file names.
    bool acceptSidTune(const char* dataFileName, const char* infoFileName,
                       Buffer_sidtt<const uint_least8_t>& buf);

    bool createNewFileName(Buffer_sidtt<char>& destString,
                           const char* sourceName, const char* sourceExt);

    int  decompressPP20(Buffer_sidtt<const uint_least8_t>& buf);

 private:    // prevent copying
    SidTune(const SidTune&);
    SidTune& operator=(SidTune&);
};

#endif  /* SIDTUNE_H */
