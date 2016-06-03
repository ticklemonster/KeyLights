//
//  HIDKeyboardLEDs.c
//  KeyLights
//
//  Created by Matt Clements on 23/03/2015.
//  Copyright (c) 2015 Matt Clements. All rights reserved.
//

#include "HIDKeyboardLEDs.h"

// private definitions - outside of header file
CFMutableDictionaryRef hu_CreateMatchingDictionaryUsagePageUsage( Boolean, UInt32, UInt32 );
//

CFDictionaryRef getKeyboards() {
    //
    // C FUNCTIONS FOR IOHIDDEVICE HANDLING
    //
    IOHIDManagerRef tIOHIDManagerRef = IOHIDManagerCreate( kCFAllocatorDefault, kIOHIDOptionsTypeNone );
    IOHIDDeviceRef *tIOHIDDeviceRefs = nil;
    CFMutableDictionaryRef rval = NULL;
    
    // Create a device matching dictionary for Keyboards only
    CFDictionaryRef matchingCFDictRef = hu_CreateMatchingDictionaryUsagePageUsage( TRUE, kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard );
    IOHIDManagerSetDeviceMatching( tIOHIDManagerRef, matchingCFDictRef );
    if ( matchingCFDictRef ) {
        CFRelease( matchingCFDictRef );
    }
    
    // Now fetch the list of keyboards
    /*IOReturn tIOReturn =*/ IOHIDManagerOpen( tIOHIDManagerRef, kIOHIDOptionsTypeNone );
    CFSetRef deviceCFSetRef = IOHIDManagerCopyDevices( tIOHIDManagerRef );
    CFIndex deviceIndex, deviceCount = CFSetGetCount( deviceCFSetRef );
    
    // allocate a block of memory to extact the device ref's from the set into
    tIOHIDDeviceRefs = (IOHIDDeviceRef *)malloc( sizeof( IOHIDDeviceRef ) * deviceCount );
    if (!tIOHIDDeviceRefs) {
        CFRelease(deviceCFSetRef);
        deviceCFSetRef = NULL;
    }
    CFSetGetValues( deviceCFSetRef, (const void **) tIOHIDDeviceRefs );
    CFRelease(deviceCFSetRef);
    deviceCFSetRef = NULL;
    
    // Run through each keyboard and add them to the list...
    rval = CFDictionaryCreateMutable(kCFAllocatorDefault, deviceCount, NULL, NULL);
    for( deviceIndex=0; deviceIndex < deviceCount; deviceIndex++ ) {
        KeyboardInfo_t *keyboard = (KeyboardInfo_t *)malloc( sizeof(KeyboardInfo_t) );
        keyboard->deviceRef = tIOHIDDeviceRefs[deviceIndex];
        keyboard->deviceName = (CFStringRef) IOHIDDeviceGetProperty(keyboard->deviceRef, CFSTR(kIOHIDProductKey) );
#ifndef NDEBUG
        printf("Keyboard: %s (%p)\n",
               CFStringGetCStringPtr(keyboard->deviceName,kCFStringEncodingUTF8), keyboard->deviceRef );
#endif
        // for each keyboard, find all the LED elements...
        CFArrayRef elementCFArrayRef = IOHIDDeviceCopyMatchingElements( keyboard->deviceRef, NULL, kIOHIDOptionsTypeNone );
        CFIndex elementCount = CFArrayGetCount( elementCFArrayRef );
        CFMutableArrayRef tempLEDs = CFArrayCreateMutable(kCFAllocatorDefault, elementCount, NULL);
        for ( CFIndex elementIndex = 0; elementIndex < elementCount; elementIndex++ ) {
            KeyboardLEDInfo_t *led = (KeyboardLEDInfo_t *)malloc( sizeof(KeyboardLEDInfo_t) );
            
            led->elementRef = ( IOHIDElementRef ) CFArrayGetValueAtIndex( elementCFArrayRef, elementIndex );
            uint32_t usagePage = IOHIDElementGetUsagePage( led->elementRef );
            if ( kHIDPage_LEDs != usagePage ) {
                // Ignore it if this is not an LED
                free( led );
            } else {
                // Save it
                uint32_t usage = IOHIDElementGetUsage( led->elementRef );
                led->minCFIndex = IOHIDElementGetLogicalMin( led->elementRef );
                led->maxCFIndex = IOHIDElementGetLogicalMax( led->elementRef );
                IOHIDValueRef valueRef; // = IOHIDValueCreateWithIntegerValue( kCFAllocatorDefault, led->elementRef, 0, led->minCFIndex );
                IOHIDDeviceGetValue(keyboard->deviceRef, led->elementRef, &valueRef);
                led->value = IOHIDValueGetIntegerValue(valueRef);
                CFMutableStringRef str;
                if( IOHIDElementGetName( led->elementRef ) )
                    str = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, IOHIDElementGetName( led->elementRef ));
                else
                    str = CFStringCreateMutable(kCFAllocatorDefault,0);
                CFStringAppendFormat(str, NULL, CFSTR(".%d"), usage );
                led->elementName = str;
#ifndef NDEBUG
                printf( "\tLED element = %s #%d (%ld-%ld) = %ld\n",
                       CFStringGetCStringPtr(led->elementName,kCFStringEncodingUTF8), usage, led->minCFIndex, led->maxCFIndex, led->value );
#endif
                CFArrayAppendValue(tempLEDs, led);
            }
        }
        
        keyboard->LEDs = tempLEDs;
        CFDictionaryAddValue(rval, keyboard->deviceName, keyboard);
    }
    
    return rval;
}


bool setKeyboardLED( IOHIDDeviceRef deviceRef, IOHIDElementRef elementRef, CFIndex value ) {
    //
    // C FUNCTIONS FOR IOHIDDEVICE HANDLING
    //
    // IOHIDManagerRef tIOHIDManagerRef = IOHIDManagerCreate( kCFAllocatorDefault, kIOHIDOptionsTypeNone );
    // IOHIDDeviceRef * tIOHIDDeviceRefs = nil;
    // CFMutableDictionaryRef rval = NULL;

    IOHIDValueRef tIOHIDValueRef = IOHIDValueCreateWithIntegerValue( kCFAllocatorDefault, elementRef, 0, value );
    IOReturn res = IOHIDDeviceSetValue( deviceRef, elementRef, tIOHIDValueRef );
    CFRelease( tIOHIDValueRef );
    
    return (res == kIOReturnSuccess);
}



// ----------------------------------------------------
// function to create a matching dictionary for usage page & usage
CFMutableDictionaryRef hu_CreateMatchingDictionaryUsagePageUsage( Boolean isDeviceNotElement,
                                                                 UInt32 inUsagePage,
                                                                 UInt32 inUsage )
{
    // create a dictionary to add usage page / usages to
    CFMutableDictionaryRef result = CFDictionaryCreateMutable( kCFAllocatorDefault,
                                                              0,
                                                              &kCFTypeDictionaryKeyCallBacks,
                                                              &kCFTypeDictionaryValueCallBacks );
    
    if ( result ) {
        if ( inUsagePage ) {
            // Add key for device type to refine the matching dictionary.
            CFNumberRef pageCFNumberRef = CFNumberCreate( kCFAllocatorDefault, kCFNumberIntType, &inUsagePage );
            
            if ( pageCFNumberRef ) {
                if ( isDeviceNotElement ) {
                    CFDictionarySetValue( result, CFSTR( kIOHIDDeviceUsagePageKey ), pageCFNumberRef );
                } else {
                    CFDictionarySetValue( result, CFSTR( kIOHIDElementUsagePageKey ), pageCFNumberRef );
                }
                CFRelease( pageCFNumberRef );
                
                // note: the usage is only valid if the usage page is also defined
                if ( inUsage ) {
                    CFNumberRef usageCFNumberRef = CFNumberCreate( kCFAllocatorDefault, kCFNumberIntType, &inUsage );
                    
                    if ( usageCFNumberRef ) {
                        if ( isDeviceNotElement ) {
                            CFDictionarySetValue( result, CFSTR( kIOHIDDeviceUsageKey ), usageCFNumberRef );
                        } else {
                            CFDictionarySetValue( result, CFSTR( kIOHIDElementUsageKey ), usageCFNumberRef );
                        }
                        CFRelease( usageCFNumberRef );
                    } else {
                        fprintf( stderr, "%s: CFNumberCreate( usage ) failed.", __PRETTY_FUNCTION__ );
                    }
                }
            } else {
                fprintf( stderr, "%s: CFNumberCreate( usage page ) failed.", __PRETTY_FUNCTION__ );
            }
        }
    } else {
        fprintf( stderr, "%s: CFDictionaryCreateMutable failed.", __PRETTY_FUNCTION__ );
    }
    return result;
}	// hu_CreateMatchingDictionaryUsagePageUsage


