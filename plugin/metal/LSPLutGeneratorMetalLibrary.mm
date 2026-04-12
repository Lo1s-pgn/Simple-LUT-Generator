#import "LSPLutGeneratorMetalLibrary.h"
#include "LSPLutGeneratorLog.h"

#include <string>

NSString* LspLutGenCopyMetalShaderSource(NSError** outErr) {
    const std::string root = LSPLutGeneratorLog::getPluginBundleRootPath();
    if (root.empty()) {
        if (outErr)
            *outErr = [NSError errorWithDomain:@"LspLutGen" code:10
                                       userInfo:@{NSLocalizedDescriptionKey: @"empty bundle path"}];
        return nil;
    }
    NSString* path = [NSString stringWithUTF8String:root.c_str()];
    NSBundle* bundle = [NSBundle bundleWithPath:path];
    NSURL* url = [bundle URLForResource:@"LSPLutGeneratorMetal" withExtension:@"metal"];
    if (!url) {
        NSString* fallback = [NSString stringWithFormat:@"%s/Contents/Resources/LSPLutGeneratorMetal.metal", root.c_str()];
        if ([[NSFileManager defaultManager] fileExistsAtPath:fallback])
            url = [NSURL fileURLWithPath:fallback];
    }
    if (!url) {
        if (outErr)
            *outErr = [NSError errorWithDomain:@"LspLutGen" code:11
                                       userInfo:@{NSLocalizedDescriptionKey: @"LSPLutGeneratorMetal.metal not in bundle"}];
        return nil;
    }
    return [NSString stringWithContentsOfURL:url encoding:NSUTF8StringEncoding error:outErr];
}

id<MTLLibrary> LspLutGenGetMetalLibraryForDevice(id<MTLDevice> device, NSError** outErr) {
    if (!device)
        return nil;
    static NSMutableDictionary<NSString*, id<MTLLibrary>>* cache = nil;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        cache = [NSMutableDictionary dictionary];
    });
    NSString* key = [NSString stringWithFormat:@"%p", (void*)device];
    id<MTLLibrary> lib = cache[key];
    if (lib)
        return lib;
    NSString* src = LspLutGenCopyMetalShaderSource(outErr);
    if (!src)
        return nil;
    NSError* err = nil;
    lib = [device newLibraryWithSource:src options:nil error:&err];
    if (!lib) {
        if (outErr)
            *outErr = err;
        return nil;
    }
    cache[key] = lib;
    return lib;
}
