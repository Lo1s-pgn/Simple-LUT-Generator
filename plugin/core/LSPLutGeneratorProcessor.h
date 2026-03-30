#ifndef LSP_LUT_GENERATOR_PROCESSOR_H
#define LSP_LUT_GENERATOR_PROCESSOR_H

#include "ofxsProcessing.h"

class LSPLutGeneratorProcessor : public OFX::ImageProcessor {
public:
    explicit LSPLutGeneratorProcessor(OFX::ImageEffect& p_Effect);

    void setSrcImg(OFX::Image* p_Img) { _srcImg = p_Img; }
    void setOperationMode(int p_Mode) { _operationMode = p_Mode; }
    void setDstFullBounds(const OfxRectI& p_Bounds) { _dstFullBounds = p_Bounds; }
    /** Generate LUT Table mode: always the **max feasible** n for the output bounds (set by plugin). */
    void setGenerateLutN(int p_N) { _generateLutN = p_N; }

    void multiThreadProcessImages(OfxRectI p_Window) override;

private:
    OFX::Image* _srcImg;
    int _operationMode;
    OfxRectI _dstFullBounds;
    int _generateLutN;
};

#endif
