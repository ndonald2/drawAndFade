//
//  SetupWindowController.h
//
//  Created by Nicholas Donaldson on 5/5/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "ofxWindowAppDelegate.h"
#import <AppKit/AppKit.h>

@interface SetupWindowController : NSWindowController
{
    NSPopUpButton *_resBox;
    NSButton *_fullscreenCheck;
    NSPopUpButton *_audioInputBox;
    NSTextField *_oscListenPortField;
    NSButton *_startButton;
    
    NSMutableArray *_audioDevices;
}

@property (retain, nonatomic) IBOutlet NSPopUpButton *resBox;
@property (retain, nonatomic) IBOutlet NSButton *fullscreenCheck;
@property (retain, nonatomic) IBOutlet NSPopUpButton *audioInputBox;
@property (retain, nonatomic) IBOutlet NSTextField *oscListenPortField;
@property (retain, nonatomic) IBOutlet NSButton *startButton;
@property (retain, nonatomic) NSMutableArray *audioDevices;

- (IBAction)startPressed:(id)sender;

@end
