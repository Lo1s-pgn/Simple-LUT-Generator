#ifndef LSP_LUT_GENERATOR_CUBE_H
#define LSP_LUT_GENERATOR_CUBE_H

#include "ofxCore.h"
#include <string>
#include <vector>

namespace OFX {
class Image;
}

// RGBA buffers are n^3 × 4 floats (A unused / 1).
/** Export / instance-changed path: CPU-only binning (Metal MTLBuffer mapped to CPU; never Metal compute here). */
bool lspLutGenBuildAnalyzedCubeForExport(OFX::Image* p_GradedStrip, int p_N, std::vector<float>& p_OutRgba);

// p_PixelDataIsMetalBuffer retained for API compat; ignored — export uses ForExport.
bool lspLutGenBuildAnalyzedCube(OFX::Image* p_GradedStrip, int p_N, std::vector<float>& p_OutRgba, bool p_PixelDataIsMetalBuffer);
// Linear RGBA float buffer at (bounds.x1,bounds.y1); rowBytes may be negative (CPU images).
bool lspLutGenBuildAnalyzedCubeFromLinearBase(const OfxRectI& B, int p_N, const float* baseRow00, int rowBytes, std::vector<float>& p_OutRgba);
bool lspLutGenCopyImageToHostRgba(OFX::Image* p_Image, std::vector<float>& p_OutRgba, OfxRectI& p_OutBounds, int& p_OutRowBytes);
bool lspLutGenWriteCubeFile(const std::string& p_Path, int p_N, const float* p_BufferRgba);
bool lspLutGenDownsampleCubeRgba(const float* p_SrcRgba, int p_NSrc, int p_NDst, std::vector<float>& p_OutRgba);
bool lspLutGenResampleCubeRgbaTrilinear(const float* p_SrcRgba, int p_NSrc, int p_NDst, std::vector<float>& p_OutRgba);

/* Empty if folder is missing, not a directory, or no free _NNN.cube in range. */
std::string lspLutGenMakeUniqueNumberedCubePath(const std::string& p_Directory, const std::string& p_RawFileBase);

#endif
