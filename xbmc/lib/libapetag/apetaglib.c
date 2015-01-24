/********************************************************************
* 
* Copyright (c) 2002 Artur Polaczynski (Ar't)  All rights reserved.
*            <artii@o2.pl>        LGPL-2.1
*       $ArtId: apetaglib.c,v 1.44 2003/04/16 21:06:27 art Exp $
********************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <math.h>
#if !defined(__BORLANDC__) && !defined(_MSC_VER)
#    include <unistd.h>
#endif
#include "file_io.h"
#include "apetaglib.h"

#include "is_tag.h"
#include "genres.h"
#ifdef ID3V2_READ
#    include "id3v2_read.h"
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#define strcasecmp stricmp
#define snprintf _snprintf
#define USE_CHSIZE
#define S_IRUSR 0
#define S_IWUSR 0
#define S_IRGRP 0
#define S_IWGRP 0
#if defined(_WIN64)
 typedef __int64 ssize_t; 
#else
 typedef long ssize_t;
#endif
#endif

/* LOCAL STRUCTURES */

/**
    \struct _apetag_footer
    \brief structure of APETAGEXT footer or/and header tag 
*/
struct _apetag_footer
{
    unsigned char id[8];        /**< magic should equal 'APETAGEX' */
    unsigned char version[4];   /**< version 1000 (v1.0) or 2000 (v 2.0) */
    unsigned char length[4];    /**< the complete size of the tag, including footer, but no header for v2.0 */
    unsigned char tagCount[4];  /**< the number of fields in the tag */
    unsigned char flags[4];     /**< the tag flags (none currently defined for v 1.0) */
    unsigned char reserved[8];  /**< reserved for later use */
};

/** 
    \struct _ape_mem_cnt
    \brief internal structure for apetag 
*/
struct _ape_mem_cnt
{
    struct tag **tag;
    int countTag;
    int memTagAlloc;    // for mem container;
    char *filename;     // for info
    struct _apetag_footer ape_header;
    struct _apetag_footer ape_footer;
    int currentPosition;
};

/* *
    \struct _id3v1Tag
    \brief for id3v1 tag 
*/
struct _id3v1Tag
{
    char magic[3];    // `TAG`
    char title[30];
    char artist[30];
    char album[30];
    char year[4];
    char comment[30]; // if ([28]==0 and [29]!=0) track = [29]
    unsigned char genre;
};

/* LOCAL FUNCTION prototypes */
unsigned long
ape2long (unsigned char *p);
void
long2ape (unsigned char *p, const unsigned long value);
struct tag *
libapetag_maloc_cont_int (apetag *mem_cnt, struct tag *mTag);
int
libapetag_maloc_cont_text (apetag *mem_cnt, unsigned long flags,
             long sizeName, char *name, long sizeValue, char *value);
int
libapetag_maloc_cont (apetag *mem_cnt, unsigned long flags,
        long sizeName, char *name, long sizeValue, char *value);
static int
libapetag_qsort (struct tag **a, struct tag **b);
int
make_id3v1_tag(apetag *mem_cnt, struct _id3v1Tag *m);

    
unsigned long
ape2long (unsigned char *p)
{
    return (((unsigned long) p[0] << 0)  |
            ((unsigned long) p[1] << 8)  |
            ((unsigned long) p[2] << 16) | 
            ((unsigned long) p[3] << 24) );
}

void
long2ape (unsigned char *p, const unsigned long value)
{
    p[0] = (unsigned char) (value >> 0);
    p[1] = (unsigned char) (value >> 8);
    p[2] = (unsigned char) (value >> 16);
    p[3] = (unsigned char) (value >> 24);
}


/*
    PL: funkcja troszczaca sie o odpowiedni� ilosc zalokowanej pamieci dla tablicy 
    PL: %mTag% przy okazji alokuje z wyprzedzeniem troche wiecej pamieci [mniej %realoc%]
    PL: zwraca %mTag[]%
    :NON_USER:!!!
 */
#define LIBAPETAG_MEM_ALLOC_AHEAD 16    /* 15 it's good for normal #of tag, Aver 4-8 */
struct tag *
libapetag_maloc_cont_int (apetag *mem_cnt, struct tag *mTag)
{
    struct tag **tag_tmp = mem_cnt->tag;

    if (mem_cnt->memTagAlloc == 0) {    /* init */
        mem_cnt->tag = (struct tag **)
            malloc (((sizeof (struct tag **)) * (LIBAPETAG_MEM_ALLOC_AHEAD)));
        mem_cnt->memTagAlloc = LIBAPETAG_MEM_ALLOC_AHEAD;
        mem_cnt->countTag = 0;
        if (mem_cnt->tag == NULL) {
            mem_cnt->memTagAlloc = mem_cnt->countTag = 0;
            PRINT_ERR ( "ERROR->libapetag->libapetag_maloc_cont_int:malloc\n");
            return NULL;
        }
    }
    
    if ((mem_cnt->memTagAlloc) <= (mem_cnt->countTag + 1)) {
        mem_cnt->tag = (struct tag **) realloc (mem_cnt->tag, ((sizeof (struct tag **)) *
                        (mem_cnt->memTagAlloc + LIBAPETAG_MEM_ALLOC_AHEAD)));
        mem_cnt->memTagAlloc += LIBAPETAG_MEM_ALLOC_AHEAD;
    }
    
    if (mem_cnt->tag == NULL) {
        int n;
        
        PRINT_ERR ( "ERROR->libapetag->libapetag_maloc_cont_int:malloc\n");
        /* free old all */
        for (n = mem_cnt->countTag-1; n >= 0; n--) { 
            free (tag_tmp[n]->value);
            free (tag_tmp[n]->name);
            free (tag_tmp[n]);
        }
        free (tag_tmp);
        mem_cnt->memTagAlloc = mem_cnt->countTag = 0;
        return NULL;
    }
    
    mem_cnt->tag[mem_cnt->countTag] = mTag;
    mem_cnt->countTag++; 
    return mTag;

}
#undef LIBAPETAG_MEM_ALLOC_AHEAD


/*
    PL: alocuje pamiec dla %mTag% przypisuje odpowiednio wartosci 
    PL: dodaje %\0% do string�w [na wszelki wypadek]
    PL: nie dopisuje takich samych 
    PL: wszystkie sizy maja byc bez \0 (jak bedzie to doliczy jeszcze jeden)
    :NON_USER:!!!
 */
int
libapetag_maloc_cont (apetag *mem_cnt, unsigned long flags,
        long sizeName, char *name, long sizeValue, char *value)
{
    struct tag *mTag;
        // TODO:: zadbac o to zeby tu czyscilo istniejace tagi jesli value=NULL
    if (!sizeName || !sizeValue)
            return ATL_BADARG;
    
    if (apefrm_getstr (mem_cnt, name) == NULL) {
        mTag = (struct tag *) malloc (sizeof (struct tag));
          
        if (mTag == NULL)
                return ATL_MALOC;
        
        mTag->value = (char *) malloc (sizeValue + 1);
        if (mTag->value==NULL) {
            free (mTag);
            return ATL_MALOC;
        }
        
        mTag->name = (char *) malloc (sizeName + 1);
        if (mTag->name==NULL) {
            free (mTag->value);
            free (mTag);
            return ATL_MALOC;
        }
    
        memcpy (mTag->value, value, sizeValue);
        memcpy (mTag->name, name, sizeName);
        mTag->value[sizeValue] = '\0';
        mTag->name[sizeName] = '\0';
        mTag->sizeName = sizeName;
        mTag->sizeValue = sizeValue;
        mTag->flags = flags;
        
        if (libapetag_maloc_cont_int (mem_cnt, mTag)==NULL) {
            PRINT_ERR(">apetaglib>libapetag_maloc_cont>> int==NULL");
            return ATL_MALOC;
        }
    }
    
    return 0;
}

/*
    PL: jezeli nie istnieje to dodaje taga, pomija ostatnie biale znaki 
    PL: pomija jesli pusty 
    PL: ! zmienia tekst wej�ciowy
    :NON_USER:!!!
*/
int
libapetag_maloc_cont_text (apetag *mem_cnt, unsigned long flags,
               long sizeName, char *name, long sizeValue,
               char *value)
{
    int n = sizeValue;
    
    if (value != NULL && value[0] != '\0' && apefrm_getstr (mem_cnt, name) == NULL) {
        while (n > 0 && (value[--n] == ' ' || value[n] == '\0' || value[n] == '\n')) {
            value[n] = '\0';
        }
        if (n > 0)
          return libapetag_maloc_cont (mem_cnt, flags, sizeName, name, n + 1, value);
    }
    
    return 0;
}


/*
    PL: dodaje taga do istniejeacych o ustawionych wartosciach %flag% %name% i %value%
    PL: wylicza odpowiednio rozmiary przy pomocy strlen!!
    PL: wraper na %libapetag_maloc_cont%
    PL: wszystko kopiuje sobie do pamieci
    PL: musi byc juz w UTF-8 dla v2 
    PL: Nadpisuje istniejace 
 */
/** 
    \brief Add text frame

    add text frame/field to object apetag (if exist then overwrite)

    \param mem_cnt     object #apetag
    \param flags     flags stored in frame
    \param name     name of frame
    \param value     value of frame
    \return 0 - OK else check #atl_return
*/
int
apefrm_add (apetag *mem_cnt, unsigned long flags, char *name,
     char *value)
{
    apefrm_remove_real (mem_cnt, name);
    return libapetag_maloc_cont (mem_cnt, flags, strlen (name), name, strlen (value), value);
}

/*
    PL: Prosty wraperek na maloc_cont - do zapisu binarnych
*/
/**
    \brief add binary frame

    add binary frame/field to object apetag (if exist then overwrite) 

    \param mem_cnt    object #apetag
    \param flags     flags stored in frame
    \param sizeName     size of name 
    \param name     name of frame
    \param sizeValue     size of value 
    \param value    value of frame
    \return 0 - OK else check #atl_return
*/
int
apefrm_add_bin (apetag *mem_cnt, unsigned long flags,
                long sizeName, char *name, 
                long sizeValue, char *value)
{
    apefrm_remove_real (mem_cnt, name);
    return libapetag_maloc_cont (mem_cnt, flags, sizeName, name, sizeValue, value);
}

/*
    PL: jak %apefrm_add ()%  z tym ze nie nadpisuje istniejacych
*/
/**
    \brief add frame if other (the same name) no exist

    if exist "name" in ape_mem then do nothing else add frame/field to ape_mem

    \param mem_cnt    object #apetag
    \param flags     flags stored in frame
    \param name     name of frame
    \param value    value of frame
    \return 0 - OK else check #atl_return
*/
int
apefrm_add_noreplace (apetag *mem_cnt, unsigned long flags,
           char *name, char *value)
{
    if ( apefrm_getstr (mem_cnt, name) == NULL )
        return apefrm_add (mem_cnt, flags, name, value);
    
    return 0;
}

/*
    PL: wyszukuje taga o nazwie %name% i zwraca structure %struct tag%
    PL: %APE_TAG_LIB_FIRST% i %APE_TAG_LIB_NEXT% to ulatwienie dla 
    PL: przesukiwania wszystkich istniejacych tag�w 
    PL: %APE_TAG_LIB_FIRST% ustawia znacznik na pierwszy tag [0] i zwraca jego warto��
    PL: %APE_TAG_LIB_NEXT% podaje nastepny tag i zwieksza znacznik, po ostatnim funkcja zwraca %NULL%
    PL: UWAGA!!! zwraca pointer do wewnetrznej struktury 
    PL: niczego nie zmieniac i nie free()-jowac skopiowac i dopiero 
    PL: zwraca teksty w UTF-8
 */
/**
    \brief search in apetag for name and return tag

    2 special names \a APE_TAG_LIB_FIRST and \a APE_TAG_LIB_NEXT. 
    FIRST return first frame and set counter to 1 
    NEXT return ++counter frame 
\code
for ((framka = apefrm_get(ape, APE_TAG_LIB_FIRST)); framka!=NULL;) {
    do_something();
    framka = apefrm_get(ape, APE_TAG_LIB_NEXT);
}
\endcode
    return NULL if no more frame exist 

    \param mem_cnt     object #apetag
    \param name     frame name for search
    \return pointer to struct tag if name exist or NULL if don't 
    \warning don't change anything in this struct make copy and work
*/
struct tag *
apefrm_get (apetag *mem_cnt, char *name)
{
    int n;
    struct tag **mTag;

    mTag = (mem_cnt->tag);

    if (mem_cnt->countTag == 0)
        return NULL;

    if (strcmp (name, APE_TAG_LIB_FIRST) == 0) {
        mem_cnt->currentPosition = 0;
        return (mTag[mem_cnt->currentPosition++]);
    }

    if (strcmp (name, APE_TAG_LIB_NEXT) == 0) {
        if (mem_cnt->currentPosition >= mem_cnt->countTag)
            return NULL;
        return (mTag[mem_cnt->currentPosition++]);
    }

    for (n = 0; (mem_cnt->countTag) > n; n++) { 
        if (strcasecmp (mTag[n]->name, name) == 0) {
            return (mTag[n]);
        }
    }

    return NULL;
}

/*
    PL:zwraca %mem_cnt->tag[x]->value% o ile znajdzie nazwe %name% taga
    PL: prosty wraper na %apefrm_get %
    PL: UWAGA zwraca pointer z wewnetrznych struktur niczego bezposrednio nie zmieniac
    PL: i nie free()-jowac bo sie rozsypie
    PL: zwraca tekst w UTF-8
 */
/**
    \brief search in apetag for name and return string

    \param mem_cnt     object #apetag
    \param name     frame name for search
    \return pointer to value of frame if name exist or NULL if don't 
    \warning don't change that string make copy before any action
    \todo check if frame type isn't binary
*/
char *
apefrm_getstr (apetag *mem_cnt, char *name)
{
    struct tag *mTag;
    
    mTag = apefrm_get (mem_cnt, name);

    if (mTag == NULL)
        return NULL;

    return (mTag->value);
}

/*
    PL: usuwanie taga o nazwie zdefiniowanej w %name%
    PL:lub wszystkich jezeli %name%=%APE_TAG_LIB_DEL_ALL%
    PL:UWAGA mozna to napisac inaczej (sprawdzanie czy %name% OR %%special%) ale to w v1.0
 */
/**
    \brief remove frame from memory

    (real) remove frame from ape_mem.
    Check #apefrm_remove for more info

    \param mem_cnt     object #apetag
    \param name     frame name for search and remove
*/
void
apefrm_remove_real (apetag *mem_cnt, char *name)
{
    int n;
    struct tag **mTag;
    
    mTag = (mem_cnt->tag);
    
    /* Delete all */
    if (strcmp (name, APE_TAG_LIB_DEL_ALL) == 0) {
        for (n = mem_cnt->countTag-1; n >= 0; n--) { 
            free (mTag[n]->name);
            free (mTag[n]->value);
            free (mTag[n]);
            --mem_cnt->countTag;
        }
        return;
    }
    /* Delete only one */
    for (n = mem_cnt->countTag-1; n >= 0; n--) {
        if (strcasecmp (mTag[n]->name, name) == 0) {
            free (mTag[n]->name);
            free (mTag[n]->value);
            free (mTag[n]);
            mTag[n] = mTag[mem_cnt->countTag];
            --mem_cnt->countTag;
            /* !no return; search for all */
        }
    }
    
    return;
}
/*
    PL: tak jakby frejuje framke oznacza do kasacji jednak tego nie robi
    PL: mechanizm ten g�ownie jest wykorzystywany do wczytania innych tag�w
    PL: poza wczesniej zkasowanymi aby to usun�c uzyj apefrm_remove_real
*/
/**
    \brief set frame to remove

    Create fake name and empty value (and set don't save flag). 
    If you use apefrm_add_norepleace then you don't change
    this not_save_flag. 
    Only apefrm_add overwrite this. 
    [it's for id3v1 but you may using this for remove frames] 

    \param mem_cnt     object #apetag
    \param name     frame name for search and remove
*/
void
apefrm_remove (apetag *mem_cnt, char *name)
{
    int n;
    struct tag **mTag;

    apefrm_add (mem_cnt, 0 , name, "delete me");

    mTag = (mem_cnt->tag);
    
    for (n = 0; (mem_cnt->countTag) > n; n++) { 
        if (strcasecmp (mTag[n]->name, name) == 0) {
            mTag[n]->sizeValue=0;
            return;
        }
    }
    
    return;
}

/*
    PL:Wypisuje na ekran wszystko to co potrzebne do debugu
    :NON_USER:!!!
*/
/** 
    debug function print all tags exclude bin (print only size for bin)
*/
void
libapetag_print_mem_cnt (apetag *mem_cnt)
{
    int n;
    struct tag **mTag;
    
    mTag = (mem_cnt->tag);
    for (n = 0; (mem_cnt->countTag) > n; n++) {
        if ( (mTag[n]->flags & ~ITEM_TEXT) == 0 ||
            (mTag[n]->flags & ~ITEM_LINK) == 0 ) {
        printf (">apetaglib>PRINT>>F=%li SN=%li SV=%li N[%s] V[%s]\n", 
            mTag[n]->flags,
            (long) mTag[n]->sizeName, (long) mTag[n]->sizeValue, 
            mTag[n]->name, mTag[n]->value);
        } else {
        printf (">apetaglib>PRINT>>F=%li SN=%li SV=%li N[%s] V=BINARY\n",
            mTag[n]->flags,
            (long) mTag[n]->sizeName, (long) mTag[n]->sizeValue, 
            mTag[n]->name);
        }
    }
    
    return;
}

/*
    PL: alokuje pamiec dla gl�wnej struktury %struct ape_mem_cnt% 
    PL: i zeruje wszystko co trzeba 
    PL: z jakiegos powodu (mojej niewiedzy) memset nie dziala 
    PL: a w sumie dziala czyszczac troche za duzo 
*/
/**
    \brief initialise new object #apetag and return
    \return new initialised object #apetag 
*/
apetag *
apetag_init (void)
{
    apetag * mem_cnt;
    
    mem_cnt = (apetag *) malloc (sizeof (apetag));
    if (mem_cnt == NULL) {
        PRINT_ERR ("ERROR->libapetag->apetag_init:malloc\n");
        return NULL;
    }
    mem_cnt->memTagAlloc = 0;
    mem_cnt->countTag = 0;
    mem_cnt->filename = NULL;
    mem_cnt->currentPosition = 0;
    mem_cnt->tag = NULL;
    
    return mem_cnt;
}

/*
    PL: Czysci z sila wodospadu wszystko co zostalo do czyszczenia
    PL: z %struct ape_mem_cnt% wlacznie, wcze�niej to nie by�o jasne
*/
/**
    \brief free all work 
    \param mem_cnt object #apetag
*/
void
apetag_free (apetag *mem_cnt)
{
    int n;
    
    for (n = mem_cnt->countTag-1; n >= 0; n--)
    {
        free (mem_cnt->tag[n]->value);
        free (mem_cnt->tag[n]->name);
        free (mem_cnt->tag[n]);
    }
    free (mem_cnt->tag);
    free (mem_cnt);
    mem_cnt = NULL;
    
    return;

}

/**
    \brief read id3v1 and add frames

    read id3v1 and add frames to ape_mem. 
    Using #apefrm_add_norepleace

    \param mem_cnt     object #apetag
    \param fp         file pointer
    \return 0 - OK else check #atl_return
*/
int
readtag_id3v1_fp (apetag *mem_cnt, ape_file * fp)
{
    struct _id3v1Tag m;
    
    if (!is_id3v1(fp))
            return 0;  /* TODO:: 0 or no_id3v1*/
    
    ape_fseek(fp, -128, SEEK_END);
    if (sizeof (struct _id3v1Tag)!=ape_fread(&m, 1, sizeof (struct _id3v1Tag), fp)){
        PRINT_ERR( "ERROR->libapetag->readtag_id3v1_fp:fread\n");
        return ATL_FREAD;
    }

    libapetag_maloc_cont_text(mem_cnt, 0, 5, "Title", 30, m.title);
    libapetag_maloc_cont_text(mem_cnt, 0, 6, "Artist", 30, m.artist);
    libapetag_maloc_cont_text(mem_cnt, 0, 5, "Album", 30, m.album);
    libapetag_maloc_cont_text(mem_cnt, 0, 4, "Year", 4, m.year);
    if (m.comment[28] == 0 && m.comment[29] != 0) {
        char track[20];
        snprintf(track, 19, "%i", m.comment[29]);
        libapetag_maloc_cont_text(mem_cnt, 0, 5, "Track", strlen(track), track);
        libapetag_maloc_cont_text(mem_cnt, 0, 7, "Comment", 28, m.comment);
    } else {
        libapetag_maloc_cont_text(mem_cnt, 0, 7, "Comment", 30, m.comment);
    }
    libapetag_maloc_cont_text(mem_cnt, 0, 5, "Genre",
                strlen(genre_no(m.genre)), genre_no(m.genre));
    
    return 0;
}

/*
    PL: wczytuje odpowiednie fra(mk)gi do pamieci w razie koniecznosci przyciecia
    PL: dodaje "..." na koniec
    PL: TODO genre

    PL: macro COMPUTE_ID3V1_TAG
*/
#define COMPUTE_ID3V1_TAG(FramkA, TagNamE, SizE, TagValuE) \
    FramkA = apefrm_get(mem_cnt, TagNamE);    \
    if (FramkA != NULL) {             \
        memcpy (TagValuE, FramkA->value,    \
            ((FramkA->sizeValue) > SizE) ? SizE : FramkA->sizeValue );    \
        if ((FramkA->sizeValue) > SizE) {    \
        TagValuE[SizE-1]='.'; TagValuE[SizE-2]='.'; TagValuE[SizE-3]='.';    \
        }                    \
    }

int
make_id3v1_tag(apetag *mem_cnt, struct _id3v1Tag *m)
{
    struct tag * framka;
    
    if (m == NULL)
        return ATL_BADARG;
    
    memset(m, '\0', sizeof(struct _id3v1Tag));
    
    memcpy (m->magic,"TAG",3);
    COMPUTE_ID3V1_TAG(framka, "Title",  30, m->title);
    COMPUTE_ID3V1_TAG(framka, "Artist", 30, m->artist);
    COMPUTE_ID3V1_TAG(framka, "Album",  30, m->album);
    COMPUTE_ID3V1_TAG(framka, "Year",    4, m->year);
    
    if ((framka=apefrm_get(mem_cnt, "Track"))!=NULL) { 
        m->comment[29]=(unsigned char) atoi(framka->value);
        m->comment[28]='\0';
            COMPUTE_ID3V1_TAG(framka, "Comment", 28, m->comment);
    } else {
            COMPUTE_ID3V1_TAG(framka, "Comment", 30, m->comment);
    }
    
    return 0;
}

/*
    PL: silnik tego liba 
    PL: %filename% jest w tej chwili tylko dla id3v2 f..k
    PL: %ape_mem_cnt% moze byc nie zainicjalizowany ale wtedy musi byc = NULL
*/
/**
    \brief read file and add frames 

    \param mem_cnt      object #apetag
    \param filename    
    \param fp        
    \param flag        
    \return 0 - OK else check #atl_return
*/
int
apetag_read_fp(apetag *mem_cnt, ape_file * fp, char *filename, int flag)
{
    int id3v1 = 0;
    int apeTag2 = 0;
    unsigned char *buff;
    struct _apetag_footer ape_footer;
    size_t savedFilePosition, buffLength;
    
    unsigned char *end;
    unsigned long tagCount;
    unsigned char *p;
    
    savedFilePosition = ape_ftell(fp);
    
    id3v1 = is_id3v1(fp);
    
    if (mem_cnt == NULL) {
        PRINT_ERR( ">apetaglib>READ_FP>FATAL>apetag_init()\n");
        ape_fseek(fp, savedFilePosition, SEEK_SET);
        return ATL_NOINIT;
    }
    
    ape_fseek(fp, id3v1 ? -128 - (ssize_t)sizeof (ape_footer) : -(ssize_t)sizeof (ape_footer), SEEK_END);
    if (sizeof (ape_footer) != ape_fread(&ape_footer, 1, sizeof (ape_footer), fp)){
        PRINT_ERR( "ERROR->libapetag->apetag_read_fp:fread1\n");
        ape_fseek(fp, savedFilePosition, SEEK_SET);
        return ATL_FREAD;
    }
    
    if (!(flag & DONT_READ_TAG_APE) &&
        (memcmp(ape_footer.id, "APETAGEX", sizeof (ape_footer.id)) == 0))
    {
        PRINT_D9(">apetaglib>READ_FP>>%s: ver %li len %li # %li fl %lx v1=%i v2=%i ape=%i[v%i]\n",
             filename, ape2long(ape_footer.version),
             ape2long(ape_footer.length),
             ape2long(ape_footer.tagCount),
             ape2long(ape_footer.flags), 
             is_id3v1 (fp), is_id3v2 (fp), is_ape (fp), is_ape_ver (fp));
        
        apeTag2 = ape2long(ape_footer.version);
        buffLength = is_ape(fp) + 128;
        buff = (unsigned char *) malloc(buffLength);
        if (buff == NULL) {
            PRINT_ERR( "ERROR->libapetag->apetag_read_fp:malloc\n");
            return ATL_MALOC;
        }
        
        ape_fseek(fp, id3v1 ? -(long)ape2long(ape_footer.length) -
              128 : -(long)ape2long(ape_footer.length), SEEK_END);
        memset(buff, 0, buffLength);
        if (ape2long(ape_footer.length) != ape_fread(buff, 1, ape2long(ape_footer.length), fp)) {
            PRINT_ERR( "ERROR->libapetag->apetag_read_fp:fread2\n");
            ape_fseek(fp, savedFilePosition, SEEK_SET);
            free(buff);
            return ATL_FREAD;
        }
        
        tagCount = ape2long(ape_footer.tagCount);
        
        end = buff + ape2long(ape_footer.length) - sizeof (ape_footer);
        
        for (p = buff; p < end && tagCount--;) {
            /* 8 = sizeof( sizeValue+flags ) */
            unsigned long flag = ape2long(p + 4);
            unsigned long sizeValue = ape2long(p);
            unsigned long sizeName;
            char *name = (char *)p + 8;
            char *value;
            
            sizeName = strlen((char *)p + 8);
            value = (char *)p + sizeName + 8 + 1;
            if (apeTag2 == 1000 && value[sizeValue - 1] == '\0') {
                libapetag_maloc_cont(mem_cnt, flag,
                             sizeName, name,
                             sizeValue - 1, value);
            } else {
                libapetag_maloc_cont(mem_cnt, flag,
                             sizeName, name,
                             sizeValue, value);
            }
            p += (sizeName + sizeValue + 8 + 1);
        }
        
        free(buff);
    } else { /* if no ape tag */
        PRINT_D5(">apetaglib>READ_FP>>%s: v1=%i v2=%i ape=%i[v%i]\n",
             filename, is_id3v1 (fp), is_id3v2 (fp), is_ape (fp), is_ape_ver (fp));
    }
    
#ifdef ID3V2_READ
    if (!(flag & DONT_READ_TAG_ID3V2) && filename!=NULL && is_id3v2(fp)!=0) {
        readtag_id3v2(mem_cnt, filename);
    }
#endif
    if (!(flag & DONT_READ_TAG_ID3V1) && (id3v1)) {
        readtag_id3v1_fp(mem_cnt, fp);
    }
    
    ape_fseek(fp, savedFilePosition, SEEK_SET);
    return 0;
}

/*
    PL: wraper na apetag_read_fp
    PL: otwiera plik wczytuje co trzeba i zamyka 
    PL: dobre do wczytywania informacji ktore sa potrzebne pozniej bez fatygi otwierania pliku
*/
/**
    \brief read file and add frames 

    \param mem_cnt      object #apetag
    \param filename    file name
    \param flag        
    \return 0 - OK else check #atl_return
*/
int
apetag_read (apetag *mem_cnt, char *filename,int flag)
{
    ape_file *fp = NULL;
    
    if (mem_cnt==NULL) {
        PRINT_ERR(">apetaglib>READ>FATAL>apetag_init()\n");
        return ATL_NOINIT;
    }
    
    fp = ape_fopen (filename, "rb");
    if (fp == NULL)
        return ATL_FOPEN;

    apetag_read_fp (mem_cnt, fp, filename,flag);
        
    ape_fclose (fp);
        
    return 0;
}

/*
    PL: Funkcja dla qsorta ze specjalnymi wyjatkami 
    PL: uzywana w apetag_save
    :NON_USER:!!!
*/
static int
libapetag_qsort (struct tag **a, struct tag **b)
{
    char *sorting[] = { "Artist", "Year", "Album", "Track", "Title", "Genre", NULL, NULL };
    int n, m;

    if (!a || !b || !*a || !*b) {
        PRINT_ERR ("ERROR->libapetag->apetag_qsort:*a ||*b = NULL : FATAL PLEASE REPORT!!!\n");
        return 0;
    }
    for (n = 0; sorting[n] != NULL; n++) {
        if (strcasecmp ((*a)->name, sorting[n]) == 0)
            break;
    }
    if (sorting[n] == NULL)
        n += (*a)->sizeValue + 1;    /* n = max entries of sorting + size of tag */
    
    for (m = 0; sorting[m] != NULL; m++) {
        if (strcasecmp ((*b)->name, sorting[m]) == 0)
            break;
    }
    if (sorting[m] == NULL)
        m += (*b)->sizeValue + 1;    /* m = max entries */

    if (n == m)
        return 0;
    if (n > m)
        return 1;
    else
        return -1;
}

#ifdef USE_CHSIZE
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
/* on winblows we don't have truncate (and ftruncate) but have chsize() */
void
truncate (char *filename, size_t fileSize)
{
    int handle;

    handle = open (filename, O_RDWR | O_CREAT,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (handle != -1) {
        if (chsize (handle, fileSize) != 0) {
            PRINT_ERR ("Error truncatng file\n");
        }
        close (handle);
    }

}

#endif

/*
    PL: domyslne %flag% = APE_TAG_V2 + SAVE_NEW_OLD_APE_TAG
*/
/**
    \brief save apetag to file 

    \param filename file name 
    \param mem_cnt  object #apetag
    \param flag     flags for read/save 
    \return 0 - OK else check #atl_return
    \warning for ape tag v 1 you must add frames in iso-1 
    for v 2 this must be in utf-8
    \todo PL: v9 sprawdzac flagi w footer i na tej podstawie zmieniac skipBytes 
    bez domniemywania ze v2 ma zawsze oba

*/
int
apetag_save (char *filename, apetag *mem_cnt, int flag)
{
    ape_file *fp;
    struct _id3v1Tag id3v1_tag;
    int id3v1;
    int apeTag, saveApe2;
    int tagCount = 0;
    int realCountTag = 0;
    struct _apetag_footer ape_footer;
    long skipBytes;
    unsigned char *buff, *p;
    struct tag **mTag;
    size_t tagSSize = 32;
    int n;
    char temp[4];
    
    if (mem_cnt==NULL) {
        PRINT_ERR("ERROR->apetaglib>apetag_save::apetag_init()\n");
        return ATL_NOINIT;
    }

    fp = ape_fopen (filename, "rb+");
    if (fp == NULL) {
        PRINT_ERR ( "ERROR->apetaglib->apetag_save::fopen (r+)\n");
        return ATL_FOPEN;
    }
    
    skipBytes = 0;
    id3v1 = is_id3v1 (fp);
    apeTag = is_ape (fp);
    saveApe2 = !(flag & APE_TAG_V1); // (flag & APE_TAG_V2) ? 1 : (flag & APE_TAG_V1);
    
    if (id3v1) {
        ape_fseek (fp, -128, SEEK_END);
        ape_fread (&id3v1_tag, 1, sizeof (struct _id3v1Tag), fp);
        skipBytes += id3v1;
    }
    skipBytes += apeTag;

    if (!(flag & SAVE_NEW_APE_TAG)) {
        apetag_read_fp (mem_cnt, fp, filename, flag);
    }
    
    mTag = (mem_cnt->tag);
    qsort( mTag , mem_cnt->countTag , sizeof(struct tag *),
        (int (*)(const void *,const void *))libapetag_qsort);
    
    for (n = 0; (mem_cnt->countTag) > n; n++) {
        if (mTag[n]->sizeValue != 0) {
            tagSSize += ((long) mTag[n]->sizeName + (long) mTag[n]->sizeValue);
            tagSSize += 4 + 4 + 1 + (saveApe2 ? 0 : 1);    // flag & sizeValue & \0
            realCountTag++; // count not deleted tag (exl. not real)
        }
    }
    if (!!(flag & SAVE_CREATE_ID3V1_TAG )) {
        make_id3v1_tag(mem_cnt, &id3v1_tag);
        tagSSize += 128;
    }
    //PRINT_D4 (">apetaglib>SAVE>>: size %li %i %i %i\n", tagSSize,
    //    mem_cnt->countTag, flag, saveApe2);
    buff = (unsigned char *) malloc (tagSSize + (saveApe2 ? 32 : 0));
    p = buff;
    
    if (buff == NULL) {
        PRINT_ERR ("ERROR->libapetag->apetag_save::malloc");
        return ATL_MALOC;
    }
    memset (ape_footer.id, 0, sizeof (ape_footer));
    memcpy (ape_footer.id, "APETAGEX", sizeof (ape_footer.id));
    long2ape (ape_footer.flags, 0l);
    if (!!(flag & SAVE_CREATE_ID3V1_TAG ))
        long2ape (ape_footer.length, tagSSize-128);
    else
        long2ape (ape_footer.length, tagSSize);
    //long2ape(ape_footer.tagCount, mem_cnt->countTag);
    long2ape(ape_footer.tagCount, realCountTag);
    long2ape (ape_footer.version, (saveApe2 ? 2000 : 1000));
    if (saveApe2) {
        long2ape (ape_footer.flags, HEADER_THIS_IS + HEADER_IS + FOOTER_IS);
        memcpy (p, ape_footer.id, sizeof (ape_footer));
        p += sizeof (ape_footer);
    }
    
    mTag = (mem_cnt->tag);
    for (n = 0; (mem_cnt->countTag) > n; n++) {
        if (saveApe2) {
            long2ape (temp, mTag[n]->sizeValue);
        } else {
            /* TODO:convert UTF8 to ASCII mTag[n]->value */
            long2ape (temp, (mTag[n]->sizeValue) + 1);
        }
        
        if (mTag[n]->sizeValue != 0) {
            memcpy (p, temp, 4);
            p += 4;
            long2ape (temp, (saveApe2!=0) ? mTag[n]->flags : 0l );
            memcpy (p, temp, 4);
            p += 4;
            
            memcpy (p, mTag[n]->name, mTag[n]->sizeName);
            p += mTag[n]->sizeName;
            memcpy (p, "\0", 1);
            p++;
            memcpy (p, mTag[n]->value, mTag[n]->sizeValue);
            p += mTag[n]->sizeValue;
            
            if (!saveApe2) {
                memcpy (p, "\0", 1);
                p++;
            }
            tagCount++;
        }
    } /* for */
    
    if (saveApe2)
        long2ape (ape_footer.flags, FOOTER_THIS_IS + FOOTER_IS + HEADER_IS);
        
    memcpy (p, ape_footer.id, sizeof (ape_footer));
    p += sizeof (ape_footer);
    
    if (!!(flag & SAVE_CREATE_ID3V1_TAG )) {
         memcpy (p, &id3v1_tag , sizeof (struct _id3v1Tag));
    }
    
    /* write tag to file and truncate */
    if (!(flag & SAVE_FAKE_SAVE)) {
        size_t fileSize;
        size_t newFileSize;
        size_t writedBytes;
        
        ape_fseek (fp, 0, SEEK_END);
        fileSize = ape_ftell (fp);
        ape_fseek (fp, fileSize - skipBytes, SEEK_SET);
        if (tagCount != 0) {
            newFileSize = (fileSize - skipBytes + tagSSize + (saveApe2 ? 32 : 0));
            writedBytes = ape_fwrite (buff, 1, tagSSize + (saveApe2 ? 32 : 0), fp);
            if (writedBytes != tagSSize + (saveApe2 ? 32 : 0)) {
                PRINT_ERR ("FATAL_ERROR->libapetag->apetag_save::fwrite [data lost]");
                ape_fclose (fp);
                free (buff);
                return ATL_FWRITE;
            }
            ape_fseek (fp, newFileSize, SEEK_SET);
            PRINT_D4 (">apetaglib>SAVE>> write:%i == tag:%i file: %i->%i\n",
                writedBytes, tagSSize + (saveApe2 ? 32 : 0), fileSize, newFileSize);
        } else {
            newFileSize = (fileSize - skipBytes);
        }
        ape_fflush (fp);
        ape_fclose (fp);
        /* ftruncate don't work */ 
        truncate (filename, newFileSize);
    } else { /* !!SAVE_FAKE_SAVE */
        libapetag_print_mem_cnt (mem_cnt);
    }
    free (buff);

    return 0;
}
