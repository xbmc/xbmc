#pragma once

namespace DIRECTORY
{
  namespace MUSICDATABASEDIRECTORY
  {
    class CQueryParams;

    typedef enum _NODE_TYPE
    {
      NODE_TYPE_NONE=0,
      NODE_TYPE_ROOT,
      NODE_TYPE_OVERVIEW,
      NODE_TYPE_TOP100,
      NODE_TYPE_GENRE,
      NODE_TYPE_ARTIST,
      NODE_TYPE_ALBUM,
      NODE_TYPE_ALBUM_RECENTLY_ADDED,
      NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS,
      NODE_TYPE_ALBUM_RECENTLY_PLAYED,
      NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS,
      NODE_TYPE_ALBUM_TOP100,
      NODE_TYPE_ALBUM_TOP100_SONGS,
      NODE_TYPE_SONG,
      NODE_TYPE_SONG_TOP100
    } NODE_TYPE;

    class CDirectoryNode
    {
    public:
      static CDirectoryNode* ParseURL(const CStdString& strPath);
      virtual ~CDirectoryNode();

      NODE_TYPE GetType();
      long GetDatabaseId();

      bool GetChilds(CFileItemList& items);
      virtual NODE_TYPE GetChildType();
      void CollectQueryParams(CQueryParams& params);

      CDirectoryNode* GetParent();

    protected:
      CDirectoryNode(NODE_TYPE Type, const CStdString& strName, CDirectoryNode* pParent);
      static CDirectoryNode* CreateNode(NODE_TYPE Type, const CStdString& strName, CDirectoryNode* pParent);

      const CStdString& GetName();
      void RemoveParent();

      virtual bool GetContent(CFileItemList& items);

      CStdString BuildPath();

    private:
      void AddQueuingFolder(CFileItemList& items);
      bool CanCache();

    private:
      NODE_TYPE m_Type;
      CStdString m_strName;
      CDirectoryNode* m_pParent;
    };
  };
};

