#include "LSPLutGeneratorCube.h"
#include "LSPLutGeneratorPattern.h"
#include "ofxsImageEffect.h"
#include <algorithm>
#include <cmath>
#include <fstream>

namespace {
struct AccumCell {
    double r;
    double g;
    double b;
    int count;
};
} // namespace

bool lspLutGenBuildAnalyzedCube(OFX::Image* p_GradedStrip, int p_N, std::vector<float>& p_OutRgba) {
    if (!p_GradedStrip || p_N < 2)
        return false;
    if (p_GradedStrip->getPixelDepth() != OFX::eBitDepthFloat)
        return false;
    if (p_GradedStrip->getPixelComponents() != OFX::ePixelComponentRGBA)
        return false;
    if (!p_GradedStrip->getPixelData())
        return false;

    const OfxRectI B = p_GradedStrip->getBounds();
    if (B.x2 <= B.x1 || B.y2 <= B.y1)
        return false;

    const size_t n3 = (size_t)p_N * (size_t)p_N * (size_t)p_N;
    std::vector<AccumCell> acc(n3);
    for (size_t i = 0; i < n3; ++i)
        acc[i] = { 0.0, 0.0, 0.0, 0 };

    const int nm1 = p_N - 1;
    for (int y = B.y1; y < B.y2; ++y) {
        for (int x = B.x1; x < B.x2; ++x) {
            float ref[4];
            lspLutGenPatternRGBA(x, y, B, p_N, ref);
            const float sr = std::clamp(ref[0], 0.0f, 1.0f);
            const float sg = std::clamp(ref[1], 0.0f, 1.0f);
            const float sb = std::clamp(ref[2], 0.0f, 1.0f);
            int ir = static_cast<int>(std::lround(static_cast<double>(sr) * static_cast<double>(nm1)));
            int ig = static_cast<int>(std::lround(static_cast<double>(sg) * static_cast<double>(nm1)));
            int ib = static_cast<int>(std::lround(static_cast<double>(sb) * static_cast<double>(nm1)));
            ir = std::clamp(ir, 0, p_N - 1);
            ig = std::clamp(ig, 0, p_N - 1);
            ib = std::clamp(ib, 0, p_N - 1);
            const size_t idx = (size_t)ir + (size_t)p_N * ((size_t)ig + (size_t)p_N * (size_t)ib);
            const float* gpx = static_cast<const float*>(p_GradedStrip->getPixelAddress(x, y));
            AccumCell& a = acc[idx];
            a.r += static_cast<double>(gpx[0]);
            a.g += static_cast<double>(gpx[1]);
            a.b += static_cast<double>(gpx[2]);
            a.count += 1;
        }
    }

    p_OutRgba.resize(n3 * 4u);
    const float inv = 1.0f / static_cast<float>(nm1);
    for (size_t ib = 0; ib < (size_t)p_N; ++ib) {
        for (size_t ig = 0; ig < (size_t)p_N; ++ig) {
            for (size_t ir = 0; ir < (size_t)p_N; ++ir) {
                const size_t idxCell = ir + (size_t)p_N * (ig + (size_t)p_N * ib);
                const size_t o = idxCell * 4u;
                const AccumCell& a = acc[idxCell];
                if (a.count > 0) {
                    const double invc = 1.0 / static_cast<double>(a.count);
                    p_OutRgba[o + 0] = static_cast<float>(a.r * invc);
                    p_OutRgba[o + 1] = static_cast<float>(a.g * invc);
                    p_OutRgba[o + 2] = static_cast<float>(a.b * invc);
                } else {
                    p_OutRgba[o + 0] = static_cast<float>(ir) * inv;
                    p_OutRgba[o + 1] = static_cast<float>(ig) * inv;
                    p_OutRgba[o + 2] = static_cast<float>(ib) * inv;
                }
                p_OutRgba[o + 3] = 1.0f;
            }
        }
    }
    return true;
}

bool lspLutGenDownsampleCubeRgba(const float* p_SrcRgba, int p_NSrc, int p_NDst, std::vector<float>& p_OutRgba) {
    if (!p_SrcRgba || p_NSrc < 2 || p_NDst < 2 || (p_NSrc % p_NDst) != 0)
        return false;
    const int s = p_NSrc / p_NDst;
    const double invVol = 1.0 / static_cast<double>(s * s * s);
    const size_t nOut3 = (size_t)p_NDst * (size_t)p_NDst * (size_t)p_NDst;
    p_OutRgba.resize(nOut3 * 4u);
    for (int bo = 0; bo < p_NDst; ++bo) {
        for (int go = 0; go < p_NDst; ++go) {
            for (int ro = 0; ro < p_NDst; ++ro) {
                double ar = 0.0;
                double ag = 0.0;
                double ab = 0.0;
                for (int dz = 0; dz < s; ++dz) {
                    for (int dy = 0; dy < s; ++dy) {
                        for (int dx = 0; dx < s; ++dx) {
                            const int ir = ro * s + dx;
                            const int ig = go * s + dy;
                            const int ib = bo * s + dz;
                            const size_t idx = (size_t)ir + (size_t)p_NSrc * ((size_t)ig + (size_t)p_NSrc * (size_t)ib);
                            ar += static_cast<double>(p_SrcRgba[idx * 4u + 0u]);
                            ag += static_cast<double>(p_SrcRgba[idx * 4u + 1u]);
                            ab += static_cast<double>(p_SrcRgba[idx * 4u + 2u]);
                        }
                    }
                }
                const size_t oidx = (size_t)ro + (size_t)p_NDst * ((size_t)go + (size_t)p_NDst * (size_t)bo);
                p_OutRgba[oidx * 4u + 0u] = static_cast<float>(ar * invVol);
                p_OutRgba[oidx * 4u + 1u] = static_cast<float>(ag * invVol);
                p_OutRgba[oidx * 4u + 2u] = static_cast<float>(ab * invVol);
                p_OutRgba[oidx * 4u + 3u] = 1.0f;
            }
        }
    }
    return true;
}

bool lspLutGenWriteCubeFile(const std::string& p_Path, int p_N, const float* p_BufferRgba) {
    if (!p_BufferRgba || p_N < 2)
        return false;
    std::ofstream out(p_Path);
    if (!out)
        return false;
    const long long n = (long long)p_N * p_N * p_N;
    out << "# LSP LUT Generator analyzed LUT " << p_N << "^3\n";
    out << "DOMAIN_MIN 0.0 0.0 0.0\n";
    out << "DOMAIN_MAX 1.0 1.0 1.0\n";
    out << "LUT_3D_SIZE " << p_N << "\n";
    for (long long i = 0; i < n; ++i) {
        const float r = p_BufferRgba[(size_t)i * 4u + 0u];
        const float g = p_BufferRgba[(size_t)i * 4u + 1u];
        const float b = p_BufferRgba[(size_t)i * 4u + 2u];
        out << r << " " << g << " " << b << "\n";
    }
    return out.good();
}
