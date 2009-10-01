//	includes
#include <stdio.h>
#include "iTunesVisualAPI.h"
#include "iTunesAPI.h"
#include "src/goom.h"
#include <CoreFoundation/CFBundle.h>
#include <Carbon/Carbon.h>

//*****************************************************
#include <stdlib.h>
#include <string.h>
//*****************************************************

CFStringRef CFBundleIdentifier;

extern void ppc_doubling(unsigned int,UInt32 *,UInt32 *,UInt32 *,UInt32,UInt32);

//	typedef's, struct's, enum's, etc.
#define kTVisualPluginName		"\piGoom"
#define	kTVisualPluginCreator		'gyom'
#define	kTVisualPluginMajorVersion	2
#define	kTVisualPluginMinorVersion	4
#define	kTVisualPluginReleaseStage	betaStage
#define	kTVisualPluginNonFinalRelease	1

#define VERSION "2k4"

#define kPixelDoublingPrefName		"PixelDoubling"
#define kShowFPSPrefName		    "ShowFPS"
#define kSensitivityPrefName		"Sensitivity"
#define kShowAboutWhenIdlePrefName	"ShowAboutWhenIdle"

//#define Timers

enum
{
    kOKSettingID = 1,
    kPixelDoublingSettingID = 2,
    kFrameRateSettingID = 3,
    kSensitivitySettingID = 4,
    kShowAboutWhenIdleSettingID = 5
};

typedef struct VisualPluginData {
    void *			appCookie;
    ITAppProcPtr		appProc;
    ITFileSpec			pluginFileSpec;

    CGrafPtr			destPort;
    Rect			destRect;
    OptionBits			destOptions;
    UInt32			destBitDepth;

    ITTrackInfo			trackInfo;
    ITStreamInfo		streamInfo;

    Boolean			playing;

    //	Plugin-specific data
    GWorldPtr			offscreen;
    signed short 		data[2][512];
} VisualPluginData;


//	local (static) globals
//static unsigned int	useSpectrum = 0;
static CGrafPtr		gSavePort;
static GDHandle		gSaveDevice;
static unsigned int	changeRes = FALSE;
static unsigned int	oldx = 0, oldy = 0;
static signed int	forced = 0;
static unsigned int	showFPS = 0;
static int		showTexte = 0, showTitle = 1;
static Boolean		doublePixels = true;
static int		sensitivity = 160;
static int		ShowAboutWhenIdle = 0;
static AbsoluteTime	backUpTime;
static char *		aboutMessage;

static PluginInfo  *    goomInfo;


//	exported function prototypes
extern OSStatus iTunesPluginMainMachO(OSType message,PluginMessageInfo *messageInfo,void *refCon);

// Calcul de diff de temps
#ifdef Timers
static void HowLong(const char* text)
{
    AbsoluteTime absTime;
    Nanoseconds nanosec;

    absTime = SubAbsoluteFromAbsolute(UpTime(), backUpTime);
    nanosec = AbsoluteToNanoseconds(absTime);
    fprintf(stderr,"Time for %s:  %f\n", text, (float) UnsignedWideToUInt64( nanosec ) / 1000000.0);
    backUpTime = UpTime();
}
#else
#define  HowLong(a)
#endif

// ProcessRenderData --> preprocessing des donnees en entrŽe
static void ProcessRenderData(VisualPluginData *visualPluginData,const RenderVisualData *renderData)
{
    SInt16		index;
    SInt32		channel;

    if (renderData == nil)
    {
        bzero(&visualPluginData->data,sizeof(visualPluginData->data));
        return;
    }
    else
    {
        for (channel = 0;channel < 2;channel++)
        {
            for (index = 0; index < 512; index++)
                visualPluginData->data[channel][index] = (renderData->waveformData[channel][index]-127)*sensitivity;
        }
    }
}

// GetPortCopyBitsBitMap
//
static BitMap* GetPortCopyBitsBitMap(CGrafPtr port)
{
    BitMap*		destBMap;

#if ACCESSOR_CALLS_ARE_FUNCTIONS
    destBMap = (BitMap*)GetPortBitMapForCopyBits(port);
#else
#if OPAQUE_TOOLBOX_STRUCTS
    PixMapHandle	pixMap;

    pixMap		= GetPortPixMap(port);
    destBMap	= (BitMap*) (*pixMap);
#else
    destBMap	= (BitMap*) &((GrafPtr)port)->portBits;
#endif
#endif
    return destBMap;
}

//	RenderVisualPort
/*	OK, This is pretty weird : if we are not in pixel doubling mode, 
the goom internal buffer is copied on destPort via CopyBits().
Now, if we are in pixel doubling mode : if we are full screen,
the goom internal buffer is copied on another buffer with ppc_doubling()
and then to destPort with CopyBits().*/

char * Str255ToC(Str255 source)
{
    static char dst[255];
    char * cur = dst, * src = (char*)source;
    int i;
    int size = *src;
    if (source == NULL) return "";
    src++;
    for (i=0; i<size; i++)
    {
        *cur = *src;
        switch (*cur)
        {
            case 'Ë' :
                *cur = 'A';
                break;
            case 'ˆ' :
            case '‡' :
            case 'Š' :
            case '‰' :
                *cur = 'a';
                break;
            case 'æ' :
            case 'é' :
            case 'ƒ' :
                *cur = 'E';
                break;
            case 'Ž' :
            case '' :
            case '' :
            case '‘' :
                *cur = 'e';
                break;
            case '“' :
            case '•' :
            case '”' :
            case '’' :
                *cur = 'i';
                break;
            case '–' :
                *cur = 'n';
                break;
            case '' :
                *cur = 'c';
                break;
            case '˜' :
            case '—' :
            case 'š' :
            case '™' :
                *cur = 'o';
                break;
            case '' :
            case 'œ' :
            case 'ž' :
            case 'Ÿ' :
                *cur = 'u';
                break;
            default : break;
        }
        src++;
        cur++;
    }
    *cur = 0;
    return dst;
}

static void RenderVisualPort(VisualPluginData *visualPluginData,CGrafPtr destPort,const Rect *destRect,Boolean onlyUpdate)
{
    BitMap*		srcBitMap;
    BitMap*		dstBitMap;
    unsigned int 	right, bottom;
    Rect		srcRect= *destRect;
    Rect		dstRect = srcRect;
    int	fullscreen;
    static GWorldPtr	offscreen;
    PixMapHandle	pixMapHdl = GetGWorldPixMap(visualPluginData->offscreen);
    Point pt = {0,0};
    static float	fpssampler = 0;
    static char textBuffer[15];
    static char titleBuffer[255];
    unsigned char * str, * str2;

    AbsoluteTime absTime;
    Nanoseconds nanosec;

    LocalToGlobal(&pt);
    fullscreen = (pt.v == 0);

    if ((NULL == destPort) || (NULL == destRect) || (NULL == visualPluginData->offscreen)) return;

    absTime = SubAbsoluteFromAbsolute(UpTime(), backUpTime);
    nanosec = AbsoluteToNanoseconds(absTime);
    fpssampler = 1000000000.0 / (float) UnsignedWideToUInt64( nanosec );
    if (fpssampler>35) return;
    backUpTime = UpTime();


    GetGWorld(&gSavePort,&gSaveDevice);
    SetGWorld(destPort,nil);

    srcBitMap	= GetPortCopyBitsBitMap(visualPluginData->offscreen);
    dstBitMap	= GetPortCopyBitsBitMap(destPort);

    OffsetRect(&srcRect,-srcRect.left,-srcRect.top);
    if (!pixMapHdl || !*pixMapHdl) return;

    right = srcRect.right;
    bottom = srcRect.bottom;
    if ((right<2) || (bottom<2)) return;

    // Update our offscreen pixmap
    if ((changeRes) || (oldx != right) || (oldy != bottom))
    {
        if (doublePixels)
            goom_set_resolution (goomInfo,right%2 + right/2,  bottom/2 + bottom%2);
        else
            goom_set_resolution (goomInfo,right, bottom);
        oldx = right;
        oldy = bottom;
        changeRes = FALSE;
    }

    str2 = NULL;
    if (showTitle == 0)
    {
        strcpy(titleBuffer, Str255ToC(visualPluginData->trackInfo.name));
        str2 = titleBuffer;
        str = " ";
        showTexte = 10000;
    }
    else
    {
        if (ShowAboutWhenIdle)
        {
            switch (showTexte)
            {
                case 0:
                    str2 = " ";
                    sprintf(textBuffer,"The iGoom %s",VERSION);
                    str = textBuffer;
                    break;
                case 500:
                    str = "http://www.ios-software.com/";
                    break;
                case 1000 :
                    str = aboutMessage;
                    break;
                default :
                    str = NULL;
                    break;
            }
        }
        else
        {
            str = " ";
        }
    }
    
    if (doublePixels)
    {
        UInt32 rowBytes = (GetPixRowBytes(pixMapHdl))>>2;
        register UInt32 *myX = (UInt32*) GetPixBaseAddr(pixMapHdl);
        register UInt32 inc = 2*rowBytes - right - right%2;
        register UInt32 *myx = (UInt32*)  goom_update (goomInfo,visualPluginData->data,forced,(showFPS > 0)?fpssampler:-1,str2,str);

        ppc_doubling(right/2 + right%2, myx, myX, myX + rowBytes, bottom/2,inc*4);
        srcBitMap = GetPortCopyBitsBitMap(visualPluginData->offscreen);
        CopyBits(srcBitMap,dstBitMap,&srcRect,&dstRect,srcCopy,nil);
    }
    else
    {
        NewGWorldFromPtr(&offscreen, k32ARGBPixelFormat,&srcRect, NULL, NULL, 0, (Ptr) goom_update (goomInfo,visualPluginData->data,forced, (showFPS > 0)?fpssampler:-1, str2, str), right*4);
        HowLong("Goom");
        srcBitMap = GetPortCopyBitsBitMap(offscreen);
        CopyBits(srcBitMap,dstBitMap,&srcRect,&dstRect,srcCopy,nil);
        DisposeGWorld(offscreen);
    }
    showTexte++;
    if (showTexte>10000) showTexte = 10000;
    showTitle = 1;
    if (forced>0) forced = -1;

    SetGWorld(gSavePort,gSaveDevice);
}


/*	AllocateVisualData is where you should allocate any information that depends
on the port or rect changing (like offscreen GWorlds). */
static OSStatus AllocateVisualData(VisualPluginData *visualPluginData,CGrafPtr destPort,const Rect *destRect)
{
    OSStatus		status;
    Rect			allocateRect;

    (void) destPort;

    GetGWorld(&gSavePort,&gSaveDevice);

    allocateRect = *destRect;
    OffsetRect(&allocateRect,-allocateRect.left,-allocateRect.top);

    status = NewGWorld(&visualPluginData->offscreen,32,&allocateRect,nil,nil,useTempMem);
    if (status == noErr)
    {
        PixMapHandle	pix = GetGWorldPixMap(visualPluginData->offscreen);
        LockPixels(pix);

        // Offscreen starts out black
        SetGWorld(visualPluginData->offscreen,nil);
        ForeColor(blackColor);
        PaintRect(&allocateRect);
    }
    SetGWorld(gSavePort,gSaveDevice);

    return status;
}

//	DeallocateVisualData is where you should deallocate the .
static void DeallocateVisualData(VisualPluginData *visualPluginData)
{
    CFPreferencesAppSynchronize(CFBundleIdentifier);

    if (visualPluginData->offscreen != nil)
    {
        DisposeGWorld(visualPluginData->offscreen);
        visualPluginData->offscreen = nil;
    }
}

// ChangeVisualPort
static OSStatus ChangeVisualPort(VisualPluginData *visualPluginData,CGrafPtr destPort,const Rect *destRect)
{
    OSStatus		status;
    Boolean			doAllocate;
    Boolean			doDeallocate;

    status = noErr;

    doAllocate	= false;
    doDeallocate	= false;

    if (destPort != nil)
    {
        if (visualPluginData->destPort != nil)
        {
            if (false == EqualRect(destRect,&visualPluginData->destRect))
            {
                doDeallocate	= true;
                doAllocate	= true;
            }
        }
        else
        {
            doAllocate = true;
        }
    }
    else
    {
        doDeallocate = true;
    }

    if (doDeallocate)
        DeallocateVisualData(visualPluginData);

    if (doAllocate)
        status = AllocateVisualData(visualPluginData,destPort,destRect);

    if (status != noErr)
        destPort = nil;

    visualPluginData->destPort = destPort;
    if (destRect != nil)
        visualPluginData->destRect = *destRect;

    return status;
}

//	ResetRenderData
static void ResetRenderData(VisualPluginData *visualPluginData)
{
    bzero(&visualPluginData->data,sizeof(visualPluginData->data));
}

//	settingsControlHandler
pascal OSStatus settingsControlHandler(EventHandlerCallRef inRef,EventRef inEvent, void* userData)
{
    WindowRef wind=NULL;
    ControlID controlID;
    ControlRef control=NULL;
    //get control hit by event
    GetEventParameter(inEvent,kEventParamDirectObject,typeControlRef,NULL,sizeof(ControlRef),NULL,&control);
    wind=GetControlOwner(control);
    GetControlID(control,&controlID);
    switch(controlID.id){

        case kShowAboutWhenIdleSettingID:
            ShowAboutWhenIdle = GetControlValue(control);
            CFPreferencesSetAppValue (CFSTR(kShowAboutWhenIdlePrefName),ShowAboutWhenIdle?CFSTR("YES"):CFSTR("NO"),CFBundleIdentifier);
            break;

        case kPixelDoublingSettingID:
            doublePixels = GetControlValue(control);
            CFPreferencesSetAppValue (CFSTR(kPixelDoublingPrefName),doublePixels?CFSTR("YES"):CFSTR("NO"),CFBundleIdentifier);
            changeRes = TRUE;
            break;

        case kFrameRateSettingID:
            showFPS = GetControlValue( control );
            CFPreferencesSetAppValue (CFSTR(kShowFPSPrefName),showFPS?CFSTR("YES"):CFSTR("NO"),CFBundleIdentifier);
            break;

        case kSensitivitySettingID:
            sensitivity = GetControlValue( control );
            {
                CFNumberRef value = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &sensitivity);
                CFPreferencesSetAppValue (CFSTR(kSensitivityPrefName), value, CFBundleIdentifier);
                CFRelease(value);
            }
            break;

        case kOKSettingID:
            HideWindow(wind);
            break;
    }
    return noErr;
}

//	VisualPluginHandler
static OSStatus VisualPluginHandler(OSType message,VisualPluginMessageInfo *messageInfo,void *refCon)
{
    OSStatus		status;
    VisualPluginData *	visualPluginData;

    visualPluginData = (VisualPluginData*) refCon;

    status = noErr;

    switch (message)
    {
        //	Sent when the visual plugin is registered.  The plugin should do minimal
        //	memory allocations here.  The resource fork of the plugin is still available.
        case kVisualPluginInitMessage:
        {
            visualPluginData = (VisualPluginData*) NewPtrClear(sizeof(VisualPluginData));
            if (visualPluginData == nil)
            {
                status = memFullErr;
                break;
            }
            visualPluginData->appCookie = messageInfo->u.initMessage.appCookie;
            visualPluginData->appProc   = messageInfo->u.initMessage.appProc;
            // Remember the file spec of our plugin file.
            // We need this so we can open our resource fork during
            // the configuration message

            status = PlayerGetPluginFileSpec(visualPluginData->appCookie, visualPluginData->appProc, &visualPluginData->pluginFileSpec);
            messageInfo->u.initMessage.refCon	= (void*) visualPluginData;
            goomInfo = goom_init(100,100);
            
            //fprintf(stderr,"Loc : %s\n", CFStringGetCStringPtr(CFCopyLocalizedStringFromTableInBundle(CFSTR("AboutString"), CFSTR("About"), CFBundleGetBundleWithIdentifier(CFBundleIdentifier), NULL),kCFStringEncodingMacRoman));

            aboutMessage = (char*)CFStringGetCStringPtr(CFCopyLocalizedStringFromTableInBundle(CFSTR("AboutString"), CFSTR("About"), CFBundleGetBundleWithIdentifier(CFBundleIdentifier), NULL),kCFStringEncodingMacRoman);

            break;
        }

            //	Sent when the visual plugin is unloaded
        case kVisualPluginCleanupMessage:
            if (visualPluginData != nil)
                DisposePtr((Ptr)visualPluginData);

            goom_close(goomInfo);
            break;

            //	Sent when the visual plugin is enabled.  iTunes currently enables all
            //	loaded visual plugins.  The plugin should not do anything here.
        case kVisualPluginEnableMessage:
            if (true == visualPluginData->playing)
            {
                showTexte = 10000;
                showTitle = 0;
            }
            else
            {
                showTexte = 0;
                showTitle = 1;
            }
        case kVisualPluginDisableMessage:
            break;

            //	Sent if the plugin requests idle messages.  Do this by setting the kVisualWantsIdleMessages
            //	option in the RegisterVisualMessage.options field.
        case kVisualPluginIdleMessage:
                RenderVisualPort(visualPluginData,visualPluginData->destPort,&visualPluginData->destRect,false);
            break;

            //	Sent if the plugin requests the ability for the user to configure it.  Do this by setting
            //	the kVisualWantsConfigure option in the RegisterVisualMessage.options field.
        case kVisualPluginConfigureMessage:
        {
            
            // kOKSettingID = 1,
             //kPixelDoublingSettingID = 2,
             //kFrameRateSettingID = 3,
             //kSensitivitySettingID = 4
                        
            static EventTypeSpec controlEvent={kEventClassControl,kEventControlHit};
            static const ControlID kPixelDoublingSettingControlID={'cbox',kPixelDoublingSettingID};
            static const ControlID kFrameRateSettingControlID={'cbox',kFrameRateSettingID};
            static const ControlID kSensitivitySettingControlID={'slid',kSensitivitySettingID};
            static const ControlID kShowAboutWhenIdleSettingControlID={'cbox',kShowAboutWhenIdleSettingID};
            static WindowRef settingsDialog=NULL;
            static ControlRef PixelDoublingCTRL=NULL;
            static ControlRef FrameRateCTRL=NULL;
            static ControlRef SensitivityCTRL=NULL;
            static ControlRef ShowAboutWhenIdleCTRL=NULL;

            if(settingsDialog==NULL){
                IBNibRef 	nibRef;
                CFBundleRef	iGoomPlugin;
                //we have to find our bundle to load the nib inside of it
                iGoomPlugin=CFBundleGetBundleWithIdentifier(CFBundleIdentifier);
                CreateNibReferenceWithCFBundle(iGoomPlugin,CFSTR("SettingsDialog"), &nibRef);
                CreateWindowFromNib(nibRef, CFSTR("PluginSettings"), &settingsDialog);
                DisposeNibReference(nibRef);

                      
                //fprintf (stderr,"Picture @ %d\n", (PicHandle)GetPicture (12866));

                InstallWindowEventHandler(settingsDialog,NewEventHandlerUPP(settingsControlHandler),
                                          1,&controlEvent,0,NULL);
                GetControlByID(settingsDialog,&kPixelDoublingSettingControlID,&PixelDoublingCTRL);
                GetControlByID(settingsDialog,&kFrameRateSettingControlID,&FrameRateCTRL);
                GetControlByID(settingsDialog,&kSensitivitySettingControlID,&SensitivityCTRL);
                GetControlByID(settingsDialog,&kShowAboutWhenIdleSettingControlID,&ShowAboutWhenIdleCTRL);
            }
            SetControlValue(PixelDoublingCTRL,doublePixels);
            SetControlValue(FrameRateCTRL,showFPS);
            SetControlValue(SensitivityCTRL,sensitivity);
            SetControlValue(ShowAboutWhenIdleCTRL,ShowAboutWhenIdle);

            ShowWindow(settingsDialog);
        }
            break;

            //	Sent when iTunes is going to show the visual plugin in a port.  At
            //	this point,the plugin should allocate any large buffers it needs.
        case kVisualPluginShowWindowMessage:
            if (true == visualPluginData->playing)
            {
                showTexte = 10000;
                showTitle = 0;
            }
            else
            {
                showTexte = 0;
                showTitle = 1;
            }
            visualPluginData->destOptions = messageInfo->u.showWindowMessage.options;
            status = ChangeVisualPort( visualPluginData, messageInfo->u.showWindowMessage.port,
                                       &messageInfo->u.showWindowMessage.drawRect);
            //FIXME setres here
            break;
            //	Sent when iTunes is no longer displayed.
        case kVisualPluginHideWindowMessage:

            (void) ChangeVisualPort(visualPluginData,nil,nil);

            bzero(&visualPluginData->trackInfo,sizeof(visualPluginData->trackInfo));
            bzero(&visualPluginData->streamInfo,sizeof(visualPluginData->streamInfo));
            break;

            //	Sent when iTunes needs to change the port or rectangle of the currently
            //	displayed visual.
        case kVisualPluginSetWindowMessage:
            visualPluginData->destOptions = messageInfo->u.setWindowMessage.options;

            status = ChangeVisualPort(	visualPluginData,
                                       messageInfo->u.setWindowMessage.port,
                                       &messageInfo->u.setWindowMessage.drawRect);
            break;

            //	Sent for the visual plugin to render a frame.
        case kVisualPluginRenderMessage:
            ProcessRenderData(visualPluginData,messageInfo->u.renderMessage.renderData);
            RenderVisualPort(visualPluginData,visualPluginData->destPort,&visualPluginData->destRect,false);
            break;

            //	Sent in response to an update event.  The visual plugin should update
            //	into its remembered port.  This will only be sent if the plugin has been
            //	previously given a ShowWindow message.
        case kVisualPluginUpdateMessage:
            RenderVisualPort(visualPluginData,visualPluginData->destPort,&visualPluginData->destRect,true);
            break;

            //	Sent when the player starts.
        case kVisualPluginPlayMessage:
            if (messageInfo->u.playMessage.trackInfo != nil)
                visualPluginData->trackInfo = *messageInfo->u.playMessage.trackInfo;
            else
                bzero(&visualPluginData->trackInfo,sizeof(visualPluginData->trackInfo));

            if (messageInfo->u.playMessage.streamInfo != nil)
                visualPluginData->streamInfo = *messageInfo->u.playMessage.streamInfo;
            else
                bzero(&visualPluginData->streamInfo,sizeof(visualPluginData->streamInfo));

            visualPluginData->playing = true;
            showTexte = 10000;
            showTitle = 0;
            break;

            //	Sent when the player changes the current track information.  This
            //	is used when the information about a track changes,or when the CD
            //	moves onto the next track.  The visual plugin should update any displayed
            //	information about the currently playing song.
        case kVisualPluginChangeTrackMessage:
            if (messageInfo->u.changeTrackMessage.trackInfo != nil)
                visualPluginData->trackInfo = *messageInfo->u.changeTrackMessage.trackInfo;
            else
                bzero(&visualPluginData->trackInfo,sizeof(visualPluginData->trackInfo));

            if (messageInfo->u.changeTrackMessage.streamInfo != nil)
                visualPluginData->streamInfo = *messageInfo->u.changeTrackMessage.streamInfo;
            else
                bzero(&visualPluginData->streamInfo,sizeof(visualPluginData->streamInfo));
            showTexte = 10000;
            showTitle = 0;
            break;

            //	Sent when the player stops.
        case kVisualPluginStopMessage:
            visualPluginData->playing = false;
            ResetRenderData(visualPluginData);
            //RenderVisualPort(visualPluginData,visualPluginData->destPort,&visualPluginData->destRect,true);
            showTexte = 0;
            showTitle = 1;
            break;

            //	Sent when the player changes position.
        case kVisualPluginSetPositionMessage:
            break;

            //	Sent when the player pauses.  iTunes does not currently use pause or unpause.
            //	A pause in iTunes is handled by stopping and remembering the position.
        case kVisualPluginPauseMessage:
            visualPluginData->playing = false;
            ResetRenderData(visualPluginData);
            //RenderVisualPort(visualPluginData,visualPluginData->destPort,&visualPluginData->destRect,true);
            break;

            //	Sent when the player unpauses.  iTunes does not currently use pause or unpause.
            //	A pause in iTunes is handled by stopping and remembering the position.
        case kVisualPluginUnpauseMessage:
            visualPluginData->playing = true;
            break;

            //	Sent to the plugin in response to a MacOS event.  The plugin should return noErr
            //	for any event it handles completely,or an error (unimpErr) if iTunes should handle it.
        case kVisualPluginEventMessage:
        {
            EventRecord* tEventPtr = messageInfo->u.eventMessage.event;
            if ((tEventPtr->what == keyDown) || (tEventPtr->what == autoKey))
            {    // charCodeMask,keyCodeMask;
                char theChar = tEventPtr->message & charCodeMask;

                switch (theChar) // Process keys here
                {
                    
                    case	't':
                    case	'T':
                        ShowAboutWhenIdle = (ShowAboutWhenIdle==0)?1:0;
                        CFPreferencesSetAppValue (CFSTR(kShowAboutWhenIdlePrefName),ShowAboutWhenIdle?CFSTR("YES"):CFSTR("NO"), CFBundleIdentifier);
                        break;
                    case	'q':
                    case	'Q':
                        doublePixels = (doublePixels==0)?1:0;
                        CFPreferencesSetAppValue (CFSTR(kPixelDoublingPrefName),doublePixels?CFSTR("YES"):CFSTR("NO"), CFBundleIdentifier);
                        changeRes = TRUE;
                        break;
                    case	'0':
                        forced = (forced == 0) ? -1 : 0;
                        break;
                    case	'1':
                    case	'2':
                    case	'3':
                    case	'4':
                    case	'5':
                    case	'6':
                    case	'7':
                    case	'8':
                    case	'9':
                        forced = theChar - '0';
                        break;
                    case	'f':
                    case	'F':
                        showFPS = (showFPS==0)?1:0;
                        CFPreferencesSetAppValue (CFSTR(kShowFPSPrefName),showFPS?CFSTR("YES"):CFSTR("NO"),CFBundleIdentifier);
                        break;
                        
                    case	'>':
                        if (sensitivity <= 240) sensitivity += 10;
                        {
                            CFNumberRef value = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &sensitivity);
                            CFPreferencesSetAppValue (CFSTR(kSensitivityPrefName), value, CFBundleIdentifier);
                            CFRelease(value);
                        }
                            break;
                    case	'<':
                        if (sensitivity >= 80) sensitivity -= 10;
                        {
                            CFNumberRef value = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &sensitivity);
                            CFPreferencesSetAppValue (CFSTR(kSensitivityPrefName), value, CFBundleIdentifier);
                            CFRelease(value);
                        }
                            break;
                        
                    case	'\r':
                    case	'\n':
                        break;
                    default:
                        status = unimpErr;
                        break;
                }
            }
            else
                status = unimpErr;
        }
            break;

        default:
            status = unimpErr;
            break;
    }
    return status;
}

//	RegisterVisualPlugin
static OSStatus RegisterVisualPlugin(PluginMessageInfo *messageInfo)
{
    OSStatus		status;
    PlayerMessageInfo	playerMessageInfo;
    Str255		pluginName = kTVisualPluginName;
#ifdef Timers
    backUpTime = UpTime();
#endif

    CFStringRef aString;
    CFNumberRef aNumber;
    CFComparisonResult result;

    CFBundleIdentifier = CFSTR("com.ios.igoom");

    // Read the preferences
    aString = CFPreferencesCopyAppValue(CFSTR(kPixelDoublingPrefName),CFBundleIdentifier);
    if (aString != NULL)
    {
        result = CFStringCompareWithOptions(aString, CFSTR("YES"), CFRangeMake(0,CFStringGetLength(aString)), kCFCompareCaseInsensitive);
        if (result ==  kCFCompareEqualTo) {
            doublePixels = true;
        }
        else doublePixels = false;
    }

    aString = CFPreferencesCopyAppValue(CFSTR(kShowFPSPrefName),CFBundleIdentifier);
    if (aString != NULL)
    {
        result = CFStringCompareWithOptions(aString, CFSTR("YES"), CFRangeMake(0,CFStringGetLength(aString)), kCFCompareCaseInsensitive);
        if (result ==  kCFCompareEqualTo) {
            showFPS = true;
        }
        else showFPS = false;
    }
    
    aString = CFPreferencesCopyAppValue(CFSTR(kShowAboutWhenIdlePrefName),CFBundleIdentifier);
    if (aString != NULL)
    {
        result = CFStringCompareWithOptions(aString, CFSTR("YES"), CFRangeMake(0,CFStringGetLength(aString)), kCFCompareCaseInsensitive);
        if (result ==  kCFCompareEqualTo) {
            ShowAboutWhenIdle = true;
        }
        else ShowAboutWhenIdle = false;
    }
    
    aNumber = CFPreferencesCopyAppValue(CFSTR(kSensitivityPrefName),CFBundleIdentifier);
    if (aNumber != NULL)
    {
        CFNumberGetValue(aNumber,kCFNumberIntType,&sensitivity);
    }
    


    
    bzero(&playerMessageInfo.u.registerVisualPluginMessage,sizeof(playerMessageInfo.u.registerVisualPluginMessage));
    BlockMoveData((Ptr)&pluginName[0],(Ptr)&playerMessageInfo.u.registerVisualPluginMessage.name[0],pluginName[0] + 1);

    SetNumVersion(&playerMessageInfo.u.registerVisualPluginMessage.pluginVersion, kTVisualPluginMajorVersion, kTVisualPluginMinorVersion, kTVisualPluginReleaseStage, kTVisualPluginNonFinalRelease);

    playerMessageInfo.u.registerVisualPluginMessage.options			= kVisualWantsIdleMessages | kVisualWantsConfigure;
    playerMessageInfo.u.registerVisualPluginMessage.handler			= (VisualPluginProcPtr)VisualPluginHandler;
    playerMessageInfo.u.registerVisualPluginMessage.registerRefCon		= 0;
    playerMessageInfo.u.registerVisualPluginMessage.creator			= kTVisualPluginCreator;

    playerMessageInfo.u.registerVisualPluginMessage.timeBetweenDataInMS	= 0xFFFFFFFF; // 16 milliseconds = 1 Tick, 0xFFFFFFFF = Often as possible.
    playerMessageInfo.u.registerVisualPluginMessage.numWaveformChannels	= 2;
    playerMessageInfo.u.registerVisualPluginMessage.numSpectrumChannels	= 0;

    playerMessageInfo.u.registerVisualPluginMessage.minWidth		= 100;
    playerMessageInfo.u.registerVisualPluginMessage.minHeight		= 100;
    playerMessageInfo.u.registerVisualPluginMessage.maxWidth		= 2000;
    playerMessageInfo.u.registerVisualPluginMessage.maxHeight		= 2000;
    playerMessageInfo.u.registerVisualPluginMessage.minFullScreenBitDepth	= 32;
    playerMessageInfo.u.registerVisualPluginMessage.maxFullScreenBitDepth	= 32;
    playerMessageInfo.u.registerVisualPluginMessage.windowAlignmentInBytes	= 0;

    status = PlayerRegisterVisualPlugin(messageInfo->u.initMessage.appCookie,messageInfo->u.initMessage.appProc,&playerMessageInfo);
/*
    Gestalt(gestaltPowerPCProcessorFeatures,&CPU);
    if (1 & (CPU >> gestaltPowerPCHasVectorInstructions)) CPU_FLAVOUR = 1;
    else CPU_FLAVOUR = 0;
*/
    
    return status;
}

//	main entrypoint
OSStatus iTunesPluginMainMachO(OSType message,PluginMessageInfo *messageInfo,void *refCon)
{
    OSStatus		status;

    (void) refCon;

    switch (message)
    {
        case kPluginInitMessage:
            status = RegisterVisualPlugin(messageInfo);
            break;
        case kPluginCleanupMessage:
            status = noErr;
            break;
        default:
            status = unimpErr;
            break;
    }

    return status;
}
