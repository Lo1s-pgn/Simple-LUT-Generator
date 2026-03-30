#include "LSPLutGeneratorProcessor.h"
#include "LSPLutGeneratorPattern.h"
#include "LSPLutGeneratorConstants.h"
#include <algorithm>

LSPLutGeneratorProcessor::LSPLutGeneratorProcessor(OFX::ImageEffect& p_Effect)
    : OFX::ImageProcessor(p_Effect)
    , _srcImg(nullptr)
    , _operationMode(0)
    , _dstFullBounds{ 0, 0, 0, 0 }
    , _generateLutN(64) {}

namespace {
void copyRGBAWindow(OFX::Image* p_Src, OFX::Image* p_Dst, const OfxRectI& p_Window) {
    if (!p_Src || !p_Dst)
        return;
    for (int y = p_Window.y1; y < p_Window.y2; ++y) {
        for (int x = p_Window.x1; x < p_Window.x2; ++x) {
            float* d = static_cast<float*>(p_Dst->getPixelAddress(x, y));
            const float* s = static_cast<const float*>(p_Src->getPixelAddress(x, y));
            d[0] = s[0];
            d[1] = s[1];
            d[2] = s[2];
            d[3] = s[3];
        }
    }
}
} // namespace

void LSPLutGeneratorProcessor::multiThreadProcessImages(OfxRectI p_Window) {
    if (!_dstImg)
        return;
    if (_operationMode == kOperationModeGenerate) {
        const int n = std::max(2, _generateLutN);
        for (int y = p_Window.y1; y < p_Window.y2; ++y) {
            for (int x = p_Window.x1; x < p_Window.x2; ++x) {
                float rgba[4];
                lspLutGenPatternRGBA(x, y, _dstFullBounds, n, rgba);
                float* d = static_cast<float*>(_dstImg->getPixelAddress(x, y));
                d[0] = rgba[0];
                d[1] = rgba[1];
                d[2] = rgba[2];
                d[3] = rgba[3];
            }
        }
        return;
    }
    if (!_srcImg)
        return;
    copyRGBAWindow(_srcImg, _dstImg, p_Window);
}
