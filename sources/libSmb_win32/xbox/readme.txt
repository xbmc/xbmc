when updating the samba source, keep the next things in mind

- different code is always enclosed with #ifdef _XBOX or #ifndef _XBOX and #endif
- different code is never! enclosed with #if 0 ir #ifdef 0, if it is, it is the intention of the samba team.
- includes\proto.h is not completely updated to samba 3.0.4, so you may expect errors like 
  'warning C4028: formal parameter 2 different from declaration' (making a new one can only done under linux).

changes to source (original text -> replaced text) not! enclosed with // _XBOX

- PRIVILEGE_SET ->    SMB_PRIVILEGE_SET
- SYSTEMTIME ->       SMB_SYSTEMTIME
- LUID ->             SMBLUID (defined in 'include\privileges.h')
- GENERIC_MAPPING ->  SMB_GENERIC_MAPPING (defined in 'include\rpc_secdes.h')
- struct stat ->      SMB_STRUCT_STAT (SMB_STRUCT_STAT = __stat64 now)

- some (not all) (int32 / size_t / long) -> (SMB_OFF_T / UINT64 / long long)


