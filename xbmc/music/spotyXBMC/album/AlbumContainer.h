
#ifndef ALBUMCONTAINER_H_
#define ALBUMCONTAINER_H_

#include "SxAlbum.h"
#include <vector>
#include "FileItem.h"


using namespace std;

namespace addon_music_spotify {

  class AlbumContainer {
  public:
    AlbumContainer();
    virtual ~AlbumContainer();

    virtual bool getAlbumItems(CFileItemList& items) = 0;

  protected:
    vector<SxAlbum*> m_albums;

    void removeAllAlbums();
    bool albumsLoaded();
  };

} /* namespace addon_music_spotify */
#endif /* ALBUMCONTAINER_H_ */
