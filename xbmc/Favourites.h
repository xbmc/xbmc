#pragma once

class CFavourites
{
public:
  static bool Load(CFileItemList& items);
  static bool AddOrRemove(CFileItem *item, DWORD contextWindow);
  static bool Save(const CFileItemList& items);
  static bool IsFavourite(CFileItem *item, DWORD contextWindow);

private:
  static CStdString GetExecutePath(const CFileItem *item, DWORD contextWindow);
};
