/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2000                                                               *
 *  Contents: memory allocation routines                                    *
 ****************************************************************************/

SubAllocator::SubAllocator()
{
  Clean();
}


void SubAllocator::Clean()
{
  SubAllocatorSize=0;
}


inline void SubAllocator::InsertNode(void* p,int indx) 
{
  ((RAR_NODE*) p)->next=FreeList[indx].next;
  FreeList[indx].next=(RAR_NODE*) p;
}


inline void* SubAllocator::RemoveNode(int indx) 
{
  RAR_NODE* RetVal=FreeList[indx].next;
  FreeList[indx].next=RetVal->next;
  return RetVal;
}


inline uint SubAllocator::U2B(int NU) 
{ 
  return /*8*NU+4*NU*/UNIT_SIZE*NU;
}


inline void SubAllocator::SplitBlock(void* pv,int OldIndx,int NewIndx)
{
  int i, UDiff=Indx2Units[OldIndx]-Indx2Units[NewIndx];
  byte* p=((byte*) pv)+U2B(Indx2Units[NewIndx]);
  if (Indx2Units[i=Units2Indx[UDiff-1]] != UDiff) 
  {
    InsertNode(p,--i);
    p += U2B(i=Indx2Units[i]);
    UDiff -= i;
  }
  InsertNode(p,Units2Indx[UDiff-1]);
}




void SubAllocator::StopSubAllocator()
{
  if ( SubAllocatorSize ) 
  {
    SubAllocatorSize=0;
    rarfree(HeapStart);
  }
}


bool SubAllocator::StartSubAllocator(int SASize)
{
  uint t=SASize << 20;
  if (SubAllocatorSize == t)
    return TRUE;
  StopSubAllocator();
  uint AllocSize=t/FIXED_UNIT_SIZE*UNIT_SIZE+UNIT_SIZE;
  if ((HeapStart=(byte *)rarmalloc(AllocSize)) == NULL)
  {
    ErrHandler.MemoryError();
    return FALSE;
  }
  HeapEnd=HeapStart+AllocSize-UNIT_SIZE;
  SubAllocatorSize=t;
  return TRUE;
}


void SubAllocator::InitSubAllocator()
{
  int i, k;
  memset(FreeList,0,sizeof(FreeList));
  pText=HeapStart;
  uint Size2=FIXED_UNIT_SIZE*(SubAllocatorSize/8/FIXED_UNIT_SIZE*7);
  uint RealSize2=Size2/FIXED_UNIT_SIZE*UNIT_SIZE;
  uint Size1=SubAllocatorSize-Size2;
  uint RealSize1=Size1/FIXED_UNIT_SIZE*UNIT_SIZE+Size1%FIXED_UNIT_SIZE;
  HiUnit=HeapStart+SubAllocatorSize;
  LoUnit=UnitsStart=HeapStart+RealSize1;
  FakeUnitsStart=HeapStart+Size1;
  HiUnit=LoUnit+RealSize2;
  for (i=0,k=1;i < N1     ;i++,k += 1)
    Indx2Units[i]=k;
  for (k++;i < N1+N2      ;i++,k += 2)
    Indx2Units[i]=k;
  for (k++;i < N1+N2+N3   ;i++,k += 3)
    Indx2Units[i]=k;
  for (k++;i < N1+N2+N3+N4;i++,k += 4)
    Indx2Units[i]=k;
  for (GlueCount=k=i=0;k < 128;k++)
  {
    i += (Indx2Units[i] < k+1);
    Units2Indx[k]=i;
  }
}


inline void SubAllocator::GlueFreeBlocks()
{
  RAR_MEM_BLK s0, * p, * p1;
  int i, k, sz;
  if (LoUnit != HiUnit)
    *LoUnit=0;
  for (i=0, s0.next=s0.prev=&s0;i < N_INDEXES;i++)
    while ( FreeList[i].next )
    {
      p=(RAR_MEM_BLK*)RemoveNode(i);
      p->insertAt(&s0);
      p->Stamp=0xFFFF;
      p->NU=Indx2Units[i];
    }
  for (p=s0.next;p != &s0;p=p->next)
    while ((p1=p+p->NU)->Stamp == 0xFFFF && int(p->NU)+p1->NU < 0x10000)
    {
      p1->remove();
      p->NU += p1->NU;
    }
  while ((p=s0.next) != &s0)
  {
    for (p->remove(), sz=p->NU;sz > 128;sz -= 128, p += 128)
      InsertNode(p,N_INDEXES-1);
    if (Indx2Units[i=Units2Indx[sz-1]] != sz)
    {
      k=sz-Indx2Units[--i];
      InsertNode(p+(sz-k),k-1);
    }
    InsertNode(p,i);
  }
}

void* SubAllocator::AllocUnitsRare(int indx)
{
  if ( !GlueCount )
  {
    GlueCount = 255;
    GlueFreeBlocks();
    if ( FreeList[indx].next )
      return RemoveNode(indx);
  }
  int i=indx;
  do
  {
    if (++i == N_INDEXES)
    {
      GlueCount--;
      i=U2B(Indx2Units[indx]);
      int j=12*Indx2Units[indx];
      if (FakeUnitsStart-pText > j)
      {
        FakeUnitsStart-=j;
        UnitsStart -= i;
        return(UnitsStart);
      }
      return(NULL);
    }
  } while ( !FreeList[i].next );
  void* RetVal=RemoveNode(i);
  SplitBlock(RetVal,i,indx);
  return RetVal;
}


inline void* SubAllocator::AllocUnits(int NU)
{
  int indx=Units2Indx[NU-1];
  if ( FreeList[indx].next )
    return RemoveNode(indx);
  void* RetVal=LoUnit;
  LoUnit += U2B(Indx2Units[indx]);
  if (LoUnit <= HiUnit)
    return RetVal;
  LoUnit -= U2B(Indx2Units[indx]);
  return AllocUnitsRare(indx);
}


void* SubAllocator::AllocContext()
{
  if (HiUnit != LoUnit)
    return (HiUnit -= UNIT_SIZE);
  if ( FreeList->next )
    return RemoveNode(0);
  return AllocUnitsRare(0);
}


void* SubAllocator::ExpandUnits(void* OldPtr,int OldNU)
{
  int i0=Units2Indx[OldNU-1], i1=Units2Indx[OldNU-1+1];
  if (i0 == i1)
    return OldPtr;
  void* ptr=AllocUnits(OldNU+1);
  if ( ptr ) 
  {
    memcpy(ptr,OldPtr,U2B(OldNU));
    InsertNode(OldPtr,i0);
  }
  return ptr;
}


void* SubAllocator::ShrinkUnits(void* OldPtr,int OldNU,int NewNU)
{
  int i0=Units2Indx[OldNU-1], i1=Units2Indx[NewNU-1];
  if (i0 == i1)
    return OldPtr;
  if ( FreeList[i1].next )
  {
    void* ptr=RemoveNode(i1);
    memcpy(ptr,OldPtr,U2B(NewNU));
    InsertNode(OldPtr,i0);
    return ptr;
  } 
  else 
  {
    SplitBlock(OldPtr,i0,i1);
    return OldPtr;
  }
}


void SubAllocator::FreeUnits(void* ptr,int OldNU)
{
  InsertNode(ptr,Units2Indx[OldNU-1]);
}
