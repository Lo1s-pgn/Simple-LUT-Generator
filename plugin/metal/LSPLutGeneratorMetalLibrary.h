#pragma once

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

NSString* LspLutGenCopyMetalShaderSource(NSError** outErr);
id<MTLLibrary> LspLutGenGetMetalLibraryForDevice(id<MTLDevice> device, NSError** outErr);
