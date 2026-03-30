#ifndef LSP_LUT_GENERATOR_PATTERN_H
#define LSP_LUT_GENERATOR_PATTERN_H

#include "ofxCore.h"

/** Supported LUT edge sizes in UI order (choice index 0..3). */
#define kLutGenChoiceCount 4

/** Aspect-balanced grid: Tx×Ty = n³, factors chosen from frame aspect (see **lspLutGenGridDimensions**). */
void lspLutGenPatternRGBA(int p_Px, int p_Py, const OfxRectI& p_DstBounds, int p_Samples, float p_Out[4]);

/** Map **LUT export size** choice index → n (16, 32, 64, or 128). */
int lspLutGenLutSizeFromChoiceIndex(int p_Index);

/** Largest n in {128, 64, 32, 16} that fits **p_FrameW×p_FrameH** (≥1 px per lattice step); falls back to **16**. */
int lspLutGenMaxFeasibleN(int p_FrameW, int p_FrameH, float p_MinPixelsPerUnit);

/** **true** iff **p_NExport ≤ p_NMax** and **p_NMax % p_NExport == 0** (box downsample from capture **p_NMax** to **p_NExport**). */
bool lspLutGenExportSizeValid(int p_NMax, int p_NExport);

/** Minimum pixels per lattice step along each axis (width / Tx, height / Ty) for a size to be offered. */
float lspLutGenMinPixelsPerLatticeUnit(void);

/** Logical grid size in cells (Tx × Ty = n³). Uses p_FrameW/H for factor choice. */
void lspLutGenGridDimensions(int p_N, int p_FrameW, int p_FrameH, int* p_OutTx, int* p_OutTy);

bool lspLutGenFeasibleN(int p_N, int p_FrameW, int p_FrameH, float p_MinPixelsPerUnit);

#endif
