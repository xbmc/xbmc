

#include "linksboks.h"

extern "C" {
#include "links-hacked/links.h"
#include "links-hacked/options_hooks.h"
}

#ifdef XBMC_LINKS_DLL
#pragma comment (lib, "lib/xbox_dx8.lib" )
#pragma comment (lib, "lib/winsockx.lib" )
#endif

#ifdef XBOX_USE_SECTIONS
// Restore code and data sections to normal.
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()
#endif

LPDIRECT3D8			g_pLBD3D;			/* Provided d3d object */
LPDIRECT3DDEVICE8	g_pLBd3dDevice;		/* Provided d3d device object */
LinksBoksOption		**g_LinksBoksOptions = NULL;
LinksBoksProtocol	**g_LinksBoksProtocols = NULL;
int (*g_LinksBoksExecFunction)(LinksBoksWindow *pLB, unsigned char *cmdline, unsigned char *filepath, int fg) = NULL;
#if defined(XBOX_USE_XFONT) || defined(XBOX_USE_FREETYPE)
LinksBoksExtFont *(*g_LinksBoksGetExtFontFunction) (unsigned char *fontname, int fonttype) = NULL;
#endif
BOOL (*g_LinksBoksMsgBoxHandlerFunction)(void *dlg, unsigned char *title, int nblabels, unsigned char *labels[], int nbbuttons, unsigned char *buttons[]) = NULL;
bool				g_bLinksBoksCoreIsOn = false;

extern LinksBoksWindow		*g_NewLinksBoksWindow = NULL;

// Yet Another Ugly Thing. Do not do this at home.
extern "C" int __LinksBoks_InitCore(unsigned char *homedir);
extern "C" int __LinksBoks_NewWindow(void);
extern "C" int __LinksBoks_FrameMove(void);
extern "C" void __LinksBoks_Terminate(void);

extern "C" struct window *get_root_or_first_window(struct terminal *term);
extern "C" void open_in_new_tab(struct terminal *term, unsigned char *exe, unsigned char *param);
extern "C" void init_font_cache(int bytes);
extern "C" void destroy_font_cache();

// The custom FVF, which describes the custom vertex structure.
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)

#ifdef XBMC_LINKS_DLL
// externals so we can load as a dll
extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ);
#else
#define d3dSetTextureStageState m_pd3dDevice->SetTextureStageState
#define d3dSetRenderState m_pd3dDevice->SetRenderState
#endif

struct CUSTOMVERTEX
{
    D3DXVECTOR3 position; // The position.
    D3DCOLOR    color;    // The color.
    FLOAT       tu, tv;   // The texture coordinates.
};





/******************* OPTIONS *********************/



BOOL LinksBoks_GetOptionBool(const char *key)
{
	return options_get_bool((unsigned char *)key);
}

INT LinksBoks_GetOptionInt(const char *key)
{
	return options_get_int((unsigned char *)key);
}

unsigned char *LinksBoks_GetOptionString(const char *key)
{
	return options_get((unsigned char *)key);
}

void LinksBoks_SetOptionBool(const char *key, BOOL value)
{
	options_set_bool((unsigned char *)key, (int)value);
}

void LinksBoks_SetOptionInt(const char *key, INT value)
{
	options_set_int((unsigned char *)key, (int)value);
}

void LinksBoks_SetOptionString(const char *key, unsigned char *value)
{
	options_set((unsigned char *)key, value);
}


extern "C" void options_copy_item(void *in, void *out);

OPTIONS_HOOK(xbox_hook)
{
	struct options *in_opt = (struct options *)current;
	struct options *out_opt = (struct options *)changed;

	for(int i = 0; g_LinksBoksOptions[i]; i++)
	{
		if(g_LinksBoksOptions[i]->m_sName == NULL)
			continue;
		if(!strcmp((const char *)in_opt->name, (const char *)g_LinksBoksOptions[i]->m_sName))
		{
			int ret;
			int iOld = 0, iNew = 0;
			switch(g_LinksBoksOptions[i]->m_iType)
			{
			case LINKSBOKS_OPTION_BOOL:
			case LINKSBOKS_OPTION_INT:
				if(in_opt->value && sscanf((const char *)in_opt->value,"%d",&iOld)
						&& out_opt->value && sscanf((const char *)out_opt->value,"%d",&iNew))
					ret = g_LinksBoksOptions[i]->OnBeforeChange(ses, iOld, iNew);

				break;
			case LINKSBOKS_OPTION_STRING:
				ret = g_LinksBoksOptions[i]->OnBeforeChange(ses, in_opt->value, out_opt->value);
				break;
			}

			if(ret)
			{
				options_copy_item(changed, current);
				g_LinksBoksOptions[i]->OnAfterChange(ses);
			}

			return ret;
		}
	}

	return -1;
}

extern "C" void register_options_xbox(void)
{
	if(g_LinksBoksOptions == NULL)
		return;

	for(int i = 0; g_LinksBoksOptions[i]; i++)
		g_LinksBoksOptions[i]->Register();
}

LinksBoksOption::LinksBoksOption(const char *name, const char *caption, int type, int depth, unsigned char *default_value)
{
	m_sName = name;
	m_sCaption = caption;
	m_iType = type;
	m_iDepth = depth;
	m_sDefaultValue = default_value;
}

LinksBoksOption::LinksBoksOption(const char *name, const char *caption, int type, int depth, int default_value)
{
	m_sName = name;
	m_sCaption = caption;
	m_iType = type;
	m_iDepth = depth;
	m_iDefaultValue = default_value;
}

BOOL LinksBoksOption::OnBeforeChange(void *session, unsigned char *oldvalue, unsigned char *newvalue)
{
	return TRUE;
}

BOOL LinksBoksOption::OnBeforeChange(void *session, int oldvalue, int newvalue)
{
	return TRUE;
}

VOID LinksBoksOption::OnAfterChange(void *session)
{
}

VOID LinksBoksOption::MsgBox(void *session, unsigned char *title, unsigned char *msg)
{
	struct session *ses = (struct session *)session;
	msg_box(ses->term, NULL, title, AL_CENTER, msg, NULL, 1, TXT(T_OK), NULL, B_ENTER | B_ESC);
}

VOID LinksBoksOption::Register()
{
	switch(m_iType)
	{
	case LINKSBOKS_OPTION_GROUP:
		register_option(NULL, (unsigned char *)m_sCaption, OPT_TYPE_CHAR, NULL, m_iDepth);
		break;
	case LINKSBOKS_OPTION_BOOL:
		register_option((unsigned char *)m_sName, (unsigned char *)m_sCaption, OPT_TYPE_BOOL, (unsigned char *)"", m_iDepth);
		options_set_bool((unsigned char *)m_sName, m_iDefaultValue);
		break;
	case LINKSBOKS_OPTION_INT:
		register_option((unsigned char *)m_sName, (unsigned char *)m_sCaption, OPT_TYPE_INT, (unsigned char *)"", m_iDepth);
		options_set_int((unsigned char *)m_sName, m_iDefaultValue);
		break;
	case LINKSBOKS_OPTION_STRING:
		register_option_char((unsigned char *)m_sName, (unsigned char *)m_sCaption, (unsigned char *)m_sDefaultValue, m_iDepth);
		break;
	}
	options_set_hook((unsigned char *)m_sName, xbox_hook);
}






/******************* CUSTOM PROTOCOLS *********************/





VOID LinksBoksExternalProtocol::Register()
{
	register_external_protocol(m_sName, m_iPort, NULL, xbox_external_protocols_func, m_bFreeSyntax, m_bNeedSlashes, m_bNeedSlashAfterHost);
}

VOID LinksBoksInternalProtocol::Register()
{
	register_external_protocol(m_sName, m_iPort, xbox_internal_protocols_func, NULL, m_bFreeSyntax, m_bNeedSlashes, m_bNeedSlashAfterHost);
}



extern "C" void xbox_internal_protocols_func(struct connection *connection)
{
	for(int i = 0; g_LinksBoksProtocols[i]; i++)
	{
		if(g_LinksBoksProtocols[i]->m_sName == NULL)
			continue;
		if(!strcmp((const char *)get_protocol_name(connection->url), (const char *)g_LinksBoksProtocols[i]->m_sName))
		{
			setcstate(connection, g_LinksBoksProtocols[i]->OnCall(connection->url, connection));
			abort_connection(connection);
			return;
		}
	}

	setcstate(connection, S_BAD_URL);
	abort_connection(connection);
}

extern "C" void xbox_external_protocols_func(struct session *session, unsigned char *url)
{
	for(int i = 0; g_LinksBoksProtocols[i]; i++)
	{
		if(g_LinksBoksProtocols[i]->m_sName == NULL)
			continue;
		if(!strcmp((const char *)get_protocol_name(url), (const char *)g_LinksBoksProtocols[i]->m_sName))
		{
			g_LinksBoksProtocols[i]->OnCall(url, session);
			return;
		}
	}
}

VOID LinksBoksExternalProtocol::MsgBox(void *session, unsigned char *title, unsigned char *msg)
{
	struct session *ses = (struct session *)session;
	msg_box(ses->term, NULL, title, AL_CENTER, msg, NULL, 1, TXT(T_OK), NULL, B_ENTER | B_ESC);
}

int LinksBoksInternalProtocol::SendResponse(void *connection, unsigned char *content_type, unsigned char *data, int data_size)
{
	struct connection *c = (struct connection *)connection;
	struct cache_entry *e;
	char *head = new char[19 + strlen((const char *)content_type)];
	if (get_cache_entry(c->url, &e))
		return -1;

	if(e->head)
		mem_free(e->head);

	snprintf(head, 18 + strlen((const char *)content_type), "\r\nContent-Type: %s\r\n", content_type);
	e->head = (unsigned char *)head;
	c->cache = e;

	add_fragment(e, 0, data, data_size);
	truncate_entry(e, data_size, 1);

	c->cache->incomplete = 0;

	return 0;
}




/********** URL LISTS (BOOKMARKS/HISTORY...) ***********/

struct bookmark_list{
	/* common for all lists */
	struct bookmark_list *next;
	struct bookmark_list *prev;
	unsigned char type;  
	int depth;
	void *fotr;

	/* bookmark specific */
	unsigned char *title;
	unsigned char *url;
};
extern "C" struct list bookmarks;

bool LinksBoksBookmarks::GetRoot(void)
{
    root = &bookmarks;
    l = &bookmarks;

    return (root != NULL);
}

bool LinksBoksBookmarks::GetNext(void)
{
    l = ((struct bookmark_list *)l)->next;

    return (l != root);
}

unsigned char *LinksBoksBookmarks::GetTitle(void)
{
    return ((struct bookmark_list *)l)->title;
}

unsigned char *LinksBoksBookmarks::GetURL(void)
{
    return ((struct bookmark_list *)l)->url;
}

int LinksBoksBookmarks::GetType(void)
{
    return ((struct bookmark_list *)l)->type;
}

int LinksBoksBookmarks::GetDepth(void)
{
    return ((struct bookmark_list *)l)->depth;
}

long LinksBoksBookmarks::GetLastVisit(void)
{
    return 0;
}

/***/

extern "C" struct global_history global_history;

bool LinksBoksHistory::GetRoot(void)
{
    root = &global_history.items;
    l = &global_history.items;

    return (root != NULL);
}

bool LinksBoksHistory::GetNext(void)
{
    l = ((struct global_history_item *)l)->next;

    return (l != root);
}

unsigned char *LinksBoksHistory::GetTitle(void)
{
    return ((struct global_history_item *)l)->title;
}

unsigned char *LinksBoksHistory::GetURL(void)
{
    return ((struct global_history_item *)l)->url;
}

int LinksBoksHistory::GetType(void)
{
    return 0;
}

int LinksBoksHistory::GetDepth(void)
{
    return 0;
}

long LinksBoksHistory::GetLastVisit(void)
{
    return ((struct global_history_item *)l)->last_visit;
}


ILinksBoksURLList *LinksBoks_GetURLList(int type)
{
    switch(type)
    {
    case LINKSBOKS_URLLIST_BOOKMARKS:
        return new LinksBoksBookmarks();
    case LINKSBOKS_URLLIST_HISTORY:
        return new LinksBoksHistory();
    default:
        return NULL;
    }
}

/***/

extern "C" struct list_description bookmark_ld;
extern "C" int can_write_bookmarks;
extern "C" void reinit_bookmarks(void);
extern "C" unsigned char *convert_to_entity_string(unsigned char *str);

bool LinksBoksBookmarksWriter::Begin()
{
    m_iDepth = 0;
    m_pFile = NULL;
    FILE *f = NULL;

#ifndef XBMC_LINKS_DLL
    // XBMC fopen wrapper doesn't set errno so can_write_bookmarks is set to 0 @bookmarks.c:584 
    if (!can_write_bookmarks)
        return false;
#endif

    ct=get_translation_table(
                              bookmark_ld.codepage,
                              get_cp_index(options_get((unsigned char *)"bookmarks_codepage")));

    f=fopen((const char *)options_get((unsigned char *)"bookmarks_file"),"w+");

    printf("[=LinksBoks=] LinksBoksBookmarksWriter::Begin - fopen(%s)=%d\n", (const char *)options_get((unsigned char *)"bookmarks_file"), f);


    if (!f)
        return false;

    fprintf(f,
"<HTML>\n"
"<HEAD>\n"
"<!-- This is an automatically generated file.\n"
"It will be read and overwritten.\n"
"Do Not Edit! -->\n"
"<TITLE>Links bookmarks</TITLE>\n"
"</HEAD>\n"
"<H1>Links bookmarks</H1>\n\n"
"<DL><P>\n");

    m_iDepth = 0;
    m_pFile = (void *)f;

    return true;
}

bool LinksBoksBookmarksWriter::AppendBookmark(unsigned char *title, unsigned char *url, int depth, bool isfolder)
{
    int a;
    FILE *f = (FILE *)m_pFile;

    for(a = depth; a < m_iDepth; a++)
        fprintf(f,"</DL>\n");

    m_iDepth = depth;
	
	if(isfolder)
	{
		unsigned char *txt, *txt1;
        txt  = convert_string(ct,title,strlen((const char *)title),NULL);
        txt1 = convert_to_entity_string(txt);
		fprintf(f,"    <DT><H3>%s</H3>\n<DL>\n",txt1);
		mem_free(txt);
		mem_free(txt1);
		depth++;
	}
	else
	{
		unsigned char *txt1, *txt2, *txt3;
		txt1 = convert_string(ct,title,strlen((const char *)title),NULL);
		txt2 = convert_string(ct,url,strlen((const char *)url),NULL);
        txt3 = convert_to_entity_string(txt1);
		fprintf(f,"    <DT><A HREF=\"%s\">%s</A>\n",txt2,txt3);
		mem_free(txt1);
		mem_free(txt2);
		mem_free(txt3);
	}

    return true;
}

bool LinksBoksBookmarksWriter::Save()
{
    int a;
    FILE *f = (FILE *)m_pFile;

    for (a = 0; a < m_iDepth; a++)
            fprintf(f,"</DL>\n");

    fprintf(f,
"</DL><P>\n"
"</HTML>\n");

    fclose(f);

    bookmark_ld.modified=0;

    reinit_bookmarks();

    return true;
}

/* gets str, converts all < = > & to appropriate entity 
 * returns allocated string with result
 */
unsigned char *LinksBoksBookmarksWriter::convert_to_entity_string(unsigned char *str)
{
	unsigned char *dst, *p, *q;
	int size;
	
	for (size=1,p=str;*p;size+=*p=='&'?5:*p=='<'||*p=='>'||*p=='='?4:1,p++);

	dst=(unsigned char *)mem_alloc(size*sizeof(unsigned char));
	if (!dst) internal((unsigned char *)"Cannot allocate memory.\n");
	
	for (p=str,q=dst;*p;p++,q++)
	{
		switch(*p)
		{
			case '<':
			case '>':
			q[0]='&',q[1]=*p=='<'?'l':'g',q[2]='t',q[3]=';',q+=3;
			break;

			case '=':
			q[0]='&',q[1]='e',q[2]='q',q[3]=';',q+=3;
			break;

			case '&':
			q[0]='&',q[1]='a',q[2]='m',q[3]='p',q[4]=';',q+=4;
			break;

			default:
			*q=*p;
			break;
		}
	}
	*q=0;
	return dst;
}


ILinksBoksBookmarksWriter *LinksBoks_GetBookmarksWriter(void)
{
    return new LinksBoksBookmarksWriter();
}





/******************* MAIN ROUTINES *********************/




extern "C" {
  
int LinksBoks_InitCore(unsigned char *homedir, LinksBoksOption *options[], LinksBoksProtocol *protocols[])
{
	if(g_bLinksBoksCoreIsOn)
	  return -1;

    printf("[=LinksBoks=] Starting Core\n");

#ifdef XBOX_USE_SECTIONS
	XLoadSection("LBKS_RD");
	XLoadSection("LBKS_RW");
	XLoadSection("LBKSDATA");
	XLoadSection("LNKSBOKS");
	//XLoadSection("BFONTS"); done when it's needed anyway
#endif

    printf("[=LinksBoks=] Registering protocols and custom options\n");
    
	g_LinksBoksOptions = options;
	g_LinksBoksProtocols = protocols;

    /* We can register the protocols right now */
	if(g_LinksBoksProtocols != NULL)
	{
		for(int i = 0; g_LinksBoksProtocols[i]; i++)
			g_LinksBoksProtocols[i]->Register();
	}

    printf("[=LinksBoks=] Starting main engines NOW!\n");
    
	int ret;
	if((ret = __LinksBoks_InitCore(stracpy(homedir))))
	{
        printf("[=LinksBoks=] Links InitCore failed, you're in trouble...\n");
    
		// bummer, bail out!
		LinksBoks_Terminate(TRUE);
		g_bLinksBoksCoreIsOn = false;
	}
	else
	{
        printf("[=LinksBoks=] Ok, Links' core is now operational!\n");
    
		g_bLinksBoksCoreIsOn = true;
	}

	return ret;
}

LinksBoksWindow *LinksBoks_CreateWindow(LPDIRECT3DDEVICE8 pd3dDevice, LinksBoksViewPort viewport)
{
	g_pLBd3dDevice = pd3dDevice;
	g_NewLinksBoksWindow = new LinksBoksWindow(viewport);

    printf("[=LinksBoks=] Creating window\n");

    if(__LinksBoks_NewWindow())
	{
        printf("[=LinksBoks=] Window creation FAILED! Expect trouble...\n");
    
		delete g_NewLinksBoksWindow;
		g_NewLinksBoksWindow = NULL;
		return NULL;
	}
	else
	{
        printf("[=LinksBoks=] Window created successfully!\n");
    
        printf("[=LinksBoks=] Registering protocols and custom options\n");
    
		LinksBoksWindow *pLB = g_NewLinksBoksWindow;
		g_NewLinksBoksWindow = NULL;
		return pLB;
	}
}

int LinksBoks_FrameMove()
{
	if(!g_bLinksBoksCoreIsOn)
	    return -1;

	return __LinksBoks_FrameMove();
}

VOID LinksBoks_EmptyCaches(void)
{
	shrink_memory(SH_FREE_ALL);
	destroy_font_cache();
	init_font_cache(options_get_int((unsigned char *)"cache_fonts_size"));
}

VOID LinksBoks_Terminate(BOOL bFreeXBESections)
{
    printf("[=LinksBoks=] Terminating core!\n");
    
	__LinksBoks_Terminate();

#ifdef LEAK_DEBUG_LIST
	check_memory_leaks();
#endif

#ifdef XBOX_USE_SECTIONS
	if(bFreeXBESections)
	{
		XFreeSection("LBKS_RD");
		XFreeSection("LBKS_RW");
		XFreeSection("LBKSDATA");
		XFreeSection("LNKSBOKS");
		XFreeSection("BFONTS");
	}
#endif

	g_bLinksBoksCoreIsOn = false;
}

VOID LinksBoks_FreezeCore(void)
{
    printf("[=LinksBoks=] Freezing core!\n");
    
	LinksBoks_EmptyCaches();

#ifdef XBOX_USE_SECTIONS
	XFreeSection("LBKS_RD");
	//XFreeSection("LBKS_RW");
	XFreeSection("LBKSDATA");
	XFreeSection("LNKSBOKS");
	XFreeSection("BFONTS");
#endif

	g_bLinksBoksCoreIsOn = false;
}

VOID LinksBoks_UnfreezeCore(void)
{
    printf("[=LinksBoks=] Unfreezing core!\n");

#ifdef XBOX_USE_SECTIONS
    XLoadSection("LBKS_RD");
	//XLoadSection("LBKS_RW");
	XLoadSection("LBKSDATA");
	XLoadSection("LNKSBOKS");
	XLoadSection("BFONTS"); // the position in memory should remain the same...
#endif

	g_bLinksBoksCoreIsOn = true;
}


/******************* TIMERS *********************/




int LinksBoks_RegisterNewTimer(long t, void (*func)(void *), void *data)
{
	return install_timer(t, func, data);
}





/******************* EXTERNAL VIEWERS *********************/




/* Replacement for function in terminal.c */
extern "C" void exec_on_terminal(struct terminal *term, unsigned char *path, unsigned char *file, int fg)
{
	LinksBoksWindow *pLB = (LinksBoksWindow *)term->dev->driver_data;

	if(g_LinksBoksExecFunction && pLB)
	{
		int ret = g_LinksBoksExecFunction(pLB, path, file, fg);

		if(ret >= 0)
			unlink((const char *)file);
	}
}

VOID LinksBoks_SetExecFunction(int (*exec_function)(LinksBoksWindow *pLB, unsigned char *cmdline, unsigned char *filepath, int fg))
{
	g_LinksBoksExecFunction = exec_function;
}



/******************* EXTERNAL "XFONT" FONTS *********************/

#if defined(XBOX_USE_XFONT) || defined(XBOX_USE_FREETYPE)
VOID LinksBoks_SetExtFontCallbackFunction(LinksBoksExtFont *(*extfont_function)(unsigned char *fontname, int fonttype))
{
	g_LinksBoksGetExtFontFunction = extfont_function;
}
#endif


/******************* EXTERNAL MESSAGEBOX HANDLER *********************/
VOID LinksBoks_SetMessageBoxFunction(BOOL (*msgbox_function)(void *dlg, unsigned char *title, int nblabels, unsigned char *labels[], int nbbuttons, unsigned char *buttons[]))
{
	g_LinksBoksMsgBoxHandlerFunction = msgbox_function;
}

int LinksBoks_MsgBox(terminal *term, dialog *dlg, memory_list *ml)
{
  unsigned char **buttons;
  unsigned char **srclabels = (unsigned char **)dlg->udata;
  unsigned char **labels;
	struct dialog_data *dd;
	struct dialog_item *d;
	int n = 0;
  bool ret;
	
  if(!g_LinksBoksMsgBoxHandlerFunction)
    return 0;

  for (d = dlg->items; d->type != D_END; d++)
    n++;

  if (!(buttons = (unsigned char **)mem_alloc((n + 1) * sizeof(unsigned char *)))) return FALSE;
	if (!(dd = (dialog_data *)mem_calloc(sizeof(struct dialog_data) + sizeof(struct dialog_item_data) * n))) return FALSE;
	dd->dlg = dlg;
	dd->n = n;
	dd->ml = ml;

  for(int i = 0; i < n; i++)
  {
    buttons[i] = _(dlg->items[i].text, term);
  }
  buttons[i] = NULL;

  for(int j = 0; srclabels[j]; j++);
  if (!(labels = (unsigned char **)mem_alloc((j + 1) * sizeof(unsigned char *)))) return FALSE;
  for(int k = 0; k < j; k++)
  {
    labels[k] = _(srclabels[k], term);
  }
  labels[k] = NULL;

  ret = g_LinksBoksMsgBoxHandlerFunction((void *)dd, _(dlg->title, term), j, labels, dd->n, buttons);
  mem_free(buttons);
  mem_free(labels);

  return (ret == FALSE) ? 0 : 1;
}

VOID LinksBoks_ValidateMessageBox(void *dlg, int choice)
{
  dialog_data *dd = (dialog_data *)dlg;
  void (*fn)(void *);
  
  fn = (void (*)(void *))dd->dlg->items[choice].udata;

  if(fn)
    fn(dd->dlg->udata2);

  freeml(dd->ml);
  mem_free(dd);
}




} /* extern "C" */

/***************************************************************************
 ***************************************************************************
 ***************************** LinksBoksWindow *****************************
 ***************************************************************************
 ***************************************************************************/




/******************* INIT/DEINIT *********************/


LinksBoksWindow::LinksBoksWindow(LinksBoksViewPort viewport)
{
	m_ViewPort = viewport;
	m_pd3dDevice = g_pLBd3dDevice;
	m_bWantFlip = FALSE;
  m_iCurrentLink = -1;
}

int LinksBoksWindow::Initialize(void *grdev)
{
	m_grdev = grdev;

	return CreateBuffers();
}


VOID LinksBoksWindow::Close(void)
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;
	dev->keyboard_handler(dev, KBD_CLOSE, 0);
}

VOID LinksBoksWindow::Terminate(void)
{
	m_pd3dDevice = NULL;
	DestroyBuffers();
  m_iCurrentLink = -1;
}

int LinksBoksWindow::Freeze(void)
{
	return DestroyBuffers();
}

int LinksBoksWindow::Unfreeze(void)
{
	int ret = CreateBuffers();
	if(ret)
	  return ret;

	struct graphics_device *dev = (struct graphics_device *)m_grdev;
	/* The resize handler schedules a full redraw ;) */
	dev->resize_handler(dev);

	m_bWantFlip = FALSE;
	RegisterFlip(0, 0, GetViewPortWidth(), GetViewPortHeight());

	return 0;
}


/******************* RENDERING *********************/



LPDIRECT3DSURFACE8 LinksBoksWindow::GetSurface(void)
{
	return m_pdSurface;
}

int LinksBoksWindow::FlipSurface(void)
{
	if( !m_bWantFlip )
		return 1;

	m_pd3dDevice->CopyRects( m_pdBkBuffer, NULL, 0, m_pdSurface, NULL );

	m_bWantFlip = FALSE;

	return 0;
}

static void xbox_flip_surface (void *pointer)
{
	LinksBoksWindow *pLB = (LinksBoksWindow *)pointer;

	pLB->FlipSurface();
}

int LinksBoksWindow::CreateBuffers(void)
{
    SetRect( &m_FlipRegion, 0, 0, m_ViewPort.width, m_ViewPort.height );

	/* Create the surfaces and initiate rendering */
    d3dSetRenderState( D3DRS_ZENABLE,          TRUE );
//    d3dSetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
//    d3dSetRenderState( D3DRS_ALPHATESTENABLE,  FALSE );
	d3dSetRenderState( D3DRS_ALPHABLENDENABLE,   FALSE );
	d3dSetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
	d3dSetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );

	d3dSetRenderState( D3DRS_CULLMODE,         D3DCULL_NONE );
    d3dSetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
    d3dSetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    d3dSetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1 );
    d3dSetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    d3dSetTextureStageState( 0, D3DTSS_MIPFILTER, D3DTEXF_NONE );
    d3dSetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
    d3dSetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );

    // Note: The hardware requires CLAMP for linear textures
    d3dSetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
    d3dSetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );

    if( FAILED( m_pd3dDevice->CreateRenderTarget( m_ViewPort.width, m_ViewPort.height, D3DFMT_LIN_X8R8G8B8, 0, 0, &m_pdSurface ) ) )
		return 1;
    if( FAILED( m_pd3dDevice->CreateRenderTarget( m_ViewPort.width, m_ViewPort.height, D3DFMT_LIN_X8R8G8B8, 0, 0, &m_pdBkBuffer ) ) )
		return 2;

	// Erase the 2 rendering surfaces
	if( FAILED( CreatePrimitive( -m_ViewPort.margin_left, -m_ViewPort.margin_top,
			m_ViewPort.width, m_ViewPort.height, 0x00000000 ) ) )
		return 3;
	
	// The first...
	RenderPrimitive( m_pdSurface );

	// And the second
	RenderPrimitive( m_pdBkBuffer );

	d3dSetRenderState( D3DRS_ALPHABLENDENABLE,   TRUE );

	m_pd3dDevice->SetVertexShader( D3DFVF_CUSTOMVERTEX );

	return 0;
}

int LinksBoksWindow::DestroyBuffers(void)
{
	while(m_pdSurface->Release());
	m_pdSurface = NULL;
	while(m_pdBkBuffer->Release());
	m_pdBkBuffer = NULL;
	unregister_bottom_half (xbox_flip_surface, this);

	return 0;
}


VOID LinksBoksWindow::RegisterFlip( int x, int y, int w, int h )
{
	if (x < 0 || y < 0 || w < 1 || h < 1)
		return;

	w = x + w - 1;
	h = y + h - 1;

	if( m_bWantFlip )
	{
		if (m_FlipRegion.left > x)  m_FlipRegion.left = (LONG)x;
		if (m_FlipRegion.top > y)  m_FlipRegion.top = (LONG)y;
		if (m_FlipRegion.right < w)  m_FlipRegion.right = (LONG)w;
		if (m_FlipRegion.bottom < h)  m_FlipRegion.bottom = (LONG)h;
	}
	else
	{
		m_FlipRegion.left = x;
		m_FlipRegion.top = y;
		m_FlipRegion.right = w;
		m_FlipRegion.bottom = h;

		m_bWantFlip = TRUE;

		register_bottom_half (xbox_flip_surface, this);
	}
}

VOID LinksBoksWindow::Blit( LPDIRECT3DSURFACE8 pSurface, int x, int y, int w, int h )
{
	RECT srcRect, dstRect;
	RECT screenRect = { 0, 0, GetViewPortWidth(), GetViewPortHeight() };

	// Don't forget to consider the clipping region
	SetRect( &srcRect, x, y, x+w, y+h );
	IntersectRect( &dstRect, &srcRect, &m_ClipArea );
	IntersectRect( &dstRect, &dstRect, &screenRect );

	if( !IsRectEmpty( &dstRect ) && dstRect.right - dstRect.left > 1 && dstRect.bottom - dstRect.top > 1 )
	{
		POINT destPoint;
		destPoint.x = dstRect.left + m_ViewPort.margin_left;
		destPoint.y = dstRect.top + m_ViewPort.margin_top;
		OffsetRect( &dstRect, -x, -y );
		m_pd3dDevice->CopyRects( pSurface, &dstRect, 1, m_pdBkBuffer, &destPoint );
	}
}


HRESULT LinksBoksWindow::CreatePrimitive( int x, int y, int w, int h, int color )
{
	LPDIRECT3DVERTEXBUFFER8 pVB = NULL; // Buffer to hold vertices
	float left, top, right, bottom;

	// Create the vertex buffer. Here we are allocating enough memory
  // (from the default pool) to hold all our 4 custom vertices. We also
    // specify the FVF, so the vertex buffer knows what data it contains.
  if( FAILED( m_pd3dDevice->CreateVertexBuffer( 4 * sizeof(CUSTOMVERTEX),
                                                  0, 
                                                  D3DFVF_CUSTOMVERTEX,
                                                  D3DPOOL_DEFAULT, &pVB ) ) )
        return E_FAIL;

	
	CUSTOMVERTEX* pVertices = NULL;

	if( FAILED( pVB->Lock( 0, 4 * sizeof(CUSTOMVERTEX), (BYTE**)&pVertices, 0L ) ) )
		return E_FAIL;


	left	= ((float)(m_ViewPort.margin_left + x) / (float)m_ViewPort.width) * 2.0f - 1.0f;
	right	= ((float)(m_ViewPort.margin_left + x + w) / (float)m_ViewPort.width) * 2.0f - 1.0f;
	top		= 1.0f - ((float)(m_ViewPort.margin_top + y) / (float)m_ViewPort.height) * 2.0f;
	bottom	= 1.0f - ((float)(m_ViewPort.margin_top + y + h) / (float)m_ViewPort.height) * 2.0f;


	pVertices[0].position=D3DXVECTOR3( left,  top, 0.5f );
	pVertices[0].color=color;
	pVertices[0].tu=0.0;
	pVertices[0].tv=0.0;
	pVertices[1].position=D3DXVECTOR3( right,  top, 0.5f );
	pVertices[1].color=color;
	pVertices[1].tu=(FLOAT)w;
	pVertices[1].tv=0.0;
	pVertices[2].position=D3DXVECTOR3( left,  bottom, 0.5f );
	pVertices[2].color=color;
	pVertices[2].tu=0.0;
	pVertices[2].tv=(FLOAT)h;
	pVertices[3].position=D3DXVECTOR3(  right,  bottom, 0.5f );
	pVertices[3].color=color;
	pVertices[3].tu=(FLOAT)w;
	pVertices[3].tv=(FLOAT)h;
	
	pVB->Unlock();

	m_pd3dDevice->SetStreamSource( 0, pVB, sizeof(CUSTOMVERTEX) );
    m_pd3dDevice->SetVertexShader( D3DFVF_CUSTOMVERTEX );

	pVB->Release();

	return S_OK;
}

VOID LinksBoksWindow::RenderPrimitive( LPDIRECT3DSURFACE8 pTargetSurface )
{
	LPDIRECT3DSURFACE8 pRenderTarget, pZBuffer;
    m_pd3dDevice->GetRenderTarget( &pRenderTarget );
    m_pd3dDevice->GetDepthStencilSurface( &pZBuffer );

	m_pd3dDevice->SetRenderTarget( pTargetSurface, NULL );

    m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );

    m_pd3dDevice->SetRenderTarget( pRenderTarget, pZBuffer );

	if( pRenderTarget ) pRenderTarget->Release();
    if( pZBuffer )      pZBuffer->Release();
}

VOID LinksBoksWindow::FillArea(int x1, int y1, int x2, int y2, long color)
{
	RECT srcRect, dstRect;
	RECT screenRect = { 0, 0, GetViewPortWidth(), GetViewPortHeight() };

    D3DLOCKED_RECT d3dlr;
    m_pdBkBuffer->LockRect( &d3dlr, 0, 0 );
	DWORD * pDst = (DWORD *)d3dlr.pBits;
	int DPitch = d3dlr.Pitch/4;

	// Don't forget to consider the clipping region
	SetRect( &srcRect, x1, y1, x2, y2 );
	IntersectRect( &dstRect, &srcRect, &m_ClipArea );
	IntersectRect( &dstRect, &dstRect, &screenRect );

	if( !IsRectEmpty( &dstRect ) )
		for (int y=dstRect.top; y<dstRect.bottom; ++y)
			for (int x=dstRect.left; x<dstRect.right; ++x)
				pDst[(y+m_ViewPort.margin_top)*DPitch + (x+m_ViewPort.margin_left)] = color;

	m_pdBkBuffer->UnlockRect( );

	RegisterFlip( x1, y1, x2 - x1, y2 - y1 );
}

VOID LinksBoksWindow::DrawHLine(int x1, int y, int x2, long color)
{
	RECT srcRect, dstRect;

	D3DLOCKED_RECT d3dlr;
    m_pdBkBuffer->LockRect( &d3dlr, 0, 0 );
	DWORD * pDst = (DWORD *)d3dlr.pBits;
	int DPitch = d3dlr.Pitch/4;

	// Don't forget to consider the clipping region
	SetRect( &srcRect, x1, y, x2, y+1 );
	IntersectRect( &dstRect, &srcRect, &m_ClipArea );

	if( !IsRectEmpty( &dstRect ) )
		for (int x=dstRect.left; x<dstRect.right; ++x)
			pDst[(y+m_ViewPort.margin_top)*DPitch + (x+m_ViewPort.margin_left)] = color;

	m_pdBkBuffer->UnlockRect( );

	RegisterFlip( x1, y, x2 - x1, 1 );
}

VOID LinksBoksWindow::DrawVLine(int x, int y1, int y2, long color)
{
	RECT srcRect, dstRect;

    D3DLOCKED_RECT d3dlr;
    m_pdBkBuffer->LockRect( &d3dlr, 0, 0 );
	DWORD * pDst = (DWORD *)d3dlr.pBits;
	int DPitch = d3dlr.Pitch/4;

	// Don't forget to consider the clipping region
	SetRect( &srcRect, x, y1, x+1, y2 );
	IntersectRect( &dstRect, &srcRect, &m_ClipArea );

	if( !IsRectEmpty( &dstRect ) )
		for (int y=dstRect.top; y<dstRect.bottom; ++y)
			pDst[(y+m_ViewPort.margin_top)*DPitch + (x+m_ViewPort.margin_left)] = color;

	m_pdBkBuffer->UnlockRect( );

	RegisterFlip( x, y1, 1, y2 - y1 );
}

VOID LinksBoksWindow::ScrollBackBuffer(int x1, int y1, int x2, int y2, int offx, int offy)
{
	LPDIRECT3DSURFACE8 pSurface = NULL;
	RECT srcRect;
	POINT destPoint = { 0, 0 };

	m_pd3dDevice->CreateImageSurface( x2 - x1, y2 - y1, D3DFMT_LIN_A8R8G8B8, &pSurface );

	SetRect( &srcRect, x1, y1, x2, y2 );
	OffsetRect( &srcRect, m_ViewPort.margin_left, m_ViewPort.margin_top );

	m_pd3dDevice->CopyRects( m_pdBkBuffer, &srcRect, 1, pSurface, &destPoint );

	OffsetRect( &srcRect, -m_ViewPort.margin_left + offx, -m_ViewPort.margin_top + offy );
	Blit( pSurface, srcRect.left, srcRect.top, srcRect.right - srcRect.left, srcRect.bottom - srcRect.top);

	pSurface->Release( );
}

VOID LinksBoksWindow::SetClipArea(int x1, int y1, int x2, int y2)
{
	SetRect( &m_ClipArea, x1, y1, x2, y2 );
}

int LinksBoksWindow::GetViewPortWidth(void)
{
	return m_ViewPort.width - m_ViewPort.margin_left - m_ViewPort.margin_right;
}

int LinksBoksWindow::GetViewPortHeight(void)
{
	return m_ViewPort.height - m_ViewPort.margin_top - m_ViewPort.margin_bottom;
}

void LinksBoksWindow::ResizeWindow(LinksBoksViewPort viewport)
{
	m_ViewPort = viewport;
	struct graphics_device *dev = (struct graphics_device *)m_grdev;

	dev->size.x2 = GetViewPortWidth();
	dev->size.y2 = GetViewPortHeight();

	dev->resize_handler(dev);

	// Black-fill the edges of the front buffer to clean up garbage
	m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	CreatePrimitive( -m_ViewPort.margin_left, -m_ViewPort.margin_top, m_ViewPort.width, m_ViewPort.margin_top, 0x000000ff );
	RenderPrimitive( m_pdBkBuffer );
	CreatePrimitive( -m_ViewPort.margin_left, -m_ViewPort.margin_top, m_ViewPort.margin_left, m_ViewPort.height, 0x000000ff );
	RenderPrimitive( m_pdBkBuffer );
	CreatePrimitive( -m_ViewPort.margin_left, GetViewPortHeight(), m_ViewPort.width, m_ViewPort.margin_bottom, 0x000000ff );
	RenderPrimitive( m_pdBkBuffer );
	CreatePrimitive( GetViewPortWidth(), -m_ViewPort.margin_top, m_ViewPort.margin_right, m_ViewPort.height, 0x000000ff );
	RenderPrimitive( m_pdBkBuffer );
	m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
}

VOID LinksBoksWindow::RedrawWindow()
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;
	dev->resize_handler(dev);
}




/******************* KEYBOARD/MOUSE ACTIONS *********************/




VOID LinksBoksWindow::KeyboardAction(int key, int flags)
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;
	dev->keyboard_handler(dev, key, flags);
}

VOID LinksBoksWindow::MouseAction(int x, int y, int buttons)
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;
	dev->mouse_handler(dev, x, y, buttons);
}

// helper function for the ones below
int GetSessionLinks(struct graphics_device *dev, struct session **ses, struct link **links, int *current, int *total)
{
  if (!dev) return 0;
	struct terminal *term = (struct terminal *)dev->user_data;
	struct window *win = get_root_or_first_window(term);
  if (!win || !win->data) return 0;
  if (win->type == 0) return 2; // hack - no idea if it'll work in general!
	if (ses) *ses = (struct session *)win->data;
	return get_links(*ses, links, current, total);
}

int GetFirstLink(struct session *ses, struct link *links, int total)
{
  int gotolink = -1;
  // find if there's any links on the page, and go to the first.
  for (int i=0; i < total; i++)
  {
    if ((links[i].type == L_LINK && links[i].where) || links[i].type != L_LINK)
    {
      gotolink = i;
      break;
    }
  }
  if (gotolink < 0) return gotolink; // no links at all
  g_goto_link(ses, gotolink);
  return gotolink;
}

VOID LinksBoksWindow::SetFocus(BOOL bFocus)
{
	if (bFocus)
	{
		struct session *ses;
		struct link *links;
		int total, current;
		if (GetSessionLinks((struct graphics_device *)m_grdev, &ses, &links, &current, &total) == 1)
		{
			if (m_iCurrentLink >= 0)
				g_goto_link(ses, m_iCurrentLink);
		}
	}
	if (!bFocus)
	{
		struct session *ses;
		struct link *links;
		int total, current;
		if (GetSessionLinks((struct graphics_device *)m_grdev, &ses, &links, &current, &total))
			m_iCurrentLink = current;
		else
			m_iCurrentLink = -1;
		g_goto_link(ses, -1);
	}
}

#if 0
double dist_left_right(int y1, int y2, int x1, int x2);
{
  return (double)((x1-y1)+(x2-y2))/2;
}

double dist_up_down(int a1, int a2, int b1, int b2);
{
  if (b1 <= a1 && b2 >= a2) // wholly contained
    return 0;
  if (b1 >= a1 && b1 <= a1) // wholly contained
    return 0;
  return (double)abs(a1 - b1); // difference between left items
}

BOOL LinksBoksWindow::MoveRight()
{
  struct session *ses;
  struct link *links;
  int current, total;
  int ret = GetSessionLinks((struct graphics_device *)m_grdev, &ses, &links, &current, &total);
  if (!ret) return FALSE;
  if (ret == 2)
  { // HACK - no idea if this will work or not in general.  Really, it would
    // be better to pick up KBD_DOWN etc. in g_frame_ev() and deal with all this there.
    KeyboardAction(LINKSBOKS_KBD_RIGHT, 0);
    return TRUE;
  }

  if (current < 0 || current >= total)
    return GetFirstLink(ses, links, total) >= 0;

  int gotolink = -1;
  double minX = INT_MAX;
  double minY = INT_MAX;
  for (int i=0; i < total; i++)
  {
    if (i == current) continue;
    if ((links[i].type == L_LINK && links[i].where) || links[i].type != L_LINK)
    {
      // have a valid link
      double x = links[i].r.x1 - links[current].r.x2;
      double y = dist_left_right(links[current].r.y1, links[current].r.y2, links[i].r.y1, links[i].r.y2)*4;
//      printf("Right: link %i to link %i, x=%f, y=%f, d=%f", current, i, x, y, y*y);
      if (links[i].r.x2 > links[current].r.x2 && (y*y < minY || y*y == minY && x*x < minX))
      { // closer link
        minY = y*y;
        minX = x*x;
        gotolink = i;
      }
    }
  }
  if (gotolink < 0)
    return FALSE;   // no links to move to
//  printf("Moving right from current link %i to link %i", current, gotolink);
//  printf("Coords: current (%i,%i)->(%i,%i)", links[current].r.x1, links[current].r.y1, links[current].r.x2, links[current].r.y2);
//  printf("Coords: next (%i,%i)->(%i,%i)", links[gotolink].r.x1, links[gotolink].r.y1, links[gotolink].r.x2, links[gotolink].r.y2);
  g_goto_link(ses, gotolink);
  return TRUE;
}

BOOL LinksBoksWindow::MoveLeft()
{
  struct session *ses;
  struct link *links;
  int current, total;
  int ret = GetSessionLinks((struct graphics_device *)m_grdev, &ses, &links, &current, &total);
  if (!ret) return FALSE;
  if (ret == 2)
  { // HACK - no idea if this will work or not in general.  Really, it would
    // be better to pick up KBD_DOWN etc. in g_frame_ev() and deal with all this there.
    KeyboardAction(LINKSBOKS_KBD_LEFT, 0);
    return TRUE;
  }

  if (current < 0 || current >= total)
    return GetFirstLink(ses, links, total) >= 0;

  int gotolink = -1;
  double minX =INT_MAX;
  double minY = INT_MAX;
  for (int i=0; i < total; i++)
  {
    if (i == current) continue;
    if ((links[i].type == L_LINK && links[i].where) || links[i].type != L_LINK)
    {
      // have a valid link
      double x = links[current].r.x1 - links[i].r.x2;
      double y = dist_left_right(links[current].r.y1, links[current].r.y2, links[i].r.y1, links[i].r.y2)*4;
//      printf("Left: link %i to link %i, x=%f, y=%f, d=%f", current, i, x, y, y*y);
      if (links[i].r.x1 < links[current].r.x1 && (y*y < minY || y*y == minY && x*x < minX))
      { // closer link
        minX = x*x;
        minY = y*y;
        gotolink = i;
      }
    }
  }
  if (gotolink < 0)
    return FALSE;   // no links to move to
//  printf("Moving left from current link %i to link %i", current, gotolink);
//  printf("Coords: current (%i,%i)->(%i,%i)", links[current].r.x1, links[current].r.y1, links[current].r.x2, links[current].r.y2);
//  printf("Coords: next (%i,%i)->(%i,%i)", links[gotolink].r.x1, links[gotolink].r.y1, links[gotolink].r.x2, links[gotolink].r.y2);
  g_goto_link(ses, gotolink);
  return TRUE;
}

BOOL LinksBoksWindow::MoveDown()
{
  struct session *ses;
  struct link *links;
  int current, total;
  int ret = GetSessionLinks((struct graphics_device *)m_grdev, &ses, &links, &current, &total);
  if (!ret) return FALSE;
  if (ret == 2)
  { // HACK - no idea if this will work or not in general.  Really, it would
    // be better to pick up KBD_DOWN etc. in g_frame_ev() and deal with all this there.
    KeyboardAction(LINKSBOKS_KBD_DOWN, 0);
    return TRUE;
  }

  if (current < 0 || current >= total)
    return GetFirstLink(ses, links, total) >= 0;

  int gotolink = -1;
  double min = INT_MAX;
  for (int i=0; i < total; i++)
  {
    if (i == current) continue;
    if ((links[i].type == L_LINK && links[i].where) || links[i].type != L_LINK)
    {
      // have a valid link
      double x = dist_up_down(links[current].r.x1, links[current].r.x2, links[i].r.x1, links[i].r.x2)/4;
      double y = links[i].r.y1 - links[current].r.y2;
//      printf("Down: link %i to link %i, x=%f, y=%f, d=%f", current, i, x, y, x*x+y*y);
      if (links[i].r.y2 > links[current].r.y2 && x*x + y*y < min)
      { // closer link
        min = x*x + y*y;
        gotolink = i;
      }
    }
  }
  if (gotolink < 0)
    return FALSE;   // no links to move to
//  printf("Moving down from current link %i to link %i", current, gotolink);
//  printf("Coords: current (%i,%i)->(%i,%i)", links[current].r.x1, links[current].r.y1, links[current].r.x2, links[current].r.y2);
//  printf("Coords: next (%i,%i)->(%i,%i)", links[gotolink].r.x1, links[gotolink].r.y1, links[gotolink].r.x2, links[gotolink].r.y2);
  g_goto_link(ses, gotolink);
  return TRUE;
}

BOOL LinksBoksWindow::MoveUp()
{
  struct session *ses;
  struct link *links;
  int current, total;
  int ret = GetSessionLinks((struct graphics_device *)m_grdev, &ses, &links, &current, &total);
  if (!ret) return FALSE;
  if (ret == 2)
  { // HACK - no idea if this will work or not in general.  Really, it would
    // be better to pick up KBD_DOWN etc. in g_frame_ev() and deal with all this there.
    KeyboardAction(LINKSBOKS_KBD_UP, 0);
    return TRUE;
  }

  if (current < 0 || current >= total)
    return GetFirstLink(ses, links, total) >= 0;

  int gotolink = -1;
  double min = INT_MAX;
  for (int i=0; i < total; i++)
  {
    if (i == current) continue;
    if ((links[i].type == L_LINK && links[i].where) || links[i].type != L_LINK)
    {
      // have a valid link
      double x = dist_up_down(links[current].r.x1, links[current].r.x2, links[i].r.x1, links[i].r.x2)/4;
      double y = links[current].r.y1 - links[i].r.y2;
//      printf("Up: link %i to link %i, x=%f, y=%f, d=%f", current, i, x, y, x*x+y*y);
      if (links[i].r.y1 < links[current].r.y1 && x*x + y*y < min)
      { // closer link
        min = x*x + y*y;
        gotolink = i;
      }
    }
  }
  if (gotolink < 0)
    return FALSE;   // no links to move to
//  printf("Moving up from current link %i to link %i", current, gotolink);
//  printf("Coords: current (%i,%i)->(%i,%i)", links[current].r.x1, links[current].r.y1, links[current].r.x2, links[current].r.y2);
//  printf("Coords: next (%i,%i)->(%i,%i)", links[gotolink].r.x1, links[gotolink].r.y1, links[gotolink].r.x2, links[gotolink].r.y2);
  g_goto_link(ses, gotolink);
  return TRUE;
}

#endif /* 0 */

BOOL LinksBoksWindow::CanInputText(unsigned char *buffer, int size)
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;
	struct terminal *term = (struct terminal *)dev->user_data;
	struct window *win = get_root_or_first_window(term);
	
	if (!win || !win->data) return FALSE;

	if (win->handler == dialog_func) {
		/* for dialogs */
		struct dialog_data *dlg = (struct dialog_data *)win->data;
		struct dialog_item_data *di = &dlg->items[dlg->selected];
		if (di->item->type == D_FIELD || di->item->type == D_FIELD_PASS) {
		    safe_strncpy(buffer, di->cdata, size);
			return TRUE;
		}
	}

	if (win->handler == win_func) {
		/* for forms in web pages */
		struct session *ses = (struct session *)win->data;
		struct link *l = get_current_link(ses);
		if (!l) return FALSE;
		if (ses->locked_link && (l->type == L_FIELD || l->type == L_AREA)) {
			if (!l->form) return FALSE;
			struct f_data_c *fd = (struct f_data_c *)current_frame(ses);
			if (!fd) return FALSE;
			struct form_state *fs = find_form_state(fd, l->form);
			if (!fs) return FALSE;
			if (!fs->value) return FALSE;
			safe_strncpy(buffer, fs->value, size);
			return TRUE;
		}
	}

	return FALSE;
}

VOID LinksBoksWindow::SendText(unsigned char *text)
{
	unsigned char *pos = text;
	struct graphics_device *dev = (struct graphics_device *)m_grdev;

	struct terminal *term = (struct terminal *)dev->user_data;
	struct window *win = get_root_or_first_window(term);

	unsigned char *buffer = (text) ? text : (unsigned char *)"";
	
	if (!win || !win->data) return;

	if (win->handler == dialog_func) {
		/* for dialogs */
		struct dialog_data *dlg = (struct dialog_data *)win->data;
		struct dialog_item_data *di = &dlg->items[dlg->selected];
		if (di->item->type == D_FIELD || di->item->type == D_FIELD_PASS) {
			mem_realloc(di->cdata, strlen((const char *)buffer) + 1);
			safe_strncpy(di->cdata, buffer, strlen((const char *)buffer) + 1);
			di->cpos = 0;
			dev->resize_handler(dev);
			return;
		}
	}

	if (win->handler == win_func) {
		/* for forms in web pages */
		struct session *ses = (struct session *)win->data;
		struct link *l = get_current_link(ses);
		if (!l) return;
		if (ses->locked_link && (l->type == L_FIELD || l->type == L_AREA)) {
			if (!l->form) return;
			struct f_data_c *fd = (struct f_data_c *)current_frame(ses);
			if (!fd) return;
			struct form_state *fs = find_form_state(fd, l->form);
			if (!fs) return;

			if (fs->value)
				mem_free(fs->value);
			fs->value = (unsigned char *)mem_calloc(strlen((const char *)buffer) + 1);
			safe_strncpy(fs->value, buffer, strlen((const char *)buffer) + 1);
			fs->state = 0;
			dev->resize_handler(dev);
			return;
		}
	}




/*
  dev->keyboard_handler(dev, KBD_HOME, 0);
  dev->keyboard_handler(dev, KBD_DEL, KBD_SHIFT);
  while (*pos)
		dev->keyboard_handler(dev, *pos++, 0);
  return;
*/
}


/******************* SESSION ACTIONS *********************/




VOID LinksBoksWindow::GoToURL(unsigned char *url)
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;

	struct terminal *term = (struct terminal *)dev->user_data;
	struct window *win = get_root_or_first_window(term);

	goto_url((struct session *)win->data, url);
}

VOID LinksBoksWindow::GoToURLInNewTab(unsigned char *url)
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;

	struct terminal *term = (struct terminal *)dev->user_data;

	open_in_new_tab(term, NULL, url);
}

int LinksBoksWindow::NumberOfTabs()
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;
	struct terminal *term = (struct terminal *)dev->user_data;

	return number_of_tabs(term);
}

VOID LinksBoksWindow::SwitchToTab(int index)
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;
	struct terminal *term = (struct terminal *)dev->user_data;

	switch_to_tab(term, index);
}

VOID LinksBoksWindow::CloseCurrentTab()
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;
	struct terminal *term = (struct terminal *)dev->user_data;

	close_tab(term);
}

BOOL LinksBoksWindow::GetCurrentURL(unsigned char *buffer, int size)
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;

	struct terminal *term = (struct terminal *)dev->user_data;
	struct window *win = get_root_or_first_window(term);

	if (!win || !win->data || win->type == 0) return false;

	get_current_url((struct session *)win->data, buffer, size);

   return true;
}


BOOL LinksBoksWindow::GetCurrentTitle(unsigned char *buffer, int size)
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;

	struct terminal *term = (struct terminal *)dev->user_data;
	struct window *win = get_root_or_first_window(term);

	if (!win || !win->data || win->type == 0) return false;

	get_current_title((struct session *)win->data, buffer, size);

   return true;
}

BOOL LinksBoksWindow::GetCurrentLinkURL(unsigned char *buffer, int size)
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;

	struct terminal *term = (struct terminal *)dev->user_data;
	struct window *win = get_root_or_first_window(term);

	if (!win || !win->data || win->type == 0) return false;

	get_current_link_url((struct session *)win->data, buffer, size);

   return true;
}

BOOL LinksBoksWindow::GetCurrentStatus(unsigned char *buffer, int size)
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;

	struct terminal *term = (struct terminal *)dev->user_data;
	struct window *win = get_root_or_first_window(term);

	if (!win || !win->data || win->type == 0) return false;

	struct session *sess = (struct session *)win->data;

	safe_strncpy(buffer, sess->st, size);

	return true;
}

int LinksBoksWindow::GetCurrentState()
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;
    if (!dev || !dev->user_data) return LINKSBOKS_S_INVALID_DEVICE;

	struct terminal *term = (struct terminal *)dev->user_data;
    if (!term) return LINKSBOKS_S_INVALID_TERM;

	struct window *win = get_root_or_first_window(term);
    if (!win || !win->data || win->type == 0) return LINKSBOKS_S_INVALID_WINDOW;

	struct session *sess = (struct session *)win->data;
    struct status *stat;

    struct f_data_c *fd = sess->screen;

    if (!fd) return LINKSBOKS_S_INVALID_FDATA;

    if (fd->af)
    {
	    struct additional_file *af;

        for(af = (struct additional_file *)fd->af->af.next;
              af != (struct additional_file *)&fd->af->af;
              af = af->next)
        {
				if (af->rq && af->rq->stat.state >= 0)
                    return af->rq->stat.state;
        }
    }

    if(sess->rq)
    {
        return sess->rq->stat.state;
    }
    else
    {
		if (fd->rq) stat = &fd->rq->stat;
		if (stat && stat->state == S_OKAY && fd->af)
        {
			struct additional_file *af;
            // foreach(af,fd->af->af)
			for(af = (struct additional_file *)fd->af->af.next;
              af != (struct additional_file *)&fd->af->af;
              af = af->next)
            {
				if (af->rq && af->rq->stat.state >= 0)
                    return af->rq->stat.state;
			}
        }
    }

    return LINKSBOKS_S_NOT_BUSY;
}

VOID LinksBoksWindow::GoBack()
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;

	struct terminal *term = (struct terminal *)dev->user_data;
	struct window *win = get_root_or_first_window(term);

	go_back((struct session *)win->data, NULL);
}

VOID LinksBoksWindow::GoForward()
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;

	struct terminal *term = (struct terminal *)dev->user_data;
	struct window *win = get_root_or_first_window(term);

	go_forward((struct session *)win->data);
}

VOID LinksBoksWindow::Stop()
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;

	struct terminal *term = (struct terminal *)dev->user_data;
	struct window *win = get_root_or_first_window(term);

	stop_button_pressed((struct session *)win->data);
}

VOID LinksBoksWindow::Reload(BOOL nocache)
{
	struct graphics_device *dev = (struct graphics_device *)m_grdev;

	struct terminal *term = (struct terminal *)dev->user_data;
	struct window *win = get_root_or_first_window(term);

	reload((struct session *)win->data, (nocache) ? -1 : 0);
}

