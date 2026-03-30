#ifndef LSP_LUT_GENERATOR_PATTERN_H
#define LSP_LUT_GENERATOR_PATTERN_H

#include "ofxCore.h"

#define kLutGenChoiceCount 4 /* UI export sizes: 17, 32, 64, 128 */

void lspLutGenPatternRGBA(int p_Px, int p_Py, const OfxRectI& p_DstBounds, int p_Samples, float p_Out[4]);
int lspLutGenLutSizeFromChoiceIndex(int p_Index);
int lspLutGenMaxFeasibleN(int p_FrameW, int p_FrameH, float p_MinPixelsPerUnit);
bool lspLutGenFeasibleN(int p_N, int p_FrameW, int p_FrameH, float p_MinPixelsPerUnit);
float lspLutGenMinPixelsPerLatticeUnit(void);

#endif
