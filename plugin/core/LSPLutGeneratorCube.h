#ifndef LSP_LUT_GENERATOR_CUBE_H
#define LSP_LUT_GENERATOR_CUBE_H

#include "ofxCore.h"
#include <string>
#include <vector>

namespace OFX {
class Image;
}

// RGBA buffers are n^3 × 4 floats (A unused / 1).
// p_PixelDataIsMetalBuffer: true when kOfxImagePropData is id<MTLBuffer> (OFX Metal render); false for CPU pointers.
bool lspLutGenBuildAnalyzedCube(OFX::Image* p_GradedStrip, int p_N, std::vector<float>& p_OutRgba, bool p_PixelDataIsMetalBuffer);
// Linear RGBA float buffer at (bounds.x1,bounds.y1); rowBytes may be negative (CPU images).
bool lspLutGenBuildAnalyzedCubeFromLinearBase(const OfxRectI& B, int p_N, const float* baseRow00, int rowBytes, std::vector<float>& p_OutRgba);
bool lspLutGenWriteCubeFile(const std::string& p_Path, int p_N, const float* p_BufferRgba);
bool lspLutGenDownsampleCubeRgba(const float* p_SrcRgba, int p_NSrc, int p_NDst, std::vector<float>& p_OutRgba);
bool lspLutGenResampleCubeRgbaTrilinear(const float* p_SrcRgba, int p_NSrc, int p_NDst, std::vector<float>& p_OutRgba);

#endif
