// Multi-threaded pixel worker: Generate writes pattern; Analyze copies source through.
#include "LSPLutGeneratorProcessor.h"
#include "LSPLutGeneratorPattern.h"
#include "LSPLutGeneratorConstants.h"
#include "LSPLutGeneratorMetalBridge.h"

LSPLutGeneratorProcessor::LSPLutGeneratorProcessor(OFX::ImageEffect& p_Effect)
    : OFX::ImageProcessor(p_Effect)
    , _srcImg(nullptr)
    , _operationMode(0)
    , _dstFullBounds{ 0, 0, 0, 0 }
    , _generateLutN(64) {}

namespace {
// Analyze mode: passthrough float RGBA within the render window.
void copyRGBAWindow(OFX::Image* p_Src, OFX::Image* p_Dst, const OfxRectI& p_Window) {
    if (!p_Src || !p_Dst)
        return;
    for (int y = p_Window.y1; y < p_Window.y2; ++y) {
        for (int x = p_Window.x1; x < p_Window.x2; ++x) {
            float* d = static_cast<float*>(p_Dst->getPixelAddress(x, y));
            const float* s = static_cast<const float*>(p_Src->getPixelAddress(x, y));
            if (!d || !s)
                continue;
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
        const int n = _generateLutN < 2 ? 2 : _generateLutN;
        for (int y = p_Window.y1; y < p_Window.y2; ++y) {
            for (int x = p_Window.x1; x < p_Window.x2; ++x) {
                float rgba[4];
                lspLutGenPatternRGBA(x, y, _dstFullBounds, n, rgba);
                float* d = static_cast<float*>(_dstImg->getPixelAddress(x, y));
                if (!d)
                    continue;
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

void LSPLutGeneratorProcessor::processImagesMetal() {
    if (!_dstImg || !_pMetalCmdQ)
        OFX::throwSuiteStatusException(kOfxStatFailed);
    void* dstData = _dstImg->getPixelData();
    if (!dstData)
        OFX::throwSuiteStatusException(kOfxStatFailed);

    const OfxRectI db = _dstImg->getBounds();
    const OfxRectI rw = _renderWindow;

    if (_operationMode == kOperationModeGenerate) {
        const int n = _generateLutN < 2 ? 2 : _generateLutN;
        int tx = 0;
        int ty = 0;
        lspLutGenPatternGridTxTy(n, db.x2 - db.x1, db.y2 - db.y1, &tx, &ty);
        if (!lspLutGenMetalDispatch(_pMetalCmdQ,
                                    dstData,
                                    _dstImg->getRowBytes(),
                                    nullptr,
                                    0,
                                    db.x1,
                                    db.y1,
                                    db.x2,
                                    db.y2,
                                    db.x1,
                                    db.y1,
                                    db.x2,
                                    db.y2,
                                    rw.x1,
                                    rw.y1,
                                    rw.x2,
                                    rw.y2,
                                    0,
                                    n,
                                    tx,
                                    ty))
            OFX::throwSuiteStatusException(kOfxStatFailed);
        return;
    }

    if (!_srcImg)
        OFX::throwSuiteStatusException(kOfxStatFailed);
    void* srcData = _srcImg->getPixelData();
    if (!srcData)
        OFX::throwSuiteStatusException(kOfxStatFailed);
    const OfxRectI sb = _srcImg->getBounds();
    if (!lspLutGenMetalDispatch(_pMetalCmdQ,
                                dstData,
                                _dstImg->getRowBytes(),
                                srcData,
                                _srcImg->getRowBytes(),
                                db.x1,
                                db.y1,
                                db.x2,
                                db.y2,
                                sb.x1,
                                sb.y1,
                                sb.x2,
                                sb.y2,
                                rw.x1,
                                rw.y1,
                                rw.x2,
                                rw.y2,
                                1,
                                _generateLutN < 2 ? 2 : _generateLutN,
                                0,
                                0))
        OFX::throwSuiteStatusException(kOfxStatFailed);
}
