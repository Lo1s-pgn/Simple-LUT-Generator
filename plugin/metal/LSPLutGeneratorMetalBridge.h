#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Dispatch Metal compute for the current render window. Buffers are host OFX pixel data (id<MTLCommandQueue>, id<MTLBuffer>). */
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
                           int32_t patternTy);

#ifdef __cplusplus
}
#endif
