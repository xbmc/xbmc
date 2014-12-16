//
//  GoomFXParam.m
//  iGoom copie
//
//  Created by Guillaume Borios on Sun Jul 20 2003.
//  Copyright (c) 2003 iOS. All rights reserved.
//

#import "GoomFXParam.h"

NSMutableDictionary * paramlist = nil;

void goom_input_stub(PluginParam *_this)
{
  [(GoomFXParam*)[paramlist objectForKey:[NSString stringWithFormat:@"%p",_this]] setValue:nil];
}

@implementation GoomFXParam

- (GoomFXParam*)initWithParam:(PluginParam*)p
{
  self = [super init];

  if (self)
  {
    parametres = p;
    if (paramlist == nil) paramlist = [[NSMutableDictionary alloc] init];
    [paramlist setObject:self forKey:[NSString stringWithFormat:@"%p",p]];
  }
  return self;
}

- (void)setValue:(id)sender
{

  switch (parametres->type)
  {
    case PARAM_INTVAL:
      if (parametres->rw == TRUE)
      {
	if (sender)
        {
            parametres->param.ival.value = [sender intValue];
            parametres->changed(parametres);
        }
	else [slider setIntValue:parametres->param.ival.value];
	[value setIntValue:parametres->param.ival.value];
      }
      else
      {
	[progress setDoubleValue:(double)parametres->param.ival.value];
      }
      break;
    case PARAM_FLOATVAL:
      if (parametres->rw == TRUE)
      {
	if (sender) 
        {
            parametres->param.fval.value = (float)[sender doubleValue];
            parametres->changed(parametres);
        }
	else [slider setDoubleValue:(double)parametres->param.fval.value];
	[value setDoubleValue:(double)parametres->param.fval.value];
      }
      else
      {
	[progress setDoubleValue:(double)parametres->param.fval.value];
      }
      break;
    case PARAM_BOOLVAL:
        if ((parametres->rw == TRUE) && (sender != nil))
        {
            parametres->param.bval.value = ([sender state]==NSOnState);
            parametres->changed(parametres);
        }
        else
        {
            [button setState:(parametres->param.bval.value == YES)?NSOnState:NSOffState];
        }
      break;
    case PARAM_STRVAL:
      break;
    case PARAM_LISTVAL:
      break;
    default:
      break;
  }
}

- (NSView*)makeViewAtHeight:(float)h
{
  NSView * container;
  NSTextField * text;

  container = [[NSView alloc] initWithFrame:NSMakeRect(20,h,420,25)];

  if (parametres->type != PARAM_BOOLVAL)
  {
          text = [[NSTextField alloc] initWithFrame:NSMakeRect(0,5,214,15)];
          [text setStringValue:[NSString stringWithCString:parametres->name]];
          [text setDrawsBackground:NO];
          [text setSelectable:NO];
          [text setEditable:NO];
          [text setBordered:NO];
          [container addSubview:[text autorelease]];
  }
  
  
  if (parametres->rw == TRUE)
  {
    switch (parametres->type)
    {
      case PARAM_INTVAL:
      case PARAM_FLOATVAL:
	// Value text field
	value = [[NSTextField alloc] initWithFrame:NSMakeRect(374,5,214,15)];
	[value setDrawsBackground:NO];
	[value setSelectable:NO];
	[value setEditable:NO];
	[value setBordered:NO];

	[container addSubview:[value autorelease]];
	//slider
	slider = [[NSSlider alloc] initWithFrame:NSMakeRect(222,2,144,20)];
	[slider setAction:@selector(setValue:)];
	[slider setTickMarkPosition:NSTickMarkAbove];
	[slider setTarget:self];
	[container addSubview:[slider autorelease]];
	//values
	if (parametres->type == PARAM_INTVAL)
	{
	  [value setIntValue:parametres->param.ival.value];
	  [slider setMaxValue:(double)parametres->param.ival.max];
	  [slider setMinValue:(double)parametres->param.ival.min];
	  [slider setIntValue:parametres->param.ival.value];
	}
	  else
	  {
	    [value setDoubleValue:parametres->param.fval.value];
	    [[value cell] setFloatingPointFormat:YES left:1 right:1];
	    [slider setMaxValue:(double)parametres->param.fval.max];
	    [slider setMinValue:(double)parametres->param.fval.min];
	    [slider setDoubleValue:(double)parametres->param.fval.value];
	  }
	  break;
      case PARAM_BOOLVAL:
          button = [[NSButton alloc] initWithFrame:NSMakeRect(0,2,344,18)];
          [button setEnabled:YES];
          [button setState:(parametres->param.bval.value == YES)?NSOnState:NSOffState];
          [button setTitle:[NSString stringWithCString:parametres->name]];
          [button setButtonType:NSSwitchButton];
          [button setTransparent:NO];
          [button setTarget:self];
          [button setAction:@selector(setValue:)];
          [container addSubview:[button autorelease]];
          break;
      case PARAM_STRVAL:
          NSLog(@"PARAM_STRVAL rw not implemented");
	break;
      case PARAM_LISTVAL:
          NSLog(@"PARAM_LISTVAL rw not implemented");
	break;
      default:
	break;
    }
  }
  else
  {
    switch (parametres->type)
    {
      case PARAM_INTVAL:
	//slider
	progress = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(222,7,144,10)];
	[progress setMaxValue:(double)parametres->param.ival.max];
	[progress setMinValue:(double)parametres->param.ival.min];
	[progress setDoubleValue:(double)parametres->param.ival.value];
	[progress setIndeterminate:NO];
	[container addSubview:[progress autorelease]];
	break;
      case PARAM_FLOATVAL:
	//slider
	progress = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(222,7,144,10)];
	[progress setMaxValue:(double)parametres->param.fval.max];
	[progress setMinValue:(double)parametres->param.fval.min];
	[progress setDoubleValue:(double)parametres->param.fval.value];
	[progress setIndeterminate:NO];
	[container addSubview:[progress autorelease]];
	break;
      case PARAM_BOOLVAL:
          button = [[NSButton alloc] initWithFrame:NSMakeRect(0,2,344,18)];
          [button setEnabled:NO];
          [button setState:(parametres->param.bval.value == YES)?NSOnState:NSOffState];
          [button setTitle:[NSString stringWithCString:parametres->name]];
          [button setButtonType:NSSwitchButton];
          [button setTransparent:NO];
          [container addSubview:[button autorelease]];
          break;
      case PARAM_STRVAL:
          NSLog(@"PARAM_STRVAL ro not implemented");
	break;
      case PARAM_LISTVAL:
          NSLog(@"PARAM_LISTVAL ro not implemented");
	break;
      default:
	break;
    }
    parametres->change_listener = goom_input_stub;
  }

  return [container autorelease];
}

@end
