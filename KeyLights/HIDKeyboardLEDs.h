//
//  HIDKeyboardLEDs.h
//  KeyLights
//
//  Created by Matt Clements on 23/03/2015.
//  Copyright (c) 2015 Matt Clements. All rights reserved.
//

#ifndef __KeyLights__HIDKeyboardLEDs__
#define __KeyLights__HIDKeyboardLEDs__

#include <stdio.h>
#include <IOKit/hid/IOHIDManager.h>

struct struct_KeyboardInfo {
    CFStringRef     deviceName;
    IOHIDDeviceRef  deviceRef;
    CFArrayRef      LEDs;
};
typedef struct struct_KeyboardInfo KeyboardInfo_t;
typedef KeyboardInfo_t * KeyboardInfoRef;

struct struct_KeyboardLEDInfo {
    CFStringRef     elementName;
    IOHIDElementRef elementRef;
    CFIndex         minCFIndex;
    CFIndex         maxCFIndex;
    CFIndex         value;
};
typedef struct struct_KeyboardLEDInfo KeyboardLEDInfo_t;
typedef KeyboardLEDInfo_t * KeyboardLEDInfoRef;

CFDictionaryRef getKeyboards();
bool setKeyboardLED( IOHIDDeviceRef , IOHIDElementRef , CFIndex  );


#endif /* defined(__KeyLights__HIDKeyboardLEDs__) */
