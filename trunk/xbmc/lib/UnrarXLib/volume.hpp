#ifndef _RAR_VOLUME_
#define _RAR_VOLUME_

void SplitArchive(Archive &Arc,FileHeader *fh,Int64 *HeaderPos,
                  ComprDataIO *DataIO);
bool MergeArchive(Archive &Arc,ComprDataIO *DataIO,bool ShowFileName,
                  char Command);
void SetVolWrite(Archive &Dest,Int64 VolSize);
bool AskNextVol(char *ArcName);

#endif
