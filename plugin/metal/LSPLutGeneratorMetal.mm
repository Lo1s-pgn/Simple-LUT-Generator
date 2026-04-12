#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include "LSPLutGeneratorMetalBridge.h"
#import "LSPLutGeneratorMetalLibrary.h"
#include "LSPLutGeneratorLog.h"

#include <string>

// Byte layout must match LSPLutGeneratorMetal.metal.
typedef struct LspLutGenMetalUniforms {
    int32_t dst_x1;
    int32_t dst_y1;
    int32_t dst_x2;
    int32_t dst_y2;
    int32_t src_x1;
    int32_t src_y1;
    int32_t src_x2;
    int32_t src_y2;
    int32_t rw_x1;
    int32_t rw_y1;
    int32_t rw_x2;
    int32_t rw_y2;
    int32_t dst_row_bytes;
    int32_t src_row_bytes;
    int32_t mode;
    int32_t n;
    int32_t Tx;
    int32_t Ty;
    int32_t _pad0;
    int32_t _pad1;
} LspLutGenMetalUniforms;

static_assert(sizeof(LspLutGenMetalUniforms) == 80, "Metal uniforms mismatch shader");

bool lspLutGenMetalDispatch(void* pMetalCommandQueue,
                            void* pDstBuffer,
                            int32_t dstRowBytes,
                            void* pSrcBuffer,
                            int32_t srcRowBytes,
                            int32_t dst_x1,
                            int32_t dst_y1,
                            int32_t dst_x2,
                            int32_t dst_y2,
                            int32_t src_x1,
                            int32_t src_y1,
                            int32_t src_x2,
                            int32_t src_y2,
                            int32_t rw_x1,
                            int32_t rw_y1,
                            int32_t rw_x2,
                            int32_t rw_y2,
                            int32_t mode,
                            int32_t generateN,
                            int32_t patternTx,
                            int32_t patternTy) {
    if (!pMetalCommandQueue || !pDstBuffer)
        return false;
    if (mode != 0 && !pSrcBuffer)
        return false;

    const int rw = rw_x2 - rw_x1;
    const int rh = rw_y2 - rw_y1;
    if (rw <= 0 || rh <= 0)
        return true;

    id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)pMetalCommandQueue;
    id<MTLBuffer> dstBuf = (__bridge id<MTLBuffer>)pDstBuffer;
    id<MTLBuffer> srcBuf = pSrcBuffer ? (__bridge id<MTLBuffer>)pSrcBuffer : nil;
    id<MTLDevice> device = queue.device;

    static id<MTLComputePipelineState> pipeGen = nil;
    static id<MTLComputePipelineState> pipeCopy = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        NSError* err = nil;
        id<MTLLibrary> lib = LspLutGenGetMetalLibraryForDevice(device, &err);
        if (!lib) {
            std::string msg = "metal_library_failed";
            if (err)
                msg += std::string(": ") + [[err localizedDescription] UTF8String];
            LSP_LUTGEN_LOG_ERROR(msg);
            return;
        }
        id<MTLFunction> fnGen = [lib newFunctionWithName:@"lutgen_generate"];
        id<MTLFunction> fnCopy = [lib newFunctionWithName:@"lutgen_copy"];
        if (!fnGen || !fnCopy) {
            LSP_LUTGEN_LOG_ERROR("metal_function_lookup_failed");
            return;
        }
        NSError* pe = nil;
        pipeGen = [device newComputePipelineStateWithFunction:fnGen error:&pe];
        if (!pipeGen) {
            std::string msg = "metal_pipeline_gen";
            if (pe)
                msg += std::string(": ") + [[pe localizedDescription] UTF8String];
            LSP_LUTGEN_LOG_ERROR(msg);
        }
        pe = nil;
        pipeCopy = [device newComputePipelineStateWithFunction:fnCopy error:&pe];
        if (!pipeCopy) {
            std::string msg = "metal_pipeline_copy";
            if (pe)
                msg += std::string(": ") + [[pe localizedDescription] UTF8String];
            LSP_LUTGEN_LOG_ERROR(msg);
        }
    });

    if (!pipeGen || !pipeCopy)
        return false;

    LspLutGenMetalUniforms u = {};
    u.dst_x1 = dst_x1;
    u.dst_y1 = dst_y1;
    u.dst_x2 = dst_x2;
    u.dst_y2 = dst_y2;
    u.src_x1 = src_x1;
    u.src_y1 = src_y1;
    u.src_x2 = src_x2;
    u.src_y2 = src_y2;
    u.rw_x1 = rw_x1;
    u.rw_y1 = rw_y1;
    u.rw_x2 = rw_x2;
    u.rw_y2 = rw_y2;
    u.dst_row_bytes = dstRowBytes;
    u.src_row_bytes = srcRowBytes;
    u.mode = mode;
    u.n = generateN;
    u.Tx = patternTx;
    u.Ty = patternTy;

    id<MTLCommandBuffer> cmd = [queue commandBuffer];
    if (!cmd)
        return false;
    id<MTLComputeCommandEncoder> enc = [cmd computeCommandEncoder];
    if (!enc)
        return false;

    const NSUInteger w = (NSUInteger)rw;
    const NSUInteger h = (NSUInteger)rh;
    MTLSize grid = MTLSizeMake(w, h, 1);
    MTLSize tpg = MTLSizeMake(16, 16, 1);

    if (mode == 0) {
        [enc setComputePipelineState:pipeGen];
        [enc setBytes:&u length:sizeof(u) atIndex:0];
        [enc setBuffer:dstBuf offset:0 atIndex:1];
        [enc dispatchThreads:grid threadsPerThreadgroup:tpg];
    } else {
        [enc setComputePipelineState:pipeCopy];
        [enc setBytes:&u length:sizeof(u) atIndex:0];
        [enc setBuffer:dstBuf offset:0 atIndex:1];
        [enc setBuffer:srcBuf offset:0 atIndex:2];
        [enc dispatchThreads:grid threadsPerThreadgroup:tpg];
    }
    [enc endEncoding];
    [cmd commit];
    [cmd waitUntilCompleted];
    if (cmd.error) {
        std::string msg = "metal_command_failed";
        msg += std::string(": ") + [[cmd.error localizedDescription] UTF8String];
        LSP_LUTGEN_LOG_ERROR(msg);
        return false;
    }
    return true;
}
