#ifndef _xbox_unrar_interface_
#define _xbox_unrar_interface_


/* This structure is used for listing archive content                       */
struct RAR20_archive_entry                  /* These infos about files are  */
{                                           /* stored in RAR v2.0 archives  */
  char          *Name;
  wchar_t       *NameW;
  unsigned short NameSize;
  unsigned long  PackSize;
  int64_t        UnpSize;
  unsigned char  HostOS;                    /* MSDOS=0,OS2=1,WIN32=2,UNIX=3 */
  unsigned long  FileCRC;
  unsigned long  FileTime;
  unsigned char  UnpVer;
  unsigned char  Method;
  unsigned long  FileAttr;
  int64_t        iOffset;
};

/* used to list archives */
typedef struct archivelist
{
  struct RAR20_archive_entry item;
  struct archivelist *next;
} ArchiveList_struct;

/*-------------------------------------------------------------------------*\
  Extract a RAR file
  rarfile      - Name of the RAR file to uncompress
  targetPath    - The path to which we want to uncompress
  fileToExtract - The file inside the archive we want to uncompress,
          or NULL for all files.
  libpassword   - Password (for encrypted archives)
\*-------------------------------------------------------------------------*/
typedef bool (*progress_callback)(void*, int, const char*);
int urarlib_get(char *rarfile, char *targetPath, char *fileToExtract, char *libpassword = NULL, int64_t* iOffset=NULL, progress_callback progress = NULL, void *context = NULL);

/*-------------------------------------------------------------------------*\
  List the files in a RAR file
  rarfile    - Name of the RAR file to uncompress
  list      - Output. A list of file data of the files in the archive.
                The list should be freed with urarlib_freelist().
  libpassword - Password (for encrypted archives)
\*-------------------------------------------------------------------------*/
int urarlib_list(char *rarfile, ArchiveList_struct **ppList, char *libpassword = NULL, bool stopattwo=false);

/*-------------------------------------------------------------------------*\
  Free the file list returned by urarlib_list()
  list - The output from urarlib_list()
\*-------------------------------------------------------------------------*/
void urarlib_freelist(ArchiveList_struct *list);

/*-------------------------------------------------------------------------*\
  Function used internally to change filenames if
  they are fatx incompatible - unnedded and unused
\*-------------------------------------------------------------------------*/
void MakeNameUsable(char* szPath,bool KeepExtension, bool IsFatx);

#endif /* _xbox_unrar_interface_ */
