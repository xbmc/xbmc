#pragma once

class CGFFPatch
{
  public:
    bool FFPatch(CStdString m_FFPatchFilePath, CStdString &strNEW_FFPatchFilePath); //
  private:
    BOOL applyPatches(BYTE* pbuffer, int patchCount);
    BOOL Patch1(BYTE* pbuffer, UINT location);
    BOOL Patch2(BYTE* pbuffer, UINT location);
    BOOL Patch3(BYTE* pbuffer, UINT location);
    BOOL Patch4(BYTE* pbuffer, UINT location);
    void replaceConditionalJump(BYTE* pbuffer, UINT &location, UINT range);
    BOOL findConditionalJump(BYTE* pbuffer, UINT &location, UINT range);
    int examinePatch(BYTE* pbuffer, UINT location);
    UINT searchData(BYTE* pBuffer, UINT startPos, UINT size);
};