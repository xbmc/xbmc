#ifndef _RAR_ULINKS_
#define _RAR_ULINKS_

void SaveLinkData(ComprDataIO &DataIO,Archive &TempArc,FileHeader &hd,
                  char *Name);
int ExtractLink(ComprDataIO &DataIO,Archive &Arc,char *DestName,
                uint &LinkCRC,bool Create);

#endif
