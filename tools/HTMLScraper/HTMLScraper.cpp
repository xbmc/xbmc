/*
 * HTMLScraper for XBox Media Center
 * Copyright (c) 2004 Team XBMC
 *
 * Ver 1.0 15 Nov 2004	Chris Barnett (Forza)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "HTMLScraper.h"
#include "RegExp.h"

char *ConvertHTMLToAnsi(const char *szHTML)
{
  if (!szHTML)
    return NULL;

  int i=0; 
  int len = (int)strlen(szHTML);
	if (len == 0)
		return NULL;

  int iAnsiPos=0;
	char *szAnsi = (char *)malloc(len*2*sizeof(char));

	while (i < len)
	{
		char kar=szHTML[i];
		if (kar=='&')
		{
			if (szHTML[i+1]=='#')
			{
				int ipos=0;
				char szDigit[12];
				i+=2;
				if (szHTML[i+2]=='x') i++;

				while ( ipos < 12 && szHTML[i] && isdigit(szHTML[i])) 
				{
					szDigit[ipos]=szHTML[i];
					szDigit[ipos+1]=0;
					ipos++;
					i++;
				}

				// is it a hex or a decimal string?
				if (szHTML[i+2]=='x')
					szAnsi[iAnsiPos++] = (char)(strtol(szDigit, NULL, 16) & 0xFF);
				else
					szAnsi[iAnsiPos++] = (char)(strtol(szDigit, NULL, 10) & 0xFF);
				i++;
			}
			else
			{
				i++;
				int ipos=0;
				char szKey[112];
				while (szHTML[i] && szHTML[i] != ';' && ipos < 12)
				{
					szKey[ipos]=(unsigned char)szHTML[i];
					szKey[ipos+1]=0;
					ipos++;
					i++;
				}
				i++;
				if (strcmp(szKey,"amp")==0)          szAnsi[iAnsiPos++] = '&';
				else if (strcmp(szKey,"quot")==0)    szAnsi[iAnsiPos++] = (char)0x22;
				else if (strcmp(szKey,"frasl")==0)   szAnsi[iAnsiPos++] = (char)0x2F;
				else if (strcmp(szKey,"lt")==0)      szAnsi[iAnsiPos++] = (char)0x3C;
				else if (strcmp(szKey,"gt")==0)      szAnsi[iAnsiPos++] = (char)0x3E;
				else if (strcmp(szKey,"trade")==0)   szAnsi[iAnsiPos++] = (char)0x99;
				else if (strcmp(szKey,"nbsp")==0)    szAnsi[iAnsiPos++] = ' ';
				else if (strcmp(szKey,"iexcl")==0)   szAnsi[iAnsiPos++] = (char)0xA1;
				else if (strcmp(szKey,"cent")==0)    szAnsi[iAnsiPos++] = (char)0xA2;
				else if (strcmp(szKey,"pound")==0)   szAnsi[iAnsiPos++] = (char)0xA3;
				else if (strcmp(szKey,"curren")==0)  szAnsi[iAnsiPos++] = (char)0xA4;
				else if (strcmp(szKey,"yen")==0)     szAnsi[iAnsiPos++] = (char)0xA5;
				else if (strcmp(szKey,"brvbar")==0)  szAnsi[iAnsiPos++] = (char)0xA6;
				else if (strcmp(szKey,"sect")==0)    szAnsi[iAnsiPos++] = (char)0xA7;
				else if (strcmp(szKey,"uml")==0)     szAnsi[iAnsiPos++] = (char)0xA8;
				else if (strcmp(szKey,"copy")==0)    szAnsi[iAnsiPos++] = (char)0xA9;
				else if (strcmp(szKey,"ordf")==0)    szAnsi[iAnsiPos++] = (char)0xAA;
				else if (strcmp(szKey,"laquo")==0)   szAnsi[iAnsiPos++] = (char)0xAB;
				else if (strcmp(szKey,"not")==0)     szAnsi[iAnsiPos++] = (char)0xAC;
				else if (strcmp(szKey,"shy")==0)     szAnsi[iAnsiPos++] = (char)0xAD;
				else if (strcmp(szKey,"reg")==0)     szAnsi[iAnsiPos++] = (char)0xAE;
				else if (strcmp(szKey,"macr")==0)    szAnsi[iAnsiPos++] = (char)0xAF;
				else if (strcmp(szKey,"deg")==0)     szAnsi[iAnsiPos++] = (char)0xB0;
				else if (strcmp(szKey,"plusmn")==0)  szAnsi[iAnsiPos++] = (char)0xB1;
				else if (strcmp(szKey,"sup2")==0)    szAnsi[iAnsiPos++] = (char)0xB2;
				else if (strcmp(szKey,"sup3")==0)    szAnsi[iAnsiPos++] = (char)0xB3;
				else if (strcmp(szKey,"acute")==0)   szAnsi[iAnsiPos++] = (char)0xB4;
				else if (strcmp(szKey,"micro")==0)   szAnsi[iAnsiPos++] = (char)0xB5;
				else if (strcmp(szKey,"para")==0)    szAnsi[iAnsiPos++] = (char)0xB6;
				else if (strcmp(szKey,"middot")==0)  szAnsi[iAnsiPos++] = (char)0xB7;
				else if (strcmp(szKey,"cedil")==0)   szAnsi[iAnsiPos++] = (char)0xB8;
				else if (strcmp(szKey,"sup1")==0)    szAnsi[iAnsiPos++] = (char)0xB9;
				else if (strcmp(szKey,"ordm")==0)    szAnsi[iAnsiPos++] = (char)0xBA;
				else if (strcmp(szKey,"raquo")==0)   szAnsi[iAnsiPos++] = (char)0xBB;
				else if (strcmp(szKey,"frac14")==0)  szAnsi[iAnsiPos++] = (char)0xBC;
				else if (strcmp(szKey,"frac12")==0)  szAnsi[iAnsiPos++] = (char)0xBD;
				else if (strcmp(szKey,"frac34")==0)  szAnsi[iAnsiPos++] = (char)0xBE;
				else if (strcmp(szKey,"iquest")==0)  szAnsi[iAnsiPos++] = (char)0xBF;
				else if (strcmp(szKey,"Agrave")==0)  szAnsi[iAnsiPos++] = (char)0xC0;
				else if (strcmp(szKey,"Aacute")==0)  szAnsi[iAnsiPos++] = (char)0xC1;
				else if (strcmp(szKey,"Acirc")==0)   szAnsi[iAnsiPos++] = (char)0xC2;
				else if (strcmp(szKey,"Atilde")==0)  szAnsi[iAnsiPos++] = (char)0xC3;
				else if (strcmp(szKey,"Auml")==0)    szAnsi[iAnsiPos++] = (char)0xC4;
				else if (strcmp(szKey,"Aring")==0)   szAnsi[iAnsiPos++] = (char)0xC5;
				else if (strcmp(szKey,"AElig")==0)   szAnsi[iAnsiPos++] = (char)0xC6;
				else if (strcmp(szKey,"Ccedil")==0)  szAnsi[iAnsiPos++] = (char)0xC7;
				else if (strcmp(szKey,"Egrave")==0)  szAnsi[iAnsiPos++] = (char)0xC8;
				else if (strcmp(szKey,"Eacute")==0)  szAnsi[iAnsiPos++] = (char)0xC9;
				else if (strcmp(szKey,"Ecirc")==0)   szAnsi[iAnsiPos++] = (char)0xCA;
				else if (strcmp(szKey,"Euml")==0)    szAnsi[iAnsiPos++] = (char)0xCB;
				else if (strcmp(szKey,"Igrave")==0)  szAnsi[iAnsiPos++] = (char)0xCC;
				else if (strcmp(szKey,"Iacute")==0)  szAnsi[iAnsiPos++] = (char)0xCD;
				else if (strcmp(szKey,"Icirc")==0)   szAnsi[iAnsiPos++] = (char)0xCE;
				else if (strcmp(szKey,"Iuml")==0)    szAnsi[iAnsiPos++] = (char)0xCF;
				else if (strcmp(szKey,"ETH")==0)     szAnsi[iAnsiPos++] = (char)0xD0;
				else if (strcmp(szKey,"Ntilde")==0)  szAnsi[iAnsiPos++] = (char)0xD1;
				else if (strcmp(szKey,"Ograve")==0)  szAnsi[iAnsiPos++] = (char)0xD2;
				else if (strcmp(szKey,"Oacute")==0)  szAnsi[iAnsiPos++] = (char)0xD3;
				else if (strcmp(szKey,"Ocirc")==0)   szAnsi[iAnsiPos++] = (char)0xD4;
				else if (strcmp(szKey,"Otilde")==0)  szAnsi[iAnsiPos++] = (char)0xD5;
				else if (strcmp(szKey,"Ouml")==0)    szAnsi[iAnsiPos++] = (char)0xD6;
				else if (strcmp(szKey,"times")==0)   szAnsi[iAnsiPos++] = (char)0xD7;
				else if (strcmp(szKey,"Oslash")==0)  szAnsi[iAnsiPos++] = (char)0xD8;
				else if (strcmp(szKey,"Ugrave")==0)  szAnsi[iAnsiPos++] = (char)0xD9;
				else if (strcmp(szKey,"Uacute")==0)  szAnsi[iAnsiPos++] = (char)0xDA;
				else if (strcmp(szKey,"Ucirc")==0)   szAnsi[iAnsiPos++] = (char)0xDB;
				else if (strcmp(szKey,"Uuml")==0)    szAnsi[iAnsiPos++] = (char)0xDC;
				else if (strcmp(szKey,"Yacute")==0)  szAnsi[iAnsiPos++] = (char)0xDD;
				else if (strcmp(szKey,"THORN")==0)   szAnsi[iAnsiPos++] = (char)0xDE;
				else if (strcmp(szKey,"szlig")==0)   szAnsi[iAnsiPos++] = (char)0xDF;
				else if (strcmp(szKey,"agrave")==0)  szAnsi[iAnsiPos++] = (char)0xE0;
				else if (strcmp(szKey,"aacute")==0)  szAnsi[iAnsiPos++] = (char)0xE1;
				else if (strcmp(szKey,"acirc")==0)   szAnsi[iAnsiPos++] = (char)0xE2;
				else if (strcmp(szKey,"atilde")==0)  szAnsi[iAnsiPos++] = (char)0xE3;
				else if (strcmp(szKey,"auml")==0)    szAnsi[iAnsiPos++] = (char)0xE4;
				else if (strcmp(szKey,"aring")==0)   szAnsi[iAnsiPos++] = (char)0xE5;
				else if (strcmp(szKey,"aelig")==0)   szAnsi[iAnsiPos++] = (char)0xE6;
				else if (strcmp(szKey,"ccedil")==0)  szAnsi[iAnsiPos++] = (char)0xE7;
				else if (strcmp(szKey,"egrave")==0)  szAnsi[iAnsiPos++] = (char)0xE8;
				else if (strcmp(szKey,"eacute")==0)  szAnsi[iAnsiPos++] = (char)0xE9;
				else if (strcmp(szKey,"ecirc")==0)   szAnsi[iAnsiPos++] = (char)0xEA;
				else if (strcmp(szKey,"euml")==0)    szAnsi[iAnsiPos++] = (char)0xEB;
				else if (strcmp(szKey,"igrave")==0)  szAnsi[iAnsiPos++] = (char)0xEC;
				else if (strcmp(szKey,"iacute")==0)  szAnsi[iAnsiPos++] = (char)0xED;
				else if (strcmp(szKey,"icirc")==0)   szAnsi[iAnsiPos++] = (char)0xEE;
				else if (strcmp(szKey,"iuml")==0)    szAnsi[iAnsiPos++] = (char)0xEF;
				else if (strcmp(szKey,"eth")==0)     szAnsi[iAnsiPos++] = (char)0xF0;
				else if (strcmp(szKey,"ntilde")==0)  szAnsi[iAnsiPos++] = (char)0xF1;
				else if (strcmp(szKey,"ograve")==0)  szAnsi[iAnsiPos++] = (char)0xF2;
				else if (strcmp(szKey,"oacute")==0)  szAnsi[iAnsiPos++] = (char)0xF3;
				else if (strcmp(szKey,"ocirc")==0)   szAnsi[iAnsiPos++] = (char)0xF4;
				else if (strcmp(szKey,"otilde")==0)  szAnsi[iAnsiPos++] = (char)0xF5;
				else if (strcmp(szKey,"ouml")==0)    szAnsi[iAnsiPos++] = (char)0xF6;
				else if (strcmp(szKey,"divide")==0)  szAnsi[iAnsiPos++] = (char)0xF7;
				else if (strcmp(szKey,"oslash")==0)  szAnsi[iAnsiPos++] = (char)0xF8;
				else if (strcmp(szKey,"ugrave")==0)  szAnsi[iAnsiPos++] = (char)0xF9;
				else if (strcmp(szKey,"uacute")==0)  szAnsi[iAnsiPos++] = (char)0xFA;
				else if (strcmp(szKey,"ucirc")==0)   szAnsi[iAnsiPos++] = (char)0xFB;
				else if (strcmp(szKey,"uuml")==0)    szAnsi[iAnsiPos++] = (char)0xFC;
				else if (strcmp(szKey,"yacute")==0)  szAnsi[iAnsiPos++] = (char)0xFD;
				else if (strcmp(szKey,"thorn")==0)   szAnsi[iAnsiPos++] = (char)0xFE;
				else if (strcmp(szKey,"yuml")==0)    szAnsi[iAnsiPos++] = (char)0xFF;
        else
        {
          // its not an ampersand code, so just copy the contents
          szAnsi[iAnsiPos++] = '&';
          for (unsigned int iLen=0; iLen<strlen(szKey); iLen++)
            szAnsi[iAnsiPos++] = (unsigned char)szKey[iLen];
        }
			}
		}
		else
		{
			szAnsi[iAnsiPos++] = kar;
			i++;
		}
	}
	szAnsi[iAnsiPos++]=0;
  return szAnsi;
}

char *RemoveWhiteSpace(char *string)
{
  if (!string) return "";
  size_t pos = strlen(string)-1;
  while ((string[pos] == ' ' || string[pos] == '\n') && string[pos] && pos)
    string[pos--] = '\0';
  while ((*string == ' ' || *string == '\n') && *string != '\0')
    string++;
  return string;
}

void AddTag(char *szXML, const char *url, const char *title, const char *year = NULL)
{
  char *ansiTitle = ConvertHTMLToAnsi(title);
  if (!ansiTitle || !url || !szXML)
    return;
  // ok - now construct tag
  if (year)
    sprintf(szXML, "%s\t<movie>\n\t\t<title>%s (%s)</title>\n\t\t<url>%s</url>\n\t</movie>\n", szXML, RemoveWhiteSpace(ansiTitle), year, url);
  else
    sprintf(szXML, "%s\t<movie>\n\t\t<title>%s</title>\n\t\t<url>%s</url>\n\t</movie>\n", szXML, RemoveWhiteSpace(ansiTitle), url);
  free(ansiTitle);
}

void AddDetailsTag(char *szXML, const char *tag, const char *value)
{
  char *ansiValue = ConvertHTMLToAnsi(value);
  if (!szXML || !ansiValue || !tag)
    return;
  sprintf(szXML, "%s\t<%s>%s</%s>\n", szXML, tag, RemoveWhiteSpace(ansiValue), tag);
  free(ansiValue);
}

void ParseDetails(char *szXML, const char *szHTML, const char *tag, const char *regexp)
{
  CRegExp reg;
  reg.RegComp(regexp);
  if (reg.RegFind(szHTML))
  {
    char *detail = reg.GetReplaceString("\\1");
    AddDetailsTag(szXML, tag, detail);
    if (detail)
      free(detail);
  }
}

extern "C" 
{
	__declspec(dllexport) int IMDbGetSearchResults(char *szXML, const char *szHTML, const char *szFromURL)
	{
		if (!szHTML || !strlen(szHTML))
			return 0;

    strcpy(szXML, "<results>\n");

    CRegExp reg;
    reg.RegComp("\"title\">.*Genre:.*Plot");
    int iFind = reg.RegFind(szHTML);
    if (iFind >= 0)
    { // found only 1 search result - fill in our XML file
      // get the title
      reg.RegComp("> *([^<]*)<");
      reg.RegFind(szHTML + iFind);
      char *pTitle = reg.GetReplaceString("\\1");
      if (pTitle)
      {
        AddTag(szXML, szFromURL, pTitle);
        strcat(szXML, "</results>\n");
        free(pTitle);
        return (int)strlen(szXML);
      }
    }
    reg.RegComp("( Titles| Matches\\))</b>");
    iFind = reg.RegFind(szHTML);
    if (iFind < 0)
    { // No matches found
      return 0;
    }

    // ok - now, let's get the info - we only need the bit within the </table> tags
    char *pStartOfMovieList = (char *)szHTML + iFind;
		char *pEndOfMovieList=strstr(pStartOfMovieList,"</table>");
		if (!pEndOfMovieList)
		{
			pEndOfMovieList=pStartOfMovieList+strlen(pStartOfMovieList);
			printf("Unable to locate end of movie list.\n");
		}
		*pEndOfMovieList=0;
    reg.RegComp("<a href=\"(/title/[t0-9]*/)[^>]*>([^<]*)</a> *\\(([0-9]*)");
		while(1)
		{
     // got to transform this:
      // <a href="/title/tt0313443/">Out of Time</a>(2003/I)</tr>
      // into this:
      // /title/tt0313443/
      // Out of Time (2003)
      iFind = reg.RegFind(pStartOfMovieList);
      if (iFind >= 0)
      {
        // extract information and add tag
        char *pURL = reg.GetReplaceString("http://akas.imdb.com\\1");
        char *pTitle = reg.GetReplaceString("\\2");
        char *pYear = reg.GetReplaceString("\\3");
        AddTag(szXML, pURL, pTitle, pYear);
        if (pURL) free(pURL);
        if (pTitle) free(pTitle);
        if (pYear) free(pYear);
        // increment start of movie list
        pStartOfMovieList = strstr(pStartOfMovieList, "</a>");
        if (!pStartOfMovieList)
          break;
        pStartOfMovieList += 5;
      }
      else
        break;
    }
    strcat(szXML, "</results>\n");
		return (int)strlen(szXML);
	}

	__declspec(dllexport) int IMDbGetDetails(char *szXML, const char *szHTML, const char *szPlotHTML)
	{
    if (!szXML || !szHTML || !szPlotHTML)
      return 0;

		strcpy(szXML, "<details>\n");

    CRegExp reg;

    // Year, Director, Top 250, MPAA certification, 
    ParseDetails(szXML, szHTML, "year", "/Sections/Years/([0-9]*)"); 
		char *pDirectedBy=strstr(szHTML,"Directed by");
    ParseDetails(szXML, pDirectedBy, "director", "<a [^>]*>([^<]*)</a>");
    ParseDetails(szXML, szHTML, "top250", "top 250: #([^<]*)<");
    ParseDetails(szXML, szHTML, "mpaa", "MPAA</a>:</b>([^<]*)<");
    ParseDetails(szXML, szHTML, "tagline", "Tagline:</b>([^<]*)<");
    ParseDetails(szXML, szHTML, "runtime", "Runtime:</b>([^<]*)<");
    ParseDetails(szXML, szHTML, "thumb", "<img border=\"0\" alt=\"cover\" src=\"([^\"]*)\"");

    // Credits
		char *pCredits=strstr(szHTML,"Writing credits");
    reg.RegComp("<a [^\n]*\n");
    if (reg.RegFind(pCredits) >= 0)
    { // found credits - there can be multiple ones, so separate with a /
      char allcredits[1024];
      *allcredits = 0;
      char *credits = reg.GetReplaceString("\\0");
      if (credits)
      {
        char *scancredits = credits;
        reg.RegComp("<a [^>]*>([^<]*)<");
        while (true)
        {
          reg.RegFind(scancredits);
          char *credit = reg.GetReplaceString("\\1");
          if (credit)
          {
            strcat(allcredits, credit);
            strcat(allcredits, " / ");
            free(credit);
          }
          else
            break;
          scancredits = strstr(scancredits, "</a>");
          if (!scancredits)
            break;
          scancredits += 5;
        }
        free(credits);
        // remove last " / "
        if (strlen(allcredits) >= 3)
          allcredits[strlen(allcredits)-3] = 0;
        AddDetailsTag(szXML, "credits", allcredits);
      }
    }
    
    // Rating and votes
    char* pRating=strstr(szHTML,"User Rating:</b>");
		if (pRating) // and votes
		{
      reg.RegComp("<b>([.0-9]*)/10</b> \\(([,0-9]*) vote");
      if (reg.RegFind(pRating) >= 0)
      {
        char *rating = reg.GetReplaceString("\\1");
        AddDetailsTag(szXML, "rating", rating);
        if (rating) free(rating);
        char *votes = reg.GetReplaceString("\\2");
        AddDetailsTag(szXML, "votes", votes);
        if (votes) free(votes);
      }
		}

    // Genre
		char* pGenre=strstr(szHTML,"Genre:");
    // looking for:
    // <a href="/Sections/Genres/Action/">Action</a> / <a href="/Sections/Genres/Comedy/">Comedy</a> / <a href="/Sections/Genres/Thriller/">Thriller</a> <a href="/rg/title-tease/keywords/title/tt0189981/keywords">(more)</a>
    reg.RegComp("Genre:([^\\(]*)\\(more\\)");
    if (reg.RegFind(szHTML) >= 0)
    {
      char *genres = reg.GetReplaceString("\\1");
      if (genres)
      {
        char allgenres[1024];
        *allgenres = 0;
        // ok - now we need to break up the strings...
        reg.RegComp("<a href[^>]*>([^<]*)</a>");
        char *start = genres;
        while (reg.RegFind(start) >= 0)
        {
          char *genre = reg.GetReplaceString("\\1");
          if (genre)
          {
            sprintf(allgenres, "%s%s / ", allgenres, genre);
            free(genre);
          }
          else
            break;
          start = strstr(start, "</a>");
          if (!start)
            break;
          start += 5;
        }
        // remove last " / "
        if (strlen(allgenres) >= 3)
          allgenres[strlen(allgenres)-3] = 0;
        AddDetailsTag(szXML, "genre", allgenres);
        free(genres);
      }
    }

    // Cast
		char* pCast=strstr(szHTML,"first billed only: </b></td></tr>");
		char* pCred=strstr(szHTML,"redited cast:"); // Complete credited cast or Credited cast
    if(!pCast) 
		{
			pCast=pCred;
		}

		if(pCast) 
		{
			char *pStart = strstr(pCast, " <tr>");
			if(pStart)
			{
        char cast[10000];
        strcpy(cast, "Cast overview:\n");

        // looking for: <td valign="top"><a href="/name/nm0004875/">Taye Diggs</a></td>
        // <tr><td valign="top"><a href="/name/nm0004875/">Taye Diggs</a></td><td valign="top" nowrap="1"> .... </td><td valign="top">The Bandleader</td></tr>
        // OR:
        // <tr><td valign="top"><a href="/name/nm0814639/">Pat Soper</a></td><td valign="top" nowrap="1"></td><td valign="top"></td></tr>

        reg.RegComp("<tr><td valign=\"top\"><a href[^>]*>([^<]*)</a></td><td valign=\"top\" nowrap=\"1\">[. ]*</td><td valign=\"top\">([^<]*)</td></tr>");
        while (reg.RegFind(pStart) >= 0)
        {
          // found - decipher
          char *actor = reg.GetReplaceString("\\1");
          if (actor)
          {
            strcat(cast, actor);
            char *role = reg.GetReplaceString("\\2");
            if (role)
            {
              strcat(cast, " as ");
              strcat(cast, role);
              free(role);
            }
            strcat(cast, "\n");
            free(actor);
          }
          pStart = strstr(pStart, "</tr>");
          if (!pStart)
            break;
          pStart += 5;
        }
        AddDetailsTag(szXML, "cast", cast);
			}
				
		}

    // Plot
		char* plot=strstr(szHTML,"<a href=\"plotsummary");
	  if (plot)
	  {
      char *plotstart=strstr(szPlotHTML,"<p class=\"plotpar\">");
		  if (plotstart)
		  {
			  plotstart += strlen("<p class=\"plotpar\">");
			  char *plotend=strstr(plotstart,"</p>");
			  if (plotend) *plotend=0;

			  // Remove any tags that may appear in the text
			  bool inTag = false;
			  int iPlot = 0;
			  char* plot = (char *)malloc(strlen(plotstart)+1);
			  for (unsigned int i = 0; i < strlen(plotstart); i++)
			  {
				  if (plotstart[i] == '<')
					  inTag = true;
				  else if (plotstart[i] == '>')
					  inTag = false;
				  else if (!inTag)
					  plot[iPlot++] = plotstart[i];
			  }
			  plot[iPlot] = '\0';

        AddDetailsTag(szXML, "plot", plot);
        free(plot);
		  }
	  }

    // plot outline or summary
    reg.RegComp("Plot (Outline|Summary):</b>([^<]*)<");
    if (reg.RegFind(szHTML) >= 0)
    {
      char *plotoutline = reg.GetReplaceString("\\2");
      AddDetailsTag(szXML, "outline", plotoutline);
      if (!plot)  // no plot block - copy across the outline
        AddDetailsTag(szXML, "plot", plotoutline);
      if (plotoutline)
        free(plotoutline);
    }

    strcat(szXML, "</details>\n");
    return (int)strlen(szXML);
	};
};