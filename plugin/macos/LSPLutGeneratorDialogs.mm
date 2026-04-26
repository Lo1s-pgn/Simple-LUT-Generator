// Objective-C++: folder chooser for export path; run on main queue (OFX UI rule).
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

static std::string LSPLutGenShowChooseFolderDialogImpl(const char* p_DefaultDir) {
    std::string result;
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:NO];
        [panel setCanChooseDirectories:YES];
        [panel setAllowsMultipleSelection:NO];
        [panel setCanCreateDirectories:YES];
        [panel setTitle:@"Choose export folder"];
        if (p_DefaultDir && p_DefaultDir[0] != '\0') {
            NSString* dir = [NSString stringWithUTF8String:p_DefaultDir];
            if (dir)
                [panel setDirectoryURL:[NSURL fileURLWithPath:dir isDirectory:YES]];
        }
        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = [panel URL];
            if (url) {
                NSString* path = [url path];
                if (path)
                    result = [path UTF8String];
            }
        }
    }
    return result;
}

std::string LSPLutGenShowChooseFolderDialog(const char* p_DefaultDir) {
    return LSPLutGenRunOnMainString(^{ return LSPLutGenShowChooseFolderDialogImpl(p_DefaultDir); });
}
