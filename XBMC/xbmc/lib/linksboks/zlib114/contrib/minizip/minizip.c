#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#ifdef unix
# include <unistd.h>
# include <utime.h>
# include <sys/types.h>
# include <sys/stat.h>
#else
# include <direct.h>
# include <io.h>
#endif

#include "zip.h"


#define WRITEBUFFERSIZE (16384)
#define MAXFILENAME (256)

#ifdef WIN32
uLong filetime(f, tmzip, dt)
    char *f;                /* name of file to get info on */
    tm_zip *tmzip;             /* return value: access, modific. and creation times */
    uLong *dt;             /* dostime */
{
  int ret = 0;
  {
      FILETIME ftLocal;
      HANDLE hFind;
      WIN32_FIND_DATA  ff32;

      hFind = FindFirstFile(f,&ff32);
      if (hFind != INVALID_HANDLE_VALUE)
      {
        FileTimeToLocalFileTime(&(ff32.ftLastWriteTime),&ftLocal);
        FileTimeToDosDateTime(&ftLocal,((LPWORD)dt)+1,((LPWORD)dt)+0);
        FindClose(hFind);
        ret = 1;
      }
  }
  return ret;
}
#else
#ifdef unix
uLong filetime(f, tmzip, dt)
    char *f;                /* name of file to get info on */
    tm_zip *tmzip;             /* return value: access, modific. and creation times */
    uLong *dt;             /* dostime */
{
  int ret=0;
  struct stat s;        /* results of stat() */
  struct tm* filedate;
  time_t tm_t=0;
  
  if (strcmp(f,"-")!=0)
  {
    char name[MAXFILENAME];
    int len = strlen(f);
    strcpy(name, f);
    if (name[len - 1] == '/')
      name[len - 1] = '\0';
    /* not all systems allow stat'ing a file with / appended */
    if (stat(name,&s)==0)
    {
      tm_t = s.st_mtime;
      ret = 1;
    }
  }
  filedate = localtime(&tm_t);

  tmzip->tm_sec  = filedate->tm_sec;
  tmzip->tm_min  = filedate->tm_min;
  tmzip->tm_hour = filedate->tm_hour;
  tmzip->tm_mday = filedate->tm_mday;
  tmzip->tm_mon  = filedate->tm_mon ;
  tmzip->tm_year = filedate->tm_year;

  return ret;
}
#else
uLong filetime(f, tmzip, dt)
    char *f;                /* name of file to get info on */
    tm_zip *tmzip;             /* return value: access, modific. and creation times */
    uLong *dt;             /* dostime */
{
    return 0;
}
#endif
#endif




int check_exist_file(filename)
    const char* filename;
{
	FILE* ftestexist;
    int ret = 1;
	ftestexist = fopen(filename,"rb");
	if (ftestexist==NULL)
        ret = 0;
    else
        fclose(ftestexist);
    return ret;
}

void do_banner()
{
	printf("MiniZip 0.15, demo of zLib + Zip package written by Gilles Vollant\n");
	printf("more info at http://wwww.winimage/zLibDll/unzip.htm\n\n");
}

void do_help()
{	
	printf("Usage : minizip [-o] file.zip [files_to_add]\n\n") ;
}

int main(argc,argv)
	int argc;
	char *argv[];
{
	int i;
	int opt_overwrite=0;
    int opt_compress_level=Z_DEFAULT_COMPRESSION;
    int zipfilenamearg = 0;
	char filename_try[MAXFILENAME];
    int zipok;
    int err=0;
    int size_buf=0;
    void* buf=NULL,


	do_banner();
	if (argc==1)
	{
		do_help();
		exit(0);
        return 0;
	}
	else
	{
		for (i=1;i<argc;i++)
		{
			if ((*argv[i])=='-')
			{
				const char *p=argv[i]+1;
				
				while ((*p)!='\0')
				{			
					char c=*(p++);;
					if ((c=='o') || (c=='O'))
						opt_overwrite = 1;
                    if ((c>='0') && (c<='9'))
                        opt_compress_level = c-'0';
				}
			}
			else
				if (zipfilenamearg == 0)
                    zipfilenamearg = i ;
		}
	}

    size_buf = WRITEBUFFERSIZE;
    buf = (void*)malloc(size_buf);
    if (buf==NULL)
    {
        printf("Error allocating memory\n");
        return ZIP_INTERNALERROR;
    }

	if (zipfilenamearg==0)
        zipok=0;
    else
	{
        int i,len;
        int dot_found=0;

        zipok = 1 ;
		strcpy(filename_try,argv[zipfilenamearg]);
        len=strlen(filename_try);
        for (i=0;i<len;i++)
            if (filename_try[i]=='.')
                dot_found=1;

        if (dot_found==0)
            strcat(filename_try,".zip");

        if (opt_overwrite==0)
            if (check_exist_file(filename_try)!=0)
			{
                char rep;
				do
				{
					char answer[128];
					printf("The file %s exist. Overwrite ? [y]es, [n]o : ",filename_try);
					scanf("%1s",answer);
					rep = answer[0] ;
					if ((rep>='a') && (rep<='z'))
						rep -= 0x20;
				}
				while ((rep!='Y') && (rep!='N'));
                if (rep=='N')
                    zipok = 0;
			}
    }

    if (zipok==1)
    {
        zipFile zf;
        int errclose;
        zf = zipOpen(filename_try,0);
        if (zf == NULL)
        {
            printf("error opening %s\n",filename_try);
            err= ZIP_ERRNO;
        }
        else 
            printf("creating %s\n",filename_try);

        for (i=zipfilenamearg+1;(i<argc) && (err==ZIP_OK);i++)
        {
            if (((*(argv[i]))!='-') && ((*(argv[i]))!='/'))
            {
                FILE * fin;
                int size_read;
                const char* filenameinzip = argv[i];
                zip_fileinfo zi;

                zi.tmz_date.tm_sec = zi.tmz_date.tm_min = zi.tmz_date.tm_hour = 
                zi.tmz_date.tm_mday = zi.tmz_date.tm_min = zi.tmz_date.tm_year = 0;
                zi.dosDate = 0;
                zi.internal_fa = 0;
                zi.external_fa = 0;
                filetime(filenameinzip,&zi.tmz_date,&zi.dosDate);


                err = zipOpenNewFileInZip(zf,filenameinzip,&zi,
                                 NULL,0,NULL,0,NULL /* comment*/,
                                 (opt_compress_level != 0) ? Z_DEFLATED : 0,
                                 opt_compress_level);

                if (err != ZIP_OK)
                    printf("error in opening %s in zipfile\n",filenameinzip);
                else
                {
                    fin = fopen(filenameinzip,"rb");
                    if (fin==NULL)
                    {
                        err=ZIP_ERRNO;
                        printf("error in opening %s for reading\n",filenameinzip);
                    }
                }

                if (err == ZIP_OK)
                    do
                    {
                        err = ZIP_OK;
                        size_read = fread(buf,1,size_buf,fin);
                        if (size_read < size_buf)
                            if (feof(fin)==0)
                        {
                            printf("error in reading %s\n",filenameinzip);
                            err = ZIP_ERRNO;
                        }

                        if (size_read>0)
                        {
                            err = zipWriteInFileInZip (zf,buf,size_read);
                            if (err<0)
                            {
                                printf("error in writing %s in the zipfile\n",
                                                 filenameinzip);
                            }
                                
                        }
                    } while ((err == ZIP_OK) && (size_read>0));

                fclose(fin);
                if (err<0)
                    err=ZIP_ERRNO;
                else
                {                    
                    err = zipCloseFileInZip(zf);
                    if (err!=ZIP_OK)
                        printf("error in closing %s in the zipfile\n",
                                    filenameinzip);
                }
            }
        }
        errclose = zipClose(zf,NULL);
        if (errclose != ZIP_OK)
            printf("error in closing %s\n",filename_try);
   }

    free(buf);
    exit(0);
	return 0;  /* to avoid warning */
}
