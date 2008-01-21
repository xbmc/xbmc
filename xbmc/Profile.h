#pragma once

class CProfile
{
public:
  CProfile(void);
  ~CProfile(void);

  const CStdString& getDate() const { return _date;}
  const CStdString& getName() const { return _name;}
  const CStdString& getDirectory() const { return _directory;}
  const CStdString& getThumb() const { return _thumb;}
  const CStdString& getLockCode() const { return _strLockCode;}
  int getLockMode() const { return _iLockMode; }

  bool hasDatabases() const { return _bDatabases; }
  bool canWriteDatabases() const { return _bCanWrite; }
  bool hasSources() const { return _bSources; }
  bool canWriteSources() const { return _bCanWriteSources; }
  bool settingsLocked() const { return _bLockSettings; }
  bool musicLocked() const { return _bLockMusic; }
  bool videoLocked() const { return _bLockVideo; }
  bool picturesLocked() const { return _bLockPictures; }
  bool filesLocked() const { return _bLockFiles; }
  bool programsLocked() const { return _bLockPrograms; }
  bool useAvpackSettings() const { return _bUseAvpackSettings; }

  void setName(const CStdString& name) {_name = name;}
  void setDirectory(const CStdString& directory) {_directory = directory;}
  void setDate(const CStdString& strDate) { _date = strDate;}
  void setDate();
  void setLockMode(int iLockMode) { _iLockMode = iLockMode;}
  void setLockCode(const CStdString& strLockCode) { _strLockCode = strLockCode; }
  void setThumb(const CStdString& thumb) {_thumb = thumb;}
  void setDatabases(bool bHas) { _bDatabases = bHas; }
  void setWriteDatabases(bool bCan) { _bCanWrite = bCan; }
  void setSources(bool bHas) { _bSources = bHas; }
  void setWriteSources(bool bCan) { _bCanWriteSources = bCan; }
  void setUseAvpackSettings(bool bUse) { _bUseAvpackSettings = bUse; }
  
  void setSettingsLocked(bool bLocked) { _bLockSettings = bLocked; }
  void setFilesLocked(bool bLocked) { _bLockFiles = bLocked; }
  void setMusicLocked(bool bLocked) { _bLockMusic = bLocked; }
  void setVideoLocked(bool bLocked) { _bLockVideo = bLocked; }
  void setPicturesLocked(bool bLocked) { _bLockPictures = bLocked; }
  void setProgramsLocked(bool bLocked) { _bLockPrograms = bLocked; }

  CStdString _directory;
  CStdString _name;
  CStdString _date;
  CStdString _thumb;
  bool _bDatabases;
  bool _bCanWrite;
  bool _bSources;
  bool _bCanWriteSources;
  bool _bUseAvpackSettings;

  // lock stuff
  int _iLockMode;
  CStdString _strLockCode;
  bool _bLockSettings;
  bool _bLockMusic;
  bool _bLockVideo;
  bool _bLockFiles;
  bool _bLockPictures;
  bool _bLockPrograms;
};

