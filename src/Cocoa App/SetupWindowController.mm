//
//  SetupWindowController.m
//
//  Created by Nicholas Donaldson on 5/5/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//


#import "SetupWindowController.h"
#import "RtAudio.h"
#import "RtMidi.h"
#import "ofApplication.h"

#define kPreviousResKey             @"previous_resolution"
#define kPreviousFullscreenKey      @"previous_fullscreen"
#define kPreviousAudioDeviceKey     @"previous_audio_device"
#define kPreviousMidiInputDeviceKey @"previous_midi_input"

#define kAudioDeviceName            @"device_name"
#define kAudioDeviceIndex           @"device_index"
#define kAudioDeviceInputChannels   @"device_input_channels"

@interface NSString (ResolutionCompare)

-(NSComparisonResult)resolutionCompare:(NSString*)resolution;

@end

@implementation SetupWindowController

@synthesize resBox = _resBox;
@synthesize fullscreenCheck = _fullscreenCheck;
@synthesize audioInputBox = _audioInputBox;
@synthesize midiInputBox = _midiInputBox;
@synthesize startButton = _startButton;
@synthesize audioDevices = _audioDevices;

-(void)dealloc{
    [_resBox release];
    [_fullscreenCheck release];
    [_audioInputBox release];
    [_midiInputBox release];
    [_startButton release];
    [_audioDevices release];
    [super dealloc];
}

-(void)windowDidLoad{
    
    [super windowDidLoad];
    
    [self.startButton becomeFirstResponder];
    
    // resolutions
    CFArrayRef allResolutions = CGDisplayCopyAllDisplayModes(CGMainDisplayID(), NULL);
    CFIndex nResolutions = CFArrayGetCount(allResolutions);
    
    NSMutableSet *dedupedRes = [NSMutableSet setWithCapacity:nResolutions];
        
    for (int dm=nResolutions-1; dm >0; dm--){
        CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(allResolutions, dm);        
        NSString *resString = [NSString stringWithFormat:@"%li x %li", CGDisplayModeGetWidth(mode), CGDisplayModeGetHeight(mode)];
        
        // no duplicates
        if (![dedupedRes containsObject:resString])
            [dedupedRes addObject:resString];
    }
    
    NSArray *resStrings = [[dedupedRes allObjects] sortedArrayUsingSelector:@selector(resolutionCompare:)];
    
    [self.resBox addItemWithTitle:@"320 x 240"];
    [self.resBox addItemsWithTitles:resStrings];
    
    NSString *lastResolution = [[NSUserDefaults standardUserDefaults] objectForKey:kPreviousResKey];
    if (lastResolution != nil){
        [self.resBox selectItemWithTitle:lastResolution];
    }
    else{    
        // select current resolution
        CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode(CGMainDisplayID());
        NSString *currentModeString = [NSString stringWithFormat:@"%li x %li", CGDisplayModeGetWidth(currentMode), CGDisplayModeGetHeight(currentMode)];
        [self.resBox selectItemWithTitle:currentModeString];
    }
    
    CFRelease(allResolutions);
    
    // Previous fullscreen state
    BOOL prevFullscreen = [[[NSUserDefaults standardUserDefaults] objectForKey:kPreviousFullscreenKey] boolValue];
    [self.fullscreenCheck setState: prevFullscreen ? NSOnState : NSOffState];
    
    // Audio inputs
    RtAudio rtAudio;
    unsigned int nDevices = rtAudio.getDeviceCount();
    if (nDevices){
        
        self.audioDevices = [NSMutableArray arrayWithCapacity:nDevices];
        
        RtAudio::DeviceInfo deviceInfo;
        for (int i=0; i<nDevices; i++){
            try{
                deviceInfo = rtAudio.getDeviceInfo(i);
            } catch (RtError &error){
                continue;
            }
        
            if (deviceInfo.inputChannels > 0){
                
                NSMutableDictionary *deviceDict = [NSMutableDictionary dictionaryWithCapacity:3];
                NSString *name = [NSString stringWithUTF8String:deviceInfo.name.c_str()];
                
                if (name){
                    [deviceDict setObject:name forKey:kAudioDeviceName];
                    [deviceDict setObject:[NSNumber numberWithInt:deviceInfo.inputChannels] forKey:kAudioDeviceInputChannels];
                    [deviceDict setObject:[NSNumber numberWithInt:i] forKey:kAudioDeviceIndex];
                    [self.audioDevices addObject:deviceDict];
                    [self.audioInputBox addItemWithTitle:name];
                }
            }
        }
        
        NSString *prevDeviceName = [[NSUserDefaults standardUserDefaults] objectForKey:kPreviousAudioDeviceKey];
        if (prevDeviceName && [self.audioInputBox.itemTitles containsObject:prevDeviceName])
        {
            [self.audioInputBox selectItemWithTitle:prevDeviceName];
        }
        else{
            [self.audioInputBox selectItemAtIndex:0];
        }
    }
    
    // midi inputs
    RtMidiIn rtMidi(RtMidi::MACOSX_CORE);
    int nMidiInputs = rtMidi.getPortCount();
    if (nMidiInputs > 0){
                
        for (int i=0; i<nMidiInputs; i++){
            NSString *inputName = [NSString stringWithUTF8String:rtMidi.getPortName(i).c_str()];
            [self.midiInputBox addItemWithTitle:inputName];
        }
        
        NSString *prevDeviceName = [[NSUserDefaults standardUserDefaults] objectForKey:kPreviousMidiInputDeviceKey];
        if (prevDeviceName && [self.audioInputBox.itemTitles containsObject:prevDeviceName])
        {
            [self.midiInputBox selectItemWithTitle:prevDeviceName];
        }
        else{
            [self.midiInputBox selectItemAtIndex:0];
        }
    
    }
}

- (IBAction)startPressed:(id)sender{
    
    NSString *selectedRes = [self.resBox titleOfSelectedItem];
    NSArray *resComponents = [selectedRes componentsSeparatedByString:@" x "];
    BOOL fullscreen = self.fullscreenCheck.state == NSOnState;
    
    NSDictionary *audioDevice = [self.audioDevices objectAtIndex:[self.audioInputBox indexOfSelectedItem]];
    NSString *audioDeviceName = [audioDevice objectForKey:kAudioDeviceName];
    int deviceIndex = [[audioDevice objectForKey:kAudioDeviceIndex] intValue];
    ofApplicationSetAudioInputDeviceId(deviceIndex);
    ofApplicationSetMidiInputDeviceId(self.midiInputBox.indexOfSelectedItem);
    
    if (resComponents.count != 2){
        // error message
        return;
    }
    else{
        int width = [[resComponents objectAtIndex:0] intValue];
        int height = [[resComponents objectAtIndex:1] intValue];
        [self.window performClose:nil];
        [(ofxWindowAppDelegate*)[[NSApplication sharedApplication] delegate] launchGLWindowWithResolution:CGSizeMake(width, height) fullscreen:self.fullscreenCheck.state == NSOnState];
        
        [[NSUserDefaults standardUserDefaults] setObject:selectedRes forKey:kPreviousResKey];
        [[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithBool:fullscreen] forKey:kPreviousFullscreenKey];
        
        if (audioDeviceName != nil){
            [[NSUserDefaults standardUserDefaults] setObject:audioDeviceName forKey:kPreviousAudioDeviceKey];
        }
        
        NSString *midiDevice = [self.midiInputBox titleOfSelectedItem];
        if (midiDevice){
            [[NSUserDefaults standardUserDefaults] setObject:midiDevice forKey:kPreviousMidiInputDeviceKey];
        }
        
        [[NSUserDefaults standardUserDefaults] synchronize];
    }
}


@end

@implementation NSString (ResolutionCompare)

-(NSComparisonResult)resolutionCompare:(NSString*)resolution{
    
    NSArray *recComponents = [self componentsSeparatedByString:@" x "];
    NSArray *argComponents = [resolution componentsSeparatedByString:@" x "];
    
    if (recComponents.count != 2 || argComponents.count != 2)
        return NSOrderedSame;
    
    if ([[recComponents objectAtIndex:0] intValue] < [[argComponents objectAtIndex:0] intValue]){
        return NSOrderedAscending;
    }
    else if ([[recComponents objectAtIndex:0] intValue] > [[argComponents objectAtIndex:0] intValue])
    {
        return NSOrderedDescending;
    }
    else{
        if ([[recComponents objectAtIndex:1] intValue] < [[argComponents objectAtIndex:1] intValue]){
            return NSOrderedAscending;
        }
        else if ([[recComponents objectAtIndex:1] intValue] > [[argComponents objectAtIndex:1] intValue])
        {
            return NSOrderedDescending;
        }
    }
    
    return NSOrderedSame;
    
}

@end
