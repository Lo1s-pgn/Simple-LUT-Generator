#ifndef LSP_LUT_GENERATOR_CUBE_H
#define LSP_LUT_GENERATOR_CUBE_H

#include <string>
#include <vector>

namespace OFX {
class Image;
}

/** Recompute balanced-grid reference RGB per pixel (same as **Generate LUT Table**); bin into n³; each cell stores mean measured RGB. Empty cells = identity. Order: i = r + n*g + n*n*b. */
bool lspLutGenBuildAnalyzedCube(OFX::Image* p_GradedStrip, int p_N, std::vector<float>& p_OutRgba);

/** ASCII .cube: LUT_3D_SIZE, DOMAIN_MIN/MAX, then n³ lines of R G B (float). p_Buffer is n³×4 RGBA (alpha ignored). */
bool lspLutGenWriteCubeFile(const std::string& p_Path, int p_N, const float* p_BufferRgba);

/** Box-average **p_NSrc³** RGBA lattice to **p_NDst³**; requires **p_NSrc % p_NDst == 0**. */
bool lspLutGenDownsampleCubeRgba(const float* p_SrcRgba, int p_NSrc, int p_NDst, std::vector<float>& p_OutRgba);

#endif
