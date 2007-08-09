#include    "FTLibrary.h"


const FTLibrary&  FTLibrary::Instance()
{
    static FTLibrary ftlib;
    return ftlib;
}


FTLibrary::~FTLibrary()
{
    if( library != 0)
    {
        FT_Done_FreeType( *library);

        delete library;
        library= 0;
    }

//  if( manager != 0)
//  {
//      FTC_Manager_Done( manager );
//
//      delete manager;
//      manager= 0;
//  }
}


FTLibrary::FTLibrary()
:   library(0),
    err(0)
{
    Initialise();
}


bool FTLibrary::Initialise()
{
    if( library != 0)
        return true;

    library = new FT_Library;
    
    err = FT_Init_FreeType( library);
    if( err)
    {
        delete library;
        library = 0;
        return false;
    }
    
//  FTC_Manager* manager;
//  
//  if( FTC_Manager_New( lib, 0, 0, 0, my_face_requester, 0, manager )
//  {
//      delete manager;
//      manager= 0;
//      return false;
//  }

    return true;
}
