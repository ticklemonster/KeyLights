//
//  AppDelegate.m
//  KeyLights
//
//  Created by Matt Clements on 18/03/2015.
//  Copyright (c) 2015 Matt Clements. All rights reserved.
//

#import "AppDelegate.h"
#import "HIDKeyboardLEDs.h"

@interface AppDelegate ()

@property (weak) IBOutlet NSWindow *window;
@property (weak) IBOutlet NSPopUpButton *devicesPullDown;
- (IBAction)deviceSelected:(NSPopUpButton *)sender;
- (IBAction)pushedLED:(NSButton *)sender;

@end

@implementation AppDelegate

NSDictionary        *keyboards;
NSArray             *buttons;
KeyboardInfoRef     selectedDevice;

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {

    // Fetch all the keboards
    keyboards = (__bridge NSDictionary *)(getKeyboards());
    
    // Load the list of keyboards into the dropdown list
    NSEnumerator *enumerator = [keyboards keyEnumerator];
    id key;
    while ((key = [enumerator nextObject])) {
        [_devicesPullDown addItemWithTitle:key];
    }
    
    // Gather all the NSButtons into an array
    // (there's probably a better way to do this)
    NSMutableArray *btns = [NSMutableArray array];
    for( NSView *btn in [[_window contentView] subviews] ) {
        if( [btn isKindOfClass:[NSButton class]] && ![btn isKindOfClass:[NSPopUpButton class]]) {
            NSButton *b = (NSButton *)btn;
            
            [btns addObject:b];
            b.tag = -1;
        }
    }
    buttons = btns;
    
    [self loadDefaults];
}


- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
    [self saveDefaults];
}


- (IBAction)deviceSelected:(NSPopUpButton *)sender {
    NSString *deviceName = [sender titleOfSelectedItem];
    [self loadDeviceDetail:deviceName];
}

- (IBAction)pushedLED:(NSButton *)sender {
    if( !selectedDevice ) return;

    // send either the Min or Max value to the LED
    Boolean bON = ([sender state] == NSOnState);
    NSInteger tag = [sender tag];
    
    // get the keyboard from the CFKeyboard dictionary for the current device
    CFIndex nLED = CFArrayGetCount( selectedDevice->LEDs );
    if( tag < 0 || tag >= nLED ) return;
    
    KeyboardLEDInfoRef LED = (KeyboardLEDInfoRef) CFArrayGetValueAtIndex(selectedDevice->LEDs,tag);
    if( !LED ) return;
    
    setKeyboardLED( selectedDevice->deviceRef, LED->elementRef, bON?LED->maxCFIndex:LED->minCFIndex );
    
}

//
// Internal functions
//
- (void)loadDeviceDetail:(NSString *)deviceName {
    selectedDevice = NULL;
    for( NSString* key in [keyboards allKeys] ) {
        if( [key isEqualToString:deviceName] ) {
            selectedDevice = (__bridge KeyboardInfoRef)[keyboards valueForKey:key];
        }
    }
    if( !selectedDevice ) return;
    
    // Enable buttons according to the available LED buttons
    [_devicesPullDown setTitle:deviceName];
    
#ifndef NDEBUG
    printf("deviceSelected: %s (%ld LEDs)\n",
           [deviceName cStringUsingEncoding:NSUTF8StringEncoding],
           CFArrayGetCount(selectedDevice->LEDs) );
#endif
    
    CFIndex nLED = CFArrayGetCount(selectedDevice->LEDs);
    for( CFIndex n=0; n < [buttons count]; n++ ) {
        NSButton *btn = (NSButton *)[buttons objectAtIndex:n];
        if( n<nLED ) {
            KeyboardLEDInfoRef LED = (KeyboardLEDInfoRef) CFArrayGetValueAtIndex(selectedDevice->LEDs,n);
            [btn setEnabled:true];
            [btn setTag:n];
            [btn setTitle:(__bridge NSString *)(LED->elementName) ];
            [btn setState:(LED->value==LED->maxCFIndex)?NSOnState:NSOffState];
        } else {
            [btn setEnabled:false];
            [btn setTitle:NULL];
        }
    }

}


- (void)saveDefaults {
    // Save the last state for next load!
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if( selectedDevice ) {
        [defaults setValue:(__bridge id)(selectedDevice->deviceName) forKey:@"LastDeviceName"];
        NSMutableArray *btnstate = [NSMutableArray arrayWithCapacity:[buttons count]];
        for( NSButton *b in buttons ) {
            NSNumber *state = [NSNumber numberWithInteger:[b state]];
            [btnstate addObject:state];
        }
        [defaults setValue:btnstate forKey:@"LastButtonState"];

#ifndef NDEBUG
        printf("saveDefault: deviceName=\"%s\"\n",
               [[defaults stringForKey:@"LastDeviceName"] cStringUsingEncoding:NSUTF8StringEncoding] );
#endif

    } else {
        [defaults removeObjectForKey:@"LastDeviceName"];
        [defaults removeObjectForKey:@"LastButtonState"];
    }
    
}

- (void)loadDefaults {
    NSString *lastDeviceName = [[NSUserDefaults standardUserDefaults] stringForKey:@"LastDeviceName"];
    NSArray *lastButtonState = [[NSUserDefaults standardUserDefaults] arrayForKey:@"LastButtonState"];

#ifndef NDEBUG
    printf("loadDefault: deviceName=\"%s\"\n",
           [lastDeviceName cStringUsingEncoding:NSUTF8StringEncoding] );
#endif
    
    [self loadDeviceDetail:lastDeviceName];
    if( selectedDevice ) {
        for( CFIndex n=0; n < [buttons count]; n++ ) {
            NSButton *btn = (NSButton *)[buttons objectAtIndex:n];
            NSNumber *ste = (NSNumber *)[lastButtonState objectAtIndex:n];
            
            if( ste && [btn state] != [ste integerValue]) {
                // if the states don't match, then change the button and the device state now
                NSInteger newstate = [ste integerValue];
                KeyboardLEDInfoRef LED = (KeyboardLEDInfoRef) CFArrayGetValueAtIndex(selectedDevice->LEDs,n);
                if( !LED ) break;
                
                [btn setState:newstate];
                setKeyboardLED( selectedDevice->deviceRef, LED->elementRef, (newstate==NSOnState)?LED->maxCFIndex:LED->minCFIndex );
                
            }
        }
    }
    
    
}


@end
