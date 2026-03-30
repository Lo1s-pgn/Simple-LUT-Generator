// Objective-C++: save panel for Export LUT; hop to main queue when called from OFX worker.
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#include "LSPLutGeneratorDialogs.h"
#include <string>

static std::string LSPLutGenRunOnMainString(std::string (^p_Block)(void)) {
    if ([NSThread isMainThread])
        return p_Block();
    __block std::string result;
    dispatch_sync(dispatch_get_main_queue(), ^{ result = p_Block(); });
    return result;
}

static std::string LSPLutGenShowSaveLUTDialogImpl(const char* p_DefaultDir) {
    std::string result;
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        [panel setTitle:@"Export analyzed LUT"];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        [panel setAllowedFileTypes:@[ @"cube" ]];
#pragma clang diagnostic pop
        [panel setCanCreateDirectories:YES];
        if (p_DefaultDir && p_DefaultDir[0] != '\0') {
            NSString* dir = [NSString stringWithUTF8String:p_DefaultDir];
            if (dir)
                [panel setDirectoryURL:[NSURL fileURLWithPath:dir]];
        }
        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = [panel URL];
            if (url)
                result = [[url path] UTF8String];
        }
    }
    return result;
}

std::string LSPLutGenShowSaveLUTDialog(const char* p_DefaultDir) {
    return LSPLutGenRunOnMainString(^{ return LSPLutGenShowSaveLUTDialogImpl(p_DefaultDir); });
}
