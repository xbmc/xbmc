#pragma once

#include "fileitem.h"

namespace PLAYLIST
{
class CPlayList
{
public:
class CPlayListItem : public CFileItem
  {
  public:
    CPlayListItem();
    CPlayListItem(const CStdString& strDescription, const CStdString& strFileName, long lDuration = 0, long lStartOffset = 0, long lEndOffset = 0);

    virtual ~CPlayListItem();

    void SetFileName(const CStdString& strFileName);
    const CStdString& GetFileName() const;

    void SetDescription(const CStdString& strDescription);
    const CStdString& GetDescription() const;

    void SetDuration(long lDuration);
    long GetDuration() const;

    void SetStartOffset(long lStartOffset);
    long GetStartOffset() const;

    void SetEndOffset(long lEndOffset);
    long GetEndOffset() const;

    virtual bool LoadMusicTag();

    void SetMusicTag(const CMusicInfoTag &tag);
    void SetVideoTag(const CVideoInfoTag &tag);
    const CMusicInfoTag* GetMusicTag() const;
    const CVideoInfoTag* GetVideoTag() const;

    bool IsUnPlayable() const;
    void SetUnPlayable() { m_bUnPlayable = true; };
    void ClearUnPlayable() { m_bUnPlayable = false; };

  protected:
    long m_lDuration;
    bool m_bUnPlayable;
  };

  CPlayList(void);
  virtual ~CPlayList(void);
  virtual bool Load(const CStdString& strFileName);
  virtual bool LoadData(std::istream &stream);
  virtual bool LoadData(const CStdString& strData);
  virtual void Save(const CStdString& strFileName) const {};

  void Add(CPlayListItem& item);
  void Add(CPlayList& playlist);
  void Add(CFileItem *pItem);
	void Add(CFileItemList& items);

  // for Party Mode
  void Insert(CPlayList& playlist, int iPosition = -1);
  void Insert(CFileItemList& items, int iPosition = -1);

  int FindOrder(int iOrder);
  const CStdString& GetName() const;
  void Remove(const CStdString& strFileName);
  void Remove(int position);
  bool Swap(int position1, int position2);
  bool Expand(int position); // expands any playlist at position into this playlist
  void Clear();
  int size() const;
  int RemoveDVDItems();

  const CPlayList::CPlayListItem& operator[] (int iItem) const;
  CPlayList::CPlayListItem& operator[] (int iItem);

  // why are these virtual functions? there is no derived child class
  void Shuffle(int iPosition = 0);
  void UnShuffle();
  bool IsShuffled() { return m_bShuffled; }

  void SetPlayed(bool bPlayed) { m_bWasPlayed = true; };
  bool WasPlayed() { return m_bWasPlayed; };

  void SetUnPlayable(int iItem);
  int GetPlayable() { return m_iPlayableItems; };

  void UpdateItem(const CFileItem *item);

protected:
  CStdString m_strPlayListName;
  CStdString m_strBasePath;
  int m_iPlayableItems;
  bool m_bShuffled;
  bool m_bWasPlayed;
  vector <CPlayListItem> m_vecItems;
  typedef vector <CPlayListItem>::iterator ivecItems;

private:
  void Add(CPlayListItem& item, int iPosition, int iOrderOffset);
  void DecrementOrder(int iOrder);
  void IncrementOrder(int iPosition, int iOrder);
};
}
