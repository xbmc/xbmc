#pragma once

class ISubManager
{
public:
  virtual void LoadInternalSubtitles(IGraphBuilder* pGB) = 0;
  virtual void LoadExternalSubtitles(const wchar_t* fn, const wchar_t* paths) = 0;
  virtual void LoadSubtitlesForFile(const wchar_t* fn, IGraphBuilder* pGB, const wchar_t* paths) = 0;
  virtual int GetCount() = 0;
  virtual int GetCurrent() = 0;
  virtual void SetCurrent(int current) = 0;
  virtual BOOL GetEnable() = 0;
  virtual void SetEnable(BOOL enable) = 0;
  virtual void SetTime(REFERENCE_TIME nsSampleTime) = 0;
  virtual void Render(int x, int y, int width, int height) = 0;
  virtual int GetDelay() = 0;
  virtual void SetDelay(int delay) = 0;
  virtual bool IsModified() = 0;
  virtual void SaveToDisk() = 0;
  virtual void SetTimePerFrame(REFERENCE_TIME timePerFrame) = 0;
  virtual void SetSegmentStart(REFERENCE_TIME segmentStart) = 0;
  virtual void SetSampleStart(REFERENCE_TIME sampleStart) = 0;
};