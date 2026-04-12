#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include "LSPLutGeneratorMetalCube.h"
#import "LSPLutGeneratorMetalLibrary.h"
#include "LSPLutGeneratorCube.h"
#include "LSPLutGeneratorLog.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

constexpr uint32_t kLutGenAtomicScale = 1048576u;

typedef struct {
    int32_t bx1;
    int32_t by1;
    int32_t bx2;
    int32_t by2;
    int32_t row_bytes;
    int32_t n;
    int32_t Tx;
    int32_t Ty;
    int32_t pad0;
    int32_t pad1;
    int32_t pad2;
    int32_t pad3;
} LspBinUniforms;

static_assert(sizeof(LspBinUniforms) == 48, "LspBinUniforms mismatch Metal shader");

typedef struct {
    int32_t n_src;
    int32_t n_dst;
    int32_t s;
    int32_t pad;
} LspCubeBoxUniforms;

typedef struct {
    int32_t n_src;
    int32_t n_dst;
    int32_t pad0;
    int32_t pad1;
} LspTriUniforms;

void finalizeCubeAccum(const uint64_t* sumR,
                       const uint64_t* sumG,
                       const uint64_t* sumB,
                       const uint32_t* cnt,
                       int n,
                       size_t n3,
                       std::vector<float>& outRgba) {
    outRgba.resize(n3 * 4u);
    const float inv = 1.0f / static_cast<float>(n - 1);
    const float scale = static_cast<float>(kLutGenAtomicScale);
    for (size_t i = 0; i < n3; ++i) {
        const size_t ir = i % (size_t)n;
        const size_t t = i / (size_t)n;
        const size_t ig = t % (size_t)n;
        const size_t ib = t / (size_t)n;
        const size_t o = i * 4u;
        if (cnt[i] > 0) {
            const float invc = 1.0f / static_cast<float>(cnt[i]);
            outRgba[o + 0] = static_cast<float>(sumR[i]) / (scale * invc);
            outRgba[o + 1] = static_cast<float>(sumG[i]) / (scale * invc);
            outRgba[o + 2] = static_cast<float>(sumB[i]) / (scale * invc);
        } else {
            outRgba[o + 0] = static_cast<float>(ir) * inv;
            outRgba[o + 1] = static_cast<float>(ig) * inv;
            outRgba[o + 2] = static_cast<float>(ib) * inv;
        }
        outRgba[o + 3] = 1.0f;
    }
}

} // namespace

bool lspLutGenMetalTryBuildAnalyzedCube(const void* srcPixelData,
                                        bool srcIsMetalBuffer,
                                        int rowBytes,
                                        int bx1,
                                        int by1,
                                        int bx2,
                                        int by2,
                                        int n,
                                        int Tx,
                                        int Ty,
                                        std::vector<float>& outRgba) {
    if (!srcPixelData || n < 2 || rowBytes < 16)
        return false;
    const int bw = bx2 - bx1;
    const int bh = by2 - by1;
    if (bw < 1 || bh < 1)
        return false;

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device)
        return false;

    NSError* err = nil;
    id<MTLLibrary> lib = LspLutGenGetMetalLibraryForDevice(device, &err);
    if (!lib) {
        if (err)
            LSP_LUTGEN_LOG_ERROR(std::string("metal_cube_library: ") + [[err localizedDescription] UTF8String]);
        return false;
    }

    id<MTLFunction> fnBin = [lib newFunctionWithName:@"lutgen_bin_pixels"];
    if (!fnBin)
        return false;
    NSError* pe = nil;
    id<MTLComputePipelineState> pipeBin = [device newComputePipelineStateWithFunction:fnBin error:&pe];
    if (!pipeBin) {
        if (pe)
            LSP_LUTGEN_LOG_ERROR(std::string("metal_cube_pipeline_bin: ") + [[pe localizedDescription] UTF8String]);
        return false;
    }

    const size_t n3 = (size_t)n * (size_t)n * (size_t)n;
    const size_t sz64 = n3 * sizeof(uint64_t);
    const size_t sz32 = n3 * sizeof(uint32_t);

    id<MTLBuffer> bufSumR = [device newBufferWithLength:sz64 options:MTLResourceStorageModeShared];
    id<MTLBuffer> bufSumG = [device newBufferWithLength:sz64 options:MTLResourceStorageModeShared];
    id<MTLBuffer> bufSumB = [device newBufferWithLength:sz64 options:MTLResourceStorageModeShared];
    id<MTLBuffer> bufCnt = [device newBufferWithLength:sz32 options:MTLResourceStorageModeShared];
    if (!bufSumR || !bufSumG || !bufSumB || !bufCnt)
        return false;
    std::memset([bufSumR contents], 0, sz64);
    std::memset([bufSumG contents], 0, sz64);
    std::memset([bufSumB contents], 0, sz64);
    std::memset([bufCnt contents], 0, sz32);

    const size_t srcLen = (size_t)std::abs(rowBytes) * (size_t)bh;
    id<MTLBuffer> srcBuf = nil;
    if (srcIsMetalBuffer) {
        srcBuf = (__bridge id<MTLBuffer>)(void*)srcPixelData;
        if (!srcBuf || [srcBuf length] < srcLen)
            return false;
    } else {
        srcBuf = [device newBufferWithBytes:srcPixelData length:srcLen options:MTLResourceStorageModeShared];
        if (!srcBuf)
            return false;
    }

    LspBinUniforms u = {};
    u.bx1 = bx1;
    u.by1 = by1;
    u.bx2 = bx2;
    u.by2 = by2;
    u.row_bytes = rowBytes;
    u.n = n;
    u.Tx = Tx;
    u.Ty = Ty;

    id<MTLCommandQueue> queue = [device newCommandQueue];
    if (!queue)
        return false;
    id<MTLCommandBuffer> cmd = [queue commandBuffer];
    id<MTLComputeCommandEncoder> enc = [cmd computeCommandEncoder];
    if (!cmd || !enc)
        return false;

    [enc setComputePipelineState:pipeBin];
    [enc setBytes:&u length:sizeof(u) atIndex:0];
    [enc setBuffer:srcBuf offset:0 atIndex:1];
    [enc setBuffer:bufSumR offset:0 atIndex:2];
    [enc setBuffer:bufSumG offset:0 atIndex:3];
    [enc setBuffer:bufSumB offset:0 atIndex:4];
    [enc setBuffer:bufCnt offset:0 atIndex:5];

    const NSUInteger gw = (NSUInteger)bw;
    const NSUInteger gh = (NSUInteger)bh;
    MTLSize grid = MTLSizeMake(gw, gh, 1);
    MTLSize tpg = MTLSizeMake(16, 16, 1);
    [enc dispatchThreads:grid threadsPerThreadgroup:tpg];
    [enc endEncoding];
    [cmd commit];
    [cmd waitUntilCompleted];
    if (cmd.error) {
        LSP_LUTGEN_LOG_ERROR(std::string("metal_cube_bin_cmd: ") + [[cmd.error localizedDescription] UTF8String]);
        return false;
    }

    finalizeCubeAccum(static_cast<const uint64_t*>([bufSumR contents]),
                      static_cast<const uint64_t*>([bufSumG contents]),
                      static_cast<const uint64_t*>([bufSumB contents]),
                      static_cast<const uint32_t*>([bufCnt contents]),
                      n,
                      n3,
                      outRgba);
    return true;
}

bool lspLutGenCpuBuildAnalyzedCubeFromMetalBuffer(void* mtlBufferPtr,
                                                  int rowBytes,
                                                  int bx1,
                                                  int by1,
                                                  int bx2,
                                                  int by2,
                                                  int n,
                                                  std::vector<float>& outRgba) {
    if (!mtlBufferPtr || n < 2 || rowBytes < 16)
        return false;
    id<MTLBuffer> buf = (__bridge id<MTLBuffer>)(void*)mtlBufferPtr;
    if (!buf)
        return false;
    const int bh = by2 - by1;
    const int bw = bx2 - bx1;
    if (bw < 1 || bh < 1)
        return false;
    const size_t minLen = (size_t)std::abs(rowBytes) * (size_t)bh;
    if ([buf length] < minLen)
        return false;
    const void* mapped = [buf contents];
    if (!mapped)
        return false;
    const float* base = static_cast<const float*>(mapped);
    OfxRectI B = {bx1, by1, bx2, by2};
    return lspLutGenBuildAnalyzedCubeFromLinearBase(B, n, base, rowBytes, outRgba);
}

bool lspLutGenMetalTryDownsampleCube(const float* srcRgba, int nSrc, int nDst, std::vector<float>& outRgba) {
    if (!srcRgba || nSrc < 2 || nDst < 2 || (nSrc % nDst) != 0)
        return false;

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device)
        return false;

    NSError* err = nil;
    id<MTLLibrary> lib = LspLutGenGetMetalLibraryForDevice(device, &err);
    if (!lib)
        return false;
    id<MTLFunction> fn = [lib newFunctionWithName:@"lutgen_downsample_cube"];
    if (!fn)
        return false;
    NSError* pe = nil;
    id<MTLComputePipelineState> pipe = [device newComputePipelineStateWithFunction:fn error:&pe];
    if (!pipe)
        return false;

    const int s = nSrc / nDst;
    const size_t n3src = (size_t)nSrc * (size_t)nSrc * (size_t)nSrc;
    const size_t n3dst = (size_t)nDst * (size_t)nDst * (size_t)nDst;
    const size_t srcBytes = n3src * 4u * sizeof(float);
    const size_t dstBytes = n3dst * 4u * sizeof(float);

    id<MTLBuffer> srcBuf = [device newBufferWithBytes:srcRgba length:srcBytes options:MTLResourceStorageModeShared];
    id<MTLBuffer> dstBuf = [device newBufferWithLength:dstBytes options:MTLResourceStorageModeShared];
    if (!srcBuf || !dstBuf)
        return false;
    std::memset([dstBuf contents], 0, dstBytes);

    LspCubeBoxUniforms u = {};
    u.n_src = nSrc;
    u.n_dst = nDst;
    u.s = s;

    id<MTLCommandQueue> queue = [device newCommandQueue];
    id<MTLCommandBuffer> cmd = [queue commandBuffer];
    id<MTLComputeCommandEncoder> enc = [cmd computeCommandEncoder];
    if (!queue || !cmd || !enc)
        return false;

    [enc setComputePipelineState:pipe];
    [enc setBytes:&u length:sizeof(u) atIndex:0];
    [enc setBuffer:srcBuf offset:0 atIndex:1];
    [enc setBuffer:dstBuf offset:0 atIndex:2];

    const NSUInteger nd = (NSUInteger)nDst;
    MTLSize grid = MTLSizeMake(nd, nd, nd);
    MTLSize tpg = MTLSizeMake(8, 8, 8);
    [enc dispatchThreads:grid threadsPerThreadgroup:tpg];
    [enc endEncoding];
    [cmd commit];
    [cmd waitUntilCompleted];
    if (cmd.error)
        return false;

    outRgba.resize(n3dst * 4u);
    std::memcpy(outRgba.data(), [dstBuf contents], dstBytes);
    return true;
}

bool lspLutGenMetalTryResampleCubeTrilinear(const float* srcRgba, int nSrc, int nDst, std::vector<float>& outRgba) {
    if (!srcRgba || nSrc < 2 || nDst < 2)
        return false;

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device)
        return false;

    NSError* err = nil;
    id<MTLLibrary> lib = LspLutGenGetMetalLibraryForDevice(device, &err);
    if (!lib)
        return false;
    id<MTLFunction> fn = [lib newFunctionWithName:@"lutgen_trilinear_cube"];
    if (!fn)
        return false;
    NSError* pe = nil;
    id<MTLComputePipelineState> pipe = [device newComputePipelineStateWithFunction:fn error:&pe];
    if (!pipe)
        return false;

    const size_t n3src = (size_t)nSrc * (size_t)nSrc * (size_t)nSrc;
    const size_t n3dst = (size_t)nDst * (size_t)nDst * (size_t)nDst;
    const size_t srcBytes = n3src * 4u * sizeof(float);
    const size_t dstBytes = n3dst * 4u * sizeof(float);

    id<MTLBuffer> srcBuf = [device newBufferWithBytes:srcRgba length:srcBytes options:MTLResourceStorageModeShared];
    id<MTLBuffer> dstBuf = [device newBufferWithLength:dstBytes options:MTLResourceStorageModeShared];
    if (!srcBuf || !dstBuf)
        return false;
    std::memset([dstBuf contents], 0, dstBytes);

    LspTriUniforms u = {};
    u.n_src = nSrc;
    u.n_dst = nDst;

    id<MTLCommandQueue> queue = [device newCommandQueue];
    id<MTLCommandBuffer> cmd = [queue commandBuffer];
    id<MTLComputeCommandEncoder> enc = [cmd computeCommandEncoder];
    if (!queue || !cmd || !enc)
        return false;

    [enc setComputePipelineState:pipe];
    [enc setBytes:&u length:sizeof(u) atIndex:0];
    [enc setBuffer:srcBuf offset:0 atIndex:1];
    [enc setBuffer:dstBuf offset:0 atIndex:2];

    const NSUInteger nd = (NSUInteger)nDst;
    MTLSize grid = MTLSizeMake(nd, nd, nd);
    MTLSize tpg = MTLSizeMake(8, 8, 8);
    [enc dispatchThreads:grid threadsPerThreadgroup:tpg];
    [enc endEncoding];
    [cmd commit];
    [cmd waitUntilCompleted];
    if (cmd.error)
        return false;

    outRgba.resize(n3dst * 4u);
    std::memcpy(outRgba.data(), [dstBuf contents], dstBytes);
    return true;
}
