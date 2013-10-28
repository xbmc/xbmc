#include "UpdateDialogCocoa.h"

#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

#include "AppInfo.h"
#include "Log.h"
#include "StringUtils.h"

@interface UpdateDialogDelegate : NSObject
{
	@public UpdateDialogPrivate* dialog;
}
- (void) finishClicked;
- (void) reportUpdateError:(id)arg;
- (void) reportUpdateProgress:(id)arg;
- (void) reportUpdateFinished:(id)arg;
@end

class UpdateDialogPrivate
{
	public:
		UpdateDialogPrivate()
		: hadError(false)
		{
		}

		UpdateDialogDelegate* delegate;
		NSAutoreleasePool* pool;
		NSWindow* window;
		NSButton* finishButton;
		NSTextField* progressLabel;
		NSProgressIndicator* progressBar;
		bool hadError;
};

@implementation UpdateDialogDelegate
- (void) finishClicked
{
	[NSApp stop:self];
}
- (void) reportUpdateError: (id)arg
{
	dialog->hadError = true;

	NSAlert* alert = [NSAlert 
		alertWithMessageText: @"Update Problem"
	    defaultButton: nil
	    alternateButton: nil
	    otherButton: nil
	    informativeTextWithFormat: @"There was a problem installing the update:\n\n%@", arg];
	[alert runModal];
}
- (void) reportUpdateProgress: (id)arg
{
	int percentage = [arg intValue];
	[dialog->progressBar setDoubleValue:(percentage/100.0)];
}
- (void) reportUpdateFinished: (id)arg
{
	NSMutableString* message = [[NSMutableString alloc] init];
	if (!dialog->hadError)
	{
		[message appendString:@"Updates installed."];
	}
	else
	{
		[message appendString:@"Update failed."];
	}

	[message appendString:@"  Click 'Finish' to restart the application."];	
	[dialog->progressLabel setTitleWithMnemonic:message];
	[message release];
}
@end

UpdateDialogCocoa::UpdateDialogCocoa()
: d(new UpdateDialogPrivate)
{
	[NSApplication sharedApplication];
	d->pool = [[NSAutoreleasePool alloc] init];
}

UpdateDialogCocoa::~UpdateDialogCocoa()
{
	[d->pool release];
}

void UpdateDialogCocoa::enableDockIcon()
{
	// convert the application to a foreground application and in
	// the process, enable the dock icon

	// the reverse transformation is not possible, according to
	//  http://stackoverflow.com/questions/2832961/is-it-possible-to-hide-the-dock-icon-programmatically 
	ProcessSerialNumber psn;
	GetCurrentProcess(&psn);
	TransformProcessType(&psn,kProcessTransformToForegroundApplication);
}

void UpdateDialogCocoa::init(int /* argc */, char** /* argv */)
{
	enableDockIcon();
	
	// make the updater the active application.  This does not
	// happen automatically because the updater starts as a
	// background application
	[NSApp activateIgnoringOtherApps:YES];

	d->delegate = [[UpdateDialogDelegate alloc] init];
	d->delegate->dialog = d;

	int width = 370;
	int height = 100;

	d->window = [[NSWindow alloc] initWithContentRect:NSMakeRect(200, 200, width, height)
	        styleMask:NSTitledWindowMask | NSMiniaturizableWindowMask
	        backing:NSBackingStoreBuffered defer:NO];
	[d->window setTitle:[NSString stringWithUTF8String:AppInfo::name().c_str()]];

	d->finishButton = [[NSButton alloc] init];
	[d->finishButton setTitle:@"Finish"];
	[d->finishButton setButtonType:NSMomentaryLightButton];
	[d->finishButton setBezelStyle:NSRoundedBezelStyle];
	[d->finishButton setTarget:d->delegate];
	[d->finishButton setAction:@selector(finishClicked)];

	d->progressBar = [[NSProgressIndicator alloc] init];
	[d->progressBar setIndeterminate:false];
	[d->progressBar setMinValue:0.0];
	[d->progressBar setMaxValue:1.0];

	d->progressLabel = [[NSTextField alloc] init];
	[d->progressLabel setEditable:false];
	[d->progressLabel setSelectable:false];
	[d->progressLabel setTitleWithMnemonic:@"Installing Updates"];
	[d->progressLabel setBezeled:false];
	[d->progressLabel setDrawsBackground:false];

	NSView* windowContent = [d->window contentView];
	[windowContent addSubview:d->progressLabel];
	[windowContent addSubview:d->progressBar];
	[windowContent addSubview:d->finishButton];

	[d->progressLabel setFrame:NSMakeRect(10,70,width - 10,20)];
	[d->progressBar setFrame:NSMakeRect(10,40,width - 20,20)];
	[d->finishButton setFrame:NSMakeRect(width - 85,5,80,30)];
}

void UpdateDialogCocoa::exec()
{
	[d->window makeKeyAndOrderFront:d->window];
	[d->window center];
	[NSApp run];
}

void UpdateDialogCocoa::updateError(const std::string& errorMessage)
{
	[d->delegate performSelectorOnMainThread:@selector(reportUpdateError:)
	             withObject:[NSString stringWithUTF8String:errorMessage.c_str()]
	             waitUntilDone:false];
}

void UpdateDialogCocoa::updateProgress(int percentage)
{
	[d->delegate performSelectorOnMainThread:@selector(reportUpdateProgress:)
	             withObject:[NSNumber numberWithInt:percentage]
	             waitUntilDone:false];
}

void UpdateDialogCocoa::updateFinished()
{
	[d->delegate performSelectorOnMainThread:@selector(reportUpdateFinished:)
	             withObject:nil
	             waitUntilDone:false];
	UpdateDialog::updateFinished();
}

void* UpdateDialogCocoa::createAutoreleasePool()
{
	return [[NSAutoreleasePool alloc] init];
}

void UpdateDialogCocoa::releaseAutoreleasePool(void* arg)
{
	[(id)arg release];
}

void UpdateDialogCocoa::quit()
{
	[NSApp performSelectorOnMainThread:@selector(stop:) withObject:d->delegate waitUntilDone:false];
}


