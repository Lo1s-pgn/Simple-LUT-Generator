#pragma once

#include <cstdint>
#include <vector>

/** GPU path for export LUT: returns false to fall back to CPU. */
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
                                        std::vector<float>& outRgba);
bool lspLutGenCpuBuildAnalyzedCubeFromMetalBuffer(void* mtlBufferPtr,
                                                  int rowBytes,
                                                  int bx1,
                                                  int by1,
                                                  int bx2,
                                                  int by2,
                                                  int n,
                                                  std::vector<float>& outRgba);

bool lspLutGenMetalTryDownsampleCube(const float* srcRgba, int nSrc, int nDst, std::vector<float>& outRgba);

bool lspLutGenMetalTryResampleCubeTrilinear(const float* srcRgba, int nSrc, int nDst, std::vector<float>& outRgba);
