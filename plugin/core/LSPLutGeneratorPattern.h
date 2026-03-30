#ifndef LSP_LUT_GENERATOR_PATTERN_H
#define LSP_LUT_GENERATOR_PATTERN_H

#include "ofxCore.h"

#define kLutGenChoiceCount 4

void lspLutGenPatternRGBA(int p_Px, int p_Py, const OfxRectI& p_DstBounds, int p_Samples, float p_Out[4]);
int lspLutGenLutSizeFromChoiceIndex(int p_Index);
int lspLutGenMaxFeasibleN(int p_FrameW, int p_FrameH, float p_MinPixelsPerUnit);
bool lspLutGenExportSizeValid(int p_NMax, int p_NExport);

float lspLutGenMinPixelsPerLatticeUnit(void);

#endif
