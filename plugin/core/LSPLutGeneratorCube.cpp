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

namespace {
void sampleCubeRgbTriLinear(const float* p_Cube, int p_N, double p_Fr, double p_Fg, double p_Fb, double* p_OutRgb) {
    const double nm1 = static_cast<double>(p_N - 1);
    double fr = std::clamp(p_Fr, 0.0, nm1);
    double fg = std::clamp(p_Fg, 0.0, nm1);
    double fb = std::clamp(p_Fb, 0.0, nm1);
    const int ir0 = static_cast<int>(std::floor(fr));
    const int ig0 = static_cast<int>(std::floor(fg));
    const int ib0 = static_cast<int>(std::floor(fb));
    const int ir1 = std::min(ir0 + 1, p_N - 1);
    const int ig1 = std::min(ig0 + 1, p_N - 1);
    const int ib1 = std::min(ib0 + 1, p_N - 1);
    const double tr = fr - static_cast<double>(ir0);
    const double tg = fg - static_cast<double>(ig0);
    const double tb = fb - static_cast<double>(ib0);
    auto at = [p_Cube, p_N](int ir, int ig, int ib) -> const float* {
        const size_t idx = (size_t)ir + (size_t)p_N * ((size_t)ig + (size_t)p_N * (size_t)ib);
        return p_Cube + idx * 4u;
    };
    const double c000r = at(ir0, ig0, ib0)[0];
    const double c000g = at(ir0, ig0, ib0)[1];
    const double c000b = at(ir0, ig0, ib0)[2];
    const double c100r = at(ir1, ig0, ib0)[0];
    const double c100g = at(ir1, ig0, ib0)[1];
    const double c100b = at(ir1, ig0, ib0)[2];
    const double c010r = at(ir0, ig1, ib0)[0];
    const double c010g = at(ir0, ig1, ib0)[1];
    const double c010b = at(ir0, ig1, ib0)[2];
    const double c110r = at(ir1, ig1, ib0)[0];
    const double c110g = at(ir1, ig1, ib0)[1];
    const double c110b = at(ir1, ig1, ib0)[2];
    const double c001r = at(ir0, ig0, ib1)[0];
    const double c001g = at(ir0, ig0, ib1)[1];
    const double c001b = at(ir0, ig0, ib1)[2];
    const double c101r = at(ir1, ig0, ib1)[0];
    const double c101g = at(ir1, ig0, ib1)[1];
    const double c101b = at(ir1, ig0, ib1)[2];
    const double c011r = at(ir0, ig1, ib1)[0];
    const double c011g = at(ir0, ig1, ib1)[1];
    const double c011b = at(ir0, ig1, ib1)[2];
    const double c111r = at(ir1, ig1, ib1)[0];
    const double c111g = at(ir1, ig1, ib1)[1];
    const double c111b = at(ir1, ig1, ib1)[2];
    const double c00r = c000r + tr * (c100r - c000r);
    const double c00g = c000g + tr * (c100g - c000g);
    const double c00b = c000b + tr * (c100b - c000b);
    const double c01r = c001r + tr * (c101r - c001r);
    const double c01g = c001g + tr * (c101g - c001g);
    const double c01b = c001b + tr * (c101b - c001b);
    const double c10r = c010r + tr * (c110r - c010r);
    const double c10g = c010g + tr * (c110g - c010g);
    const double c10b = c010b + tr * (c110b - c010b);
    const double c11r = c011r + tr * (c111r - c011r);
    const double c11g = c011g + tr * (c111g - c011g);
    const double c11b = c011b + tr * (c111b - c011b);
    const double c0r = c00r + tg * (c10r - c00r);
    const double c0g = c00g + tg * (c10g - c00g);
    const double c0b = c00b + tg * (c10b - c00b);
    const double c1r = c01r + tg * (c11r - c01r);
    const double c1g = c01g + tg * (c11g - c01g);
    const double c1b = c01b + tg * (c11b - c01b);
    p_OutRgb[0] = c0r + tb * (c1r - c0r);
    p_OutRgb[1] = c0g + tb * (c1g - c0g);
    p_OutRgb[2] = c0b + tb * (c1b - c0b);
}
} // namespace

bool lspLutGenResampleCubeRgbaTrilinear(const float* p_SrcRgba, int p_NSrc, int p_NDst, std::vector<float>& p_OutRgba) {
    if (!p_SrcRgba || p_NSrc < 2 || p_NDst < 2)
        return false;
    const double srcNm1 = static_cast<double>(p_NSrc - 1);
    const double dstNm1 = static_cast<double>(p_NDst - 1);
    const size_t nOut3 = (size_t)p_NDst * (size_t)p_NDst * (size_t)p_NDst;
    p_OutRgba.resize(nOut3 * 4u);
    double rgb[3];
    for (int bo = 0; bo < p_NDst; ++bo) {
        for (int go = 0; go < p_NDst; ++go) {
            for (int ro = 0; ro < p_NDst; ++ro) {
                const double ur = (dstNm1 > 0.0) ? static_cast<double>(ro) / dstNm1 : 0.0;
                const double ug = (dstNm1 > 0.0) ? static_cast<double>(go) / dstNm1 : 0.0;
                const double ub = (dstNm1 > 0.0) ? static_cast<double>(bo) / dstNm1 : 0.0;
                const double fr = ur * srcNm1;
                const double fg = ug * srcNm1;
                const double fb = ub * srcNm1;
                sampleCubeRgbTriLinear(p_SrcRgba, p_NSrc, fr, fg, fb, rgb);
                const size_t oidx = (size_t)ro + (size_t)p_NDst * ((size_t)go + (size_t)p_NDst * (size_t)bo);
                p_OutRgba[oidx * 4u + 0u] = static_cast<float>(rgb[0]);
                p_OutRgba[oidx * 4u + 1u] = static_cast<float>(rgb[1]);
                p_OutRgba[oidx * 4u + 2u] = static_cast<float>(rgb[2]);
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
