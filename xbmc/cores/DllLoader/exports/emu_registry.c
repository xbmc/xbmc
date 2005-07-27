#include <xtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>

#include "emu_registry.h"
#include "emu_dummy.h"

//can use this global overide default path name
char* regpathname = NULL;

//carry default regisrty.dat path+filename
static char* localregpathname = "Q:\\system\\players\\mplayer\\codecs\\registry.dat";

static void dbgprintf(char* fmt, ...)
{
  char buffer[1024];
  va_list va;
  va_start(va, fmt);
  _vsnprintf(buffer, sizeof(buffer), fmt, va);
  OutputDebugString(buffer);
  va_end(va);
}

typedef struct reg_handle_s
{
  int handle;
  char* name;
  struct reg_handle_s* next;
  struct reg_handle_s* prev;
} reg_handle_t;

struct reg_value
{
  int type;
  char* name;
  int len;
  char* value;
};

static struct reg_value* regs = NULL;
static int reg_size;
static reg_handle_t* head = NULL;

#define DIR -25

static void create_registry(void);
static void open_registry(void);
//static void save_registry(void);
static void init_registry(void);


static void create_registry(void)
{
  if(regs)
  {
    OutputDebugString("Logic error: create_registry() called with existing registry\n");
    save_registry();
    return;
  }
  regs=(struct reg_value*)malloc(3*sizeof(struct reg_value));
  regs[0].type=regs[1].type=DIR;
  regs[0].name=(char*)malloc(5);
  strcpy(regs[0].name, "HKLM");
  regs[1].name=(char*)malloc(5);
  strcpy(regs[1].name, "HKCU");
  regs[0].value=regs[1].value=NULL;
  regs[0].len=regs[1].len=0;
  reg_size=2;
  head = 0;
  //save_registry();
}

static void open_registry(void)
{
  int fd;
  int i;
  unsigned int len;
  if(regs)
  {
    OutputDebugString("Multiple open_registry()\n");
    return;
  }
  fd = open(localregpathname, O_RDONLY|O_BINARY);
  if (fd == -1)
  {
    OutputDebugString("Creating new registry\n");
    create_registry();
    return;
  }
  read(fd, &reg_size, 4);
  regs=(struct reg_value*)malloc(reg_size*sizeof(struct reg_value));
  head = 0;
  for(i=0; i<reg_size; i++)
  {
    read(fd,&regs[i].type,4);
    read(fd,&len,4);
    regs[i].name=(char*)malloc(len+1);
    if(regs[i].name==0)
    {
      reg_size=i+1;
      goto error;
    }
    read(fd, regs[i].name, len);
    regs[i].name[len]=0;
    read(fd,&regs[i].len,4);
    regs[i].value=(char*)malloc(regs[i].len+1);
    if(regs[i].value==0)
    {
      free(regs[i].name);
      reg_size=i+1;
      goto error;
    }
    read(fd, regs[i].value, regs[i].len);
    regs[i].value[regs[i].len]=0;
  }
error:
  close(fd);
  return;
}

void save_registry(void)
{
  int fd, i;
  if (!regs) return;
  fd = open(localregpathname, O_WRONLY | O_CREAT|O_BINARY, 00666);
  if (fd == -1)
  {
    OutputDebugString("Failed to open registry file for writing.\n");
    return;
  }
  write(fd, &reg_size, 4);
  for(i=0; i<reg_size; i++)
  {
    unsigned len=strlen(regs[i].name);
    write(fd, &regs[i].type, 4);
    write(fd, &len, 4);
    write(fd, regs[i].name, len);
    write(fd, &regs[i].len, 4);
    write(fd, regs[i].value, regs[i].len);
  }
  close(fd);
}

void free_registry(void)
{
  reg_handle_t* t = head;
  while (t)
  {
    reg_handle_t* f = t;
    if (t->name)
      free(t->name);
    t=t->prev;
    free(f);
  }
  head = 0;
  if (regs)
  {
    int i;
    for(i=0; i<reg_size; i++)
    {
      free(regs[i].name);
      free(regs[i].value);
    }
    free(regs);
    regs = 0;
  }
}


static reg_handle_t* find_handle_by_name(const char* name)
{
  reg_handle_t* t;
  for(t=head; t; t=t->prev)
  {
    if(!strcmp(t->name, name))
    {
      return t;
    }
  }
  return 0;
}
static struct reg_value* find_value_by_name(const char* name)
{
  int i;
  for(i=0; i<reg_size; i++)
    if(!strcmp(regs[i].name, name))
      return regs+i;
  return 0;
}
static reg_handle_t* find_handle(int handle)
{
  reg_handle_t* t;
  for(t=head; t; t=t->prev)
  {
    if(t->handle==handle)
    {
      return t;
    }
  }
  return 0;
}
static int generate_handle()
{
  static unsigned int zz=249;
  zz++;
  while((zz==(unsigned int)HKEY_LOCAL_MACHINE) || (zz==(unsigned int)HKEY_CURRENT_USER))
    zz++;
  return zz;
}

static reg_handle_t* insert_handle(long handle, const char* name)
{
  reg_handle_t* t;
  t=(reg_handle_t*)malloc(sizeof(reg_handle_t));
  if(head==0)
  {
    t->prev=0;
  }
  else
  {
    head->next=t;
    t->prev=head;
  }
  t->next=0;
  t->name=(char*)malloc(strlen(name)+1);
  strcpy(t->name, name);
  t->handle=handle;
  head=t;
  return t;
}
static char* build_keyname(long key, const char* subkey)
{
  char* full_name;
  reg_handle_t* t;
  if((t=find_handle(key))==0)
  {
    OutputDebugString("Registery:Invalid key\n");
    return NULL;
  }
  if(subkey==NULL)
    subkey="<default>";
  full_name=(char*)malloc(strlen(t->name)+strlen(subkey)+10);
  strcpy(full_name, t->name);
  strcat(full_name, "\\");
  strcat(full_name, subkey);
  return full_name;
}
static struct reg_value* insert_reg_value(int handle, const char* name, int type, const void* value, int len)
{
  // reg_handle_t* t;
  struct reg_value* v;
  char* fullname;
  if((fullname=build_keyname(handle, name))==NULL)
  {
    OutputDebugString("Registery:Invalid handle\n");
    return NULL;
  }

  if((v=find_value_by_name(fullname))==0)
    //creating new value in registry
  {
    if(regs==0)
      create_registry
      ();
    regs=(struct reg_value*)realloc(regs, sizeof(struct reg_value)*(reg_size+1));
    //regs=(struct reg_value*)my_realloc(regs, sizeof(struct reg_value)*(reg_size+1));
    v=regs+reg_size;
    reg_size++;
  }
  else
    //replacing old one
  {
    free(v->value);
    free(v->name);
  }
  //mp_msg(0,0,"RegInsert '%s' %p v:%d len:%d\n", name, value, *(int*)value, len);
  v->type=type;
  v->len=len;
  v->value=(char*)malloc(len);
  memcpy(v->value, value, len);
  v->name=(char*)malloc(strlen(fullname)+1);
  strcpy(v->name, fullname);
  free(fullname);
  //save_registry ();
  return v;
}

static void init_registry(void)
{
  // can't be free-ed - it's static and probably thread
  // unsafe structure which is stored in glibc

  if( regpathname != NULL )
    localregpathname = regpathname;

  open_registry();
  insert_handle((long)HKEY_LOCAL_MACHINE, "HKLM");
  insert_handle((long)HKEY_CURRENT_USER, "HKCU");
}

static reg_handle_t* find_handle_2(long key, const char* subkey)
{
  char* full_name;
  reg_handle_t* t;
  if((t=find_handle(key))==0)
  {
    OutputDebugString("Registery:Invalid key\n");
    return (reg_handle_t*)-1;
  }
  if(subkey==NULL)
    return t;
  full_name=(char*)malloc(strlen(t->name)+strlen(subkey)+10);
  strcpy(full_name, t->name);
  strcat(full_name, "\\");
  strcat(full_name, subkey);
  t=find_handle_by_name(full_name);
  free(full_name);
  return t;
}

/////////////////////////////////////////////////////////////
LONG WINAPI dllRegOpenKeyExA(HKEY key, LPCSTR subkey, DWORD reserved, REGSAM access, PHKEY newkey)
{
  char* full_name;
  reg_handle_t* t;
  struct reg_value* v;

  //char szBuf[256]; //debug varibabe

  if(!regs)
    init_registry
    ()
    ;
  /* t=find_handle_2(key, subkey);

  if(t==0)
  return -1;

  if(t==(reg_handle_t*)-1)
  return -1;
  */
  full_name=build_keyname((long)key, subkey);
  if(!full_name)
    return -1;

  //for debuging purpose
  //sprintf(szBuf,"RegOpenKey Full Name %s\n",full_name);
  //OutputDebugString(szBuf);
  dbgprintf("RegOpenKeyExA(key 0x%x, subkey %s, reserved %d, access 0x%x, pnewkey 0x%x) => 0\n",
    key, subkey, reserved, access, newkey);
  if(newkey)dbgprintf(" New key: 0x%x\n", *newkey);


  v=find_value_by_name(full_name);

  t=insert_handle(generate_handle(), full_name);
  *newkey=(HKEY)t->handle;
  free(full_name);

  return 0;
}

LONG WINAPI dllRegCloseKey(HKEY key)
{
  reg_handle_t *handle;
  if(key==HKEY_LOCAL_MACHINE)
    return 0;
  if(key==HKEY_CURRENT_USER)
    return 0;
  handle=find_handle((int)key);
  if(handle==0)
    return 0;
  if(handle->prev)
    handle->prev->next=handle->next;
  if(handle->next)
    handle->next->prev=handle->prev;
  if(handle->name)
    free(handle->name);
  if(handle==head)
    head=head->prev;
  free(handle);
  return 1;
}

LONG WINAPI dllRegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)
{
  LONG result;

  // DWORD rtn_addr;
  // char szBuf[256];
  // __asm { mov eax, [ebp+4] }
  // __asm { mov rtn_addr, eax }

  result=dllRegOpenKeyExA(hKey, lpSubKey, 0, 0/*KEY_ALL_ACCESS*/, phkResult);

  // sprintf(szBuf,"called from 0x%08X RegOpenKeyA(DWORD:0x%08X, SubKey:%s, [phkResult]=0x%08X)\n",rtn_addr,hKey,lpSubKey,*phkResult);
  // OutputDebugString(szBuf);

  return result;
}

LONG WINAPI dllRegQueryValueA(HKEY hKey, LPCTSTR lpSubKey, LPTSTR lpValue, PLONG lpcbValue)
{
  //not_implement("advapi32.dll fake function RegQueryValueA called\n");  //warning
  return ERROR_INVALID_FUNCTION;
}

LONG WINAPI dllRegQueryValueExA (HKEY key, LPCTSTR value, LPDWORD reserved,
                                 LPDWORD type, LPBYTE data, LPDWORD count)
{
  struct reg_value* t;
  char* c;
  //char szBuf[256]; //debug strings

  //mp_msg(0,0,"Querying value %s\n", value);
  if(!regs)
    init_registry
    ();

  c=build_keyname((long)key, value);

  //sprintf(szBuf,"RegOpenKey Full Name %s\n",c); //debug output
  //OutputDebugString(szBuf);

  if (!c)
    return 1;
  t=find_value_by_name(c);
  free(c);
  if (t==0){
    dbgprintf("RegQueryValueExA(key 0x%x, value %s, reserved 0x%x, data 0x%x, count 0x%x)"
      " => 0x%x\n", key, value, reserved, data, count, 2);
    memset(data, 0, *count);
    if(data && count)dbgprintf(" read %d bytes: '%s'\n", *count, data);
    return 2;
  }
  if (type)
    *type=t->type;
  if (data)
  {
    memcpy(data, t->value, (t->len<(int)*count)?t->len:(int)*count);
    //mp_msg(0,0,"returning %d bytes: %d\n", t->len, *(int*)data);
  }
  if((int)*count<t->len)
  {
    *count=t->len;
    dbgprintf("RegQueryValueExA(key 0x%x, value %s, reserved 0x%x, data 0x%x, count 0x%x)"
      " => 0x%x\n", key, value, reserved, data, count, ERROR_MORE_DATA);
    if(data && count)dbgprintf(" read %d bytes: '%s'\n", *count, data);

    return ERROR_MORE_DATA;
  }
  else
  {
    *count=t->len;
  }

  dbgprintf("RegQueryValueExA(key 0x%x, value %s, reserved 0x%x, data 0x%x, count 0x%x)"
    " => 0x%x\n", key, value, reserved, data, count, 0);
  if(data && count)dbgprintf(" read %d bytes: '%s'\n", *count, data);

  return 0;
}

LONG WINAPI dllRegCreateKeyExA (HKEY key, LPCTSTR name, DWORD reserved,
                                LPTSTR classs, DWORD options, REGSAM security,
                                LPSECURITY_ATTRIBUTES sec_attr,
                                PHKEY newkey, LPDWORD status)
{
  reg_handle_t* t;
  char* fullname;
  struct reg_value* v;
  // mp_msg(0,0,"Creating/Opening key %s\n", name);
  if(!regs)
    init_registry
    ();

  fullname=build_keyname((long)key, name);
  if (!fullname)
    return 1;
  //mp_msg(0,0,"Creating/Opening key %s\n", fullname);
  v=find_value_by_name(fullname);
  if(v==0)
  {
    int qw=45708;
    v=insert_reg_value((int)key, name, DIR, &qw, 4);
    if (status)
      *status=REG_CREATED_NEW_KEY;
    // return 0;
  }

  t=insert_handle(generate_handle(), fullname);
  *newkey=(HKEY)t->handle;
  free(fullname);
  return 0;
}

//This from WINE
LONG WINAPI dllRegCreateKeyA( HKEY hkey, LPCSTR name, PHKEY retkey )
{
  return dllRegCreateKeyExA( hkey, name, 0, NULL, 0/*REG_OPTION_NON_VOLATILE*/,
    0/*KEY_ALL_ACCESS*/, NULL, retkey, NULL );
}

LONG WINAPI dllRegEnumValueA (HKEY hkey, DWORD index, LPSTR value, LPDWORD val_count,
                              LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD count)
{
  // currenly just made to support MSZH & ZLIB
  //mp_msg(0,0,"Reg Enum 0x%x %d %s %d data: %p %d %d >%s<\n", hkey, index,
  // value, *val_count, data, *count, reg_size, data);
  reg_handle_t* t = find_handle((int)hkey);
  if (t && index < 10)
  {
    struct reg_value* v=find_value_by_name(t->name);
    if (v)
    {
      memcpy(data, v->value, (v->len < (int)*count) ? v->len : *count);
      if((int)*count < v->len)
        *count = v->len;
      if (type)
        *type = v->type;
      //mp_msg(0,0,"Found handle %s\n", v->name);
      return 0;
    }
  }
  return ERROR_NO_MORE_ITEMS;
}

LONG WINAPI dllRegSetValueExA(HKEY key, LPCTSTR name, DWORD v1,
                              DWORD v2, const BYTE * data, DWORD size)
{
  // struct reg_value* t;
  char* c;
  //mp_msg(0,0,"Request to set value %s %d\n", name, *(const int*)data);
  if(!regs)
    init_registry
    ();

  c=build_keyname((long)key, name);
  if(c==NULL)
    return 1;
  insert_reg_value((int)key, name, v2, data, size);
  free(c);
  return 0;
}

//This from WINE
LONG WINAPI dllRegSetValueA( HKEY hkey, LPCTSTR name, DWORD type, LPCTSTR data, DWORD count )
{
  HKEY subkey = hkey;
  DWORD ret;

  if (type != REG_SZ) return ERROR_INVALID_PARAMETER;

  if (name && name[0])/* need to create the subkey */
  {
    if ((ret = dllRegCreateKeyA( hkey, name, &subkey )) != ERROR_SUCCESS) return ret;
  }
  ret = dllRegSetValueExA( subkey, NULL, 0, REG_SZ, (LPBYTE)data, strlen(data)+1 );
  if (subkey != hkey) dllRegCloseKey( subkey );
  return ret;
}

/*LONG WINAPI dllRegOpenKeyExA (HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions,
REGSAM samDesired, PHKEY phkResult)
{
not_implement("advapi32.dll fake function RegOpenKeyExA called\n"); //warning
return 1;
}

LONG WINAPI dllRegCloseKey(HKEY hKey)
{
not_implement("advapi32.dll fake function RegCloseKey called\n"); //warning
return 0; //fake success signal
}

LONG WINAPI dllRegOpenKeyA(HKEY hKey, LPCTSTR lpSubKey, PHKEY phkResult)
{
not_implement("advapi32.dll fake function RegOpenKeyA called\n"); //warning
return 1;
}*/

/*LONG WINAPI dllRegSetValueA (HKEY hKey, LPCTSTR lpSubKey, DWORD dwType,
LPCTSTR lpData, DWORD cbData)
{
not_implement("advapi32.dll fake function RegSetValueA called\n"); //warning
return 1;
}*/

LONG WINAPI dllRegEnumKeyExA (HKEY hKey, DWORD dwIndex, LPTSTR lpName,
                              LPDWORD lpcName, LPDWORD lpReserved, LPTSTR lpClass,
                              LPDWORD lpcClass, PFILETIME lpftLastWriteTime)
{
  not_implement("advapi32.dll fake function RegEnumKeyExA called\n"); //warning
  return 1;
}

LONG WINAPI dllRegDeleteKeyA (HKEY hKey, LPCTSTR lpSubKey)
{
  not_implement("advapi32.dll fake function RegDeleteKeyA called\n"); //warning
  return 1;
}

LONG WINAPI dllRegQueryInfoKeyA( HKEY hkey, LPSTR class_, LPDWORD class_len, LPDWORD reserved,
                                LPDWORD subkeys, LPDWORD max_subkey, LPDWORD max_class,
                                LPDWORD values, LPDWORD max_value, LPDWORD max_data,
                                LPDWORD security, FILETIME *modif )
{
  not_implement("advapi32.dll fake function RegQueryInfoKeyA called\n"); //warning
  return 1;
}

/*LONG WINAPI dllRegQueryValueExA (HKEY hKey, LPCTSTR lpValueName, LPDWORD lpReserved,
LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
not_implement("advapi32.dll fake function RegQueryValueExA called\n"); //warning
return 1;
}*/

/*LONG WINAPI dllRegCreateKeyA (HKEY hKey, LPCTSTR lpSubKey, PHKEY phkResult)
{
not_implement("advapi32.dll fake function RegCreatKeyA called\n"); //warning
return 1;
}*/

/*LONG WINAPI dllRegSetValueExA (HKEY hKey, LPCTSTR lpValueName, DWORD Reserved,
DWORD dwType, const BYTE* lpData, DWORD cbData)
{
not_implement("advapi32.dll fake function RegSetValueExA called\n"); //warning
return 1;
}*/

/*LONG WINAPI dllRegCreateKeyExA (HKEY hKey, LPCTSTR lpSubKey, DWORD Reserved,
LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired,
LPSECURITY_ATTRIBUTES lpSecurityAttributes,
PHKEY phkResult, LPDWORD lpdwDisposition)
{
not_implement("advapi32.dll fake function RegCreateKeyExA called\n"); //warning
return 1;
}*/

/*LONG WINAPI dllRegEnumValueA (HKEY hKey, DWORD dwIndex, LPTSTR lpValueName,
LPDWORD lpcValueName, LPDWORD lpReserved,
LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
not_implement("advapi32.dll fake function RegEnumValueA called\n"); //warning
return 1;
}*/

BOOL WINAPI dllCryptAcquireContextA(HCRYPTPROV* phProv, LPCTSTR pszContainer, LPCTSTR pszProvider, DWORD dwProvType, DWORD dwFlags)
{
  not_implement("advapi32.dll fake function dllCryptAcquireContext() called\n");
  return 1;
}

BOOL WINAPI dllCryptGenRandom(HCRYPTPROV hProv, DWORD dwLen, BYTE* pbBuffer)
{
  unsigned int i;
  // just create some random data
  for (i = 0; i < dwLen; i = i + 4)
  {
    pbBuffer[i] = rand() & 0xFF;
  }
  return 1;
}

BOOL WINAPI dllCryptReleaseContext(HCRYPTPROV hProv, DWORD dwFlags)
{
  not_implement("advapi32.dll fake function dllCryptReleaseContext() called\n");
  return 1;
}
