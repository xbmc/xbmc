/*****************************************************************
|
|   Platinum - Tool text to .h
|
| Copyright (c) 2004-2008, Plutinosoft, LLC.
| All rights reserved.
| http://www.plutinosoft.com
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of the GNU General Public License
| as published by the Free Software Foundation; either version 2
| of the License, or (at your option) any later version.
|
| OEMs, ISVs, VARs and other distributors that combine and 
| distribute commercially licensed software with Platinum software
| and do not wish to distribute the source code for the commercially
| licensed software under version 2, or (at your option) any later
| version, of the GNU General Public License (the "GPL") must enter
| into a commercial license agreement with Plutinosoft, LLC.
| 
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; see the file LICENSE.txt. If not, write to
| the Free Software Foundation, Inc., 
| 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
| http://www.gnu.org/licenses/gpl-2.0.html
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/
static struct {
    const char* in_filename;
    const char* header_filename;
} Options;

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit(char** args)
{
    fprintf(stderr, "usage: %s [-kh <header>] <filename>\n", args[0]);
    fprintf(stderr, "-kh : optional output .h header filename\n");
	fprintf(stderr, "<filename> : input text filename\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   ParseCommandLine
+---------------------------------------------------------------------*/
static void
ParseCommandLine(char** args)
{
    const char* arg;
    char** tmp = args+1;

    /* default values */
    Options.in_filename     = NULL;
    Options.header_filename = NULL;

    while ((arg = *tmp++)) {
        if (!strcmp(arg, "-kh")) {
            Options.header_filename = *tmp++;
        } else if (Options.in_filename == NULL) {
            Options.in_filename = arg;
        } else {
            fprintf(stderr, "ERROR: too many arguments\n");
            PrintUsageAndExit(args);
        }
    }

    /* check args */
    if (Options.in_filename == NULL) {
        fprintf(stderr, "ERROR: input filename missing\n");
        PrintUsageAndExit(args);
    }
}

/*----------------------------------------------------------------------
|   PrintHex
+---------------------------------------------------------------------*/
static void
PrintHex(unsigned char* h, unsigned int size)
{
    unsigned int i;
    for (i=0; i<size; i++) {
        printf("%c%c", 
               h[i]>>4 >= 10 ? 
               'A' + (h[i]>>4)-10 : 
               '0' + (h[i]>>4),
               (h[i]&0xF) >= 10 ? 
               'A' + (h[i]&0xF)-10 : 
               '0' + (h[i]&0xF));
    }
}

/*----------------------------------------------------------------------
|   PrintHexForHeader
+---------------------------------------------------------------------*/
static void
PrintHexForHeader(FILE* out, unsigned char h)
{
    fprintf(out, "0x%c%c", 
           h>>4 >= 10 ? 
           'A' + (h>>4)-10 : 
           '0' + (h>>4),
           (h&0xF) >= 10 ? 
           'A' + (h&0xF)-10 : 
           '0' + (h&0xF));
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int /*argc*/, char** argv)
{
    FILE*           in;
    FILE*           header;
    unsigned char*  data_block = NULL;
    unsigned long   data_block_size;
    unsigned long   k;
    unsigned char   col;
    
    /* parse command line */
    ParseCommandLine(argv);

    /* open input */
    in = fopen(Options.in_filename, "rb");
    if (in == NULL) {
        fprintf(stderr, "ERROR: cannot open input file (%s): %s\n", 
                Options.in_filename, strerror(errno));
    }

    /* read data in one chunk */
    {
        struct stat info;
        if (stat(Options.in_filename, &info)) {
            fprintf(stderr, "ERROR: cannot get input file size\n");
            return 1;
        }

        data_block_size = info.st_size;
        data_block = (unsigned char*)new unsigned char[data_block_size+1];
        if (data_block == NULL) {
            fprintf(stderr, "ERROR: out of memory\n");
            return 1;
        }

        if (fread(data_block, data_block_size, 1, in) != 1) {
            fprintf(stderr, "ERROR: cannot read input file\n");
            return 1;
        }
        data_block[data_block_size++] = 0;
    }

    if (Options.header_filename != NULL) {
        
        /* open header output */
        header = fopen(Options.header_filename, "w+");
        if (header == NULL) {
            fprintf(stderr, "ERROR: cannot open header output file (%s): %s\n", 
                Options.header_filename, strerror(errno));
        }

        /* print header */
        fprintf(header, "#include \"NptTypes.h\"\n\n");
        //fprintf(header, "#ifndef _DATABLOCK_H_\n");
        //fprintf(header, "#define _DATABLOCK_H_\n\n");
        fprintf(header, "NPT_UInt8 kDataBlock[%ld] =\n", data_block_size);
        fprintf(header, "{\n  ");
        col = 0;
        
        /* rewind the input file */
        fseek(in, 0, SEEK_SET);

        for (k = 0; k < data_block_size; k++) {
            PrintHex(&data_block[k], 1);
            PrintHexForHeader(header, data_block[k]);
            if (k < data_block_size - 1) fprintf(header, ", ");

            /* wrap around 20 columns */
            if (++col > 19) {
                col = 0;
                fprintf(header, "\n  ");
            }
        }

        /* print footer */
        fprintf(header, "\n};\n\n");  
        //fprintf(header, "#endif /* _DATABLOCK_H_ */\n");
        
        /* close file */
        fclose(header);
    }

    /* close file */
    fclose(in);

    if (data_block) {
        delete[] data_block;
    }
    return 0;
}
